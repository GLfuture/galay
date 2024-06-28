#include "client.h"
#include "../util/netiofunction.h"
#include "../util/random.h"
#include <regex>
#include <spdlog/spdlog.h>

galay::GY_HttpAsyncClient::GY_HttpAsyncClient()
{
    
    this->m_keepalive = false;
    this->m_isconnected = false;
    this->m_version = "1.1";
}

void 
galay::GY_HttpAsyncClient::KeepAlive()
{
    this->m_keepalive = true;
}

void 
galay::GY_HttpAsyncClient::CancleKeepalive()
{
    this->m_keepalive = false;
}

// void
// galay::GY_HttpAsyncClient::Upgrade(const std::string& version)
// {
//     if(version.compare(this->m_version) > 0){
//         this->m_version = version;
//     }
//     spdlog::debug("[{}:{}] [now version] [{}]",__FILE__,__LINE__,this->m_version);
// }

// void
// galay::GY_HttpAsyncClient::Downgrade(const std::string& version)
// {
//     if(version.compare(this->m_version) < 0){
//         this->m_version = version;
//     }
//     spdlog::debug("[{}:{}] [now version] [{}]",__FILE__,__LINE__,this->m_version);
// }

void 
galay::GY_HttpAsyncClient::SetVersion(const std::string& version)
{
    this->m_version = version;
}

void 
galay::GY_HttpAsyncClient::AddHeaderPair(const std::string& key, const std::string& value)
{
    this->m_headers[key] = value;
}

void 
galay::GY_HttpAsyncClient::RemoveHeaderPair(const std::string& key)
{
    this->m_headers.erase(key);
}

galay::HttpAwaiter
galay::GY_HttpAsyncClient::Get(const std::string& url)
{
    protocol::http::Http1_1_Request::ptr request = std::make_shared<protocol::http::Http1_1_Request>();
    request->SetMethod("GET");
    request->SetVersion(this->m_version);
    std::tuple<std::string,uint16_t,std::string> res = ParseUrl(url);
    request->SetUri(std::get<2>(res));
    SetHttpHeaders(request);
    std::string host = std::get<0>(res);
    uint16_t port = std::get<1>(res);
    this->m_ExecMethod = [this, request, host , port]() -> galay::protocol::http::Http1_1_Response::ptr
    {
        if(Connect(host,port) == -1) return nullptr;
        return ExecMethod(request->EncodePdu());
    };
    return HttpAwaiter(true, this->m_ExecMethod,this->m_futures);
}

galay::HttpAwaiter
galay::GY_HttpAsyncClient::Post(const std::string& url ,std::string &&body)
{
    auto request = std::make_shared<protocol::http::Http1_1_Request>();
    request->SetMethod("POST");
    request->SetVersion(this->m_version);
    std::tuple<std::string,uint16_t,std::string> res = ParseUrl(url);
    request->SetUri(std::get<2>(res));
    SetHttpHeaders(request);
    std::string host = std::get<0>(res);
    uint16_t port = std::get<1>(res);
    request->SetBody(std::forward<std::string&&>(body));
    this->m_ExecMethod = [this, request,host,port]() -> galay::protocol::http::Http1_1_Response::ptr
    {
        if(Connect(host,port) == -1) return nullptr;
        return ExecMethod(request->EncodePdu());
    };
    return HttpAwaiter(true, this->m_ExecMethod,this->m_futures);
}

galay::HttpAwaiter
galay::GY_HttpAsyncClient::Options(const std::string& url)
{
    protocol::http::Http1_1_Request::ptr request = std::make_shared<protocol::http::Http1_1_Request>();
    request->SetMethod("OPTIONS");
    request->SetVersion(this->m_version);
    std::tuple<std::string,uint16_t,std::string> res = ParseUrl(url);
    request->SetUri(std::get<2>(res));
    SetHttpHeaders(request);
    std::string host = std::get<0>(res);
    uint16_t port = std::get<1>(res);
    this->m_ExecMethod = [this, request, host , port]() -> galay::protocol::http::Http1_1_Response::ptr
    {
        if(Connect(host,port) == -1) return nullptr;
        return ExecMethod(request->EncodePdu());
    };
    return HttpAwaiter(true, this->m_ExecMethod,this->m_futures);
}

