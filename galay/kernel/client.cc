#include "client.h"
#include "../util/random.h"
#include "task.h"
#include <regex>
#include <spdlog/spdlog.h>

galay::kernel::TcpResult::TcpResult(common::GY_NetCoroutinePool::wptr coPool)
{
    m_success = false;
    m_errMsg = "";
    m_waitGroup = std::make_shared<common::WaitGroup>(coPool);
}

bool 
galay::kernel::TcpResult::Success()
{
    return m_success;
}

std::string
galay::kernel::TcpResult::Error()
{
    return m_errMsg;
}

void 
galay::kernel::TcpResult::AddTaskNum(uint16_t taskNum)
{
    m_waitGroup->Add(taskNum);
}

galay::common::GroupAwaiter&
galay::kernel::TcpResult::Wait()
{
    return m_waitGroup->Wait();
}

void 
galay::kernel::TcpResult::Done()
{
    m_waitGroup->Done();
}

galay::kernel::GY_TcpClient::GY_TcpClient(common::GY_NetCoroutinePool::wptr coPool, GY_IOScheduler::wptr scheduler)
{
    this->m_threadPool = nullptr;
    this->m_scheduler = scheduler;
    this->m_coPool = coPool;
    this->m_recvTask = nullptr;
    this->m_sendTask = nullptr;
    this->m_isConnected = false;
}


galay::kernel::TcpResult::ptr 
galay::kernel::GY_TcpClient::Socket()
{
    galay::kernel::TcpResult::ptr result = std::make_shared<galay::kernel::TcpResult>(this->m_coPool);
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
end:
    return result;
}

galay::kernel::TcpResult::ptr  
galay::kernel::GY_TcpClient::Connect(const std::string &ip, uint16_t port)
{
    galay::kernel::TcpResult::ptr result = std::make_shared<galay::kernel::TcpResult>(this->m_coPool);
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
            GY_ClientExcutor::ptr executor = std::make_shared<GY_ClientExcutor>([this,result](){
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
                m_scheduler.lock()->DelEvent(this->m_fd, kEventWrite);
                result->Done();
            });
            m_scheduler.lock()->RegisterObjector(this->m_fd, executor);
            m_scheduler.lock()->AddEvent(this->m_fd, kEventWrite);
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

galay::kernel::TcpResult::ptr  
galay::kernel::GY_TcpClient::Send(std::string &&buffer)
{
    galay::kernel::TcpResult::ptr result = std::make_shared<galay::kernel::TcpResult>(this->m_coPool);
    if(!m_sendTask)
    {
        m_sendTask = std::make_shared<galay::kernel::GY_SendTask>(this->m_fd, this->m_ssl, this->m_scheduler);
    }
    m_sendTask->AppendWBuffer(std::forward<std::string>(buffer));
    m_sendTask->SendAll();
    if(m_sendTask->Empty()){
        result->m_success = true;
        goto end;
    }else{
        result->AddTaskNum(1);
        GY_ClientExcutor::ptr excutor = std::make_shared<GY_ClientExcutor>([this, result](){
            m_sendTask->SendAll();
            SetResult(result, m_sendTask->Success(), m_sendTask->Error());
            if(m_sendTask->Empty()){
                m_scheduler.lock()->DelEvent(this->m_fd, EventType::kEventWrite);
                result->Done();
            }
        });
        result->m_errMsg = "Waiting";
        m_scheduler.lock()->RegisterObjector(this->m_fd, excutor);
        m_scheduler.lock()->AddEvent(this->m_fd, EventType::kEventWrite);
    }
end:
    return result;
}

galay::kernel::TcpResult::ptr
galay::kernel::GY_TcpClient::Recv(std::string &buffer)
{
    galay::kernel::TcpResult::ptr result = std::make_shared<galay::kernel::TcpResult>(this->m_coPool);
    if(!m_recvTask){
        m_recvTask = std::make_shared<galay::kernel::GY_RecvTask>(this->m_fd, this->m_ssl, this->m_scheduler);
    }
    m_recvTask->RecvAll();
    if(!m_recvTask->GetRBuffer().empty()){
        result->m_success = true;
        buffer.assign(m_recvTask->GetRBuffer().begin(), m_recvTask->GetRBuffer().end());
        m_recvTask->GetRBuffer().clear();
    }else{
        result->AddTaskNum(1);
        GY_ClientExcutor::ptr executor = std::make_shared<GY_ClientExcutor>([this, result, &buffer](){
            m_recvTask->RecvAll();
            SetResult(result, m_recvTask->Success(), m_recvTask->Error());
            buffer.assign(m_recvTask->GetRBuffer().begin(), m_recvTask->GetRBuffer().end());
            m_recvTask->GetRBuffer().clear();
            m_scheduler.lock()->DelEvent(this->m_fd, EventType::kEventRead);
            result->Done();
        });
        result->m_errMsg = "Waiting";
        m_scheduler.lock()->RegisterObjector(this->m_fd, executor);
        m_scheduler.lock()->AddEvent(this->m_fd, EventType::kEventRead);
    }
end:
    return result;
}

galay::kernel::TcpResult::ptr 
galay::kernel::GY_TcpClient::Close()
{
    galay::kernel::TcpResult::ptr result = std::make_shared<galay::kernel::TcpResult>(this->m_coPool);
    if(close(this->m_fd) == -1) {
        spdlog::error("[{}:{}] [Close(fd:{}) failed, Error:{}]", __FILE__, __LINE__, this->m_fd, strerror(errno));
        result->m_errMsg = strerror(errno);
    }else{
        result->m_success = true;
    }
    if(m_isConnected) m_isConnected = false;
    return result;
}

galay::kernel::TcpResult::ptr  
galay::kernel::GY_TcpClient::SSLSocket(long minVersion, long maxVersion)
{
    galay::kernel::TcpResult::ptr result = std::make_shared<galay::kernel::TcpResult>(this->m_coPool);
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
    spdlog::debug("[{}:{}] [SSLCreateObj(fd:{}) success]", __FILE__, __LINE__, this->m_fd);
end:
    return result;
}


galay::kernel::TcpResult::ptr 
galay::kernel::GY_TcpClient::SSLConnect(const std::string &ip, uint16_t port)
{
    galay::kernel::TcpResult::ptr result = std::make_shared<galay::kernel::TcpResult>(this->m_coPool);
    int ret = IOFunction::NetIOFunction::TcpFunction::Conncet(this->m_fd, ip , port);
    if (ret < 0 )
    {
        if(errno != EINPROGRESS)
        {
            m_isConnected = false;
            spdlog::error("[{}:{}] [Connect(fd:{}) to {}:{} failed, error is '{}']", __FILE__, __LINE__, this->m_fd, ip, port, strerror(errno));
            result->m_errMsg = strerror(errno);
            Close();
            goto end;
        }
        else
        {
            result->AddTaskNum(1);
            // connect 回调
            GY_ClientExcutor::ptr executor = std::make_shared<GY_ClientExcutor>([this,result](){
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
                        GY_ClientExcutor::ptr executor = std::make_shared<GY_ClientExcutor>([this, result, func](){
                            func();
                        });
                        m_scheduler.lock()->RegisterObjector(this->m_fd, executor);
                    }
                }
            });
            m_scheduler.lock()->RegisterObjector(this->m_fd, executor);
            m_scheduler.lock()->AddEvent(this->m_fd, kEventWrite);
            result->m_errMsg = "Waiting";
            goto end;
        }
    }
    m_isConnected = true;
    result->m_success = true;
    spdlog::debug("[{}:{}] [GY_ClientBase::SSLConnect] SSLConnect success", __FILE__, __LINE__);
