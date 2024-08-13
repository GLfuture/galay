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


galay::kernel::GY_TcpClient::GY_TcpClient(std::weak_ptr<GY_IOScheduler> scheduler)
{
    this->m_scheduler = scheduler;
    this->m_recvTask = nullptr;
    this->m_sendTask = nullptr;
    this->m_isConnected = false;
    this->m_isSocket = false;
    this->m_isSSL = false;
}

void 
galay::kernel::GY_TcpClient::IsSSL(bool isSSL)
{
    this->m_isSSL = isSSL;
}

galay::common::GY_NetCoroutine 
galay::kernel::GY_TcpClient::Unary(std::string host, uint16_t port, std::string &&buffer, NetResult::ptr result, bool autoClose)
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
                res->m_errMsg = "Connect:" + res->Error();
                spdlog::error("[{}:{}] [Unary.Connect(host:{}, port:{}) failed, Error:{}]", __FILE__, __LINE__, host, port, res->Error());
                result->Done();
                co_return common::CoroutineStatus::kCoroutineFinished;
            }
        }
        auto res = Send(std::move(request));
        co_await res->Wait();
        if(!res->Success()) {
            res->m_errMsg = "Send:" + res->Error();
            spdlog::error("[{}:{}] [Unary.Send(buffer:{}) failed, Error:{}]", __FILE__, __LINE__, request, res->Error());
            result->Done();
            co_return common::CoroutineStatus::kCoroutineFinished;
        }
        std::string temp;
        res = Recv(temp);
        co_await res->Wait();
        if(!res->Success()) {
            res->m_errMsg = "Send:" + res->Error();
            spdlog::error("[{}:{}] [Unary.Recv(buffer:{}) failed, Error:{}]", __FILE__, __LINE__, request, res->Error());
            result->Done();
            co_return common::CoroutineStatus::kCoroutineFinished;
        }
        spdlog::debug("[{}:{}] [Unary.Recv Buffer: {}]", __FILE__, __LINE__, temp);
        result->m_result = std::move(temp);
        result->m_success = true;
        if(!result->m_errMsg.empty()) result->m_errMsg.clear();
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
                res->m_errMsg = "SSLConnect:" + res->Error();
                spdlog::error("[{}:{}] [Unary.SSLConnect(host:{}, port:{}) failed, Error:{}]", __FILE__, __LINE__, host, port, res->Error());
                result->Done();
                co_return common::CoroutineStatus::kCoroutineFinished;
            }
        }
        auto res = SSLSend(std::move(request));
        co_await res->Wait();
        if(!res->Success()) {
            res->m_errMsg = "SSLSend:" + res->Error();
            spdlog::error("[{}:{}] [Unary.SSLSend(buffer:{}) failed, Error:{}]", __FILE__, __LINE__, request, res->Error());
            result->Done();
            co_return common::CoroutineStatus::kCoroutineFinished;
        }
        std::string temp;
        res = SSLRecv(temp);
        co_await res->Wait();
        if(!res->Success()) {
            res->m_errMsg = "SSLRecv:" + res->Error();
            spdlog::error("[{}:{}] [Unary.SSLRecv(buffer:{}) failed, Error:{}]", __FILE__, __LINE__, request, res->Error());
            result->Done();
            co_return common::CoroutineStatus::kCoroutineFinished;
        }
        spdlog::debug("[{}:{}] [Unary.SSLRecv Buffer: {}]", __FILE__, __LINE__, temp);
        result->m_result = std::move(temp);
        result->m_success = true;
        if(!result->m_errMsg.empty()) result->m_errMsg.clear();
        if(autoClose){
            SSLClose();
        }
        result->Done();
    }
    co_return common::CoroutineStatus::kCoroutineFinished;
}

galay::common::GY_NetCoroutine 
galay::kernel::GY_TcpClient::Unary(std::string host, uint16_t port, std::queue<std::string> requests, std::shared_ptr<NetResult> result, bool autoClose)
{
    std::queue<std::string> responses;
    while(!requests.empty())
    {
        std::string request = requests.front();
        requests.pop();
        std::shared_ptr<NetResult> res = std::make_shared<NetResult>();
        res->AddTaskNum(1);
        Unary(host, port, std::move(request), res, autoClose);
        co_await res->Wait();
        if(!res->Success()) {
            result->m_errMsg = res->Error();
            result->m_result = responses;
            spdlog::error("[{}:{}] [UnaryError:{}]", __FILE__, __LINE__, res->Error());
            result->Done();
            co_return common::CoroutineStatus::kCoroutineFinished;
        } 
        else
        {
            responses.push(std::any_cast<std::string>(res->m_result));
        }
    }
    result->m_success = true;
    result->m_result = responses;
    if(!result->m_errMsg.empty()) result->m_errMsg.clear();
    result->Done();
    co_return common::CoroutineStatus::kCoroutineFinished;
}

