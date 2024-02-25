#include "callback.h"

std::function<void(int)> Callback_ConnClose::m_callback = nullptr;

void Callback_ConnClose::set(std::function<void(int)> &&func)
{
    Callback_ConnClose::m_callback = func;
}

void Callback_ConnClose::call(int fd)
{
    if (m_callback) m_callback(fd);
}

bool Callback_ConnClose::empty()
{
    return m_callback == nullptr;
}

std::function<void(SSL*)> Callback_SSL_ConnClose::m_callback = nullptr;

void Callback_SSL_ConnClose::set(std::function<void(SSL*)> &&func)
{
    Callback_SSL_ConnClose::m_callback = func;
}

void Callback_SSL_ConnClose::call(SSL* ssl)
{
    if (m_callback) m_callback(ssl);
}

bool Callback_SSL_ConnClose::empty()
{
    return m_callback == nullptr;
}