void galay::GY_HttpAsyncClient::Close()
{
    if (m_isconnected)
    {
        close(this->m_fd);
        m_isconnected = false;
    }
}

std::tuple<std::string,uint16_t,std::string> 
galay::GY_HttpAsyncClient::ParseUrl(const std::string& url)
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

void 
galay::GY_HttpAsyncClient::SetHttpHeaders(protocol::http::Http1_1_Request::ptr request)
{
    if (!this->m_keepalive)
        request->SetHeadPair({"Connection", "close"});
    else
        request->SetHeadPair({"Connection", "keepalive"});
    request->SetHeadPair({"Server", "galay-server"});
    request->SetHeaders(this->m_headers);
}

int 
galay::GY_HttpAsyncClient::Connect(const std::string& ip, uint16_t port)
{
    if (!this->m_isconnected)
    {
        this->m_fd = IOFunction::NetIOFunction::TcpFunction::Sock();
        spdlog::info("[{}:{}] [Socket(fd :{}) init]", __FILE__, __LINE__, this->m_fd);
        int ret = IOFunction::NetIOFunction::TcpFunction::Conncet(this->m_fd, ip , port);
        if (ret < 0)
        {
            spdlog::error("[{}:{}] [Connect(fd:{}) to {}:{} failed]", __FILE__, __LINE__, this->m_fd, ip , port);
            Close();
            return -1;
        }
        spdlog::info("[{}:{}] [Connect(fd:{}) to {}:{} success]", __FILE__, __LINE__, this->m_fd,  ip , port);
        this->m_isconnected = true;
    }
    return 0;
}

galay::protocol::http::Http1_1_Response::ptr
galay::GY_HttpAsyncClient::ExecMethod(std::string reqStr)
{
    int len = 0 , offset = 0;
    do
    {
        len = IOFunction::NetIOFunction::TcpFunction::Send(this->m_fd, reqStr, reqStr.size());
        if (len > 0)
        {
            spdlog::debug("[{}:{}] [Send(fd:{}) [Len:{} Bytes] [Msg:{}]]", __FILE__, __LINE__, this->m_fd, len, reqStr.substr(0, len));
            offset += len;
        }
        else
        {
            spdlog::error("[{}:{}] [Send(fd:{}) Errmsg:[{}]]", __FILE__, __LINE__, this->m_fd , strerror(errno));
            Close();
            return nullptr;
        }
    } while (offset < reqStr.size());
    protocol::http::Http1_1_Response::ptr response = std::make_shared<protocol::http::Http1_1_Response>();
    char buffer[1024];
    std::string respStr;
    do
    {
        bzero(buffer, 1024);
        len = IOFunction::NetIOFunction::TcpFunction::Recv(this->m_fd, buffer, 1024);
        if (len >= 0)
        {
            respStr.append(buffer, len);
            spdlog::debug("[{}:{}] [Recv(fd:{}) [Len:{} Bytes] [Msg:{}]]", __FILE__, __LINE__, this->m_fd, len, respStr);
            if (!respStr.empty())
            {
                ProtoJudgeType type = response->DecodePdu(respStr);
                if (type == ProtoJudgeType::kProtoIncomplete)
                {
                    continue;
                }
                else if (type == ProtoJudgeType::kProtoFinished)
                {
                    spdlog::info("[{}:{}] [Recv(fd:{}) [Success]]", __FILE__, __LINE__, this->m_fd);
                    if (!this->m_keepalive)
                        Close();
                    return response;
                }
                else
                {
                    spdlog::error("[{}:{}] [Recv(fd:{}) [Protocol is Illegal]", __FILE__, __LINE__, this->m_fd);
                    Close();
                    return nullptr;
                }
            }
            else
            {
                spdlog::error("[{}:{}] [Recv(fd:{}) [Errmsg:[{}]]]", __FILE__, __LINE__, this->m_fd, strerror(errno));
                Close();
                return nullptr;
            }
        }
        else
        {
            spdlog::error("[{}:{}] [Recv(fd:{}) Errmsg:[{}]]", __FILE__, __LINE__, this->m_fd,strerror(errno));
            Close();
            return nullptr;
        }
    } while (1);
    return nullptr;
}

