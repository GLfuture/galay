#ifndef __GALAY_STEP_H__
#define __GALAY_STEP_H__

#include <functional>
#include <openssl/ssl.h>
#include "../common/Base.h"

namespace galay::scheduler {
    class EventScheduler;
    class CoroutineScheduler;
}

namespace galay {

extern bool InitialSSLServerEnv(const char* cert_file, const char* key_file);
extern bool InitialSSLClientEnv();
extern bool DestroySSLEnv();
extern SSL_CTX* GetGlobalSSLCtx();

extern void DynamicResizeCoroutineSchedulers(int num);
extern void DynamicResizeNetIOSchedulers(int num);

extern int GetCoroutineSchedulerNum();
extern int GetNetIOSchedulerNum();

extern scheduler::EventScheduler* GetNetIOScheduler(int index);
extern scheduler::CoroutineScheduler* GetCoroutineScheduler(int index);

extern void StartCoroutineSchedulers(int timeout = -1);
extern void StartNetIOSchedulers(int timeout = -1);
extern void StopCoroutineSchedulers();
extern void StopNetIOSchedulers(); 
    
}

#endif