end:
    return result;
}

galay::kernel::TcpResult::ptr 
galay::kernel::GY_TcpClient::SSLSend(std::string &&buffer)
{
    return Send(std::forward<std::string>(buffer));
}

galay::kernel::TcpResult::ptr 
galay::kernel::GY_TcpClient::SSLRecv(std::string &buffer)
{
    return Recv(buffer);
}

galay::kernel::TcpResult::ptr 
galay::kernel::GY_TcpClient::SSLClose()
{
    if(this->m_ssl) IOFunction::NetIOFunction::TcpFunction::SSLDestory(this->m_ssl);
    if(this->m_ctx) IOFunction::NetIOFunction::TcpFunction::SSLDestory({},this->m_ctx);
    galay::kernel::TcpResult::ptr result = std::make_shared<galay::kernel::TcpResult>(this->m_coPool);
    if(close(this->m_fd) == -1) {
        spdlog::error("[{}:{}] [Close(fd:{}) failed, Error:{}]", __FILE__, __LINE__, this->m_fd, strerror(errno));
        result->m_errMsg = strerror(errno);
    }else{
        result->m_success = true;
    }
    if(m_isConnected) m_isConnected = false;
    return result;
}

bool 
galay::kernel::GY_TcpClient::Connected()
{
    return this->m_isConnected;
}

galay::common::GY_NetCoroutinePool::wptr 
galay::kernel::GY_TcpClient::GetCoPool()
{
    return this->m_coPool;
}

galay::kernel::GY_TcpClient::~GY_TcpClient()
{
    this->m_scheduler.lock()->DelObjector(this->m_fd);
}

void 
galay::kernel::GY_TcpClient::SetResult(TcpResult::ptr result, bool success, std::string &&errMsg)
{
    result->m_success = success;
    result->m_errMsg = std::forward<std::string>(errMsg);
}

galay::kernel::HttpResult::HttpResult(common::GY_NetCoroutinePool::wptr coPool)
    : TcpResult(coPool)
{
    this->m_response = std::make_shared<protocol::http::HttpResponse>();
}

galay::protocol::http::HttpResponse::ptr 
galay::kernel::HttpResult::GetResponse()
{
    return this->m_response;
}