std::shared_ptr<galay::kernel::NetResult>
galay::kernel::GY_TcpClient::CloseConn()
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

galay::kernel::NetResult::ptr 
galay::kernel::GY_TcpClient::Socket()
{
    galay::kernel::NetResult::ptr result = std::make_shared<NetResult>();
    this->m_fd = IOFunction::NetIOFunction::TcpFunction::Sock();
    if(this->m_fd <= 0) {
        spdlog::error("[{}:{}] [Socket(fd :{}) failed]", __FILE__, __LINE__, this->m_fd);
        result->m_errMsg = strerror(errno);
        goto end;
    }
    spdlog::debug("[{}:{}] [Socket(fd :{}) success]", __FILE__, __LINE__, this->m_fd);
    if(IOFunction::NetIOFunction::TcpFunction::IO_Set_No_Block(this->m_fd) == -1){
        spdlog::error("[{}:{}] [IO_Set_No_Block(fd:{}) failed, Error:{}]", __FILE__, __LINE__, this->m_fd, strerror(errno));
        result->m_errMsg = strerror(errno);
        goto end;
    }
    result->m_success = true;
    spdlog::debug("[{}:{}] [IO_Set_No_Block(fd:{}) success]", __FILE__, __LINE__, this->m_fd);
    this->m_isSocket = true;
end:
    return result;
}

galay::kernel::NetResult::ptr  
galay::kernel::GY_TcpClient::Connect(const std::string &ip, uint16_t port)
{
    galay::kernel::NetResult::ptr result = std::make_shared<NetResult>();
    int ret = IOFunction::NetIOFunction::TcpFunction::Conncet(this->m_fd, ip , port);
    if (ret < 0 )
    {
        if(errno != EINPROGRESS)
        {
            spdlog::error("[{}:{}] [Connect(fd:{}) to {}:{} failed, error is '{}']", __FILE__, __LINE__, this->m_fd, ip, port, strerror(errno));
            m_isConnected = false;
            result->m_errMsg = strerror(errno);
            Close();
            goto end;
        }
        else
        {
            result->AddTaskNum(1);
            GY_ClientExcutor::ptr executor = std::make_shared<GY_ClientExcutor>();
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
                m_scheduler.lock()->DelEvent(this->m_fd, kEventWrite | kEventEpollET);
                result->Done();
            };
            m_scheduler.lock()->RegisterObjector(this->m_fd, executor);
            m_scheduler.lock()->AddEvent(this->m_fd, kEventWrite | kEventEpollET);
            result->m_errMsg = "Waiting";
            goto end;
        }
    }
    m_isConnected = true;
    result->m_success = true;
    spdlog::debug("[{}:{}] [GY_ClientBase::Connect] connect success", __FILE__, __LINE__);

end:
    return result;
}

galay::kernel::NetResult::ptr  
galay::kernel::GY_TcpClient::Send(std::string &&buffer)
{
    galay::kernel::NetResult::ptr result = std::make_shared<NetResult>();
    if(!m_sendTask)
    {
        m_sendTask = std::make_shared<galay::kernel::GY_TcpSendTask>(this->m_fd, this->m_ssl, this->m_scheduler);
    }
    m_sendTask->AppendWBuffer(std::forward<std::string>(buffer));
    m_sendTask->SendAll();
    if(m_sendTask->Empty()){
        result->m_success = true;
        goto end;
    }else{
        result->AddTaskNum(1);
        GY_ClientExcutor::ptr executor = std::make_shared<GY_ClientExcutor>();
        executor->OnWrite() += [this, result](){
            m_sendTask->SendAll();
            SetResult(result, m_sendTask->Success(), m_sendTask->Error());
            if(m_sendTask->Empty()){
                m_scheduler.lock()->DelEvent(this->m_fd, EventType::kEventWrite | kEventEpollET);
                result->Done();
            }
        };
        result->m_errMsg = "Waiting";
        m_scheduler.lock()->RegisterObjector(this->m_fd, executor);
        m_scheduler.lock()->AddEvent(this->m_fd, EventType::kEventWrite | kEventEpollET);
    }
end:
    return result;
}

