#include "galay/galay.h"
#include "galay/util/Io.h"
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
    galay::async::AsyncFileNativeAio fileio((int)128, galay::GetEventScheduler(0)->GetEngine());
    void* buffer;
    posix_memalign(&buffer, PEER_SIZE, PEER_SIZE * 10);
    for(int i = 0; i < PEER_SIZE * 10; ++i) {
        ((char*)buffer)[i] = 'a';
    }
    fileio.PrepareWrite(handle, (char*)buffer, PEER_SIZE * 10, 0);
    int ret = co_await fileio.Commit();
    printf("ret: %d\n", ret);
    printf("error is %s\n", galay::error::GetErrorString(fileio.GetErrorCode()).c_str());

    void* after = nullptr;
    posix_memalign(&after, PEER_SIZE, PEER_SIZE * 10);
    memset(after, 0, PEER_SIZE * 10);
    fileio.PrepareRead(handle, (char*)after, PEER_SIZE * 10, 0);
    ret = co_await fileio.Commit();
    printf("ret: %d\n", ret);
    if(strcmp((char*)buffer, (char*)after) == 0) {
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
    galay::async::AsyncFileDescriptor fileio(galay::GetEventScheduler(0)->GetEngine());
    fileio.GetHandle() = handle;
    galay::FileIOVec vec {
        .m_handle = handle,
        .m_iovec = {
            .m_buffer = static_cast<char*>(calloc(10 * 1024 * 1024)),
            .m_length = 10 * 1024 * 1024,
            .m_offset = 0
        }
    };
    for(int i = 0; i < 10 * 1024 * 1024; ++i) {
        if( i % 128 == 0) {
            vec.m_iovec.m_buffer[i] = '\n';
        } else {
            vec.m_iovec.m_buffer[i] = 'a';
        }
    }
    printf("write file\n");
    int length = 10 * 1024 * 1024;
    while(true){
        int size = co_await galay::AsyncFileWrite(&fileio, &vec);
        printf("write size: %d\n", size);
        if(size < 0) {
            printf("write error: %s\n", galay::error::GetErrorString(fileio.GetErrorCode()).c_str());
            break;
        }
        length -= size;
        vec.m_iovec.m_offset += size;
        vec.m_iovec.m_length -= size;
        if(length <= 0) {
            break;
        }
    }
    getchar();
    free(vec.m_iovec.m_buffer);
    fflush(nullptr);

    // 再次读取
    lseek(handle.fd, 0, SEEK_SET);  // 将文件指针移回文件开头
    galay::FileIOVec vec2 {
        .m_handle = handle,
        .m_iovec = {
            .m_buffer = static_cast<char*>(calloc(10 * 1024 * 1024)),
            .m_length = 10 * 1024 * 1024,
            .m_offset = 0
        }
    };
    length = 10 * 1024 * 1024;
    while(true)
    {
        int size = co_await galay::AsyncFileRead(&fileio, &vec2);
        printf("read size %d\n", size);
        if(size <= 0) {
            printf("write error: %s\n", galay::error::GetErrorString(fileio.GetErrorCode()).c_str());
            break;
        }
        length -= size;
        vec.m_iovec.m_offset += size;
        vec.m_iovec.m_length -= size;
        if(length <= 0) {
            break;
        }
    }
    for(int i = 0; i < 10 * 1024 * 1024; ++i) {
        if( i % 128 == 0) {
            if(vec2.m_iovec.m_buffer[i] != '\n') {
                printf("error\n");
            }
        } else {
            if(vec2.m_iovec.m_buffer[i] != 'a'){
                printf("error\n");
            }
        }
    }
    printf("success\n");
    free(vec2.m_iovec.m_buffer);
    close(handle.fd);
}

#endif



int main()
{
    spdlog::set_level(spdlog::level::debug);
    galay::DynamicResizeEventSchedulers(1);
    galay::DynamicResizeCoroutineSchedulers(1);
    galay::StartEventSchedulers(-1);
    galay::StartCoroutineSchedulers();
    std::this_thread::sleep_for(std::chrono::seconds(1));
    test();
    getchar();
    galay::StopCoroutineSchedulers();
    galay::StopEventSchedulers();
    remove("test.txt");
    return 0;
}