galay::GY_SmtpAsyncClient::GY_SmtpAsyncClient(const std::string &host, uint16_t port)
{
    m_host = host;
    m_port = port;
    m_isconnected = false;
}

galay::SmtpAwaiter
galay::GY_SmtpAsyncClient::Auth(std::string account, std::string password)
{
    std::queue<std::string> requests;
    galay::protocol::smtp::Smtp_Request request;
    requests.push(request.Hello()->GetContent());
    requests.push(request.Auth()->GetContent());
    requests.push(request.Account(account)->GetContent());
    requests.push(request.Password(password)->GetContent());
    m_ExecSendMsg = [this, requests]() -> std::vector<protocol::smtp::Smtp_Response::ptr>
    {
        return ExecSendMsg(requests);
    };
    return SmtpAwaiter(true,m_ExecSendMsg,this->m_futures);
}

galay::SmtpAwaiter
galay::GY_SmtpAsyncClient::SendEmail(std::string FromEmail,const std::vector<std::string>& ToEmails,galay::protocol::smtp::SmtpMsgInfo msg)
{
    std::queue<std::string> requests;
    galay::protocol::smtp::Smtp_Request request;
    requests.push(request.MailFrom(FromEmail)->GetContent());
    for(const auto& mail: ToEmails){
        requests.push(request.RcptTo(mail)->GetContent());
    }
    requests.push(request.Data()->GetContent());
    requests.push(request.Msg(msg)->GetContent());
    m_ExecSendMsg = [this,requests]() -> std::vector<protocol::smtp::Smtp_Response::ptr>
    {
        return ExecSendMsg(requests);
    };
    return SmtpAwaiter(true,m_ExecSendMsg,this->m_futures);
}

galay::SmtpAwaiter
galay::GY_SmtpAsyncClient::Quit()
{
    std::queue<std::string> requests;
    galay::protocol::smtp::Smtp_Request request;
    requests.push(request.Quit()->GetContent());
    m_ExecSendMsg = [this, requests]() -> std::vector<protocol::smtp::Smtp_Response::ptr>
    {
        return ExecSendMsg(requests);
    };
    return SmtpAwaiter(true,m_ExecSendMsg,this->m_futures);
}

void galay::GY_SmtpAsyncClient::Close()
{
    if (m_isconnected)
    {
        close(this->m_fd);
        m_isconnected = false;
    }
}

int 
galay::GY_SmtpAsyncClient::Connect() 
{
    if (!this->m_isconnected)
    {
        this->m_fd = IOFunction::NetIOFunction::TcpFunction::Sock();
        spdlog::info("[{}:{}] [Socket(fd :{}) init]", __FILE__, __LINE__, this->m_fd);
        int ret = IOFunction::NetIOFunction::TcpFunction::Conncet(this->m_fd, this->m_host, this->m_port);
        if (ret < 0)
        {
            spdlog::error("[{}:{}] [Connect(fd:{}) to {}:{} failed]", __FILE__, __LINE__, this->m_fd, this->m_host, this->m_port);
            Close();
            return -1;
        }
        spdlog::info("[{}:{}] [Connect(fd:{}) to {}:{} success]", __FILE__, __LINE__, this->m_fd, this->m_host, this->m_port);
        this->m_isconnected = true;
    }
    return 0;
}