galay::kernel::NetResult::ptr
galay::kernel::GY_TcpClient::Recv(std::string &buffer)
{
    galay::kernel::NetResult::ptr result = std::make_shared<NetResult>();
    if(!m_recvTask){
        m_recvTask = std::make_shared<galay::kernel::GY_TcpRecvTask>(this->m_fd, this->m_ssl, this->m_scheduler);
    }
    m_recvTask->RecvAll();
    if(!m_recvTask->GetRBuffer().empty()){
        result->m_success = true;
        buffer.assign(m_recvTask->GetRBuffer().begin(), m_recvTask->GetRBuffer().end());
        m_recvTask->GetRBuffer().clear();
    }else{
        result->AddTaskNum(1);
        GY_ClientExcutor::ptr executor = std::make_shared<GY_ClientExcutor>();
        executor->OnRead() += [this, result, &buffer](){
            m_recvTask->RecvAll();
            SetResult(result, m_recvTask->Success(), m_recvTask->Error());
            buffer.assign(m_recvTask->GetRBuffer().begin(), m_recvTask->GetRBuffer().end());
            m_recvTask->GetRBuffer().clear();
            m_scheduler.lock()->DelEvent(this->m_fd, EventType::kEventRead);
            result->Done();
        };
        result->m_errMsg = "Waiting";
        m_scheduler.lock()->RegisterObjector(this->m_fd, executor);
        m_scheduler.lock()->AddEvent(this->m_fd, EventType::kEventRead | EventType::kEventEpollET);
    }
end:
    return result;
}

galay::kernel::NetResult::ptr 
galay::kernel::GY_TcpClient::Close()
{
    galay::kernel::NetResult::ptr result = std::make_shared<NetResult>();
    if(this->m_isSocket)
    {
        if(close(this->m_fd) == -1) {
            spdlog::error("[{}:{}] [Close(fd:{}) failed, Error:{}]", __FILE__, __LINE__, this->m_fd, strerror(errno));
            result->m_errMsg = strerror(errno);
        }else{
            result->m_success = true;
            this->m_isSocket = false;
        }
        if(m_isConnected) m_isConnected = false;
    }
    else 
    {
        result->m_errMsg = "Not Socket";
        result->m_success = false;
    }
    return result;
}

galay::kernel::NetResult::ptr  
galay::kernel::GY_TcpClient::SSLSocket(long minVersion, long maxVersion)
{
    galay::kernel::NetResult::ptr result = std::make_shared<NetResult>();
    this->m_ctx = IOFunction::NetIOFunction::TcpFunction::SSL_Init_Client(minVersion, maxVersion);
    if(this->m_ctx == nullptr) {
        unsigned long error = ERR_get_error();
        spdlog::error("[{}:{}] [SSL_Init_Client failed, Error:{}]", __FILE__, __LINE__, ERR_error_string(error, nullptr));
        result->m_errMsg = ERR_error_string(error, nullptr);
        goto end;
    }
    this->m_fd = IOFunction::NetIOFunction::TcpFunction::Sock();
    if(this->m_fd <= 0) {
        spdlog::error("[{}:{}] [Socket(fd :{}) failed]", __FILE__, __LINE__, this->m_fd);
        result->m_errMsg = strerror(errno);
        goto end;
    }
    spdlog::debug("[{}:{}] [Socket(fd :{}) success]", __FILE__, __LINE__, this->m_fd);
    this->m_ssl = IOFunction::NetIOFunction::TcpFunction::SSLCreateObj(this->m_ctx, this->m_fd);
    if(this->m_ssl == nullptr){
        unsigned long error = ERR_get_error();
        spdlog::error("[{}:{}] [SSLCreateObj failed, Error:{}]", __FILE__, __LINE__, ERR_error_string(error, nullptr));
        IOFunction::NetIOFunction::TcpFunction::SSLDestory({}, this->m_ctx);
        this->m_ctx = nullptr;
        result->m_errMsg = ERR_error_string(error, nullptr);
        goto end;
    }
    result->m_success = true;
    this->m_isSocket = true;
    spdlog::debug("[{}:{}] [SSLCreateObj(fd:{}) success]", __FILE__, __LINE__, this->m_fd);
end:
    return result;
}


