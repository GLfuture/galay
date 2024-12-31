#include "galay/galay.hpp"
#include "galay/utils/System.h"
#include <fcntl.h>
#include <unistd.h>
#include <iostream>
#include <spdlog/spdlog.h>

#define PEER_SIZE 512

using galay::Coroutine;

#ifdef __linux__
Coroutine<void> test(galay::RoutineCtx ctx)
{
    galay::details::InternelLogger::GetInstance()->GetLogger()->SpdLogger()->set_level(spdlog::level::trace);
    galay::AsyncFileNativeAioDescriptor descriptor(galay::EventSchedulerHolder::GetInstance()->GetScheduler(0)->GetEngine(),1024);
    bool res = descriptor.Open("test.txt", O_RDWR | O_CREAT, 0644);
    if(!res) {
        printf("open file failed\n");
        co_return;
    }
    void* buffer;
    posix_memalign(&buffer, PEER_SIZE, PEER_SIZE * 10);
    for(int i = 0; i < PEER_SIZE * 10; ++i) {
        ((char*)buffer)[i] = 'a';
    }
    descriptor.PrepareWrite((char*)buffer, PEER_SIZE * 10, 0);
    spdlog::info("write file");
    int ret = co_await descriptor.Commit();
    printf("ret: %d\n", ret);
    printf("error is %s\n", galay::error::GetErrorString(descriptor.GetErrorCode()).c_str());

    void* after = nullptr;
    posix_memalign(&after, PEER_SIZE, PEER_SIZE * 10);
    memset(after, 0, PEER_SIZE * 10);
    descriptor.PrepareRead((char*)after, PEER_SIZE * 10, 0);
    ret = co_await descriptor.Commit();
    printf("ret: %d\n", ret);
    if(strncmp((char*)buffer, (char*)after, PEER_SIZE * 10) == 0) {
        printf("fileio test success\n");
    } else {
        printf("fileio test failed\n");
    }
    free(after);
    free(buffer);
    res = co_await descriptor.Close();
    co_return;
}
#else
galay::Coroutine<void> test(galay::RoutineCtx ctx)
{
    galay::AsyncFileDescriptor descriptor(galay::EventSchedulerHolder::GetInstance()->GetScheduler(0)->GetEngine());
    bool sunccess = descriptor.Open("test.txt", O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
    galay::IOVecHolder<galay::FileIOVec> holder(10 * 1024 * 1024);
    for(int i = 0; i < 10 * 1024 * 1024; ++i) {
        if( i % 128 == 0) {
            holder->m_buffer[i] = '\n';
        } else {
            holder->m_buffer[i] = 'a';
        }
    }
    printf("write file\n");
    int length = 10 * 1024 * 1024;
    while(true){
        int size = co_await descriptor.Write(&holder, holder->m_size);
        printf("write size: %d\n", size);
        if(size < 0) {
            printf("write error: %s\n", galay::error::GetErrorString(descriptor.GetErrorCode()).c_str());
            break;
        }
        if(holder->m_offset == holder->m_size) {
            break;
        }
        std::cout << holder->m_offset << " " << holder->m_size << std::endl;
        getchar();
    }
    getchar();
    fflush(nullptr);
    holder.ClearBuffer();
    // 再次读取
    lseek(descriptor.GetHandle().fd, 0, SEEK_SET);  // 将文件指针移回文件开头
    while(true)
    {
        int size = co_await descriptor.Read(&holder, holder->m_size);
        printf("read size %d\n", size);
        if(size <= 0) {
            printf("write error: %s\n", galay::error::GetErrorString(descriptor.GetErrorCode()).c_str());
            break;
        }
        if(holder->m_offset == holder->m_size ) {
            break;
        }
    }
    for(int i = 0; i < 10 * 1024 * 1024; ++i) {
        if( i % 128 == 0) {
            if(holder->m_buffer[i] != '\n') {
                printf("error\n");
            }
        } else {
            if(holder->m_buffer[i] != 'a'){
                printf("error\n");
            }
        }
    }
    printf("success\n");
    bool success = co_await descriptor.Close();
}

#endif



int main()
{
    galay::GalayEnv env({{1, -1}, {1, -1}, {1, -1}});
    std::this_thread::sleep_for(std::chrono::seconds(1));
    test({});
    getchar();
    remove("test.txt");
    return 0;
}
