#include "galay/galay.h"
#include "galay/utils/Io.h"
#include <fcntl.h>
#include <unistd.h>
#include <iostream>
#include <spdlog/spdlog.h>

#define PEER_SIZE 512

using galay::coroutine::Coroutine;

#ifdef __linux__
Coroutine test()
{
    GHandle handle = co_await galay::AsyncFileOpen("test.txt", O_RDWR | O_CREAT, 0644);
    galay::async::AsyncFileNativeAio fileio((int)128, galay::EeventSchedulerHolder::GetInstance()->GetScheduler(0)->GetEngine());
    void* buffer;
    posix_memalign(&buffer, PEER_SIZE, PEER_SIZE * 10);
    for(int i = 0; i < PEER_SIZE * 10; ++i) {
        ((char*)buffer)[i] = 'a';
    }
    fileio.PrepareWrite(handle, (char*)buffer, PEER_SIZE * 10, 0);
    spdlog::info("write file");
    int ret = co_await fileio.Commit();
    printf("ret: %d\n", ret);
    printf("error is %s\n", galay::error::GetErrorString(fileio.GetErrorCode()).c_str());

    void* after = nullptr;
    posix_memalign(&after, PEER_SIZE, PEER_SIZE * 10);
    memset(after, 0, PEER_SIZE * 10);
    fileio.PrepareRead(handle, (char*)after, PEER_SIZE * 10, 0);
    ret = co_await fileio.Commit();
    printf("ret: %d\n", ret);
    if(strncmp((char*)buffer, (char*)after, PEER_SIZE * 10) == 0) {
        printf("fileio test success\n");
    } else {
        printf("fileio test failed\n");
    }
    free(after);
    free(buffer);
    close(handle.fd);
    co_return;
}
#else
galay::coroutine::Coroutine test()
{
    const GHandle handle = co_await galay::AsyncFileOpen("test.txt", O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
    galay::async::AsyncFileDescriptor fileio(galay::EeventSchedulerHolder::GetInstance()->GetScheduler(0)->GetEngine());
    fileio.GetHandle() = handle;
    galay::IOVecHolder holder;
    galay::VecMalloc(&holder, 10 * 1024 * 1024);
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
        int size = co_await galay::AsyncFileWrite(&fileio, &holder, holder->m_size);
        printf("write size: %d\n", size);
        if(size < 0) {
            printf("write error: %s\n", galay::error::GetErrorString(fileio.GetErrorCode()).c_str());
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
    lseek(handle.fd, 0, SEEK_SET);  // 将文件指针移回文件开头
    while(true)
    {
        int size = co_await galay::AsyncFileRead(&fileio, &holder, holder->m_size);
        printf("read size %d\n", size);
        if(size <= 0) {
            printf("write error: %s\n", galay::error::GetErrorString(fileio.GetErrorCode()).c_str());
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
    close(handle.fd);
}

#endif



int main()
{
GALAY_APP_MAIN(
    std::this_thread::sleep_for(std::chrono::seconds(1));
    test();
    spdlog::info("test");
    getchar();
    galay::DestroyGalayEnv();
    remove("test.txt");
);
    return 0;
}
