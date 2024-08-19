#include "../util/random.h"
#include "../common/coroutine.h"
#include "../common/waitgroup.h"
#include "client.h"
#include "task.h"
#include "scheduler.h"
#include "result.h"
#include "objector.h"
#include <regex>
#include <spdlog/spdlog.h>

namespace galay::client
{
GY_TcpClient::GY_TcpClient(std::weak_ptr<poller::GY_IOScheduler> scheduler)
{
    this->m_scheduler = scheduler;
    this->m_recvTask = nullptr;
    this->m_sendTask = nullptr;
    this->m_isConnected = false;
    this->m_isSocket = false;
    this->m_isSSL = false;
}

void 
GY_TcpClient::IsSSL(bool isSSL)
{
    this->m_isSSL = isSSL;
}

galay::coroutine::GY_NetCoroutine 
GY_TcpClient::Unary(std::string host, uint16_t port, std::string &&buffer, std::shared_ptr<result::NetResultInner> result, bool autoClose)
{
    std::string request = std::forward<std::string>(buffer);
    if(!m_isSSL) {
        if(!Socketed()) Socket();
    }else {
        if(!Socketed()) SSLSocket(TLS1_2_VERSION, TLS1_3_VERSION);
    }
    if(!m_isSSL) {
        if(!Connected()){
            auto res = Connect(host, port);
            co_await res->Wait();
            if(!res->Success()) {
                result->SetErrorMsg("Connect:" + res->Error());
                spdlog::error("[{}:{}] [Unary.Connect(host:{}, port:{}) failed, Error:{}]", __FILE__, __LINE__, host, port, res->Error());
                result->Done();
                co_return coroutine::CoroutineStatus::kCoroutineFinished;
            }
        }
        auto res = Send(std::move(request));
        co_await res->Wait();
        if(!res->Success()) {
            result->SetErrorMsg("Send:" + res->Error());
            spdlog::error("[{}:{}] [Unary.Send(buffer:{}) failed, Error:{}]", __FILE__, __LINE__, request, res->Error());
            result->Done();
            co_return coroutine::CoroutineStatus::kCoroutineFinished;
        }
        std::string temp;
        res = Recv(temp);
        co_await res->Wait();
        if(!res->Success()) {
            result->SetErrorMsg("Send:" + res->Error());
            spdlog::error("[{}:{}] [Unary.Recv(buffer:{}) failed, Error:{}]", __FILE__, __LINE__, temp, res->Error());
            result->Done();
            co_return coroutine::CoroutineStatus::kCoroutineFinished;
        }
        spdlog::debug("[{}:{}] [Unary.Recv Buffer: {}]", __FILE__, __LINE__, temp);
        result->SetResult(std::move(temp));
        result->SetSuccess(true);
        if(autoClose){
            Close();
        }
        result->Done();
    }
    else
    {
        if(!Connected()) 
        {
            auto res = SSLConnect(host, port);
            co_await res->Wait();
            if(!res->Success()) {
                result->SetErrorMsg("SSLConnect:" + res->Error());
                spdlog::error("[{}:{}] [Unary.SSLConnect(host:{}, port:{}) failed, Error:{}]", __FILE__, __LINE__, host, port, res->Error());
                result->Done();
                co_return coroutine::CoroutineStatus::kCoroutineFinished;
            }
        }
        auto res = SSLSend(std::move(request));
        co_await res->Wait();
        if(!res->Success()) {
            result->SetErrorMsg("SSLSend:" + res->Error());
            spdlog::error("[{}:{}] [Unary.SSLSend(buffer:{}) failed, Error:{}]", __FILE__, __LINE__, request, res->Error());
            result->Done();
            co_return coroutine::CoroutineStatus::kCoroutineFinished;
        }
        std::string temp;
        res = SSLRecv(temp);
        co_await res->Wait();
        if(!res->Success()) {
            result->SetErrorMsg("SSLRecv:" + res->Error());
            spdlog::error("[{}:{}] [Unary.SSLRecv(buffer:{}) failed, Error:{}]", __FILE__, __LINE__, request, res->Error());
            result->Done();
            co_return coroutine::CoroutineStatus::kCoroutineFinished;
        }
        spdlog::debug("[{}:{}] [Unary.SSLRecv Buffer: {}]", __FILE__, __LINE__, temp);
        result->SetResult(std::move(temp));
        result->SetSuccess(true);
        if(autoClose){
            SSLClose();
        }
        result->Done();
    }
    co_return coroutine::CoroutineStatus::kCoroutineFinished;
}

galay::coroutine::GY_NetCoroutine 
GY_TcpClient::Unary(std::string host, uint16_t port, std::queue<std::string> requests, std::shared_ptr<result::NetResultInner> result, bool autoClose)
{
    std::queue<std::string> responses;
    while(!requests.empty())
    {
        std::string request = requests.front();
        requests.pop();
        std::shared_ptr<result::NetResultInner> res = std::make_shared<result::NetResultInner>();
        res->AddTaskNum(1);
        Unary(host, port, std::move(request), res, autoClose);
        co_await res->Wait();
        if(!res->Success()) {
            result->SetErrorMsg(res->Error());
            result->SetResult(responses);
            spdlog::error("[{}:{}] [UnaryError:{}]", __FILE__, __LINE__, res->Error());
            result->Done();
            co_return coroutine::CoroutineStatus::kCoroutineFinished;
        } 
        else
        {
            responses.push(std::any_cast<std::string>(res->Result()));
        }
    }
    result->SetSuccess(true);
    result->SetResult(responses);
    result->Done();
    co_return coroutine::CoroutineStatus::kCoroutineFinished;
}

std::shared_ptr<result::NetResult>
GY_TcpClient::CloseConn()
{
    if(this->m_isSSL) 
    {
        return Close();
    }
    else
    {
        return SSLClose();
    }
}

result::NetResult::ptr 
GY_TcpClient::Socket()
{
    result::NetResultInner::ptr result = std::make_shared<result::NetResultInner>();
    this->m_fd = IOFunction::NetIOFunction::TcpFunction::Sock();
    if(this->m_fd <= 0) {
        spdlog::error("[{}:{}] [Socket(fd :{}) failed]", __FILE__, __LINE__, this->m_fd);
        result->SetErrorMsg(strerror(errno));
        goto end;
    }
    spdlog::debug("[{}:{}] [Socket(fd :{}) success]", __FILE__, __LINE__, this->m_fd);
    if(IOFunction::NetIOFunction::TcpFunction::IO_Set_No_Block(this->m_fd) == -1){
        spdlog::error("[{}:{}] [IO_Set_No_Block(fd:{}) failed, Error:{}]", __FILE__, __LINE__, this->m_fd, strerror(errno));
        result->SetErrorMsg(strerror(errno));
        goto end;
    }
    result->SetSuccess(true);
    spdlog::debug("[{}:{}] [IO_Set_No_Block(fd:{}) success]", __FILE__, __LINE__, this->m_fd);
    this->m_isSocket = true;
end:
    return result;
}

result::NetResult::ptr  
GY_TcpClient::Connect(const std::string &ip, uint16_t port)
{
    result::NetResultInner::ptr result = std::make_shared<result::NetResultInner>();
    int ret = IOFunction::NetIOFunction::TcpFunction::Conncet(this->m_fd, ip , port);
    if (ret < 0 )
    {
        if(errno != EINPROGRESS)
        {
            spdlog::error("[{}:{}] [Connect(fd:{}) to {}:{} failed, error is '{}']", __FILE__, __LINE__, this->m_fd, ip, port, strerror(errno));
            m_isConnected = false;
            result->SetErrorMsg(strerror(errno));
            Close();
            goto end;
        }
        else
        {
            result->AddTaskNum(1);
            objector::GY_ClientExcutor::ptr executor = std::make_shared<objector::GY_ClientExcutor>();
            executor->OnWrite() += [this,result](){
                int error = 0;
                socklen_t len = sizeof(error);
                getsockopt(this->m_fd, SOL_SOCKET, SO_ERROR, &error, &len);
                if(error){
                    SetResult(result, false, strerror(error));
                    m_isConnected = false;
                    spdlog::error("[{}:{}] [GY_ClientBase::Connect] connect error: {}", __FILE__, __LINE__, strerror(errno));
                }else{
                    SetResult(result, true, "");
                    m_isConnected = true;
                    spdlog::debug("[{}:{}] [GY_ClientBase::Connect] connect success", __FILE__, __LINE__);
                }
                m_scheduler.lock()->DelEvent(this->m_fd, poller::kEventWrite | poller::kEventEpollET);
                result->Done();
            };
            m_scheduler.lock()->RegisterObjector(this->m_fd, executor);
            m_scheduler.lock()->AddEvent(this->m_fd, poller::kEventWrite | poller::kEventEpollET);
            result->SetErrorMsg("Waiting");
            goto end;
        }
    }
    m_isConnected = true;
    result->SetSuccess(true);
    spdlog::debug("[{}:{}] [GY_ClientBase::Connect] connect success", __FILE__, __LINE__);

end:
    return result;
}

result::NetResult::ptr  
GY_TcpClient::Send(std::string &&buffer)
{
    result::NetResultInner::ptr result = std::make_shared<result::NetResultInner>();
    if(!m_sendTask)
    {
        m_sendTask = std::make_shared<task::GY_TcpSendTask>(this->m_fd, this->m_ssl, this->m_scheduler);
    }
    m_sendTask->AppendWBuffer(std::forward<std::string>(buffer));
    m_sendTask->SendAll();
    if(m_sendTask->Empty()){
        result->SetSuccess(true);
        goto end;
    }else{
        result->AddTaskNum(1);
        objector::GY_ClientExcutor::ptr executor = std::make_shared<objector::GY_ClientExcutor>();
        executor->OnWrite() += [this, result](){
            m_sendTask->SendAll();
            SetResult(result, m_sendTask->Success(), m_sendTask->Error());
            if(m_sendTask->Empty()){
                m_scheduler.lock()->DelEvent(this->m_fd, poller::kEventWrite | poller::kEventEpollET);
                result->Done();
            }
        };
        result->SetErrorMsg("Waiting");
        m_scheduler.lock()->RegisterObjector(this->m_fd, executor);
        m_scheduler.lock()->AddEvent(this->m_fd, poller::kEventWrite | poller::kEventEpollET);
    }
end:
    return result;
}

result::NetResult::ptr
GY_TcpClient::Recv(std::string &buffer)
{
    result::NetResultInner::ptr result = std::make_shared<result::NetResultInner>();
    if(!m_recvTask){
        m_recvTask = std::make_shared<task::GY_TcpRecvTask>(this->m_fd, this->m_ssl, this->m_scheduler);
    }
    m_recvTask->RecvAll();
    if(!m_recvTask->GetRBuffer().empty()){
        result->SetSuccess(true);
        buffer.assign(m_recvTask->GetRBuffer().begin(), m_recvTask->GetRBuffer().end());
        m_recvTask->GetRBuffer().clear();
    }else{
        result->AddTaskNum(1);
        objector::GY_ClientExcutor::ptr executor = std::make_shared<objector::GY_ClientExcutor>();
        executor->OnRead() += [this, result, &buffer](){
            m_recvTask->RecvAll();
            SetResult(result, m_recvTask->Success(), m_recvTask->Error());
            buffer.assign(m_recvTask->GetRBuffer().begin(), m_recvTask->GetRBuffer().end());
            m_recvTask->GetRBuffer().clear();
            m_scheduler.lock()->DelEvent(this->m_fd, poller::kEventRead);
            result->Done();
        };
        result->SetErrorMsg("Waiting");
        m_scheduler.lock()->RegisterObjector(this->m_fd, executor);
        m_scheduler.lock()->AddEvent(this->m_fd, poller::kEventRead | poller::kEventEpollET);
    }
end:
    return result;
}

result::NetResult::ptr 
GY_TcpClient::Close()
{
    result::NetResultInner::ptr result = std::make_shared<result::NetResultInner>();
    if(this->m_isSocket)
    {
        if(close(this->m_fd) == -1) {
            spdlog::error("[{}:{}] [Close(fd:{}) failed, Error:{}]", __FILE__, __LINE__, this->m_fd, strerror(errno));
            result->SetErrorMsg(strerror(errno));
        }else{
            result->SetSuccess(true);
            this->m_isSocket = false;
        }
        if(m_isConnected) m_isConnected = false;
    }
    else 
    {
        result->SetErrorMsg("Not Call Scokect");
    }
    return result;
}

result::NetResult::ptr  
GY_TcpClient::SSLSocket(long minVersion, long maxVersion)
{
    result::NetResultInner::ptr result = std::make_shared<result::NetResultInner>();
    this->m_ctx = IOFunction::NetIOFunction::TcpFunction::SSL_Init_Client(minVersion, maxVersion);
    if(this->m_ctx == nullptr) {
        unsigned long error = ERR_get_error();
        spdlog::error("[{}:{}] [SSL_Init_Client failed, Error:{}]", __FILE__, __LINE__, ERR_error_string(error, nullptr));
        result->SetErrorMsg(ERR_error_string(error, nullptr));
        goto end;
    }
    this->m_fd = IOFunction::NetIOFunction::TcpFunction::Sock();
    if(this->m_fd <= 0) {
        spdlog::error("[{}:{}] [Socket(fd :{}) failed]", __FILE__, __LINE__, this->m_fd);
        result->SetErrorMsg(strerror(errno));
        goto end;
    }
    spdlog::debug("[{}:{}] [Socket(fd :{}) success]", __FILE__, __LINE__, this->m_fd);
    this->m_ssl = IOFunction::NetIOFunction::TcpFunction::SSLCreateObj(this->m_ctx, this->m_fd);
    if(this->m_ssl == nullptr){
        unsigned long error = ERR_get_error();
        spdlog::error("[{}:{}] [SSLCreateObj failed, Error:{}]", __FILE__, __LINE__, ERR_error_string(error, nullptr));
        IOFunction::NetIOFunction::TcpFunction::SSLDestory({}, this->m_ctx);
        this->m_ctx = nullptr;
        result->SetErrorMsg(ERR_error_string(error, nullptr));
        goto end;
    }
    result->SetSuccess(true);
    this->m_isSocket = true;
    spdlog::debug("[{}:{}] [SSLCreateObj(fd:{}) success]", __FILE__, __LINE__, this->m_fd);
end:
    return result;
}


result::NetResult::ptr 
GY_TcpClient::SSLConnect(const std::string &ip, uint16_t port)
{
    result::NetResultInner::ptr result = std::make_shared<result::NetResultInner>();
    int ret = IOFunction::NetIOFunction::TcpFunction::Conncet(this->m_fd, ip , port);
    if (ret < 0 )
    {
        if(errno != EINPROGRESS)
        {
            m_isConnected = false;
            spdlog::error("[{}:{}] [Connect(fd:{}) to {}:{} failed, error is '{}']", __FILE__, __LINE__, this->m_fd, ip, port, strerror(errno));
            result->SetErrorMsg(strerror(errno));
            Close();
            return result;
        }
        else
        {
            result->AddTaskNum(1);
            // connect 回调
            objector::GY_ClientExcutor::ptr executor = std::make_shared<objector::GY_ClientExcutor>();
            executor->OnWrite() += [this,result](){
                int error = 0;
                socklen_t len = sizeof(error);
                getsockopt(this->m_fd, SOL_SOCKET, SO_ERROR, &error, &len);
                if(error){
                    m_isConnected = false;
                    SetResult(result, false, strerror(error));
                    spdlog::error("[{}:{}] [GY_ClientBase::Connect] connect error: {}", __FILE__, __LINE__, strerror(errno));
                    m_scheduler.lock()->DelEvent(this->m_fd, poller::kEventWrite | poller::kEventRead | poller::kEventError);
                    result->Done();
                }else{
                    m_isConnected = true;
                    SSL_set_connect_state(m_ssl);
                    //ssl connect回调
                    auto func = [this, result](){
                        int r = SSL_do_handshake(m_ssl);
                        if (r == 1){
                            SetResult(result, true, "");
                            spdlog::debug("[{}:{}] [GY_ClientBase::Connect] SSL_do_handshake success", __FILE__, __LINE__);
                            m_scheduler.lock()->DelEvent(this->m_fd, poller::kEventWrite | poller::kEventRead | poller::kEventError);
                            result->Done();
                            return true;
                        }
                        int err = SSL_get_error(m_ssl, r);
                        if (err == SSL_ERROR_WANT_WRITE) {
                            SetResult(result, false, "SSL_ERROR_WANT_WRITE");
                            spdlog::debug("[{}:{}] [SSL_do_handshake SSL_ERROR_WANT_WRITE]", __FILE__, __LINE__);
                            m_scheduler.lock()->ModEvent(this->m_fd, poller::kEventRead, poller::kEventWrite);
                        } else if (err == SSL_ERROR_WANT_READ) {
                            SetResult(result, false, "SSL_ERROR_WANT_READ");
                            spdlog::debug("[{}:{}] [SSL_do_handshake SSL_ERROR_WANT_READ]", __FILE__, __LINE__);
                            m_scheduler.lock()->ModEvent(this->m_fd, poller::kEventWrite, poller::kEventRead);
                        } else {
                            SetResult(result, false, strerror(errno));
                            spdlog::error("[{}:{}] [SSL_do_handshake error:{}]", __FILE__, __LINE__, strerror(errno));
                            m_scheduler.lock()->DelEvent(this->m_fd, poller::kEventWrite | poller::kEventRead | poller::kEventError);
                            result->Done();
                            return true;
                        }
                        return false;
                    };
                    bool finish = func();
                    if(!finish){
                        objector::GY_ClientExcutor::ptr executor = std::make_shared<objector::GY_ClientExcutor>();
                        executor->OnWrite() += [this, result, func](){
                            func();
                        };
                        m_scheduler.lock()->RegisterObjector(this->m_fd, executor);
                    }
                }
            };
            m_scheduler.lock()->RegisterObjector(this->m_fd, executor);
            m_scheduler.lock()->AddEvent(this->m_fd, poller::kEventWrite);
            result->SetErrorMsg("Waiting");
            return result;
        }
    } 
    else 
    {
        m_isConnected = true;
        SSL_set_connect_state(m_ssl);
        //ssl connect回调
        auto func = [this, result](){
            int r = SSL_do_handshake(m_ssl);
            if (r == 1){
                SetResult(result, true, "");
                spdlog::debug("[{}:{}] [GY_ClientBase::SSLConnect] SSL_do_handshake success", __FILE__, __LINE__);
                m_scheduler.lock()->DelEvent(this->m_fd, poller::kEventWrite | poller::kEventRead | poller::kEventError);
                result->Done();
                return true;
            }
            int err = SSL_get_error(m_ssl, r);
            if (err == SSL_ERROR_WANT_WRITE) {
                SetResult(result, false, "SSL_ERROR_WANT_WRITE");
                spdlog::debug("[{}:{}] [SSL_do_handshake SSL_ERROR_WANT_WRITE]", __FILE__, __LINE__);
                m_scheduler.lock()->ModEvent(this->m_fd, poller::kEventRead, poller::kEventWrite);
            } else if (err == SSL_ERROR_WANT_READ) {
                SetResult(result, false, "SSL_ERROR_WANT_READ");
                spdlog::debug("[{}:{}] [SSL_do_handshake SSL_ERROR_WANT_READ]", __FILE__, __LINE__);
                m_scheduler.lock()->ModEvent(this->m_fd, poller::kEventWrite, poller::kEventRead);
            } else {
                char err_string[256];
                ERR_error_string_n(err, err_string, sizeof(err_string));
                std::string errStr = err_string;
                spdlog::error("[{}:{}] [SSL_do_handshake error:{}]", __FILE__, __LINE__, errStr);
                SetResult(result, false, std::move(errStr));
                m_scheduler.lock()->DelEvent(this->m_fd, poller::kEventWrite | poller::kEventRead | poller::kEventError);
                result->Done();
                return true;
            }
            return false;
        };
        bool finish = func();
        if(!finish){
            objector::GY_ClientExcutor::ptr executor = std::make_shared<objector::GY_ClientExcutor>();
            result->AddTaskNum(1);
            executor->OnWrite() += [this, result, func](){
                func();
            };
            m_scheduler.lock()->RegisterObjector(this->m_fd, executor);
        }
        else
        {
            result->SetSuccess(true);
            spdlog::debug("[{}:{}] [GY_ClientBase::SSLConnect] SSLConnect success", __FILE__, __LINE__);
        }
    }
end:
    return result;
}

result::NetResult::ptr 
GY_TcpClient::SSLSend(std::string &&buffer)
{
    return Send(std::forward<std::string>(buffer));
}

result::NetResult::ptr 
GY_TcpClient::SSLRecv(std::string &buffer)
{
    return Recv(buffer);
}

result::NetResult::ptr 
GY_TcpClient::SSLClose()
{
    result::NetResultInner::ptr result = std::make_shared<result::NetResultInner>();
    if(this->m_isSocket)
    {
        if(this->m_ssl) IOFunction::NetIOFunction::TcpFunction::SSLDestory(this->m_ssl);
        if(this->m_ctx) IOFunction::NetIOFunction::TcpFunction::SSLDestory({},this->m_ctx);
        if(close(this->m_fd) == -1) {
            spdlog::error("[{}:{}] [Close(fd:{}) failed, Error:{}]", __FILE__, __LINE__, this->m_fd, strerror(errno));
            result->SetErrorMsg(strerror(errno));
        }else{
            this->m_isSocket = false;
            if(this->m_isConnected) this->m_isConnected = false;
            result->SetSuccess(true);
        }
    }
    else
    {
        result->SetErrorMsg("Not Call SSLSocket");
    }
    return result;
}

bool 
GY_TcpClient::Socketed()
{
    return this->m_isSocket;
}

bool 
GY_TcpClient::Connected()
{
    return this->m_isConnected;
}

GY_TcpClient::~GY_TcpClient()
{
    this->m_scheduler.lock()->DelObjector(this->m_fd);
}

void 
GY_TcpClient::SetResult(result::NetResultInner::ptr result, bool success, std::string &&errMsg)
{
    result->SetErrorMsg(std::forward<std::string>(errMsg));
    result->SetSuccess(success);
}


GY_HttpAsyncClient::GY_HttpAsyncClient(std::weak_ptr<poller::GY_IOScheduler> scheduler)
{
    this->m_tcpClient = std::make_shared<GY_TcpClient>(scheduler);
}

result::HttpResult::ptr 
GY_HttpAsyncClient::Get(const std::string &url, protocol::http::HttpRequest::ptr request, bool autoClose)
{
    result::HttpResultInner::ptr result = std::make_shared<result::HttpResultInner>();
    result->AddTaskNum(1);
    std::tuple<std::string,uint16_t,std::string> res = ParseUrl(url);
    request->Header()->Uri() = std::get<2>(res);
    request->Header()->Method() = "GET";
    this->m_tcpClient->Unary(std::get<0>(res), std::get<1>(res), request->EncodePdu(), result, autoClose);
    return result;
}

result::HttpResult::ptr 
GY_HttpAsyncClient::Post(const std::string &url, protocol::http::HttpRequest::ptr request, bool autoClose)
{
    result::HttpResultInner::ptr result = std::make_shared<result::HttpResultInner>();
    result->AddTaskNum(1);
    std::tuple<std::string,uint16_t,std::string> res = ParseUrl(url);
    request->Header()->Uri() = std::get<2>(res);
    request->Header()->Method() = "POST";
    this->m_tcpClient->Unary(std::get<0>(res), std::get<1>(res), request->EncodePdu(), result, autoClose);
    return result;
}

result::HttpResult::ptr 
GY_HttpAsyncClient::Options(const std::string &url, protocol::http::HttpRequest::ptr request, bool autoClose)
{
    result::HttpResultInner::ptr result = std::make_shared<result::HttpResultInner>();
    result->AddTaskNum(1);
    std::tuple<std::string,uint16_t,std::string> res = ParseUrl(url);
    request->Header()->Uri() = std::get<2>(res);
    request->Header()->Method() = "OPTIONS";
    this->m_tcpClient->Unary(std::get<0>(res), std::get<1>(res), request->EncodePdu(), result, autoClose);
    return result;
}

result::HttpResult::ptr 
GY_HttpAsyncClient::Put(const std::string &url, protocol::http::HttpRequest::ptr request, bool autoClose)
{
    result::HttpResultInner::ptr result = std::make_shared<result::HttpResultInner>();
    result->AddTaskNum(1);
    std::tuple<std::string,uint16_t,std::string> res = ParseUrl(url);
    request->Header()->Uri() = std::get<2>(res);
    request->Header()->Method() = "PUT";
    this->m_tcpClient->Unary(std::get<0>(res), std::get<1>(res), request->EncodePdu(), result, autoClose);
    return result;
}

result::HttpResult::ptr 
GY_HttpAsyncClient::Delete(const std::string &url, protocol::http::HttpRequest::ptr request, bool autoClose)
{
    result::HttpResultInner::ptr result = std::make_shared<result::HttpResultInner>();
    result->AddTaskNum(1);
    std::tuple<std::string,uint16_t,std::string> res = ParseUrl(url);
    request->Header()->Uri() = std::get<2>(res);
    request->Header()->Method() = "DELETE";
    this->m_tcpClient->Unary(std::get<0>(res), std::get<1>(res), request->EncodePdu(), result, autoClose);
    return result;
}

result::HttpResult::ptr 
GY_HttpAsyncClient::Head(const std::string &url, protocol::http::HttpRequest::ptr request, bool autoClose)
{
    result::HttpResultInner::ptr result = std::make_shared<result::HttpResultInner>();
    result->AddTaskNum(1);
    std::tuple<std::string,uint16_t,std::string> res = ParseUrl(url);
    request->Header()->Uri() = std::get<2>(res);
    request->Header()->Method() = "HEAD";
    this->m_tcpClient->Unary(std::get<0>(res), std::get<1>(res), request->EncodePdu(), result, autoClose);
    return result;
}

result::HttpResult::ptr 
GY_HttpAsyncClient::Trace(const std::string &url, protocol::http::HttpRequest::ptr request, bool autoClose)
{
    result::HttpResultInner::ptr result = std::make_shared<result::HttpResultInner>();
    result->AddTaskNum(1);
    std::tuple<std::string,uint16_t,std::string> res = ParseUrl(url);
    request->Header()->Uri() = std::get<2>(res);
    request->Header()->Method() = "TRACE";
    this->m_tcpClient->Unary(std::get<0>(res), std::get<1>(res), request->EncodePdu(), result, autoClose);
    return result;
}

result::HttpResult::ptr 
GY_HttpAsyncClient::Patch(const std::string &url, protocol::http::HttpRequest::ptr request, bool autoClose)
{
    result::HttpResultInner::ptr result = std::make_shared<result::HttpResultInner>();
    result->AddTaskNum(1);
    std::tuple<std::string,uint16_t,std::string> res = ParseUrl(url);
    request->Header()->Uri() = std::get<2>(res);
    request->Header()->Method() = "PATCH";
    this->m_tcpClient->Unary(std::get<0>(res), std::get<1>(res), request->EncodePdu(), result, autoClose);
    return result;
}

std::shared_ptr<result::NetResult> 
GY_HttpAsyncClient::CloseConn()
{
    result::HttpResultInner::ptr result = std::make_shared<result::HttpResultInner>();
    return m_tcpClient->CloseConn();
}

std::tuple<std::string,uint16_t,std::string> 
GY_HttpAsyncClient::ParseUrl(const std::string& url)
{
    std::regex uriRegex(R"((http|https):\/\/(\d{1,3}.\d{1,3}.\d{1,3}.\d{1,3})(:\d+)?(/.*)?)");
    std::smatch match;
    std::tuple<std::string,uint16_t,std::string> res;
    std::string uri,host;
    uint16_t port;
    if (std::regex_search(url, match, uriRegex))
    {
        host = match.str(2);
        if (match[3].matched)
        {
            std::string port_str = match.str(3);
            port_str.erase(0, 1);
            port = std::stoi(port_str);
        } else {
            if(match[1].str() == "http"){
                port = 80;
            }else if(match[1].str() == "https"){
                port = 443;
                m_tcpClient->IsSSL(true);
            }
        }
        uri = match.str(4).empty() ? "/" : match.str(4);
    }
    else
    {
        spdlog::error("[{}:{}] [[Invalid url] [Url:{}]] ", __FILE__, __LINE__, url);
        return {};
    }
    res = std::make_tuple(host,port,uri);
    return res;
}

GY_SmtpAsyncClient::GY_SmtpAsyncClient(std::weak_ptr<poller::GY_IOScheduler> scheduler)
{
    this->m_tcpClient = std::make_shared<GY_TcpClient>(scheduler);
}

void 
GY_SmtpAsyncClient::Connect(const std::string& url)
{
    std::tuple<std::string,uint16_t> res = ParseUrl(url);
    this->m_host = std::get<0>(res);
    this->m_port = std::get<1>(res);
}

std::shared_ptr<result::SmtpResult> 
GY_SmtpAsyncClient::Auth(std::string account, std::string password)
{
    result::SmtpResultInner::ptr result = std::make_shared<result::SmtpResultInner>();
    result->AddTaskNum(1);
    std::queue<std::string> requests;
    protocol::smtp::SmtpRequest request;
    requests.push(protocol::smtp::SmtpHelper::Hello(request));
    requests.push(protocol::smtp::SmtpHelper::Auth(request));
    requests.push(protocol::smtp::SmtpHelper::Account(request, account));
    requests.push(protocol::smtp::SmtpHelper::Password(request, password));
    this->m_tcpClient->Unary(this->m_host, this->m_port, requests, result, false);
    return result;
}

std::shared_ptr<result::SmtpResult> 
GY_SmtpAsyncClient::SendEmail(std::string FromEmail, const std::vector<std::string> &ToEmails, galay::protocol::smtp::SmtpMsgInfo msg)
{
    result::SmtpResultInner::ptr result = std::make_shared<result::SmtpResultInner>();
    result->AddTaskNum(1);
    std::queue<std::string> requests;
    protocol::smtp::SmtpRequest request;
    requests.push(protocol::smtp::SmtpHelper::MailFrom(request, FromEmail));
    for (auto &to : ToEmails)
    {
        requests.push(protocol::smtp::SmtpHelper::RcptTo(request, to));
    }
    requests.push(protocol::smtp::SmtpHelper::Data(request));
    requests.push(protocol::smtp::SmtpHelper::Msg(request, msg));
    this->m_tcpClient->Unary(this->m_host, this->m_port, requests, result, false);
    return result;
}

std::shared_ptr<result::SmtpResult> 
GY_SmtpAsyncClient::Quit()
{
    result::SmtpResultInner::ptr result = std::make_shared<result::SmtpResultInner>();
    result->AddTaskNum(1);
    std::queue<std::string> requests;
    protocol::smtp::SmtpRequest request;
    requests.push(protocol::smtp::SmtpHelper::Quit(request));
    this->m_tcpClient->Unary(this->m_host, this->m_port, requests, result, false);
    return result;
}

std::shared_ptr<result::NetResult>  
GY_SmtpAsyncClient::CloseConn()
{
    return m_tcpClient->CloseConn();
}

std::tuple<std::string,uint16_t> 
GY_SmtpAsyncClient::ParseUrl(const std::string& url)
{
    std::regex uriRegex(R"((smtp|smtps):\/\/(\d{1,3}.\d{1,3}.\d{1,3}.\d{1,3})(:\d+)?)");
    std::smatch match;
    std::tuple<std::string,uint16_t> res;
    std::string uri,host;
    uint16_t port;
    if (std::regex_search(url, match, uriRegex))
    {
        host = match.str(2);
        if (match[3].matched)
        {
            std::string port_str = match.str(3);
            port_str.erase(0, 1);
            port = std::stoi(port_str);
        } 
        else 
        {
            if(match[1].str() == "smtp"){
                port = 25;
            }else if(match[1].str() == "smtps"){
                port = 465;
                m_tcpClient->IsSSL(true);
            }
        }
    }
    else
    {
        spdlog::error("[{}:{}] [[Invalid url] [Url:{}]] ", __FILE__, __LINE__, url);
        return {};
    }
    res = std::make_tuple(host,port);
    return res;
}


GY_UdpClient::GY_UdpClient(std::weak_ptr<poller::GY_IOScheduler> scheduler)
{
    this->m_scheduler = scheduler;
    this->m_isSocketed = false;
}

galay::coroutine::GY_NetCoroutine 
GY_UdpClient::Unary(std::string host, uint16_t port, std::string &&buffer, std::shared_ptr<result::NetResultInner> result)
{
    result->AddTaskNum(1);
    if(!this->m_isSocketed)
    {
        Socket();
        this->m_isSocketed = true;
    }
    auto res = SendTo(host, port, std::forward<std::string>(buffer));
    co_await res->Wait();
    if(!res->Success())
    {
        result->SetErrorMsg(res->Error());
        spdlog::error("[{}:{}] [Unary.SendTo(fd:{}, host:{}, port:{}) failed, Error:{}]", __FILE__, __LINE__, this->m_fd, host, port, res->Error());
        result->Done();
        co_return coroutine::CoroutineStatus::kCoroutineFinished;
    } 
    result::UdpResInfo info;
    res = RecvFrom(info.m_host, info.m_port, info.m_buffer);
    co_await res->Wait();
    if(!res->Success())
    {
        result->SetErrorMsg(res->Error());
        spdlog::error("[{}:{}] [Unary.RecvFrom(fd:{}, host:{}, port:{}) failed, Error:{}]", __FILE__, __LINE__, this->m_fd, host, port, res->Error());
        result->Done();
        co_return coroutine::CoroutineStatus::kCoroutineFinished;
    }
    result->SetResult(info);
    result->SetSuccess(true);
    result->Done();
    co_return coroutine::CoroutineStatus::kCoroutineFinished;
}

std::shared_ptr<result::NetResult> 
GY_UdpClient::Socket()
{
    result::NetResultInner::ptr result = std::make_shared<result::NetResultInner>();
    this->m_fd = IOFunction::NetIOFunction::UdpFunction::Sock();
    if(this->m_fd <= 0) {
        spdlog::error("[{}:{}] [Socket(fd :{}) failed]", __FILE__, __LINE__, this->m_fd);
        result->SetErrorMsg(strerror(errno));
        goto end;
    }
    spdlog::debug("[{}:{}] [Socket(fd :{}) success]", __FILE__, __LINE__, this->m_fd);
    if(IOFunction::NetIOFunction::UdpFunction::IO_Set_No_Block(this->m_fd) == -1){
        spdlog::error("[{}:{}] [IO_Set_No_Block(fd:{}) failed, Error:{}]", __FILE__, __LINE__, this->m_fd, strerror(errno));
        result->SetErrorMsg(strerror(errno));
        goto end;
    }
    result->SetSuccess(true);
    spdlog::debug("[{}:{}] [IO_Set_No_Block(fd:{}) success]", __FILE__, __LINE__, this->m_fd);
    this->m_isSocketed = true;
end:
    return result;
}

std::shared_ptr<result::NetResult> 
GY_UdpClient::SendTo(std::string host, uint16_t port, std::string&& buffer)
{
    this->m_request = std::forward<std::string>(buffer);
    result::NetResultInner::ptr result = std::make_shared<result::NetResultInner>();
    IOFunction::NetIOFunction::Addr addr;
    addr.ip = host;
    addr.port = port;
    int len = galay::IOFunction::NetIOFunction::UdpFunction::SendTo(this->m_fd, addr, this->m_request);
    if(len == -1){
        spdlog::error("[{}:{}] [SendTo(fd:{}, host:{}, port:{}) failed, Error:{}]", __FILE__, __LINE__, this->m_fd, host, port, strerror(errno));
        result->SetErrorMsg(strerror(errno));
        goto end;
    }
    this->m_request.erase(0, len);
    spdlog::debug("[{}:{}] [SendTo(fd:{}, host:{}, port:{}), len:{}]", __FILE__, __LINE__, this->m_fd, host, port, len);
    if(this->m_request.empty()){
        spdlog::debug("[{}:{}] [SendTo All]", __FILE__, __LINE__);
        result->SetSuccess(true);
        goto end;
    }else{
        result->AddTaskNum(1);
        objector::GY_ClientExcutor::ptr executor = std::make_shared<objector::GY_ClientExcutor>();
        executor->OnWrite() += [this, addr, result](){
            int len = galay::IOFunction::NetIOFunction::UdpFunction::SendTo(m_fd, addr, m_request);
            spdlog::debug("[{}:{}] [SendTo(fd:{}, host:{}, port:{}), len:{}]", __FILE__, __LINE__, this->m_fd, addr.ip, addr.port, len);
            m_request.erase(0,len);   
            if(m_request.empty()){
                SetResult(result, true, "");
                m_scheduler.lock()->DelEvent(this->m_fd, poller::kEventWrite | poller::kEventEpollET);
                result->Done();
            }
        };
        result->SetErrorMsg("Waiting");
        m_scheduler.lock()->RegisterObjector(this->m_fd, executor);
        m_scheduler.lock()->AddEvent(this->m_fd, poller::kEventWrite | poller::kEventEpollET);
    }
end:
    return result;
}

std::shared_ptr<result::NetResult> 
GY_UdpClient::RecvFrom(std::string& host, uint16_t& port, std::string& buffer)
{
    result::NetResultInner::ptr result = std::make_shared<result::NetResultInner>();
    IOFunction::NetIOFunction::Addr addr;
    char buf[MAX_UDP_LENGTH];
    int len = galay::IOFunction::NetIOFunction::UdpFunction::RecvFrom(this->m_fd, addr, buf,MAX_UDP_LENGTH);
    if(len == -1)
    {
        if(errno != EWOULDBLOCK && errno != EAGAIN && errno != EINTR)
        {
            spdlog::error("[{}:{}] [RecvFrom(fd:{}, host:{}, port:{}) failed, Error:{}]", __FILE__, __LINE__, this->m_fd, host, port, strerror(errno));
            result->SetErrorMsg(strerror(errno));
        }
        else
        {
            result->AddTaskNum(1);
            objector::GY_ClientExcutor::ptr executor = std::make_shared<objector::GY_ClientExcutor>();
            executor->OnRead() += [this, result, &host, &port, &buffer](){
                spdlog::debug("[{}:{}] [Call RecvFrom.OnRead]", __FILE__, __LINE__);
                IOFunction::NetIOFunction::Addr addr;
                char buf[MAX_UDP_LENGTH];
                int len = galay::IOFunction::NetIOFunction::UdpFunction::RecvFrom(m_fd, addr, buf,MAX_UDP_LENGTH);
                if(len == -1)
                {
                    spdlog::error("[{}:{}] [RecvFrom(fd:{}, host:{}, port:{}) failed, Error:{}]", __FILE__, __LINE__, this->m_fd, host, port, strerror(errno));
                    SetResult(result, false, strerror(errno));
                }
                else
                {
                    spdlog::debug("[{}:{}] [RecvFrom(fd:{}, host:{}, port:{}), len:{}]", __FILE__, __LINE__, this->m_fd, addr.ip, addr.port, len);
                    buffer.assign(buf, len);
                    host = addr.ip;
                    port = addr.port;
                    SetResult(result, true, "");
                }
                m_scheduler.lock()->DelEvent(this->m_fd, poller::kEventRead | poller::kEventEpollET);
                result->Done();
            };
            result->SetErrorMsg("Waiting");
            m_scheduler.lock()->RegisterObjector(this->m_fd, executor);
            m_scheduler.lock()->AddEvent(this->m_fd, poller::kEventRead | poller::kEventEpollET);
        }
    }
    else
    {
        spdlog::debug("[{}:{}] [RecvFrom(fd:{}, host:{}, port:{}), len:{}]", __FILE__, __LINE__, this->m_fd, host, port, len);
        buffer.assign(buf, len);
        host = addr.ip;
        port = addr.port;
        result->SetSuccess(true);
    }
    return result;
}

std::shared_ptr<result::NetResult> 
GY_UdpClient::CloseSocket()
{
    result::NetResultInner::ptr result = std::make_shared<result::NetResultInner>();
    if(this->m_isSocketed)
    {
        if(close(this->m_fd) == -1) {
            spdlog::error("[{}:{}] [Close(fd:{}) failed, Error:{}]", __FILE__, __LINE__, this->m_fd, strerror(errno));
            result->SetErrorMsg(strerror(errno));
        }else{
            result->SetSuccess(true);
        }
    }
    else
    {
        result->SetErrorMsg("Not Call Socket");
    }
    return result;
}

bool 
GY_UdpClient::Socketed()
{
    return this->m_isSocketed;
}

GY_UdpClient::~GY_UdpClient()
{
    this->m_scheduler.lock()->DelObjector(this->m_fd);
}

void 
GY_UdpClient::SetResult(std::shared_ptr<result::NetResultInner> result, bool success, std::string &&errMsg)
{
    result->SetErrorMsg(std::forward<std::string>(errMsg));
    result->SetSuccess(success);
}


DnsAsyncClient::DnsAsyncClient(std::weak_ptr<poller::GY_IOScheduler> scheduler)
{
    this->m_udpClient = std::make_shared<GY_UdpClient>(scheduler);
}


std::shared_ptr<result::DnsResult> 
DnsAsyncClient::QueryA(const std::string& host, const uint16_t& port, const std::string& domain)
{
    result::DnsResultInner::ptr result = std::make_shared<result::DnsResultInner>();
    protocol::dns::DnsRequest request;
    protocol::dns::DnsHeader header;
    header.m_flags.m_rd = 1;
    header.m_id = galay::util::Random::RandomInt(0,MAX_UDP_LENGTH);
    header.m_questions = 1;
    galay::protocol::dns::DnsQuestion question;
    question.m_class = 1;
    question.m_qname = domain;
    question.m_type = galay::protocol::dns::kDnsQueryA;
    std::queue<galay::protocol::dns::DnsQuestion> questions;
    questions.push(question);
    request.SetQuestionQueue(std::move(questions));
    this->m_udpClient->Unary(host, port, request.EncodePdu(), result);
    return result;
}

std::shared_ptr<result::DnsResult> 
DnsAsyncClient::QueryAAAA(const std::string& host, const uint16_t& port, const std::string& domain)
{
    result::DnsResultInner::ptr result = std::make_shared<result::DnsResultInner>();
    protocol::dns::DnsRequest request;
    protocol::dns::DnsHeader header;
    header.m_flags.m_rd = 1;
    header.m_id = galay::util::Random::RandomInt(0,MAX_UDP_LENGTH);
    header.m_questions = 1;
    galay::protocol::dns::DnsQuestion question;
    question.m_class = 1;
    question.m_qname = domain;
    question.m_type = galay::protocol::dns::kDnsQueryAAAA;
    std::queue<galay::protocol::dns::DnsQuestion> questions;
    questions.push(question);
    request.SetQuestionQueue(std::move(questions));
    this->m_udpClient->Unary(host, port, request.EncodePdu(), result);
    return result;
}

std::shared_ptr<result::DnsResult> 
DnsAsyncClient::QueryPtr(const std::string& host, const uint16_t& port, const std::string& want)
{
    result::DnsResultInner::ptr result = std::make_shared<result::DnsResultInner>();
    protocol::dns::DnsRequest request;
    protocol::dns::DnsHeader header;
    header.m_flags.m_rd = 1;
    header.m_id = galay::util::Random::RandomInt(0,MAX_UDP_LENGTH);
    header.m_questions = 1;
    galay::protocol::dns::DnsQuestion question;
    question.m_class = 1;
    question.m_qname = HostToPtr(want);
    question.m_type = galay::protocol::dns::kDnsQueryPtr;
    std::queue<galay::protocol::dns::DnsQuestion> questions;
    questions.push(question);
    request.SetQuestionQueue(std::move(questions));
    this->m_udpClient->Unary(host, port, request.EncodePdu(), result);
    return result;
}

std::shared_ptr<result::NetResult> 
DnsAsyncClient::CloseSocket()
{
    return this->m_udpClient->CloseSocket();
}


std::string 
DnsAsyncClient::HostToPtr(const std::string& host)
{
    auto res = util::StringUtil::SpiltWithChar(host, '.');
    std::string Ptr;
    for(std::vector<std::string>::reverse_iterator rit = res.rbegin(); rit != res.rend(); ++rit)
    {
        Ptr = Ptr + *rit + '.';
    }
    return Ptr + "in-addr.arpa.";
}

}