galay::kernel::NetResult::ptr 
galay::kernel::GY_TcpClient::SSLConnect(const std::string &ip, uint16_t port)
{
    galay::kernel::NetResult::ptr result = std::make_shared<NetResult>();
    int ret = IOFunction::NetIOFunction::TcpFunction::Conncet(this->m_fd, ip , port);
    if (ret < 0 )
    {
        if(errno != EINPROGRESS)
        {
            m_isConnected = false;
            spdlog::error("[{}:{}] [Connect(fd:{}) to {}:{} failed, error is '{}']", __FILE__, __LINE__, this->m_fd, ip, port, strerror(errno));
            result->m_errMsg = strerror(errno);
            Close();
            return result;
        }
        else
        {
            result->AddTaskNum(1);
            // connect 回调
            GY_ClientExcutor::ptr executor = std::make_shared<GY_ClientExcutor>();
            executor->OnWrite() += [this,result](){
                int error = 0;
                socklen_t len = sizeof(error);
                getsockopt(this->m_fd, SOL_SOCKET, SO_ERROR, &error, &len);
                if(error){
                    m_isConnected = false;
                    SetResult(result, false, strerror(error));
                    spdlog::error("[{}:{}] [GY_ClientBase::Connect] connect error: {}", __FILE__, __LINE__, strerror(errno));
                    m_scheduler.lock()->DelEvent(this->m_fd, kEventWrite | kEventRead | kEventError);
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
                            m_scheduler.lock()->DelEvent(this->m_fd, kEventWrite | kEventRead | kEventError);
                            result->Done();
                            return true;
                        }
                        int err = SSL_get_error(m_ssl, r);
                        if (err == SSL_ERROR_WANT_WRITE) {
                            SetResult(result, false, "SSL_ERROR_WANT_WRITE");
                            spdlog::debug("[{}:{}] [SSL_do_handshake SSL_ERROR_WANT_WRITE]", __FILE__, __LINE__);
                            m_scheduler.lock()->ModEvent(this->m_fd, kEventRead, kEventWrite);
                        } else if (err == SSL_ERROR_WANT_READ) {
                            SetResult(result, false, "SSL_ERROR_WANT_READ");
                            spdlog::debug("[{}:{}] [SSL_do_handshake SSL_ERROR_WANT_READ]", __FILE__, __LINE__);
                            m_scheduler.lock()->ModEvent(this->m_fd, kEventWrite, kEventRead);
                        } else {
                            SetResult(result, false, strerror(errno));
                            spdlog::error("[{}:{}] [SSL_do_handshake error:{}]", __FILE__, __LINE__, strerror(errno));
                            m_scheduler.lock()->DelEvent(this->m_fd, kEventWrite | kEventRead | kEventError);
                            result->Done();
                            return true;
                        }
                        return false;
                    };
                    bool finish = func();
                    if(!finish){
                        GY_ClientExcutor::ptr executor = std::make_shared<GY_ClientExcutor>();
                        executor->OnWrite() += [this, result, func](){
                            func();
                        };
                        m_scheduler.lock()->RegisterObjector(this->m_fd, executor);
                    }
                }
            };
            m_scheduler.lock()->RegisterObjector(this->m_fd, executor);
            m_scheduler.lock()->AddEvent(this->m_fd, kEventWrite);
            result->m_errMsg = "Waiting";
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
                m_scheduler.lock()->DelEvent(this->m_fd, kEventWrite | kEventRead | kEventError);
                result->Done();
                return true;
            }
            int err = SSL_get_error(m_ssl, r);
            if (err == SSL_ERROR_WANT_WRITE) {
                SetResult(result, false, "SSL_ERROR_WANT_WRITE");
                spdlog::debug("[{}:{}] [SSL_do_handshake SSL_ERROR_WANT_WRITE]", __FILE__, __LINE__);
                m_scheduler.lock()->ModEvent(this->m_fd, kEventRead, kEventWrite);
            } else if (err == SSL_ERROR_WANT_READ) {
                SetResult(result, false, "SSL_ERROR_WANT_READ");
                spdlog::debug("[{}:{}] [SSL_do_handshake SSL_ERROR_WANT_READ]", __FILE__, __LINE__);
                m_scheduler.lock()->ModEvent(this->m_fd, kEventWrite, kEventRead);
            } else {
                char err_string[256];
                ERR_error_string_n(err, err_string, sizeof(err_string));
                std::string errStr = err_string;
                spdlog::error("[{}:{}] [SSL_do_handshake error:{}]", __FILE__, __LINE__, errStr);
                SetResult(result, false, std::move(errStr));
                m_scheduler.lock()->DelEvent(this->m_fd, kEventWrite | kEventRead | kEventError);
                result->Done();
                return true;
            }
            return false;
        };
        bool finish = func();
        if(!finish){
            GY_ClientExcutor::ptr executor = std::make_shared<GY_ClientExcutor>();
            result->AddTaskNum(1);
            executor->OnWrite() += [this, result, func](){
                func();
            };
            m_scheduler.lock()->RegisterObjector(this->m_fd, executor);
        }
        else
        {
            result->m_success = true;
            spdlog::debug("[{}:{}] [GY_ClientBase::SSLConnect] SSLConnect success", __FILE__, __LINE__);
        }
    }
end:
    return result;
}

galay::kernel::NetResult::ptr 
galay::kernel::GY_TcpClient::SSLSend(std::string &&buffer)
{
    return Send(std::forward<std::string>(buffer));
}

galay::kernel::NetResult::ptr 
galay::kernel::GY_TcpClient::SSLRecv(std::string &buffer)
{
    return Recv(buffer);
}

