#include "io.h"
#include <filesystem>
#include <fstream>
#include <spdlog/spdlog.h>

#ifdef __linux__
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <netinet/tcp.h>
#endif


namespace galay::io
{
int 
BlockFuction::SetNoBlock(int fd)
{
    int flag = fcntl(fd, F_GETFL, 0);
    flag |= O_NONBLOCK;
    return fcntl(fd, F_SETFL, flag);
}

int 
BlockFuction::SetBlock(int fd)
{
    int flag = fcntl(fd, F_GETFL, 0);
    flag &= ~O_NONBLOCK;
    return fcntl(fd, F_SETFL, flag);
}

}

namespace galay::io::net
{

SSLConfig::SSLConfig()
{
    m_min_ssl_version.store(DEFAULT_SSL_MIN_VERSION);
    m_max_ssl_version.store(DEFAULT_SSL_MAX_VERSION);
    m_ssl_cert_path = DEFAULT_SSL_CERT_PATH;
    m_ssl_key_path = DEFAULT_SSL_KEY_PATH;
}

void 
SSLConfig::SetSSLVersion(int32_t min_ssl_version, int32_t max_ssl_version)
{
    m_min_ssl_version.store(min_ssl_version);
    m_max_ssl_version.store(max_ssl_version);
}

void 
SSLConfig::SetCertPath(const std::string& cert_path)
{
    m_ssl_cert_path = cert_path;
}

void 
SSLConfig::SetKeyPath(const std::string& key_path)
{
    m_ssl_key_path = key_path;
}

int32_t 
SSLConfig::GetMinSSLVersion() const
{
    return m_min_ssl_version.load();
}

int32_t 
SSLConfig::GetMaxSSLVersion() const
{
    return m_max_ssl_version.load();
}


std::string 
SSLConfig::GetCertPath() const
{
    return m_ssl_cert_path;
}

std::string 
SSLConfig::GetKeyPath() const 
{
    return m_ssl_key_path;
}

//tcp
int 
TcpFunction::Sock()
{
    return socket(AF_INET, SOCK_STREAM, 0);
}

int 
TcpFunction::Sock6()
{
    return socket(AF_INET6, SOCK_STREAM, 0);
}

int 
TcpFunction::SockKeepalive(int fd , int t_idle , int t_interval , int retry)
{
    int optval = 1;
    int ret ;
    ret = setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, (const void *)&optval, sizeof(optval));
    if(ret != 0) return ret;
    ret = setsockopt(fd,SOL_TCP,TCP_KEEPIDLE,(void*)&t_idle,sizeof(t_idle));
    if(ret != 0) return ret;
    ret = setsockopt(fd,SOL_TCP,TCP_KEEPINTVL,(void*)&t_interval,sizeof(t_interval));
    if(ret != 0) return ret;
    ret = setsockopt(fd,SOL_TCP,TCP_KEEPCNT,(void*)&retry,sizeof(retry));
    return ret;
}

int 
TcpFunction::ReuseAddr(int fd)
{
    int option = 1;
    return setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (void *)&option, sizeof(option));
}

int 
TcpFunction::ReusePort(int fd)
{
    int option = 1;
    return setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, (void *)&option, sizeof(option));
}

int 
TcpFunction::Conncet(int fd , std::string sip, uint32_t sport)
{
    sockaddr_in sin = {0};
    memset(&sin, 0, sizeof(sin));
    sin.sin_addr.s_addr = inet_addr(sip.c_str());
    sin.sin_family = AF_INET;
    sin.sin_port = htons(sport);
    int ret = connect(fd, (sockaddr *)&sin, sizeof(sin));
    if (ret == -1) return ret;
    return 0;
}

int 
TcpFunction::Bind(int fd, uint16_t port)
{
    sockaddr_in sin = {0};
    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_port = htons(port);
    sin.sin_addr.s_addr = INADDR_ANY;
    return bind(fd, (sockaddr *)&sin, sizeof(sin));
}

int 
TcpFunction::Listen(int fd,uint16_t backlog)
{
    return listen(fd, backlog);
}

int 
TcpFunction::Accept(int fd)
{
    sockaddr_in sin = {0};
    memset(&sin, 0, sizeof(sin));
    socklen_t len = sizeof(sin);
    return accept(fd, (sockaddr *)&sin, &len);
}

ssize_t 
TcpFunction::Recv(int fd, char* buffer, uint32_t len)
{
    return recv(fd, buffer, len, 0);
}

