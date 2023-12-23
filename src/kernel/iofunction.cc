#include "iofunction.h"

//io simple function
int galay::iofunction::Simple_Fuction::IO_Set_No_Block(int fd)
{
    int flag = fcntl(fd, F_GETFL, 0);
    flag |= O_NONBLOCK;
    return fcntl(fd, F_SETFL, flag);
}

int galay::iofunction::Simple_Fuction::IO_Set_Block(int fd)
{
    int flag = fcntl(fd, F_GETFL, 0);
    flag &= ~O_NONBLOCK;
    return fcntl(fd, F_SETFL, flag);
}

int galay::iofunction::Net_Function::Reuse_Fd(int fd)
{
    int option = 1;
    return setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (void *)&option, sizeof(option));
}

//tcp
int galay::iofunction::Tcp_Function::Sock()
{
    return socket(AF_INET, SOCK_STREAM, 0);
}


int galay::iofunction::Tcp_Function::Conncet(std::string sip, uint32_t sport)
{
    int conn_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (conn_fd <= 0) return conn_fd;
    sockaddr_in sin = {0};
    memset(&sin, 0, sizeof(sin));
    sin.sin_addr.s_addr = inet_addr(sip.c_str());
    sin.sin_family = AF_INET;
    sin.sin_port = htons(sport);
    int ret = connect(conn_fd, (sockaddr *)&sin, sizeof(sin));
    if (ret == -1) return ret;
    return conn_fd;
}

int galay::iofunction::Tcp_Function::Bind(int fd, uint32_t port)
{
    sockaddr_in sin = {0};
    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_port = htons(port);
    sin.sin_addr.s_addr = INADDR_ANY;
    return bind(fd, (sockaddr *)&sin, sizeof(sin));
}

int galay::iofunction::Tcp_Function::Listen(int fd,uint32_t backlog)
{
    return listen(fd, backlog);
}

int galay::iofunction::Tcp_Function::Accept(int fd)
{
    sockaddr_in sin = {0};
    memset(&sin, 0, sizeof(sin));
    socklen_t len = sizeof(sin);
    return accept(fd, (sockaddr *)&sin, &len);
}

ssize_t galay::iofunction::Tcp_Function::Recv(int fd , std::string& buffer,uint32_t len)
{
    char* temp = new char[len];
    memset(temp, 0, len);
    ssize_t ret = recv(fd, temp, len, 0);
    if(ret <= 0){
        delete temp;
        return ret;
    }
    buffer.append(temp,ret);
    delete temp;
    return ret;
}

ssize_t galay::iofunction::Tcp_Function::Send(int fd,const std::string& buffer,uint32_t len)
{
    return send(fd, buffer.c_str() ,len,0);
}

SSL_CTX* galay::iofunction::Tcp_Function::SSL_Init(long min_version, long max_version)
{
    SSL_Init_Env();
    return SSL_Init_CTX(min_version,max_version);
}

SSL* galay::iofunction::Tcp_Function::SSL_Create_Obj( SSL_CTX* ctx , int fd)
{
    SSL* ssl = SSL_new(ctx);
    SSL_set_fd(ssl,fd);
    return ssl;
}

int galay::iofunction::Tcp_Function::SSL_Reset(SSL *ssl, int fd)
{
    return SSL_set_fd(ssl,fd);
}

int galay::iofunction::Tcp_Function::SSL_Connect(SSL* ssl)
{
    return SSL_connect(ssl);
}

int galay::iofunction::Tcp_Function::SSL_Recv(SSL* ssl,std::string& buffer,int len)
{
    char* temp = new char[len];
    memset(temp,0,len);
    int ret = SSL_read(ssl,temp,len);
    if(ret <= 0){
        delete temp;
        return ret;
    }
    buffer.append(temp,ret);
    delete temp;
    return ret;
}


int galay::iofunction::Tcp_Function::SSL_Send(SSL* ssl,const std::string& buffer,int len)
{
    return SSL_write(ssl,buffer.c_str(),len);
}

int galay::iofunction::Tcp_Function::SSL_Accept(SSL* ssl)
{
    return SSL_accept(ssl);
}

void galay::iofunction::Tcp_Function::SSL_Destory(SSL* ssl)
{
    SSL_shutdown(ssl);
    SSL_free(ssl);
}

void galay::iofunction::Tcp_Function::SSL_Destory(std::vector<SSL*> ssls, SSL_CTX *ctx) 
{
    for(auto ssl:ssls)
    {
        SSL_shutdown(ssl);
        SSL_free(ssl);
    }
    SSL_Destory_CTX(ctx);
    SSL_Destory_Env();
}


void galay::iofunction::Tcp_Function::SSL_Init_Env()
{
    SSL_library_init();
    ERR_load_crypto_strings();
    SSL_load_error_strings();
}

SSL_CTX* galay::iofunction::Tcp_Function::SSL_Init_CTX(long min_version,long max_version)
{
    SSL_CTX* ctx = SSL_CTX_new(TLS_client_method()); // 创建客户端 SSL 上下文对象
    SSL_CTX_set_min_proto_version(ctx, min_version); // 最小支持 TLSv1.3 协议
    SSL_CTX_set_max_proto_version(ctx, max_version); // 最大支持 TLSv1.3 协议
    return ctx;
}


void galay::iofunction::Tcp_Function::SSL_Destory_CTX(SSL_CTX* ctx)
{
    SSL_CTX_free(ctx);
}

void galay::iofunction::Tcp_Function::SSL_Destory_Env()
{
    ERR_free_strings();
    EVP_cleanup();
}

//udp
int galay::iofunction::Udp_Function::Sock()
{
    return socket(AF_INET,SOCK_DGRAM,0);
}

int galay::iofunction::Udp_Function::Bind(int fd, uint32_t port)
{
    sockaddr_in sin = {0};
    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_port = htons(port);
    sin.sin_addr.s_addr = INADDR_ANY;
    return bind(fd,(sockaddr*)&sin,sizeof(sin));
}

ssize_t galay::iofunction::Udp_Function::Recv_From(int fd , Addr& addr , std::string& buffer , int len)
{
    char* temp = new char[len];
    memset(temp,0,len);
    sockaddr_in sin = {0};
    socklen_t slen = sizeof(sin);
    ssize_t ret = recvfrom(fd,temp,len,0,(sockaddr*) &sin,&slen);
    if(ret <= 0)
    {
        delete temp;
        return ret;
    }
    char ip[INET_ADDRSTRLEN] = {0};
    inet_ntop(AF_INET,&sin.sin_addr,ip,INET_ADDRSTRLEN);
    addr.ip.assign(ip,INET_ADDRSTRLEN);
    addr.port = ntohs(sin.sin_port);
    buffer.append(temp,ret);
    delete temp;
    return ret;
}

ssize_t galay::iofunction::Udp_Function::Send_To(int fd,const Addr& addr,const std::string& buffer)
{
    sockaddr_in sin = {0};
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = inet_addr(addr.ip.c_str());
    sin.sin_port = htons(addr.port);
    return sendto(fd,buffer.c_str(),buffer.length(),0,(sockaddr*)&sin,sizeof(sin));
}