galay::kernel::GY_HttpAsyncClient::GY_HttpAsyncClient(common::GY_NetCoroutinePool::wptr coPool, GY_IOScheduler::wptr scheduler)
{
    this->m_tcpClient = std::make_shared<GY_TcpClient>(coPool, scheduler);
    this->m_tcpClient->Socket();
}

galay::kernel::HttpResult::ptr 
galay::kernel::GY_HttpAsyncClient::Get(const std::string &url, protocol::http::HttpRequest::ptr request, bool autoClose)
{
    HttpResult::ptr result = std::make_shared<HttpResult>(this->m_tcpClient->GetCoPool());
    result->AddTaskNum(1);
    std::tuple<std::string,uint16_t,std::string> res = ParseUrl(url);
    request->Header()->Uri() = std::get<2>(res);
    request->Header()->Method() = "GET";
    auto co = Unary(std::get<0>(res), std::get<1>(res), request->EncodePdu(), result, autoClose);
    uint64_t coId = co.GetCoId();
    m_tcpClient->GetCoPool().lock()->AddCoroutine(std::move(co));
    m_tcpClient->GetCoPool().lock()->Resume(coId);
    return result;
}

galay::kernel::HttpResult::ptr 
galay::kernel::GY_HttpAsyncClient::Post(const std::string &url, protocol::http::HttpRequest::ptr request, bool autoClose)
{
    HttpResult::ptr result = std::make_shared<HttpResult>(this->m_tcpClient->GetCoPool());
    result->AddTaskNum(1);
    std::tuple<std::string,uint16_t,std::string> res = ParseUrl(url);
    request->Header()->Uri() = std::get<2>(res);
    request->Header()->Method() = "POST";
    auto co = Unary(std::get<0>(res), std::get<1>(res), request->EncodePdu(), result, autoClose);
    uint64_t coId = co.GetCoId();
    m_tcpClient->GetCoPool().lock()->AddCoroutine(std::move(co));
    m_tcpClient->GetCoPool().lock()->Resume(coId);
    return result;
}

galay::kernel::HttpResult::ptr 
galay::kernel::GY_HttpAsyncClient::Options(const std::string &url, protocol::http::HttpRequest::ptr request, bool autoClose)
{
    HttpResult::ptr result = std::make_shared<HttpResult>(this->m_tcpClient->GetCoPool());
    result->AddTaskNum(1);
    std::tuple<std::string,uint16_t,std::string> res = ParseUrl(url);
    request->Header()->Uri() = std::get<2>(res);
    request->Header()->Method() = "OPTIONS";
    auto co = Unary(std::get<0>(res), std::get<1>(res), request->EncodePdu(), result, autoClose);
    uint64_t coId = co.GetCoId();
    m_tcpClient->GetCoPool().lock()->AddCoroutine(std::move(co));
    m_tcpClient->GetCoPool().lock()->Resume(coId);
    return result;
}

galay::kernel::HttpResult::ptr 
galay::kernel::GY_HttpAsyncClient::Put(const std::string &url, protocol::http::HttpRequest::ptr request, bool autoClose)
{
    HttpResult::ptr result = std::make_shared<HttpResult>(this->m_tcpClient->GetCoPool());
    result->AddTaskNum(1);
    std::tuple<std::string,uint16_t,std::string> res = ParseUrl(url);
    request->Header()->Uri() = std::get<2>(res);
    request->Header()->Method() = "PUT";
    auto co = Unary(std::get<0>(res), std::get<1>(res), request->EncodePdu(), result, autoClose);
    uint64_t coId = co.GetCoId();
    m_tcpClient->GetCoPool().lock()->AddCoroutine(std::move(co));
    m_tcpClient->GetCoPool().lock()->Resume(coId);
    return result;
}

galay::kernel::HttpResult::ptr 
galay::kernel::GY_HttpAsyncClient::Delete(const std::string &url, protocol::http::HttpRequest::ptr request, bool autoClose)
{
    HttpResult::ptr result = std::make_shared<HttpResult>(this->m_tcpClient->GetCoPool());
    result->AddTaskNum(1);
    std::tuple<std::string,uint16_t,std::string> res = ParseUrl(url);
    request->Header()->Uri() = std::get<2>(res);
    request->Header()->Method() = "DELETE";
    auto co = Unary(std::get<0>(res), std::get<1>(res), request->EncodePdu(), result, autoClose);
    uint64_t coId = co.GetCoId();
    m_tcpClient->GetCoPool().lock()->AddCoroutine(std::move(co));
    m_tcpClient->GetCoPool().lock()->Resume(coId);
    return result;
}