galay::kernel::NetResult::ptr 
galay::kernel::GY_TcpClient::SSLClose()
{
    galay::kernel::NetResult::ptr result = std::make_shared<NetResult>();
    if(this->m_isSocket)
    {
        if(this->m_ssl) IOFunction::NetIOFunction::TcpFunction::SSLDestory(this->m_ssl);
        if(this->m_ctx) IOFunction::NetIOFunction::TcpFunction::SSLDestory({},this->m_ctx);
        if(close(this->m_fd) == -1) {
            spdlog::error("[{}:{}] [Close(fd:{}) failed, Error:{}]", __FILE__, __LINE__, this->m_fd, strerror(errno));
            result->m_errMsg = strerror(errno);
        }else{
            this->m_isSocket = false;
            if(this->m_isConnected) this->m_isConnected = false;
            result->m_success = true;
        }
    }
    else
    {
        result->m_errMsg = "Not SSLSocket";
        result->m_success = false;
    }
    return result;
}

bool 
galay::kernel::GY_TcpClient::Socketed()
{
    return this->m_isSocket;
}

bool 
galay::kernel::GY_TcpClient::Connected()
{
    return this->m_isConnected;
}

galay::kernel::GY_TcpClient::~GY_TcpClient()
{
    this->m_scheduler.lock()->DelObjector(this->m_fd);
}

void 
galay::kernel::GY_TcpClient::SetResult(NetResult::ptr result, bool success, std::string &&errMsg)
{
    result->m_success = success;
    result->m_errMsg = std::forward<std::string>(errMsg);
}


galay::kernel::GY_HttpAsyncClient::GY_HttpAsyncClient(std::weak_ptr<GY_IOScheduler> scheduler)
{
    this->m_tcpClient = std::make_shared<GY_TcpClient>(scheduler);
}

galay::kernel::HttpResult::ptr 
galay::kernel::GY_HttpAsyncClient::Get(const std::string &url, protocol::http::HttpRequest::ptr request, bool autoClose)
{
    HttpResult::ptr result = std::make_shared<HttpResult>();
    result->AddTaskNum(1);
    std::tuple<std::string,uint16_t,std::string> res = ParseUrl(url);
    request->Header()->Uri() = std::get<2>(res);
    request->Header()->Method() = "GET";
    this->m_tcpClient->Unary(std::get<0>(res), std::get<1>(res), request->EncodePdu(), result, autoClose);
    return result;
}

galay::kernel::HttpResult::ptr 
galay::kernel::GY_HttpAsyncClient::Post(const std::string &url, protocol::http::HttpRequest::ptr request, bool autoClose)
{
    HttpResult::ptr result = std::make_shared<HttpResult>();
    result->AddTaskNum(1);
    std::tuple<std::string,uint16_t,std::string> res = ParseUrl(url);
    request->Header()->Uri() = std::get<2>(res);
    request->Header()->Method() = "POST";
    this->m_tcpClient->Unary(std::get<0>(res), std::get<1>(res), request->EncodePdu(), result, autoClose);
    return result;
}

galay::kernel::HttpResult::ptr 
galay::kernel::GY_HttpAsyncClient::Options(const std::string &url, protocol::http::HttpRequest::ptr request, bool autoClose)
{
    HttpResult::ptr result = std::make_shared<HttpResult>();
    result->AddTaskNum(1);
    std::tuple<std::string,uint16_t,std::string> res = ParseUrl(url);
    request->Header()->Uri() = std::get<2>(res);
    request->Header()->Method() = "OPTIONS";
    this->m_tcpClient->Unary(std::get<0>(res), std::get<1>(res), request->EncodePdu(), result, autoClose);
    return result;
}

galay::kernel::HttpResult::ptr 
galay::kernel::GY_HttpAsyncClient::Put(const std::string &url, protocol::http::HttpRequest::ptr request, bool autoClose)
{
    HttpResult::ptr result = std::make_shared<HttpResult>();
    result->AddTaskNum(1);
    std::tuple<std::string,uint16_t,std::string> res = ParseUrl(url);
    request->Header()->Uri() = std::get<2>(res);
    request->Header()->Method() = "PUT";
    this->m_tcpClient->Unary(std::get<0>(res), std::get<1>(res), request->EncodePdu(), result, autoClose);
    return result;
}

galay::kernel::HttpResult::ptr 
galay::kernel::GY_HttpAsyncClient::Delete(const std::string &url, protocol::http::HttpRequest::ptr request, bool autoClose)
{
    HttpResult::ptr result = std::make_shared<HttpResult>();
    result->AddTaskNum(1);
    std::tuple<std::string,uint16_t,std::string> res = ParseUrl(url);
    request->Header()->Uri() = std::get<2>(res);
    request->Header()->Method() = "DELETE";
    this->m_tcpClient->Unary(std::get<0>(res), std::get<1>(res), request->EncodePdu(), result, autoClose);
    return result;
}

