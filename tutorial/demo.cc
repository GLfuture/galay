#include <coroutine>
#include "../galay/galay.h"

struct await_int
{

    bool await_ready()
    {

    }
    void await_suspend(std::coroutine_handle<> handle);
    std::any await_resume();
};

class AsyncSocket
{
public:
    AsyncSocket(int fd);
    await_int connect(const char *ip, int port);
    await_int read(char *buf, int len);
    await_int write(char *buf, int len);
private:
    int m_fd;
};



int main()
{
    
    return 0;
}