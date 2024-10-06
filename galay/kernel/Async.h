#ifndef __GALAY_ASYNC_H__
#define __GALAY_ASYNC_H__

#include "Base.h"
#include "Awaiter.h"

namespace galay::event
{
    class EventEngine;
}

namespace galay::async
{
class HandleOption
{
public:
    HandleOption(GHandle handle);
    bool HandleBlock();
    bool HandleNonBlock();
    bool HandleReuseAddr();
    bool HandleReusePort();
    std::string GetLastError();
private:
    GHandle m_handle;
};

class AsyncTcpSocket
{
public:
    AsyncTcpSocket(GHandle handle, event::EventEngine* engine);
    AsyncTcpSocket(event::EventEngine* engine);
    HandleOption GetOption();
    coroutine::Awaiter_bool InitialHandle();
    coroutine::Awaiter_bool InitialHandle(GHandle handle);
    
    coroutine::Awaiter_bool Connect(const char* host, int port);
    //return send length, -1 has error 0 disconnect 
    coroutine::Awaiter_int Recv(action::WaitAction* action);
    coroutine::Awaiter_int Send(action::WaitAction* action);
    coroutine::Awaiter_bool Close();
    
    //获取rbuffer，清空视图，注意内存释放(attention)
    inline std::string_view GetRBuffer() { std::string_view view = m_rbuffer; m_rbuffer = ""; return view; }
    inline void SetRBuffer(std::string_view view) { m_rbuffer = view; }
    inline void SetWBuffer(std::string_view view) { m_wbuffer = view; }
    inline std::string_view GetWBuffer() { std::string_view view = m_wbuffer; m_wbuffer = ""; return view; }

    inline GHandle GetHandle() { return m_handle; }
    inline event::EventEngine* GetEngine() { return m_engine; }
    int GetRemotePort();
    std::string GetRemoteAddr();
    
    void SetLastError(std::string error);
    std::string GetLastError();
    ~AsyncTcpSocket();
private:
    GHandle m_handle;
    bool m_handle_closed;
    event::EventEngine* m_engine;
    std::string m_last_error;
    std::string_view m_rbuffer;
    std::string_view m_wbuffer;
};



}
#endif