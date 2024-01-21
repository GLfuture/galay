#ifndef GALAY_IOFUNCTION_H
#define GALAY_IOFUNCTION_H

#include <iostream>
#include <string>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/socket.h>
#include <string.h>
#include <memory>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <variant>
#include <vector>
#include <fcntl.h>
#include <netinet/tcp.h>

namespace galay{
    namespace iofunction
    {
        class Simple_Fuction
        {
        public:
            static int IO_Set_No_Block(int fd);

            static int IO_Set_Block(int fd);
        };

        //network  io
        struct Addr{
            std::string ip;
            int port;
        };

        class Net_Function: public Simple_Fuction
        {
        public:
            static int Reuse_Fd(int fd);
        };

        class Tcp_Function:public Net_Function
        {
        public:
            using SSL_Tup = std::tuple<SSL *, SSL_CTX *>;
            using ptr = std::shared_ptr<Tcp_Function>;

            static int Sock();
            
            static int Sock_Keepalive(int fd , int t_idle , int t_interval , int retry);

            static int Conncet(int fd , std::string sip, uint32_t sport);

            static int Bind(int fd, uint32_t port);

            static int Listen(int fd, uint32_t backlog);

            static int Accept(int fd);

            static ssize_t Recv(int fd , char *buffer,uint32_t len);

            static ssize_t Send(int fd, const std::string &buffer, uint32_t len);

            //for server
            static SSL_CTX *SSL_Init_Server(long min_version,long max_version);

            //for client
            static SSL_CTX *SSL_Init_Client(long min_version, long max_version);

            static int SSL_Config_Cert_And_Key(SSL_CTX *ctx , const char* cert_filepath , const char* key_filepath);

            // 修改ssl的绑定
            static int SSL_Reset(SSL *ssl, int fd);

            // 创建一个ssl对象并绑定fd
            static SSL *SSL_Create_Obj(SSL_CTX *ctx, int fd);

            static int SSL_Accept(SSL *ssl);

            static int SSL_Connect(SSL *ssl);

            static int SSL_Recv(SSL* ssl,char* buffer,int len);

            static int SSL_Send(SSL *ssl, const std::string &buffer, int len);

            static void SSL_Destory(SSL* ssl);

            static void SSL_Destory(std::vector<SSL *> ssls, SSL_CTX *ctx);

        private:
            static void SSL_Init_Env();

            static SSL_CTX *SSL_Init_CTX_Server(long min_version, long max_version);

            static SSL_CTX *SSL_Init_CTX_Client(long min_version, long max_version);

            static void SSL_Destory_CTX(SSL_CTX *ctx);

            static void SSL_Destory_Env();
        };


        class Udp_Function:public Net_Function
        {
        public:
            static int Sock();

            static int Bind(int fd, uint32_t port);

            static ssize_t Recv_From(int fd , Addr& addr , char* buffer , int len);

            static ssize_t Send_To(int fd,const Addr& addr,const std::string& buffer);
        };
    }
}


#endif