std::vector<galay::protocol::smtp::Smtp_Response::ptr>
galay::GY_SmtpAsyncClient::ExecSendMsg(std::queue<std::string> requests)
{
    if(Connect() == -1) return {};
    std::vector<galay::protocol::smtp::Smtp_Response::ptr> res;
    while (!requests.empty())
    {
        auto reqStr = requests.front();
        requests.pop();
        int len;
        do
        {
            len = IOFunction::NetIOFunction::TcpFunction::Send(this->m_fd, reqStr, reqStr.size());
            if (len > 0)
            {
                spdlog::debug("[{}:{}] [Send(fd:{}) to {}:{} [Len:{} Bytes] [Msg:{}]]", __FILE__, __LINE__, this->m_fd, this->m_host, this->m_port, len, reqStr);
                reqStr.erase(0, len);
            }
            else
            {
                spdlog::error("[{}:{}] [Send(fd:{}) to {}:{} Errmsg:[{}]]", __FILE__, __LINE__, this->m_fd, this->m_host, this->m_port, strerror(errno));
                Close();
                return {};
            }
        } while (!reqStr.empty());
        char buffer[1024];
        std::string respStr;
        do
        {
            bzero(buffer, 1024);
            len = IOFunction::NetIOFunction::TcpFunction::Recv(this->m_fd, buffer, 1024);
            if (len >= 0)
            {
                respStr.append(buffer, len);
                spdlog::debug("[{}:{}] [Recv(fd:{}) from {}:{} [Len:{} Bytes] [Msg:{}]]", __FILE__, __LINE__, this->m_fd, this->m_host, this->m_port, len, respStr.substr(0, len));
                if (!respStr.empty())
                {
                    galay::protocol::smtp::Smtp_Response::ptr response = std::make_shared<galay::protocol::smtp::Smtp_Response>();
                    ProtoJudgeType type = response->Resp()->DecodePdu(respStr);
                    if (type == ProtoJudgeType::kProtoIncomplete)
                    {
                        continue;
                    }
                    else if (type == ProtoJudgeType::kProtoFinished)
                    {
                        spdlog::info("[{}:{}] [Recv(fd:{}) from {}:{} [Success]]", __FILE__, __LINE__, this->m_fd, this->m_host, this->m_port);
                        res.push_back(response);
                        break;
                    }
                    else
                    {
                        spdlog::error("[{}:{}] [Recv(fd:{}) from {}:{} [Protocol is Illegal]", __FILE__, __LINE__, this->m_fd, this->m_host, this->m_port);
                        Close();
                        return {};
                    }
                }
                else
                {
                    spdlog::error("[{}:{}] [Recv(fd:{}) from {}:{} [Errmsg:[{}]]]", __FILE__, __LINE__, this->m_fd, this->m_host, this->m_port, strerror(errno));
                    Close();
                    return {};
                }
            }
            else
            {
                spdlog::error("[{}:{}] [Recv(fd:{}) from {}:{} Errmsg:[{}]]", __FILE__, __LINE__, this->m_fd, this->m_host, this->m_port, strerror(errno));
                Close();
                return {};
            }
        } while (1);
    }
    return res;
}

galay::DnsAsyncClient::DnsAsyncClient(const std::string& host, uint16_t port)
{
    this->m_host = host;
    this->m_port = port;
    this->m_IsInit = false;
}

galay::DnsAwaiter 
galay::DnsAsyncClient::QueryA(std::queue<std::string> domains)
{
    if(!this->m_IsInit){
        this->m_fd = galay::IOFunction::NetIOFunction::UdpFunction::Sock();
        this->m_IsInit = true;
    }
    int num = domains.size();
    protocol::dns::Dns_Request request;
    protocol::dns::DnsHeader header;
    header.m_flags.m_rd = 1;
    header.m_id = galay::util::Random::random(0,MAX_UDP_LENGTH);
    header.m_questions = 1;
    galay::protocol::dns::DnsQuestion question;
    question.m_class = 1;
    question.m_type = galay::protocol::dns::kDnsQueryA;
    std::queue<galay::protocol::dns::DnsQuestion> questions;
    while(!domains.empty())
    {
        std::string domain = domains.front();
        domains.pop();
        question.m_qname = domain;
        questions.push(question);
    }
    request.SetQuestionQueue(std::move(questions));
    std::queue<std::string> reqStrs;
    for(int i = 0 ; i < num ; ++i){
        reqStrs.push(request.EncodePdu());
    }
    this->m_ExecSendMsg = [this,reqStrs](){
        return ExecSendMsg(reqStrs);
    };
    return DnsAwaiter(true,this->m_ExecSendMsg,this->m_futures);
}

