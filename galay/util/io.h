#ifndef GALAY_IOFUNCTION_H
#define GALAY_IOFUNCTION_H

#include <string>
#include <string.h>
#include <memory>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <vector>

namespace galay::io
{
    class BlockFuction
    {
    public:
        static int SetNoBlock(int fd);
        static int SetBlock(int fd);
    };
}

namespace galay::io::net
{
    // network  io
    struct Addr
    {
        std::string ip;
        int port;
    };

    class TcpFunction : public BlockFuction
    {
    public:
        using SSL_Tup = std::tuple<SSL *, SSL_CTX *>;
        using ptr = std::shared_ptr<TcpFunction>;

        static int Sock();
        static int Sock6();
        static int SockKeepalive(int fd, int t_idle, int t_interval, int retry);
        // 重用地址(避免timewait导致的无法连接)
        static int ReuseAddr(int fd);
        // 重用端口(允许不同fd监听同一端口)
        static int ReusePort(int fd);
        static int Conncet(int fd, std::string sip, uint32_t sport);
        static int Bind(int fd, uint16_t port);
        static int Listen(int fd, uint16_t backlog);
        static int Accept(int fd);
        static ssize_t Recv(int fd, char* buffer, uint32_t len);
        static ssize_t Send(int fd, const char* buffer, uint32_t len);
        // for server
        static SSL_CTX *InitSSLServer(long min_version, long max_version);
        // for client
        static SSL_CTX *InitSSLClient(long min_version, long max_version);
        static int SetSSLCertAndKey(SSL_CTX *ctx, const char *cert_filepath, const char *key_filepath);
        // 修改ssl的绑定
        static int SSLReset(SSL *ssl, int fd);
        // 创建一个ssl对象并绑定fd
        static SSL *SSLCreateObj(SSL_CTX *ctx, int fd);
        static int SSLAccept(SSL *ssl);
        static int SSLConnect(SSL *ssl);
        static int SSLRecv(SSL *ssl, char *buffer, int len);
        static int SSLSend(SSL *ssl, const char* buffer, int len);
        static void SSLDestory(SSL *ssl);
        static void SSLDestory(std::vector<SSL *> ssls, SSL_CTX *ctx);

    private:
        static void InitSSLEnv();
        static SSL_CTX *InitServerSSLCtx(long min_version, long max_version);
        static SSL_CTX *InitClientSSLCtx(long min_version, long max_version);
        static void SSLDestoryCtx(SSL_CTX *ctx);
        static void SSLDestoryEnv();
    };

    class UdpFunction : public BlockFuction
    {
    public:
        static int Sock();
        static int Bind(int fd, uint32_t port);
        static ssize_t RecvFrom(int fd, Addr &addr, char *buffer, int len);
        static ssize_t SendTo(int fd, const Addr &addr, const std::string &buffer);
    };
}

namespace galay::io::file
{
    class SyncFileStream
    {
    public:
        static std::string ReadFile(const std::string &FileName , bool IsBinary = false);
        static void WriteFile(const std::string &FileName, const std::string &Content , bool IsBinary = false);
    };

    class ZeroCopyFile
    {
    public:
    #ifdef __linux__
        static std::string ReadFile(const std::string &FileName);
        static void WriteFile(const std::string &FileName, const std::string &Content,bool IsBinary = false);
    #endif
    };
}

#endif