galay::kernel::HttpResult::ptr 
galay::kernel::GY_HttpAsyncClient::Head(const std::string &url, protocol::http::HttpRequest::ptr request, bool autoClose)
{
    HttpResult::ptr result = std::make_shared<HttpResult>(this->m_tcpClient->GetCoPool());
    result->AddTaskNum(1);
    std::tuple<std::string,uint16_t,std::string> res = ParseUrl(url);
    request->Header()->Uri() = std::get<2>(res);
    request->Header()->Method() = "HEAD";
    auto co = Unary(std::get<0>(res), std::get<1>(res), request->EncodePdu(), result, autoClose);
    uint64_t coId = co.GetCoId();
    m_tcpClient->GetCoPool().lock()->AddCoroutine(std::move(co));
    m_tcpClient->GetCoPool().lock()->Resume(coId);
    return result;
}

galay::kernel::HttpResult::ptr 
galay::kernel::GY_HttpAsyncClient::Trace(const std::string &url, protocol::http::HttpRequest::ptr request, bool autoClose)
{
    HttpResult::ptr result = std::make_shared<HttpResult>(this->m_tcpClient->GetCoPool());
    result->AddTaskNum(1);
    std::tuple<std::string,uint16_t,std::string> res = ParseUrl(url);
    request->Header()->Uri() = std::get<2>(res);
    request->Header()->Method() = "TRACE";
    auto co = Unary(std::get<0>(res), std::get<1>(res), request->EncodePdu(), result, autoClose);
    uint64_t coId = co.GetCoId();
    m_tcpClient->GetCoPool().lock()->AddCoroutine(std::move(co));
    m_tcpClient->GetCoPool().lock()->Resume(coId);
    return result;
}

galay::kernel::HttpResult::ptr 
galay::kernel::GY_HttpAsyncClient::Patch(const std::string &url, protocol::http::HttpRequest::ptr request, bool autoClose)
{
    HttpResult::ptr result = std::make_shared<HttpResult>(this->m_tcpClient->GetCoPool());
    result->AddTaskNum(1);
    std::tuple<std::string,uint16_t,std::string> res = ParseUrl(url);
    request->Header()->Uri() = std::get<2>(res);
    request->Header()->Method() = "PATCH";
    auto co = Unary(std::get<0>(res), std::get<1>(res), request->EncodePdu(), result, autoClose);
    uint64_t coId = co.GetCoId();
    m_tcpClient->GetCoPool().lock()->AddCoroutine(std::move(co));
    m_tcpClient->GetCoPool().lock()->Resume(coId);
    return result;
}

galay::common::GY_NetCoroutine<galay::common::CoroutineStatus> 
galay::kernel::GY_HttpAsyncClient::Unary(std::string host, uint16_t port, std::string &&buffer, HttpResult::ptr result, bool autoClose)
{
    std::string request = std::forward<std::string>(buffer);
    co_await std::suspend_always{};
    if(!m_tcpClient->Connected()){
        auto res = m_tcpClient->Connect(host, port);
        co_await res->Wait();
        if(!res->Success()) {
            res->m_errMsg = "Connect:" + res->Error();
            spdlog::error("[{}:{}] [Unary.Connect(host:{}, port:{}) failed, Error:{}]", __FILE__, __LINE__, host, port, res->Error());
            result->Done();
            co_return common::CoroutineStatus::kCoroutineFinished;
        }
    }
    auto res = m_tcpClient->Send(std::move(request));
    co_await res->Wait();
    if(!res->Success()) {
        res->m_errMsg = "Send:" + res->Error();
        result->Done();
        co_return common::CoroutineStatus::kCoroutineFinished;
    }
    std::string responseStr;
    do{
        std::string temp;
        res = m_tcpClient->Recv(temp);
        co_await res->Wait();
        spdlog::debug("[{}:{}] [Unary.Recv Buffer: {}]", __FILE__, __LINE__, temp);
        responseStr += temp;
        if(result->m_response->DecodePdu(responseStr) == protocol::ProtoJudgeType::kProtoFinished) break;
    }while(1);
    result->m_success = true;
    if(!result->m_errMsg.empty()) result->m_errMsg.clear();
    if(autoClose){
        m_tcpClient->Close();
    }
    result->Done();
    co_return common::CoroutineStatus::kCoroutineFinished;
}

