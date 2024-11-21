#ifndef __GALAY_EXTERNAPI_H__
#define __GALAY_EXTERNAPI_H__

#include <functional>
#include <openssl/ssl.h>
#include "galay/common/Base.h"

namespace galay::scheduler {
    class EventScheduler;
    class CoroutineScheduler;
    class TimerScheduler;
}

namespace galay::coroutine {
    class Coroutine;
}

namespace galay {

extern "C" {

#define MAX_GET_COROUTINE_SCHEDULER_RETRY_TIMES     50

/*
    Coroutine
*/

void AddCoroutineToStore(coroutine::Coroutine* co);
void RemoveCoroutineFromStore(coroutine::Coroutine* co);
void ClearCoroutineStore();


/*
   OpenSSL 
*/

bool InitialSSLServerEnv(const char* cert_file, const char* key_file);
bool InitialSSLClientEnv();
bool DestroySSLEnv();
SSL_CTX* GetGlobalSSLCtx();


/*
    Scheduler
*/
void DynamicResizeCoroutineSchedulers(int num);
void DynamicResizeEventSchedulers(int num);
void DynamicResizeTimerSchedulers(int num);

int GetCoroutineSchedulerNum();
int GetEventSchedulerNum();
int GetTimerSchedulerNum();

scheduler::EventScheduler* GetEventScheduler(int index);
scheduler::CoroutineScheduler* GetCoroutineSchedulerInOrder();
scheduler::CoroutineScheduler* GetCoroutineScheduler(int index);
scheduler::TimerScheduler* GetTimerSchedulerInOrder();
scheduler::TimerScheduler* GetTimerScheduler(int index);

/*
    Start all schedulers
    [timeout]: timeout in milliseconds, -1 will block
*/
void StartAllSchedulers(int timeout = -1);
void StartCoroutineSchedulers();
void StartEventSchedulers(int timeout);
void StartTimerSchedulers(int timeout);
/*
    Stop all schedulers
*/
void StopAllSchedulers();
void StopCoroutineSchedulers();
void StopEventSchedulers(); 
void StopTimerSchedulers();

}


}

#endif