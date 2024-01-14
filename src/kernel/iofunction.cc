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


int galay::iofunction::Tcp_Function::Sock_Keepalive(int fd , int t_idle , int t_interval , int retry)
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


int galay::iofunction::Tcp_Function::Conncet(int fd , std::string sip, uint32_t sport)
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

ssize_t galay::iofunction::Tcp_Function::Recv(int fd , char *buffer,uint32_t len)
{
    return recv(fd, buffer, len, 0);
}

ssize_t galay::iofunction::Tcp_Function::Send(int fd,const std::string& buffer,uint32_t len)
{
    return send(fd, buffer.c_str() ,len,0);
}

SSL_CTX* galay::iofunction::Tcp_Function::SSL_Init_Server(long min_version, long max_version)
{
    SSL_Init_Env();
    return SSL_Init_CTX_Server(min_version,max_version);
}


SSL_CTX* galay::iofunction::Tcp_Function::SSL_Init_Client(long min_version, long max_version)
{
    SSL_Init_Env();
    return SSL_Init_CTX_Client(min_version,max_version);
}

int galay::iofunction::Tcp_Function::SSL_Config_Cert_And_Key(SSL_CTX *ctx , const char* cert_filepath , const char* key_filepath)
{
    if (SSL_CTX_use_certificate_file(ctx, cert_filepath, SSL_FILETYPE_PEM) <= 0) {
        return -1;
    }

    if (SSL_CTX_use_PrivateKey_file(ctx, key_filepath, SSL_FILETYPE_PEM) <= 0 ) {
        return -1;
    }
    return 0;
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

int galay::iofunction::Tcp_Function::SSL_Recv(SSL* ssl,char* buffer,int len)
{
    return SSL_read(ssl,buffer,len);
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

SSL_CTX* galay::iofunction::Tcp_Function::SSL_Init_CTX_Server(long min_version,long max_version)
{
    SSL_CTX* ctx = SSL_CTX_new(TLS_server_method()); // 创建客户端 SSL 上下文对象
    SSL_CTX_set_min_proto_version(ctx, min_version); // 最小支持 TLSv1.3 协议
    SSL_CTX_set_max_proto_version(ctx, max_version); // 最大支持 TLSv1.3 协议
    return ctx;
}

SSL_CTX* galay::iofunction::Tcp_Function::SSL_Init_CTX_Client(long min_version,long max_version)
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

ssize_t galay::iofunction::Udp_Function::Recv_From(int fd , Addr& addr , char* buffer , int len)
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

ssize_t galay::iofunction::Udp_Function::Send_To(int fd,const Addr& addr,const std::string& buffer)
{
    sockaddr_in sin = {0};
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = inet_addr(addr.ip.c_str());
    sin.sin_port = htons(addr.port);
    return sendto(fd,buffer.c_str(),buffer.length(),0,(sockaddr*)&sin,sizeof(sin));
}