galay::kernel::HttpResult::ptr 
galay::kernel::GY_HttpAsyncClient::Head(const std::string &url, protocol::http::HttpRequest::ptr request, bool autoClose)
{
    HttpResult::ptr result = std::make_shared<HttpResult>();
    result->AddTaskNum(1);
    std::tuple<std::string,uint16_t,std::string> res = ParseUrl(url);
    request->Header()->Uri() = std::get<2>(res);
    request->Header()->Method() = "HEAD";
    this->m_tcpClient->Unary(std::get<0>(res), std::get<1>(res), request->EncodePdu(), result, autoClose);
    return result;
}

galay::kernel::HttpResult::ptr 
galay::kernel::GY_HttpAsyncClient::Trace(const std::string &url, protocol::http::HttpRequest::ptr request, bool autoClose)
{
    HttpResult::ptr result = std::make_shared<HttpResult>();
    result->AddTaskNum(1);
    std::tuple<std::string,uint16_t,std::string> res = ParseUrl(url);
    request->Header()->Uri() = std::get<2>(res);
    request->Header()->Method() = "TRACE";
    this->m_tcpClient->Unary(std::get<0>(res), std::get<1>(res), request->EncodePdu(), result, autoClose);
    return result;
}

galay::kernel::HttpResult::ptr 
galay::kernel::GY_HttpAsyncClient::Patch(const std::string &url, protocol::http::HttpRequest::ptr request, bool autoClose)
{
    HttpResult::ptr result = std::make_shared<HttpResult>();
    result->AddTaskNum(1);
    std::tuple<std::string,uint16_t,std::string> res = ParseUrl(url);
    request->Header()->Uri() = std::get<2>(res);
    request->Header()->Method() = "PATCH";
    this->m_tcpClient->Unary(std::get<0>(res), std::get<1>(res), request->EncodePdu(), result, autoClose);
    return result;
}

std::shared_ptr<galay::kernel::NetResult> 
galay::kernel::GY_HttpAsyncClient::CloseConn()
{
    return m_tcpClient->CloseConn();
}

std::tuple<std::string,uint16_t,std::string> 
galay::kernel::GY_HttpAsyncClient::ParseUrl(const std::string& url)
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

galay::kernel::GY_SmtpAsyncClient::GY_SmtpAsyncClient(std::weak_ptr<GY_IOScheduler> scheduler)
{
    this->m_tcpClient = std::make_shared<GY_TcpClient>(scheduler);
}

void 
galay::kernel::GY_SmtpAsyncClient::Connect(const std::string& url)
{
    std::tuple<std::string,uint16_t> res = ParseUrl(url);
    this->m_host = std::get<0>(res);
    this->m_port = std::get<1>(res);
}

std::shared_ptr<galay::kernel::SmtpResult> 
galay::kernel::GY_SmtpAsyncClient::Auth(std::string account, std::string password)
{
    SmtpResult::ptr result = std::make_shared<SmtpResult>();
    result->AddTaskNum(1);
    std::queue<std::string> requests;
    protocol::smtp::SmtpRequest request;
    requests.push(request.Hello()->EncodePdu());
    requests.push(request.Auth()->EncodePdu());
    requests.push(request.Account(account)->EncodePdu());
    requests.push(request.Password(password)->EncodePdu());
    this->m_tcpClient->Unary(this->m_host, this->m_port, requests, result, false);
    return result;
}

std::shared_ptr<galay::kernel::SmtpResult> 
galay::kernel::GY_SmtpAsyncClient::SendEmail(std::string FromEmail, const std::vector<std::string> &ToEmails, galay::protocol::smtp::SmtpMsgInfo msg)
{
    SmtpResult::ptr result = std::make_shared<SmtpResult>();
    result->AddTaskNum(1);
    std::queue<std::string> requests;
    protocol::smtp::SmtpRequest request;
    requests.push(request.MailFrom(FromEmail)->EncodePdu());
    for (auto &to : ToEmails)
    {
        requests.push(request.RcptTo(to)->EncodePdu());
    }
    requests.push(request.Data()->EncodePdu());
    requests.push(request.Msg(msg)->EncodePdu());
    this->m_tcpClient->Unary(this->m_host, this->m_port, requests, result, false);
    return result;
}

std::shared_ptr<galay::kernel::SmtpResult> 
galay::kernel::GY_SmtpAsyncClient::Quit()
{
    SmtpResult::ptr result = std::make_shared<SmtpResult>();
    result->AddTaskNum(1);
    std::queue<std::string> requests;
    protocol::smtp::SmtpRequest request;
    requests.push(request.Quit()->EncodePdu());
    this->m_tcpClient->Unary(this->m_host, this->m_port, requests, result, false);
    return result;
}

std::shared_ptr<galay::kernel::NetResult>  
galay::kernel::GY_SmtpAsyncClient::CloseConn()
{
    return m_tcpClient->CloseConn();
}