std::tuple<std::string,uint16_t,std::string> 
galay::kernel::GY_HttpAsyncClient::ParseUrl(const std::string& url)
{
    std::regex uriRegex(R"((http|https):\/\/(\d{1,3}.\d{1,3}.\d{1,3}.\d{1,3})(:\d+)?(/.*)?)");
    std::smatch match;
    std::tuple<std::string,uint16_t,std::string> res;
    std::string uri,host;
    uint16_t port = 80;
    if (std::regex_search(url, match, uriRegex))
    {
        host = match.str(2);
        if (match[3].matched)
        {
            std::string port_str = match.str(3);
            port_str.erase(0, 1);
            port = std::stoi(port_str);
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

// galay::kernel::GY_HttpAsyncClient::GY_HttpAsyncClient()
// {
//     this->m_tcpClient = std::make_shared<GY_TcpClient>();
// }

// galay::common::HttpAwaiter
// galay::kernel::GY_HttpAsyncClient::Get(const std::string& url, bool keepalive)
// {
//     protocol::http::HttpRequest::ptr request = std::make_shared<protocol::http::HttpRequest>();
//     request->Header()->Method() = "GET";
//     request->Header()->Version() = this->m_version;
//     std::tuple<std::string,uint16_t,std::string> res = ParseUrl(url);
//     request->Header()->Uri() = std::get<2>(res);
//     SetHttpHeaders(request);
//     std::string host = std::get<0>(res);
//     uint16_t port = std::get<1>(res);
//     std::function func = [this, request, host , port]() -> galay::protocol::http::HttpResponse::ptr
//     {
//         if(Connect(host,port) == -1) return nullptr;
//         return ExecMethod(request->EncodePdu());
//     };
//     return common::HttpAwaiter(true, func,this->m_futures);
// }

// galay::common::HttpAwaiter
// galay::kernel::GY_HttpAsyncClient::Post(const std::string& url ,std::string &&body, bool keepalive)
// {
//     auto request = std::make_shared<protocol::http::HttpRequest>();
//     request->Header()->Method() = "POST";
//     request->Header()->Version() = this->m_version;
//     std::tuple<std::string,uint16_t,std::string> res = ParseUrl(url);
//     request->Header()->Uri() = std::get<2>(res);
//     SetHttpHeaders(request);
//     std::string host = std::get<0>(res);
//     uint16_t port = std::get<1>(res);
//     request->Body() = std::forward<std::string&&>(body);
//     std::function func = [this, request,host,port]() -> galay::protocol::http::HttpResponse::ptr
//     {
//         if(Connect(host,port) == -1) return nullptr;
//         return ExecMethod(request->EncodePdu());
//     };
//     return common::HttpAwaiter(true, func, this->m_futures);
// }

// galay::common::HttpAwaiter
// galay::kernel::GY_HttpAsyncClient::Options(const std::string& url)
// {
//     protocol::http::HttpRequest::ptr request = std::make_shared<protocol::http::HttpRequest>();
//     request->Header()->Method() = "OPTIONS";
//     request->Header()->Version() = this->m_version;
//     std::tuple<std::string,uint16_t,std::string> res = ParseUrl(url);
//     request->Header()->Uri() = std::get<2>(res);
//     SetHttpHeaders(request);
//     std::string host = std::get<0>(res);
//     uint16_t port = std::get<1>(res);
//     std::function func = [this, request, host , port]() -> galay::protocol::http::HttpResponse::ptr
//     {
//         if(Connect(host,port) == -1) return nullptr;
//         return ExecMethod(request->EncodePdu());
//     };
//     return common::HttpAwaiter(true, func, this->m_futures);
// }

// void galay::kernel::GY_HttpAsyncClient::Close()
// {
//     m_tcpClient->Close();
// }



// void 
// galay::kernel::GY_HttpAsyncClient::SetHttpHeaders(protocol::http::HttpRequest::ptr request)
// {
//     if (!this->m_keepalive)
//         request->Header()->Headers()["Connection"] = "close";
//     else
//         request->Header()->Headers()["Connection"] = "keepalive";
//     request->Header()->Headers()["Server"] = "galay-server";
//     for(auto &[k,v] : this->m_headers){
//         request->Header()->Headers()[k] = v;
//     }
// }

// int 
// galay::kernel::GY_HttpAsyncClient::Connect(const std::string& ip, uint16_t port)
// {
//     if (!this->m_isconnected)
//     {
//         this->m_fd = IOFunction::NetIOFunction::TcpFunction::Sock();
//         spdlog::info("[{}:{}] [Socket(fd :{}) init]", __FILE__, __LINE__, this->m_fd);
//         int ret = IOFunction::NetIOFunction::TcpFunction::Conncet(this->m_fd, ip , port);
//         if (ret < 0)
//         {
//             spdlog::error("[{}:{}] [Connect(fd:{}) to {}:{} failed]", __FILE__, __LINE__, this->m_fd, ip , port);
//             Close();
//             return -1;
//         }
//         spdlog::info("[{}:{}] [Connect(fd:{}) to {}:{} success]", __FILE__, __LINE__, this->m_fd,  ip , port);
//         this->m_isconnected = true;
//     }
//     return 0;
// }

// galay::protocol::http::HttpResponse::ptr
// galay::kernel::GY_HttpAsyncClient::ExecMethod(std::string reqStr)
// {
//     int len = 0 , offset = 0;
//     do
//     {
//         len = IOFunction::NetIOFunction::TcpFunction::Send(this->m_fd, reqStr.data()+offset, reqStr.size());
//         if (len > 0)
//         {
//             spdlog::debug("[{}:{}] [Send(fd:{}) [Len:{} Bytes] [Msg:{}]]", __FILE__, __LINE__, this->m_fd, len, reqStr.substr(0, len));
//             offset += len;
//         }
//         else
//         {
//             spdlog::error("[{}:{}] [Send(fd:{}) Errmsg:[{}]]", __FILE__, __LINE__, this->m_fd , strerror(errno));
//             Close();
//             return nullptr;
//         }
//     } while (offset < reqStr.size());
//     protocol::http::HttpResponse::ptr response = std::make_shared<protocol::http::HttpResponse>();
//     char buffer[1024];
//     std::string respStr;
//     do
//     {
//         bzero(buffer, 1024);
//         len = IOFunction::NetIOFunction::TcpFunction::Recv(this->m_fd, buffer, 1024);
//         if (len >= 0)
//         {
//             respStr.append(buffer, len);
//             spdlog::debug("[{}:{}] [Recv(fd:{}) [Len:{} Bytes] [Msg:{}]]", __FILE__, __LINE__, this->m_fd, len, respStr);
//             if (!respStr.empty())
//             {
//                 protocol::ProtoJudgeType type = response->DecodePdu(respStr);
//                 if (type == protocol::ProtoJudgeType::kProtoIncomplete)
//                 {
//                     continue;
//                 }
//                 else if (type == protocol::ProtoJudgeType::kProtoFinished)
//                 {
//                     spdlog::info("[{}:{}] [Recv(fd:{}) [Success]]", __FILE__, __LINE__, this->m_fd);
//                     if (!this->m_keepalive)
//                         Close();
//                     return response;
//                 }
//                 else
//                 {
//                     spdlog::error("[{}:{}] [Recv(fd:{}) [Protocol is Illegal]", __FILE__, __LINE__, this->m_fd);
//                     Close();
//                     return nullptr;
//                 }
//             }
//             else
//             {
//                 spdlog::error("[{}:{}] [Recv(fd:{}) [Errmsg:[{}]]]", __FILE__, __LINE__, this->m_fd, strerror(errno));
//                 Close();
//                 return nullptr;
//             }
//         }
//         else
//         {
//             spdlog::error("[{}:{}] [Recv(fd:{}) Errmsg:[{}]]", __FILE__, __LINE__, this->m_fd,strerror(errno));
//             Close();
//             return nullptr;
//         }
//     } while (1);
//     return nullptr;
// }

// galay::kernel::GY_SmtpAsyncClient::GY_SmtpAsyncClient(const std::string &host, uint16_t port)
// {
//     m_host = host;
//     m_port = port;
//     m_isconnected = false;
// }

// galay::common::SmtpAwaiter
// galay::kernel::GY_SmtpAsyncClient::Auth(std::string account, std::string password)
// {
//     std::queue<std::string> requests;
//     galay::protocol::smtp::Smtp_Request request;
//     requests.push(request.Hello()->EncodePdu());
//     requests.push(request.Auth()->EncodePdu());
//     requests.push(request.Account(account)->EncodePdu());
//     requests.push(request.Password(password)->EncodePdu());
//     m_ExecSendMsg = [this, requests]() -> std::vector<protocol::smtp::Smtp_Response::ptr>
//     {
//         return ExecSendMsg(requests);
//     };
//     return common::SmtpAwaiter(true,m_ExecSendMsg,this->m_futures);
// }

// galay::common::SmtpAwaiter
// galay::kernel::GY_SmtpAsyncClient::SendEmail(std::string FromEmail,const std::vector<std::string>& ToEmails,galay::protocol::smtp::SmtpMsgInfo msg)
// {
//     std::queue<std::string> requests;
//     galay::protocol::smtp::Smtp_Request request;
//     requests.push(request.MailFrom(FromEmail)->EncodePdu());
//     for(const auto& mail: ToEmails){
//         requests.push(request.RcptTo(mail)->EncodePdu());
//     }
//     requests.push(request.Data()->EncodePdu());
//     requests.push(request.Msg(msg)->EncodePdu());
//     m_ExecSendMsg = [this,requests]() -> std::vector<protocol::smtp::Smtp_Response::ptr>
//     {
//         return ExecSendMsg(requests);
//     };
//     return common::SmtpAwaiter(true,m_ExecSendMsg,this->m_futures);
// }

// galay::common::SmtpAwaiter
// galay::kernel::GY_SmtpAsyncClient::Quit()
// {
//     std::queue<std::string> requests;
//     galay::protocol::smtp::Smtp_Request request;
//     requests.push(request.Quit()->EncodePdu());
//     m_ExecSendMsg = [this, requests]() -> std::vector<protocol::smtp::Smtp_Response::ptr>
//     {
//         return ExecSendMsg(requests);
//     };
//     return common::SmtpAwaiter(true,m_ExecSendMsg,this->m_futures);
// }

// void galay::kernel::GY_SmtpAsyncClient::Close()
// {
//     if (m_isconnected)
//     {
//         close(this->m_fd);
//         m_isconnected = false;
//     }
// }

// int 
// galay::kernel::GY_SmtpAsyncClient::Connect() 
// {
//     if (!this->m_isconnected)
//     {
//         this->m_fd = IOFunction::NetIOFunction::TcpFunction::Sock();
//         spdlog::info("[{}:{}] [Socket(fd :{}) init]", __FILE__, __LINE__, this->m_fd);
//         int ret = IOFunction::NetIOFunction::TcpFunction::Conncet(this->m_fd, this->m_host, this->m_port);
//         if (ret < 0)
//         {
//             spdlog::error("[{}:{}] [Connect(fd:{}) to {}:{} failed]", __FILE__, __LINE__, this->m_fd, this->m_host, this->m_port);
//             Close();
//             return -1;
//         }
//         spdlog::info("[{}:{}] [Connect(fd:{}) to {}:{} success]", __FILE__, __LINE__, this->m_fd, this->m_host, this->m_port);
//         this->m_isconnected = true;
//     }
//     return 0;
// }

// std::vector<galay::protocol::smtp::Smtp_Response::ptr>
// galay::kernel::GY_SmtpAsyncClient::ExecSendMsg(std::queue<std::string> requests)
// {
//     if(Connect() == -1) return {};
//     std::vector<galay::protocol::smtp::Smtp_Response::ptr> res;
//     while (!requests.empty())
//     {
//         auto reqStr = requests.front();
//         requests.pop();
//         int len;
//         do
//         {
//             len = IOFunction::NetIOFunction::TcpFunction::Send(this->m_fd, reqStr.data(), reqStr.size());
//             if (len > 0)
//             {
//                 spdlog::debug("[{}:{}] [Send(fd:{}) to {}:{} [Len:{} Bytes] [Msg:{}]]", __FILE__, __LINE__, this->m_fd, this->m_host, this->m_port, len, reqStr);
//                 reqStr.erase(0, len);
//             }
//             else
//             {
//                 spdlog::error("[{}:{}] [Send(fd:{}) to {}:{} Errmsg:[{}]]", __FILE__, __LINE__, this->m_fd, this->m_host, this->m_port, strerror(errno));
//                 Close();
//                 return {};
//             }
//         } while (!reqStr.empty());
//         char buffer[1024];
//         std::string respStr;
//         do
//         {
//             bzero(buffer, 1024);
//             len = IOFunction::NetIOFunction::TcpFunction::Recv(this->m_fd, buffer, 1024);
//             if (len >= 0)
//             {
//                 respStr.append(buffer, len);
//                 spdlog::debug("[{}:{}] [Recv(fd:{}) from {}:{} [Len:{} Bytes] [Msg:{}]]", __FILE__, __LINE__, this->m_fd, this->m_host, this->m_port, len, respStr.substr(0, len));
//                 if (!respStr.empty())
//                 {
//                     galay::protocol::smtp::Smtp_Response::ptr response = std::make_shared<galay::protocol::smtp::Smtp_Response>();
//                     protocol::ProtoJudgeType type = response->Resp()->DecodePdu(respStr);
//                     if (type == protocol::ProtoJudgeType::kProtoIncomplete)
//                     {
//                         continue;
//                     }
//                     else if (type == protocol::ProtoJudgeType::kProtoFinished)
//                     {
//                         spdlog::info("[{}:{}] [Recv(fd:{}) from {}:{} [Success]]", __FILE__, __LINE__, this->m_fd, this->m_host, this->m_port);
//                         res.push_back(response);
//                         break;
//                     }
//                     else
//                     {
//                         spdlog::error("[{}:{}] [Recv(fd:{}) from {}:{} [Protocol is Illegal]", __FILE__, __LINE__, this->m_fd, this->m_host, this->m_port);
//                         Close();
//                         return {};
//                     }
//                 }
//                 else
//                 {
//                     spdlog::error("[{}:{}] [Recv(fd:{}) from {}:{} [Errmsg:[{}]]]", __FILE__, __LINE__, this->m_fd, this->m_host, this->m_port, strerror(errno));
//                     Close();
//                     return {};
//                 }
//             }
//             else
//             {
//                 spdlog::error("[{}:{}] [Recv(fd:{}) from {}:{} Errmsg:[{}]]", __FILE__, __LINE__, this->m_fd, this->m_host, this->m_port, strerror(errno));
//                 Close();
//                 return {};
//             }
//         } while (1);
//     }
//     return res;
// }

// galay::kernel::DnsAsyncClient::DnsAsyncClient(const std::string& host, uint16_t port)
// {
//     this->m_host = host;
//     this->m_port = port;
//     this->m_IsInit = false;
// }

// galay::common::DnsAwaiter 
// galay::kernel::DnsAsyncClient::QueryA(std::queue<std::string> domains)
// {
//     if(!this->m_IsInit){
//         this->m_fd = galay::IOFunction::NetIOFunction::UdpFunction::Sock();
//         this->m_IsInit = true;
//     }
//     int num = domains.size();
//     protocol::dns::Dns_Request request;
//     protocol::dns::DnsHeader header;
//     header.m_flags.m_rd = 1;
//     header.m_id = galay::util::Random::RandomInt(0,MAX_UDP_LENGTH);
//     header.m_questions = 1;
//     galay::protocol::dns::DnsQuestion question;
//     question.m_class = 1;
//     question.m_type = galay::protocol::dns::kDnsQueryA;
//     std::queue<galay::protocol::dns::DnsQuestion> questions;
//     while(!domains.empty())
//     {
//         std::string domain = domains.front();
//         domains.pop();
//         question.m_qname = domain;
//         questions.push(question);
//     }
//     request.SetQuestionQueue(std::move(questions));
//     std::queue<std::string> reqStrs;
//     for(int i = 0 ; i < num ; ++i){
//         reqStrs.push(request.EncodePdu());
//     }
//     this->m_ExecSendMsg = [this,reqStrs](){
//         return ExecSendMsg(reqStrs);
//     };
//     return common::DnsAwaiter(true,this->m_ExecSendMsg,this->m_futures);
// }

// galay::common::DnsAwaiter 
// galay::kernel::DnsAsyncClient::QueryAAAA(std::queue<std::string> domains)
// {
//     if(!this->m_IsInit){
//         this->m_fd = galay::IOFunction::NetIOFunction::UdpFunction::Sock();
//         this->m_IsInit = true;
//     }
//     int num = domains.size();
//     protocol::dns::Dns_Request request;
//     protocol::dns::DnsHeader header;
//     header.m_flags.m_rd = 1;
//     header.m_id = galay::util::Random::RandomInt(0,MAX_UDP_LENGTH);
//     header.m_questions = 1;
//     galay::protocol::dns::DnsQuestion question;
//     question.m_class = 1;
//     question.m_type = galay::protocol::dns::kDnsQueryAAAA;
//     std::queue<galay::protocol::dns::DnsQuestion> questions;
//     while(!domains.empty())
//     {
//         std::string domain = domains.front();
//         domains.pop();
//         question.m_qname = domain;
//         questions.push(question);
//     }
//     request.SetQuestionQueue(std::move(questions));
//     std::queue<std::string> reqStrs;
//     for(int i = 0 ; i < num ; ++i){
//         reqStrs.push(request.EncodePdu());
//     }
//     this->m_ExecSendMsg = [this,reqStrs](){
//         return ExecSendMsg(reqStrs);
//     };
//     return common::DnsAwaiter(true,this->m_ExecSendMsg,this->m_futures);
// }

// galay::protocol::dns::Dns_Response::ptr 
// galay::kernel::DnsAsyncClient::ExecSendMsg(std::queue<std::string> reqStr)
// {
//     protocol::dns::Dns_Response::ptr response = std::make_shared<protocol::dns::Dns_Response>();
//     char buffer[MAX_UDP_LENGTH] = {0};
//     while(!reqStr.empty()){
//         std::string req = reqStr.front();
//         reqStr.pop();
//         IOFunction::NetIOFunction::Addr toaddr;
//         toaddr.ip = this->m_host;
//         toaddr.port = this->m_port;
//         int ret = IOFunction::NetIOFunction::UdpFunction::SendTo(this->m_fd,toaddr,req);
//         if(ret == -1){
//             this->m_IsInit = false;
//             close(this->m_fd);
//             spdlog::error("[{}:{}] [SendTo(fd:{}) to {}:{} ErrMsg:[{}]]",__FILE__,__LINE__,this->m_fd,this->m_host,this->m_port,strerror(errno));
//             return response;
//         }
//         spdlog::info("[{}:{}] [SendTo(fd:{}) to {}:{} [Success]]", __FILE__, __LINE__, this->m_fd, this->m_host, this->m_port);
//         spdlog::debug("[{}:{}] [SendTo(fd:{}) to {}:{} [Len:{} Bytes]]", __FILE__, __LINE__, this->m_fd, this->m_host, this->m_port, ret);
//         IOFunction::NetIOFunction::Addr fromaddr;
//         ret = IOFunction::NetIOFunction::UdpFunction::RecvFrom(this->m_fd,fromaddr,buffer,MAX_UDP_LENGTH);
//         if(ret == -1){
//             this->m_IsInit = false;
//             close(this->m_fd);
//             spdlog::error("[{}:{}] [RecvFrom(fd:{}) from {}:{} ErrMsg:[{}]]",__FILE__,__LINE__,this->m_fd,this->m_host,this->m_port,strerror(errno));
//             return response;
//         }
//         std::string temp(buffer,ret);
//         spdlog::info("[{}:{}] [RecvFrom(fd:{}) from {}:{} [Success]]", __FILE__, __LINE__, this->m_fd, this->m_host, this->m_port);
//         spdlog::debug("[{}:{}] [RecvFrom(fd:{}) from {}:{} [Len:{} Bytes] [Msg:{}]]", __FILE__, __LINE__, this->m_fd, this->m_host, this->m_port, ret, temp);
//         response->DecodePdu(temp);
//         bzero(buffer,MAX_UDP_LENGTH);
//     }
//     return response;
// }

// void 
// galay::kernel::DnsAsyncClient::Close()
// {
//     if(this->m_IsInit){
//         close(this->m_fd);
//         this->m_IsInit = false;
//     }
// }