galay::DnsAwaiter 
galay::DnsAsyncClient::QueryAAAA(std::queue<std::string> domains)
{
    if(!this->m_IsInit){
        this->m_fd = galay::IOFunction::NetIOFunction::UdpFunction::Sock();
        this->m_IsInit = true;
    }
    int num = domains.size();
    protocol::dns::Dns_Request request;
    protocol::dns::DnsHeader header;
    header.m_flags.m_rd = 1;
    header.m_id = galay::util::Random::random(0,MAX_UDP_LENGTH);
    header.m_questions = 1;
    galay::protocol::dns::DnsQuestion question;
    question.m_class = 1;
    question.m_type = galay::protocol::dns::kDnsQueryAAAA;
    std::queue<galay::protocol::dns::DnsQuestion> questions;
    while(!domains.empty())
    {
        std::string domain = domains.front();
        domains.pop();
        question.m_qname = domain;
        questions.push(question);
    }
    request.SetQuestionQueue(std::move(questions));
    std::queue<std::string> reqStrs;
    for(int i = 0 ; i < num ; ++i){
        reqStrs.push(request.EncodePdu());
    }
    this->m_ExecSendMsg = [this,reqStrs](){
        return ExecSendMsg(reqStrs);
    };
    return DnsAwaiter(true,this->m_ExecSendMsg,this->m_futures);
}

galay::protocol::dns::Dns_Response::ptr 
galay::DnsAsyncClient::ExecSendMsg(std::queue<std::string> reqStr)
{
    protocol::dns::Dns_Response::ptr response = std::make_shared<protocol::dns::Dns_Response>();
    char buffer[MAX_UDP_LENGTH] = {0};
    while(!reqStr.empty()){
        std::string req = reqStr.front();
        reqStr.pop();
        IOFunction::NetIOFunction::Addr toaddr;
        toaddr.ip = this->m_host;
        toaddr.port = this->m_port;
        int ret = IOFunction::NetIOFunction::UdpFunction::SendTo(this->m_fd,toaddr,req);
        if(ret == -1){
            this->m_IsInit = false;
            close(this->m_fd);
            spdlog::error("[{}:{}] [SendTo(fd:{}) to {}:{} ErrMsg:[{}]]",__FILE__,__LINE__,this->m_fd,this->m_host,this->m_port,strerror(errno));
            return response;
        }
        spdlog::info("[{}:{}] [SendTo(fd:{}) to {}:{} [Success]]", __FILE__, __LINE__, this->m_fd, this->m_host, this->m_port);
        spdlog::debug("[{}:{}] [SendTo(fd:{}) to {}:{} [Len:{} Bytes]]", __FILE__, __LINE__, this->m_fd, this->m_host, this->m_port, ret);
        IOFunction::NetIOFunction::Addr fromaddr;
        ret = IOFunction::NetIOFunction::UdpFunction::RecvFrom(this->m_fd,fromaddr,buffer,MAX_UDP_LENGTH);
        if(ret == -1){
            this->m_IsInit = false;
            close(this->m_fd);
            spdlog::error("[{}:{}] [RecvFrom(fd:{}) from {}:{} ErrMsg:[{}]]",__FILE__,__LINE__,this->m_fd,this->m_host,this->m_port,strerror(errno));
            return response;
        }
        std::string temp(buffer,ret);
        spdlog::info("[{}:{}] [RecvFrom(fd:{}) from {}:{} [Success]]", __FILE__, __LINE__, this->m_fd, this->m_host, this->m_port);
        spdlog::debug("[{}:{}] [RecvFrom(fd:{}) from {}:{} [Len:{} Bytes] [Msg:{}]]", __FILE__, __LINE__, this->m_fd, this->m_host, this->m_port, ret, temp);
        response->DecodePdu(temp);
        bzero(buffer,MAX_UDP_LENGTH);
    }
    return response;
}



void 
galay::DnsAsyncClient::Close()
{
    if(this->m_IsInit){
        close(this->m_fd);
        this->m_IsInit = false;
    }
}