std::tuple<std::string,uint16_t> 
galay::kernel::GY_SmtpAsyncClient::ParseUrl(const std::string& url)
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


galay::kernel::GY_UdpClient::GY_UdpClient(std::weak_ptr<GY_IOScheduler> scheduler)
{
    this->m_scheduler = scheduler;
    this->m_isSocketed = false;
}

galay::common::GY_NetCoroutine 
galay::kernel::GY_UdpClient::Unary(std::string host, uint16_t port, std::string &&buffer, std::shared_ptr<NetResult> result)
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
        result->m_errMsg = res->Error();
        spdlog::error("[{}:{}] [Unary.SendTo(fd:{}, host:{}, port:{}) failed, Error:{}]", __FILE__, __LINE__, this->m_fd, host, port, res->Error());
        result->Done();
        co_return common::CoroutineStatus::kCoroutineFinished;
    } 
    UdpResInfo info;
    res = RecvFrom(info.m_host, info.m_port, info.m_buffer);
    co_await res->Wait();
    if(!res->Success())
    {
        result->m_errMsg = res->Error();
        spdlog::error("[{}:{}] [Unary.RecvFrom(fd:{}, host:{}, port:{}) failed, Error:{}]", __FILE__, __LINE__, this->m_fd, host, port, res->Error());
        result->Done();
        co_return common::CoroutineStatus::kCoroutineFinished;
    }
    if(!result->m_errMsg.empty()) result->m_errMsg.clear();
    result->m_result = info;
    result->m_success = true;
    result->Done();
    co_return common::CoroutineStatus::kCoroutineFinished;
}

std::shared_ptr<galay::kernel::NetResult> 
galay::kernel::GY_UdpClient::Socket()
{
    galay::kernel::NetResult::ptr result = std::make_shared<NetResult>();
    this->m_fd = IOFunction::NetIOFunction::UdpFunction::Sock();
    if(this->m_fd <= 0) {
        spdlog::error("[{}:{}] [Socket(fd :{}) failed]", __FILE__, __LINE__, this->m_fd);
        result->m_errMsg = strerror(errno);
        goto end;
    }
    spdlog::debug("[{}:{}] [Socket(fd :{}) success]", __FILE__, __LINE__, this->m_fd);
    if(IOFunction::NetIOFunction::UdpFunction::IO_Set_No_Block(this->m_fd) == -1){
        spdlog::error("[{}:{}] [IO_Set_No_Block(fd:{}) failed, Error:{}]", __FILE__, __LINE__, this->m_fd, strerror(errno));
        result->m_errMsg = strerror(errno);
        goto end;
    }
    result->m_success = true;
    spdlog::debug("[{}:{}] [IO_Set_No_Block(fd:{}) success]", __FILE__, __LINE__, this->m_fd);
    this->m_isSocketed = true;
end:
    return result;
}

std::shared_ptr<galay::kernel::NetResult> 
galay::kernel::GY_UdpClient::SendTo(std::string host, uint16_t port, std::string&& buffer)
{
    this->m_request = std::forward<std::string>(buffer);
    galay::kernel::NetResult::ptr result = std::make_shared<NetResult>();
    IOFunction::NetIOFunction::Addr addr;
    addr.ip = host;
    addr.port = port;
    int len = galay::IOFunction::NetIOFunction::UdpFunction::SendTo(this->m_fd, addr, this->m_request);
    if(len == -1){
        spdlog::error("[{}:{}] [SendTo(fd:{}, host:{}, port:{}) failed, Error:{}]", __FILE__, __LINE__, this->m_fd, host, port, strerror(errno));
        result->m_errMsg = strerror(errno);
        goto end;
    }
    this->m_request.erase(0, len);
    spdlog::debug("[{}:{}] [SendTo(fd:{}, host:{}, port:{}), len:{}]", __FILE__, __LINE__, this->m_fd, host, port, len);
    if(this->m_request.empty()){
        spdlog::debug("[{}:{}] [SendTo All]", __FILE__, __LINE__);
        result->m_success = true;
        goto end;
    }else{
        result->AddTaskNum(1);
        GY_ClientExcutor::ptr executor = std::make_shared<GY_ClientExcutor>();
        executor->OnWrite() += [this, addr, result](){
            int len = galay::IOFunction::NetIOFunction::UdpFunction::SendTo(m_fd, addr, m_request);
            spdlog::debug("[{}:{}] [SendTo(fd:{}, host:{}, port:{}), len:{}]", __FILE__, __LINE__, this->m_fd, addr.ip, addr.port, len);
            m_request.erase(0,len);   
            if(m_request.empty()){
                SetResult(result, true, "");
                m_scheduler.lock()->DelEvent(this->m_fd, EventType::kEventWrite | kEventEpollET);
                result->Done();
            }
        };
        result->m_errMsg = "Waiting";
        m_scheduler.lock()->RegisterObjector(this->m_fd, executor);
        m_scheduler.lock()->AddEvent(this->m_fd, EventType::kEventWrite | kEventEpollET);
    }
end:
    return result;
}

