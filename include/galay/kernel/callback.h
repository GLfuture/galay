#ifndef GY_CALLBACK_H
#define GY_CALLBACK_H

#include <openssl/ssl.h>
#include <functional>

class Callback_ConnClose
{
public:
    static void set(std::function<void(int)>&& func);

    static void call(int fd);

    static bool empty();
private:
    static std::function<void(int)> m_callback;
};


//To addvance
class Callback_SSL_ConnClose
{
public:
    public:
    static void set(std::function<void(SSL*)>&& func);

    static void call(SSL* ssl);

    static bool empty();
private:
    static std::function<void(SSL*)> m_callback;
};

#endif