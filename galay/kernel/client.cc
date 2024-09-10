#include "../common/coroutine.h"
#include "../common/waitgroup.h"
#include "../util/random.h"
#include "../util/io.h"
#include "../util/stringtools.h"
#include "poller.h"
#include "client.h"
#include <regex>
#include <spdlog/spdlog.h>

namespace galay::client
{

NetResult::NetResult(result::ResultInterface::ptr result)
{
    this->m_result = result;
}

bool 
NetResult::Success()
{
    return this->m_result->Success();
}

std::string 
NetResult::Error()
{
    return this->m_result->Error();
}

coroutine::GroupAwaiter& 
NetResult::Wait()
{
    return this->m_result->Wait();
}

TcpClient::TcpClient(std::weak_ptr<poller::IOScheduler> scheduler)
{
    this->m_scheduler = scheduler;
    this->m_isConnected = false;
    this->m_isSocket = false;
    this->m_isSSL = false;
}

void 
TcpClient::IsSSL(bool isSSL)
{
    this->m_isSSL = isSSL;
}

galay::coroutine::NetCoroutine 
TcpClient::Unary(std::string host, uint16_t port, std::string &&buffer, std::shared_ptr<result::ResultInterface> result, bool autoClose)
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
            co_await res.Wait();
            if(!res.Success()) {
                result->SetErrorMsg("Connect:" + res.Error());
                spdlog::error("[{}:{}] [Unary.Connect(host:{}, port:{}) failed, Error:{}]", __FILE__, __LINE__, host, port, res.Error());
                result->Done();
                co_return coroutine::CoroutineStatus::kCoroutineFinished;
            }
        }
        auto res = Send(std::move(request));
        co_await res.Wait();
        if(!res.Success()) {
            result->SetErrorMsg("Send:" + res.Error());
            spdlog::error("[{}:{}] [Unary.Send(buffer:{}) failed, Error:{}]", __FILE__, __LINE__, request, res.Error());
            result->Done();
            co_return coroutine::CoroutineStatus::kCoroutineFinished;
        }
        std::string temp;
        res = Recv(temp);
        co_await res.Wait();
        if(!res.Success() && errno != EWOULDBLOCK && errno != EAGAIN && errno != EINTR) {
            result->SetErrorMsg("Send:" + res.Error());
            spdlog::error("[{}:{}] [Unary.Recv(buffer:{}) failed, Error:{}]", __FILE__, __LINE__, temp, res.Error());
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
            co_await res.Wait();
            if(!res.Success()) {
                result->SetErrorMsg("SSLConnect:" + res.Error());
                spdlog::error("[{}:{}] [Unary.SSLConnect(host:{}, port:{}) failed, Error:{}]", __FILE__, __LINE__, host, port, res.Error());
                result->Done();
                co_return coroutine::CoroutineStatus::kCoroutineFinished;
            }
        }
        auto res = SSLSend(std::move(request));
        co_await res.Wait();
        if(!res.Success()) {
            result->SetErrorMsg("SSLSend:" + res.Error());
            spdlog::error("[{}:{}] [Unary.SSLSend(buffer:{}) failed, Error:{}]", __FILE__, __LINE__, request, res.Error());
            result->Done();
            co_return coroutine::CoroutineStatus::kCoroutineFinished;
        }
        std::string temp;
        res = SSLRecv(temp);
        co_await res.Wait();
        if(!res.Success()) {
            result->SetErrorMsg("SSLRecv:" + res.Error());
            spdlog::error("[{}:{}] [Unary.SSLRecv(buffer:{}) failed, Error:{}]", __FILE__, __LINE__, request, res.Error());
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

galay::coroutine::NetCoroutine 
TcpClient::Unary(std::string host, uint16_t port, std::queue<std::string> requests, std::shared_ptr<result::ResultInterface> result, bool autoClose)
{
    std::queue<std::string> responses;
    while(!requests.empty())
    {
        std::string request = requests.front();
        requests.pop();
        std::shared_ptr<result::ResultInterface> res = std::make_shared<result::ResultInterface>();
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

NetResult
TcpClient::CloseConn()
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

NetResult 
TcpClient::Socket()
{
    result::ResultInterface::ptr result = std::make_shared<result::ResultInterface>();
    this->m_fd = io::net::TcpFunction::Sock();
    if(this->m_fd <= 0) {
        spdlog::error("[{}:{}] [Socket(fd :{}) failed]", __FILE__, __LINE__, this->m_fd);
        result->SetErrorMsg(strerror(errno));
        goto end;
    }
    spdlog::debug("[{}:{}] [Socket(fd :{}) success]", __FILE__, __LINE__, this->m_fd);
    if(io::net::TcpFunction::SetNoBlock(this->m_fd) == -1){
        spdlog::error("[{}:{}] [SetNoBlock(fd:{}) failed, Error:{}]", __FILE__, __LINE__, this->m_fd, strerror(errno));
        result->SetErrorMsg(strerror(errno));
        goto end;
    }
    result->SetSuccess(true);
    spdlog::debug("[{}:{}] [SetNoBlock(fd:{}) success]", __FILE__, __LINE__, this->m_fd);
    this->m_isSocket = true;
end:
    return NetResult(result);
}

NetResult  
TcpClient::Connect(const std::string &ip, uint16_t port)
{
    result::ResultInterface::ptr result = std::make_shared<result::ResultInterface>();
    int ret = io::net::TcpFunction::Conncet(this->m_fd, ip , port);
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
            poller::ClientExcutor::ptr executor = std::make_shared<poller::ClientExcutor>();
            executor->OnWrite() += [this,result](){
                int error = 0;
                socklen_t len = sizeof(error);
                getsockopt(this->m_fd, SOL_SOCKET, SO_ERROR, &error, &len);
                if(error){
                    SetResult(result, false, strerror(error));
                    m_isConnected = false;
                    spdlog::error("[{}:{}] [ClientBase::Connect] connect error: {}", __FILE__, __LINE__, strerror(errno));
                }else{
                    SetResult(result, true, "");
                    m_isConnected = true;
                    spdlog::debug("[{}:{}] [ClientBase::Connect] connect success", __FILE__, __LINE__);
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
    spdlog::debug("[{}:{}] [ClientBase::Connect] connect success", __FILE__, __LINE__);

end:
    return NetResult(result);
}

NetResult  
TcpClient::Send(std::string &&buffer)
{
    result::ResultInterface::ptr result = std::make_shared<result::ResultInterface>();
    this->m_wbuffer = std::forward<std::string>(buffer);
    int ret = poller::SendAll(this->m_scheduler.lock(), this->m_fd, this->m_ssl, this->m_wbuffer);
    if(m_wbuffer.empty()){
        result->SetSuccess(true);
        goto end;
    }else{
        result->AddTaskNum(1);
        poller::ClientExcutor::ptr executor = std::make_shared<poller::ClientExcutor>();
        executor->OnWrite() += [this, result](){
            int ret = SendAll(this->m_scheduler.lock(), this->m_fd, this->m_ssl, this->m_wbuffer);
            bool success = ret == 0 ? true:false;
            SetResult(result, success, strerror(errno));
            if(m_wbuffer.empty()){
                m_scheduler.lock()->DelEvent(this->m_fd, poller::kEventWrite | poller::kEventEpollET);
                result->Done();
            }
        };
        result->SetErrorMsg("Waiting");
        m_scheduler.lock()->RegisterObjector(this->m_fd, executor);
        m_scheduler.lock()->AddEvent(this->m_fd, poller::kEventWrite | poller::kEventEpollET);
    }
end:
    return NetResult(result);
}

NetResult
TcpClient::Recv(std::string &buffer)
{
    result::ResultInterface::ptr result = std::make_shared<result::ResultInterface>();
    int ret = poller::RecvAll(this->m_scheduler.lock(), this->m_fd, this->m_ssl, this->m_rbuffer);
    if(!this->m_rbuffer.empty()){
        result->SetSuccess(true);
        buffer = this->m_rbuffer;
        m_rbuffer.clear();
    }else{
        result->AddTaskNum(1);
        poller::ClientExcutor::ptr executor = std::make_shared<poller::ClientExcutor>();
        executor->OnRead() += [this, result, &buffer](){
            int ret = poller::RecvAll(this->m_scheduler.lock(), this->m_fd, this->m_ssl, this->m_rbuffer);
            bool success = (ret == 0 ? true:false);
            if(success) SetResult(result, success, "");
            else SetResult(result, success, strerror(errno));
            buffer.assign(this->m_rbuffer.begin(), this->m_rbuffer.end());
            this->m_rbuffer.clear();
            m_scheduler.lock()->DelEvent(this->m_fd, poller::kEventRead);
            result->Done();
        };
        result->SetErrorMsg("Waiting");
        m_scheduler.lock()->RegisterObjector(this->m_fd, executor);
        m_scheduler.lock()->AddEvent(this->m_fd, poller::kEventRead | poller::kEventEpollET);
    }
end:
    return NetResult(result);
}

NetResult 
TcpClient::Close()
{
    result::ResultInterface::ptr result = std::make_shared<result::ResultInterface>();
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
    return NetResult(result);
}

NetResult  
TcpClient::SSLSocket(long minVersion, long maxVersion)
{
    result::ResultInterface::ptr result = std::make_shared<result::ResultInterface>();
    this->m_ctx = io::net::TcpFunction::InitSSLClient(minVersion, maxVersion);
    if(this->m_ctx == nullptr) {
        unsigned long error = ERR_get_error();
        spdlog::error("[{}:{}] [InitSSLClient failed, Error:{}]", __FILE__, __LINE__, ERR_error_string(error, nullptr));
        result->SetErrorMsg(ERR_error_string(error, nullptr));
        goto end;
    }
    Socket();
    if(this->m_fd <= 0) {
        spdlog::error("[{}:{}] [Socket(fd :{}) failed]", __FILE__, __LINE__, this->m_fd);
        result->SetErrorMsg(strerror(errno));
        goto end;
    }
    spdlog::debug("[{}:{}] [Socket(fd :{}) success]", __FILE__, __LINE__, this->m_fd);
    this->m_ssl = io::net::TcpFunction::SSLCreateObj(this->m_ctx, this->m_fd);
    if(this->m_ssl == nullptr){
        unsigned long error = ERR_get_error();
        spdlog::error("[{}:{}] [SSLCreateObj failed, Error:{}]", __FILE__, __LINE__, ERR_error_string(error, nullptr));
        io::net::TcpFunction::SSLDestory({}, this->m_ctx);
        this->m_ctx = nullptr;
        result->SetErrorMsg(ERR_error_string(error, nullptr));
        goto end;
    }
    result->SetSuccess(true);
    this->m_isSocket = true;
    spdlog::debug("[{}:{}] [SSLCreateObj(fd:{}) success]", __FILE__, __LINE__, this->m_fd);
end:
    return NetResult(result);
}


NetResult 
TcpClient::SSLConnect(const std::string &ip, uint16_t port)
{
    result::ResultInterface::ptr result = std::make_shared<result::ResultInterface>();
    int ret = io::net::TcpFunction::Conncet(this->m_fd, ip , port);
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
            poller::ClientExcutor::ptr executor = std::make_shared<poller::ClientExcutor>();
            executor->OnWrite() += [this,result](){
                int error = 0;
                socklen_t len = sizeof(error);
                getsockopt(this->m_fd, SOL_SOCKET, SO_ERROR, &error, &len);
                if(error){
                    m_isConnected = false;
                    SetResult(result, false, strerror(error));
                    spdlog::error("[{}:{}] [ClientBase::Connect] connect error: {}", __FILE__, __LINE__, strerror(errno));
                    m_scheduler.lock()->DelEvent(this->m_fd, poller::kEventWrite | poller::kEventRead | poller::kEventError);
                    m_scheduler.lock()->DelObjector(this->m_fd);
                    result->Done();
                }else{
                    m_isConnected = true;
                    SSL_set_connect_state(m_ssl);
                    //ssl connect回调
                    auto func = [this, result](){
                        int r = SSL_do_handshake(m_ssl);
                        if (r == 1){
                            SetResult(result, true, "");
                            spdlog::debug("[{}:{}] [ClientBase::Connect] SSL_do_handshake success", __FILE__, __LINE__);
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
                        poller::ClientExcutor::ptr executor = std::make_shared<poller::ClientExcutor>();
                        executor->OnWrite() += [this, result, func](){
                            func();
                        };
                        executor->OnRead() += [this, result, func](){
                            func();
                        };
                        m_scheduler.lock()->RegisterObjector(this->m_fd, executor);
                    }
                }
            };
            m_scheduler.lock()->RegisterObjector(this->m_fd, executor);
            m_scheduler.lock()->AddEvent(this->m_fd, poller::kEventWrite | poller::kEventEpollET );
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
                spdlog::debug("[{}:{}] [ClientBase::SSLConnect] SSL_do_handshake success", __FILE__, __LINE__);
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
            poller::ClientExcutor::ptr executor = std::make_shared<poller::ClientExcutor>();
            result->AddTaskNum(1);
            executor->OnWrite() += [this, result, func](){
                func();
            };
            m_scheduler.lock()->RegisterObjector(this->m_fd, executor);
        }
        else
        {
            result->SetSuccess(true);
            spdlog::debug("[{}:{}] [ClientBase::SSLConnect] SSLConnect success", __FILE__, __LINE__);
        }
    }
end:
    return NetResult(result);
}

NetResult 
TcpClient::SSLSend(std::string &&buffer)
{
    return Send(std::forward<std::string>(buffer));
}

NetResult 
TcpClient::SSLRecv(std::string &buffer)
{
    return Recv(buffer);
}

NetResult 
TcpClient::SSLClose()
{
    result::ResultInterface::ptr result = std::make_shared<result::ResultInterface>();
    if(this->m_isSocket)
    {
        if(this->m_ssl) io::net::TcpFunction::SSLDestory(this->m_ssl);
        if(this->m_ctx) io::net::TcpFunction::SSLDestory({},this->m_ctx);
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
    return NetResult(result);
}

bool 
TcpClient::Socketed()
{
    return this->m_isSocket;
}

bool 
TcpClient::Connected()
{
    return this->m_isConnected;
}

TcpClient::~TcpClient()
{
    this->m_scheduler.lock()->DelObjector(this->m_fd);
}

void 
TcpClient::SetResult(result::ResultInterface::ptr result, bool success, std::string &&errMsg)
{
    result->SetErrorMsg(std::forward<std::string>(errMsg));
    result->SetSuccess(success);
}


HttpResult::HttpResult(result::ResultInterface::ptr result)
{
    this->m_result = result;
}

bool 
HttpResult::Success()
{
    return this->m_result->Success();
}

std::string 
HttpResult::Error()
{
    return this->m_result->Error();
}

coroutine::GroupAwaiter& 
HttpResult::Wait()
{
    return this->m_result->Wait();
}

protocol::http::HttpResponse 
HttpResult::GetResponse()
{
    std::string res = std::any_cast<std::string>(this->m_result->Result());
    protocol::http::HttpResponse resp;
    resp.DecodePdu(res);
    return resp;
}


HttpAsyncClient::HttpAsyncClient(std::weak_ptr<poller::IOScheduler> scheduler)
{
    this->m_tcpClient = std::make_shared<TcpClient>(scheduler);
}

HttpResult 
HttpAsyncClient::Get(const std::string &url, protocol::http::HttpRequest::ptr request, bool autoClose)
{
    result::ResultInterface::ptr result = std::make_shared<result::ResultInterface>();
    result->AddTaskNum(1);
    std::tuple<std::string,uint16_t,std::string> res = ParseUrl(url);
    request->Header()->Uri() = std::get<2>(res);
    request->Header()->Method() = "GET";
    this->m_tcpClient->Unary(std::get<0>(res), std::get<1>(res), request->EncodePdu(), result, autoClose);
    return HttpResult(result);
}

HttpResult 
HttpAsyncClient::Post(const std::string &url, protocol::http::HttpRequest::ptr request, bool autoClose)
{
    result::ResultInterface::ptr result = std::make_shared<result::ResultInterface>();
    result->AddTaskNum(1);
    std::tuple<std::string,uint16_t,std::string> res = ParseUrl(url);
    request->Header()->Uri() = std::get<2>(res);
    request->Header()->Method() = "POST";
    this->m_tcpClient->Unary(std::get<0>(res), std::get<1>(res), request->EncodePdu(), result, autoClose);
    return HttpResult(result);
}

HttpResult 
HttpAsyncClient::Options(const std::string &url, protocol::http::HttpRequest::ptr request, bool autoClose)
{
    result::ResultInterface::ptr result = std::make_shared<result::ResultInterface>();
    result->AddTaskNum(1);
    std::tuple<std::string,uint16_t,std::string> res = ParseUrl(url);
    request->Header()->Uri() = std::get<2>(res);
    request->Header()->Method() = "OPTIONS";
    this->m_tcpClient->Unary(std::get<0>(res), std::get<1>(res), request->EncodePdu(), result, autoClose);
    return HttpResult(result);
}

HttpResult 
HttpAsyncClient::Put(const std::string &url, protocol::http::HttpRequest::ptr request, bool autoClose)
{
    result::ResultInterface::ptr result = std::make_shared<result::ResultInterface>();
    result->AddTaskNum(1);
    std::tuple<std::string,uint16_t,std::string> res = ParseUrl(url);
    request->Header()->Uri() = std::get<2>(res);
    request->Header()->Method() = "PUT";
    this->m_tcpClient->Unary(std::get<0>(res), std::get<1>(res), request->EncodePdu(), result, autoClose);
    return HttpResult(result);
}

HttpResult 
HttpAsyncClient::Delete(const std::string &url, protocol::http::HttpRequest::ptr request, bool autoClose)
{
    result::ResultInterface::ptr result = std::make_shared<result::ResultInterface>();
    result->AddTaskNum(1);
    std::tuple<std::string,uint16_t,std::string> res = ParseUrl(url);
    request->Header()->Uri() = std::get<2>(res);
    request->Header()->Method() = "DELETE";
    this->m_tcpClient->Unary(std::get<0>(res), std::get<1>(res), request->EncodePdu(), result, autoClose);
    return HttpResult(result);
}

HttpResult 
HttpAsyncClient::Head(const std::string &url, protocol::http::HttpRequest::ptr request, bool autoClose)
{
    result::ResultInterface::ptr result = std::make_shared<result::ResultInterface>();
    result->AddTaskNum(1);
    std::tuple<std::string,uint16_t,std::string> res = ParseUrl(url);
    request->Header()->Uri() = std::get<2>(res);
    request->Header()->Method() = "HEAD";
    this->m_tcpClient->Unary(std::get<0>(res), std::get<1>(res), request->EncodePdu(), result, autoClose);
    return HttpResult(result);
}

HttpResult 
HttpAsyncClient::Trace(const std::string &url, protocol::http::HttpRequest::ptr request, bool autoClose)
{
    result::ResultInterface::ptr result = std::make_shared<result::ResultInterface>();
    result->AddTaskNum(1);
    std::tuple<std::string,uint16_t,std::string> res = ParseUrl(url);
    request->Header()->Uri() = std::get<2>(res);
    request->Header()->Method() = "TRACE";
    this->m_tcpClient->Unary(std::get<0>(res), std::get<1>(res), request->EncodePdu(), result, autoClose);
    return HttpResult(result);
}

HttpResult 
HttpAsyncClient::Patch(const std::string &url, protocol::http::HttpRequest::ptr request, bool autoClose)
{
    result::ResultInterface::ptr result = std::make_shared<result::ResultInterface>();
    result->AddTaskNum(1);
    std::tuple<std::string,uint16_t,std::string> res = ParseUrl(url);
    request->Header()->Uri() = std::get<2>(res);
    request->Header()->Method() = "PATCH";
    this->m_tcpClient->Unary(std::get<0>(res), std::get<1>(res), request->EncodePdu(), result, autoClose);
    return HttpResult(result);
}

NetResult 
HttpAsyncClient::CloseConn()
{
    return m_tcpClient->CloseConn();
}

std::tuple<std::string,uint16_t,std::string> 
HttpAsyncClient::ParseUrl(const std::string& url)
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

SmtpAsyncClient::SmtpAsyncClient(std::weak_ptr<poller::IOScheduler> scheduler)
{
    this->m_tcpClient = std::make_shared<TcpClient>(scheduler);
}

SmtpResult::SmtpResult(result::ResultInterface::ptr result)
{
    this->m_result = result;
}

bool 
SmtpResult::Success()
{
    return this->m_result->Success();
}

std::string 
SmtpResult::Error()
{
    return this->m_result->Error();
}

coroutine::GroupAwaiter& 
SmtpResult::Wait()
{
    return this->m_result->Wait();
}

std::queue<protocol::smtp::SmtpResponse> 
SmtpResult::GetResponse()
{
    auto res = std::any_cast<std::queue<std::string>>(this->m_result->Result());
    std::queue<protocol::smtp::SmtpResponse> responses;
    while (!res.empty())
    {
        protocol::smtp::SmtpResponse response;
        response.DecodePdu(res.front());
        res.pop();
        responses.push(response);
    }
    return responses;
}

void 
SmtpAsyncClient::Connect(const std::string& url)
{
    std::tuple<std::string,uint16_t> res = ParseUrl(url);
    this->m_host = std::get<0>(res);
    this->m_port = std::get<1>(res);
}

SmtpResult 
SmtpAsyncClient::Auth(std::string account, std::string password)
{
    result::ResultInterface::ptr result = std::make_shared<result::ResultInterface>();
    result->AddTaskNum(1);
    std::queue<std::string> requests;
    protocol::smtp::SmtpRequest request;
    requests.push(protocol::smtp::SmtpHelper::Hello(request));
    requests.push(protocol::smtp::SmtpHelper::Auth(request));
    requests.push(protocol::smtp::SmtpHelper::Account(request, account));
    requests.push(protocol::smtp::SmtpHelper::Password(request, password));
    this->m_tcpClient->Unary(this->m_host, this->m_port, requests, result, false);
    return SmtpResult(result);
}

SmtpResult 
SmtpAsyncClient::SendEmail(std::string FromEmail, const std::vector<std::string> &ToEmails, galay::protocol::smtp::SmtpMsgInfo msg)
{
    result::ResultInterface::ptr result = std::make_shared<result::ResultInterface>();
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
    return SmtpResult(result);
}

SmtpResult 
SmtpAsyncClient::Quit()
{
    result::ResultInterface::ptr result = std::make_shared<result::ResultInterface>();
    result->AddTaskNum(1);
    std::queue<std::string> requests;
    protocol::smtp::SmtpRequest request;
    requests.push(protocol::smtp::SmtpHelper::Quit(request));
    this->m_tcpClient->Unary(this->m_host, this->m_port, requests, result, false);
    return SmtpResult(result);
}

NetResult  
SmtpAsyncClient::CloseConn()
{
    return m_tcpClient->CloseConn();
}

std::tuple<std::string,uint16_t> 
SmtpAsyncClient::ParseUrl(const std::string& url)
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


UdpClient::UdpClient(std::weak_ptr<poller::IOScheduler> scheduler)
{
    this->m_scheduler = scheduler;
    this->m_isSocketed = false;
}

galay::coroutine::NetCoroutine 
UdpClient::Unary(std::string host, uint16_t port, std::string &&buffer, std::shared_ptr<result::ResultInterface> result)
{
    result->AddTaskNum(1);
    if(!this->m_isSocketed)
    {
        Socket();
        this->m_isSocketed = true;
    }
    auto res = SendTo(host, port, std::forward<std::string>(buffer));
    co_await res.Wait();
    if(!res.Success())
    {
        result->SetErrorMsg(res.Error());
        spdlog::error("[{}:{}] [Unary.SendTo(fd:{}, host:{}, port:{}) failed, Error:{}]", __FILE__, __LINE__, this->m_fd, host, port, res.Error());
        result->Done();
        co_return coroutine::CoroutineStatus::kCoroutineFinished;
    } 
    UdpRespAddrInfo info;
    res = RecvFrom(info.m_host, info.m_port, info.m_buffer);
    co_await res.Wait();
    if(!res.Success())
    {
        result->SetErrorMsg(res.Error());
        spdlog::error("[{}:{}] [Unary.RecvFrom(fd:{}, host:{}, port:{}) failed, Error:{}]", __FILE__, __LINE__, this->m_fd, host, port, res.Error());
        result->Done();
        co_return coroutine::CoroutineStatus::kCoroutineFinished;
    }
    result->SetResult(info);
    result->SetSuccess(true);
    result->Done();
    co_return coroutine::CoroutineStatus::kCoroutineFinished;
}

NetResult
UdpClient::Socket()
{
    result::ResultInterface::ptr result = std::make_shared<result::ResultInterface>();
    this->m_fd = io::net::UdpFunction::Sock();
    if(this->m_fd <= 0) {
        spdlog::error("[{}:{}] [Socket(fd :{}) failed]", __FILE__, __LINE__, this->m_fd);
        result->SetErrorMsg(strerror(errno));
        goto end;
    }
    spdlog::debug("[{}:{}] [Socket(fd :{}) success]", __FILE__, __LINE__, this->m_fd);
    if(io::net::UdpFunction::SetNoBlock(this->m_fd) == -1){
        spdlog::error("[{}:{}] [SetNoBlock(fd:{}) failed, Error:{}]", __FILE__, __LINE__, this->m_fd, strerror(errno));
        result->SetErrorMsg(strerror(errno));
        goto end;
    }
    result->SetSuccess(true);
    spdlog::debug("[{}:{}] [SetNoBlock(fd:{}) success]", __FILE__, __LINE__, this->m_fd);
    this->m_isSocketed = true;
end:
    return NetResult(result);
}

NetResult 
UdpClient::SendTo(std::string host, uint16_t port, std::string&& buffer)
{
    this->m_request = std::forward<std::string>(buffer);
    result::ResultInterface::ptr result = std::make_shared<result::ResultInterface>();
    io::net::Addr addr;
    addr.ip = host;
    addr.port = port;
    int len = galay::io::net::UdpFunction::SendTo(this->m_fd, addr, this->m_request);
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
        poller::ClientExcutor::ptr executor = std::make_shared<poller::ClientExcutor>();
        executor->OnWrite() += [this, addr, result](){
            int len = galay::io::net::UdpFunction::SendTo(m_fd, addr, m_request);
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
    return NetResult(result);
}

NetResult
UdpClient::RecvFrom(std::string& host, uint16_t& port, std::string& buffer)
{
    result::ResultInterface::ptr result = std::make_shared<result::ResultInterface>();
    io::net::Addr addr;
    char buf[MAX_UDP_LENGTH];
    int len = galay::io::net::UdpFunction::RecvFrom(this->m_fd, addr, buf,MAX_UDP_LENGTH);
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
            poller::ClientExcutor::ptr executor = std::make_shared<poller::ClientExcutor>();
            executor->OnRead() += [this, result, &host, &port, &buffer](){
                spdlog::debug("[{}:{}] [Call RecvFrom.OnRead]", __FILE__, __LINE__);
                io::net::Addr addr;
                char buf[MAX_UDP_LENGTH];
                int len = galay::io::net::UdpFunction::RecvFrom(m_fd, addr, buf,MAX_UDP_LENGTH);
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
    return NetResult(result);
}

NetResult
UdpClient::CloseSocket()
{
    result::ResultInterface::ptr result = std::make_shared<result::ResultInterface>();
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
    return NetResult(result);
}

bool 
UdpClient::Socketed()
{
    return this->m_isSocketed;
}

UdpClient::~UdpClient()
{
    this->m_scheduler.lock()->DelObjector(this->m_fd);
}

void 
UdpClient::SetResult(std::shared_ptr<result::ResultInterface> result, bool success, std::string &&errMsg)
{
    result->SetErrorMsg(std::forward<std::string>(errMsg));
    result->SetSuccess(success);
}

DnsResult::DnsResult(result::ResultInterface::ptr result)
{
    this->m_result = result;
    this->m_isParsed = false;
}

bool 
DnsResult::Success()
{
    return m_result->Success();
}

std::string 
DnsResult::Error()
{
    return m_result->Error();
}

coroutine::GroupAwaiter& 
DnsResult::Wait()
{
    return m_result->Wait();
}

bool 
DnsResult::HasCName()
{
    if(!m_isParsed) {
        Parse();
        m_isParsed = true;
    }
    return !m_cNames.empty();
}

std::string 
DnsResult::GetCName()
{
    if(!m_isParsed) {
        Parse();
        m_isParsed = true;
    }
    std::string cname = m_cNames.front();
    m_cNames.pop();
    return std::move(cname);
}

bool 
DnsResult::HasA()
{
    if(!m_isParsed) {
        Parse();
        m_isParsed = true;
    }
    return !m_a.empty();
}

std::string 
DnsResult::GetA()
{
    if(!m_isParsed) {
        Parse();
        m_isParsed = true;
    }
    std::string a = m_a.front();
    m_a.pop();
    return std::move(a);
}

bool 
DnsResult::HasAAAA()
{
    if(!m_isParsed) {
        Parse();
        m_isParsed = true;
    }
    return !m_aaaa.empty();
}

std::string 
DnsResult::GetAAAA()
{
    if(!m_isParsed) {
        Parse();
        m_isParsed = true;
    }
    std::string aaaa = m_aaaa.front();
    m_aaaa.pop();
    return std::move(aaaa);
}

bool 
DnsResult::HasPtr()
{
    if(!m_isParsed) {
        Parse();
        m_isParsed = true;
    }
    return !m_ptr.empty();
}

std::string 
DnsResult::GetPtr()
{
    if(!m_isParsed) {
        Parse();
        m_isParsed = true;
    }
    std::string ptr = m_ptr.front();
    m_ptr.pop();
    return std::move(ptr);
}

void
DnsResult::Parse()
{
    UdpRespAddrInfo info = std::any_cast<UdpRespAddrInfo>(this->m_result);
    protocol::dns::DnsResponse response;
    response.DecodePdu(info.m_buffer);
    auto answers = response.GetAnswerQueue();
    while (!answers.empty())
    {
        auto answer = answers.front();
        answers.pop();
        spdlog::debug("[{}:{}] DnsResult::Parse: type:{}, data:{}",__FILE__, __LINE__, answer.m_type, answer.m_data);
        switch (answer.m_type)
        {
        case protocol::dns::kDnsQueryCname:
            m_cNames.push(answer.m_data);
            break;
        case protocol::dns::kDnsQueryA:
            m_a.push(answer.m_data);
            break;
        case protocol::dns::kDnsQueryAAAA:
            m_aaaa.push(answer.m_data);
            break;
        case protocol::dns::kDnsQueryPtr:
            m_ptr.push(answer.m_data);
            break;
        default:
            break;
        }
    }
    
}


DnsAsyncClient::DnsAsyncClient(std::weak_ptr<poller::IOScheduler> scheduler)
{
    this->m_udpClient = std::make_shared<UdpClient>(scheduler);
}


DnsResult 
DnsAsyncClient::QueryA(const std::string& host, const uint16_t& port, const std::string& domain)
{
    result::ResultInterface::ptr result = std::make_shared<result::ResultInterface>();
    protocol::dns::DnsRequest request;
    protocol::dns::DnsHeader header;
    header.m_flags.m_rd = 1;
    header.m_id = galay::tools::Randomizer::RandomInt(0,MAX_UDP_LENGTH);
    header.m_questions = 1;
    galay::protocol::dns::DnsQuestion question;
    question.m_class = 1;
    question.m_qname = domain;
    question.m_type = galay::protocol::dns::kDnsQueryA;
    std::queue<galay::protocol::dns::DnsQuestion> questions;
    questions.push(question);
    request.SetQuestionQueue(std::move(questions));
    this->m_udpClient->Unary(host, port, request.EncodePdu(), result);
    return DnsResult(result);
}

DnsResult 
DnsAsyncClient::QueryAAAA(const std::string& host, const uint16_t& port, const std::string& domain)
{
    result::ResultInterface::ptr result = std::make_shared<result::ResultInterface>();
    protocol::dns::DnsRequest request;
    protocol::dns::DnsHeader header;
    header.m_flags.m_rd = 1;
    header.m_id = galay::tools::Randomizer::RandomInt(0,MAX_UDP_LENGTH);
    header.m_questions = 1;
    galay::protocol::dns::DnsQuestion question;
    question.m_class = 1;
    question.m_qname = domain;
    question.m_type = galay::protocol::dns::kDnsQueryAAAA;
    std::queue<galay::protocol::dns::DnsQuestion> questions;
    questions.push(question);
    request.SetQuestionQueue(std::move(questions));
    this->m_udpClient->Unary(host, port, request.EncodePdu(), result);
    return DnsResult(result);
}

DnsResult 
DnsAsyncClient::QueryPtr(const std::string& host, const uint16_t& port, const std::string& want)
{
    result::ResultInterface::ptr result = std::make_shared<result::ResultInterface>();
    protocol::dns::DnsRequest request;
    protocol::dns::DnsHeader header;
    header.m_flags.m_rd = 1;
    header.m_id = galay::tools::Randomizer::RandomInt(0,MAX_UDP_LENGTH);
    header.m_questions = 1;
    galay::protocol::dns::DnsQuestion question;
    question.m_class = 1;
    question.m_qname = HostToPtr(want);
    question.m_type = galay::protocol::dns::kDnsQueryPtr;
    std::queue<galay::protocol::dns::DnsQuestion> questions;
    questions.push(question);
    request.SetQuestionQueue(std::move(questions));
    this->m_udpClient->Unary(host, port, request.EncodePdu(), result);
    return DnsResult(result);
}

NetResult 
DnsAsyncClient::CloseSocket()
{
    return this->m_udpClient->CloseSocket();
}


std::string 
DnsAsyncClient::HostToPtr(const std::string& host)
{
    auto res = tools::StringSplitter::SpiltWithChar(host, '.');
    std::string Ptr;
    for(std::vector<std::string>::reverse_iterator rit = res.rbegin(); rit != res.rend(); ++rit)
    {
        Ptr = Ptr + *rit + '.';
    }
    return Ptr + "in-addr.arpa.";
}

}
