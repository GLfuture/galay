#ifndef __GALAY_STEP_H__
#define __GALAY_STEP_H__

#include <functional>
#include <openssl/ssl.h>
#include "common/Base.h"

namespace galay::scheduler {
    class EventScheduler;
    class CoroutineScheduler;
}

namespace galay {

extern "C" {

bool InitialSSLServerEnv(const char* cert_file, const char* key_file);
bool InitialSSLClientEnv();
bool DestroySSLEnv();
SSL_CTX* GetGlobalSSLCtx();

void DynamicResizeCoroutineSchedulers(int num);
void DynamicResizeEventSchedulers(int num);

int GetCoroutineSchedulerNum();
int GetEventSchedulerNum();

scheduler::EventScheduler* GetEventScheduler(int index);
scheduler::CoroutineScheduler* GetCoroutineScheduler(int index);

void StartCoroutineSchedulers(int timeout = -1);
void StartEventSchedulers(int timeout = -1);
void StopCoroutineSchedulers();
void StopEventSchedulers(); 

}


}

#endif