ssize_t 
TcpFunction::Send(int fd, const char* buffer , uint32_t len)
{
    return send(fd, buffer, len, 0);
}

SSL_CTX* 
TcpFunction::InitSSLServer(long min_version, long max_version)
{
    InitSSLEnv();
    return InitServerSSLCtx(min_version,max_version);
}


SSL_CTX* 
TcpFunction::InitSSLClient(long min_version, long max_version)
{
    InitSSLEnv();
    return InitClientSSLCtx(min_version,max_version);
}

int 
TcpFunction::SetSSLCertAndKey(SSL_CTX *ctx , const char* cert_filepath , const char* key_filepath)
{
    if (SSL_CTX_use_certificate_file(ctx, cert_filepath, SSL_FILETYPE_PEM) <= 0) {
        return -1;
    }

    if (SSL_CTX_use_PrivateKey_file(ctx, key_filepath, SSL_FILETYPE_PEM) <= 0 ) {
        return -1;
    }
    return 0;
}


SSL* 
TcpFunction::SSLCreateObj( SSL_CTX* ctx , int fd)
{
    SSL* ssl = SSL_new(ctx);
    SSL_set_fd(ssl,fd);
    return ssl;
}

int 
TcpFunction::SSLReset(SSL *ssl, int fd)
{
    return SSL_set_fd(ssl,fd);
}

int 
TcpFunction::SSLConnect(SSL* ssl)
{
    return SSL_connect(ssl);
}

int 
TcpFunction::SSLRecv(SSL* ssl,char* buffer,int len)
{
    return SSL_read(ssl,buffer,len);
}


int 
TcpFunction::SSLSend(SSL* ssl,const char* buffer,int len)
{
    return SSL_write(ssl,buffer,len);
}

int 
TcpFunction::SSLAccept(SSL* ssl)
{
    return SSL_accept(ssl);
}

void 
TcpFunction::SSLDestory(SSL* ssl)
{
    SSL_shutdown(ssl);
    SSL_free(ssl);
}

void 
TcpFunction::SSLDestory(std::vector<SSL*> ssls, SSL_CTX *ctx) 
{
    for(auto ssl:ssls)
    {
        SSL_shutdown(ssl);
        SSL_free(ssl);
    }
    SSLDestoryCtx(ctx);
    SSLDestoryEnv();
}


void 
TcpFunction::InitSSLEnv()
{
    SSL_library_init();
    ERR_load_crypto_strings();
    SSL_load_error_strings();
}

SSL_CTX* 
TcpFunction::InitServerSSLCtx(long min_version,long max_version)
{
    SSL_CTX* ctx = SSL_CTX_new(TLS_server_method()); // 创建客户端 SSL 上下文对象
    SSL_CTX_set_min_proto_version(ctx, min_version); // 最小支持 TLSv1.3 协议
    SSL_CTX_set_max_proto_version(ctx, max_version); // 最大支持 TLSv1.3 协议
    return ctx;
}

SSL_CTX* 
TcpFunction::InitClientSSLCtx(long min_version,long max_version)
{
    SSL_CTX* ctx = SSL_CTX_new(TLS_client_method()); // 创建客户端 SSL 上下文对象
    SSL_CTX_set_min_proto_version(ctx, min_version); // 最小支持 TLSv1.3 协议
    SSL_CTX_set_max_proto_version(ctx, max_version); // 最大支持 TLSv1.3 协议
    return ctx;
}


void 
TcpFunction::SSLDestoryCtx(SSL_CTX* ctx)
{
    SSL_CTX_free(ctx);
}

void 
TcpFunction::SSLDestoryEnv()
{
    ERR_free_strings();
    EVP_cleanup();
}

//udp
int 
UdpFunction::Sock()
{
    return socket(AF_INET,SOCK_DGRAM,0);
}

int 
UdpFunction::Bind(int fd, uint32_t port)
{
    sockaddr_in sin = {0};
    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_port = htons(port);
    sin.sin_addr.s_addr = INADDR_ANY;
    return bind(fd,(sockaddr*)&sin,sizeof(sin));
}

ssize_t 
UdpFunction::RecvFrom(int fd , Addr& addr , char* buffer , int len)
{
    sockaddr_in sin = {0};
    socklen_t slen = sizeof(sin);
    ssize_t ret = recvfrom(fd,buffer,len,0,(sockaddr*) &sin,&slen);
    if(ret <= 0)
    {
        return ret;
    }
    char ip[INET_ADDRSTRLEN] = {0};
    inet_ntop(AF_INET,&sin.sin_addr,ip,INET_ADDRSTRLEN);
    addr.ip.assign(ip,INET_ADDRSTRLEN);
    addr.port = ntohs(sin.sin_port);
    return ret;
}