std::shared_ptr<galay::kernel::NetResult> 
galay::kernel::GY_UdpClient::RecvFrom(std::string& host, uint16_t& port, std::string& buffer)
{
    galay::kernel::NetResult::ptr result = std::make_shared<NetResult>();
    IOFunction::NetIOFunction::Addr addr;
    char buf[MAX_UDP_LENGTH];
    int len = galay::IOFunction::NetIOFunction::UdpFunction::RecvFrom(this->m_fd, addr, buf,MAX_UDP_LENGTH);
    if(len == -1)
    {
        if(errno != EWOULDBLOCK && errno != EAGAIN && errno != EINTR)
        {
            spdlog::error("[{}:{}] [RecvFrom(fd:{}, host:{}, port:{}) failed, Error:{}]", __FILE__, __LINE__, this->m_fd, host, port, strerror(errno));
            result->m_errMsg = strerror(errno);
        }
        else
        {
            result->AddTaskNum(1);
            GY_ClientExcutor::ptr executor = std::make_shared<GY_ClientExcutor>();
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
                m_scheduler.lock()->DelEvent(this->m_fd, EventType::kEventRead | kEventEpollET);
                result->Done();
            };
            result->m_errMsg = "Waiting";
            m_scheduler.lock()->RegisterObjector(this->m_fd, executor);
            m_scheduler.lock()->AddEvent(this->m_fd, EventType::kEventRead | kEventEpollET);
        }
    }
    else
    {
        spdlog::debug("[{}:{}] [RecvFrom(fd:{}, host:{}, port:{}), len:{}]", __FILE__, __LINE__, this->m_fd, host, port, len);
        buffer.assign(buf, len);
        host = addr.ip;
        port = addr.port;
        result->m_success = true;
    }
    return result;
}

std::shared_ptr<galay::kernel::NetResult> 
galay::kernel::GY_UdpClient::CloseSocket()
{
    galay::kernel::NetResult::ptr result = std::make_shared<NetResult>();
    if(this->m_isSocketed)
    {
        if(close(this->m_fd) == -1) {
            spdlog::error("[{}:{}] [Close(fd:{}) failed, Error:{}]", __FILE__, __LINE__, this->m_fd, strerror(errno));
            result->m_errMsg = strerror(errno);
        }else{
            result->m_success = true;
        }
    }
    else
    {
        result->m_errMsg = "Not Socket";
        result->m_success = false;
    }
    return result;
}

bool 
galay::kernel::GY_UdpClient::Socketed()
{
    return this->m_isSocketed;
}

galay::kernel::GY_UdpClient::~GY_UdpClient()
{
    this->m_scheduler.lock()->DelObjector(this->m_fd);
}

void 
galay::kernel::GY_UdpClient::SetResult(NetResult::ptr result, bool success, std::string &&errMsg)
{
    result->m_success = success;
    result->m_errMsg = std::forward<std::string>(errMsg);
}


galay::kernel::DnsAsyncClient::DnsAsyncClient(std::weak_ptr<GY_IOScheduler> scheduler)
{
    this->m_udpClient = std::make_shared<GY_UdpClient>(scheduler);
}


std::shared_ptr<galay::kernel::DnsResult> 
galay::kernel::DnsAsyncClient::QueryA(const std::string& host, const uint16_t& port, const std::string& domain)
{
    galay::kernel::DnsResult::ptr result = std::make_shared<DnsResult>();
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

std::shared_ptr<galay::kernel::DnsResult> 
galay::kernel::DnsAsyncClient::QueryAAAA(const std::string& host, const uint16_t& port, const std::string& domain)
{
    galay::kernel::DnsResult::ptr result = std::make_shared<DnsResult>();
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

std::shared_ptr<galay::kernel::DnsResult> 
galay::kernel::DnsAsyncClient::QueryPtr(const std::string& host, const uint16_t& port, const std::string& want)
{
    galay::kernel::DnsResult::ptr result = std::make_shared<DnsResult>();
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

std::shared_ptr<galay::kernel::NetResult> 
galay::kernel::DnsAsyncClient::CloseSocket()
{
    return this->m_udpClient->CloseSocket();
}


std::string 
galay::kernel::DnsAsyncClient::HostToPtr(const std::string& host)
{
    auto res = util::StringUtil::SpiltWithChar(host, '.');
    std::string Ptr;
    for(std::vector<std::string>::reverse_iterator rit = res.rbegin(); rit != res.rend(); ++rit)
    {
        Ptr = Ptr + *rit + '.';
    }
    return Ptr + "in-addr.arpa.";
}