ssize_t 
UdpFunction::SendTo(int fd,const Addr& addr,const std::string& buffer)
{
    sockaddr_in sin = {0};
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = inet_addr(addr.ip.c_str());
    sin.sin_port = htons(addr.port);
    return sendto(fd,buffer.c_str(),buffer.length(),0,(sockaddr*)&sin,sizeof(sin));
}
}


namespace galay::io::file
{
std::string 
SyncFileStream::ReadFile(const std::string& FileName, bool IsBinary)
{
    std::ifstream in;
    if(IsBinary){
        in.open(FileName, std::ios::binary);
    }else{
        in.open(FileName);
    }
    if(in.fail()) {
        spdlog::error("[{}:{}] [open file error]",__FILE__,__LINE__);
        return "";
    }
    uintmax_t size = std::filesystem::file_size(FileName);
    char *buffer = new char[size];
    in.read(buffer, size);
    spdlog::debug("[{}:{}] [read from file: {} Bytes]",__FILE__,__LINE__,size);
    in.close();
    std::string res(buffer,size);
    delete[] buffer;
    return res;
}

void 
SyncFileStream::WriteFile(const std::string& FileName,const std::string& Content, bool IsBinary)
{
    std::ofstream out;
    if(IsBinary){
        out.open(FileName, std::ios::binary);
    }else{
        out.open(FileName);
    }
    if(out.fail()){
        std::string path = std::filesystem::current_path();
        spdlog::error("[{}:{}] [open file: {} failed , now path is {}]",__FILE__,__LINE__,FileName,path);
        return ;
    }
    spdlog::debug("[{}:{}] [write to file: {} Bytes]",__FILE__,__LINE__,Content.length());
    out.write(Content.c_str(),Content.length());
    out.close();
}

#ifdef __linux__
std::string 
ZeroCopyFile::ReadFile(const std::string &FileName)
{
    int fd = open(FileName.c_str(),O_RDONLY);
    if(fd == -1){
        std::string path = std::filesystem::current_path();
        spdlog::error("[{}:{}] [open file: {} failed , now path is {}]",__FILE__,__LINE__,FileName,path);
        return "";
    }
    struct stat st;
    if(fstat(fd,&st) == -1){
        spdlog::error("[{}:{}] [fstat file: {} failed]",__FILE__,__LINE__,FileName);
        close(fd);
        return "";
    }
    char *p = reinterpret_cast<char*>(mmap(nullptr,st.st_size,PROT_READ,MAP_PRIVATE,fd,0));
    if( p == MAP_FAILED) 
    {
        spdlog::error("[{}:{}] [mmap file: {} failed]",__FILE__,__LINE__,FileName);
        close(fd);
        return "";
    }
    std::string res(p,st.st_size);
    munmap(p,st.st_size);
    close(fd);
    return std::move(res);
}


void 
ZeroCopyFile::WriteFile(const std::string &FileName, const std::string &Content,bool IsBinary)
{
    int fd = open(FileName.c_str(),O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR);
    if(fd == -1) {
        std::string path = std::filesystem::current_path();
        spdlog::error("[{}:{}] [open file: {} failed, now path is {}]",__FILE__,__LINE__ ,FileName,path);
        return;
    }
    if(ftruncate(fd,Content.size()) == -1){
        spdlog::error("[{}:{}] [ftruncate '{}' fail] [size:{} Bytes]",__FILE__,__LINE__ ,FileName,Content.length());
        return;
    }
    char* p = reinterpret_cast<char*>(mmap(nullptr,Content.size(),PROT_READ | PROT_WRITE,MAP_SHARED,fd,0));
    if (p == MAP_FAILED) {
        spdlog::error("[{}:{}] [mmap '{}' fail]",__FILE__,__LINE__ ,FileName);
        return;
    }
    memcpy(p,Content.c_str(),Content.size());
    if (msync(p,Content.size(),MS_SYNC) == -1) {
        spdlog::error("[{}:{}] [msync '{}' fail]",__FILE__,__LINE__ ,FileName);
        return;
    }
    if (munmap(p,Content.size()) == -1) {
        spdlog::error("[{}:{}] [munmap '{}' fail]",__FILE__,__LINE__ ,FileName);
        return;
    }
    close(fd);
}
#endif
}