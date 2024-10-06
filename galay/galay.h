#ifndef GALAY_H
#define GALAY_H

//common
#include "common/base.h"
#include "common/signalhandler.h"
#include "common/reflection.h"

//kernel
#include "kernel/Coroutine.h"
#include "kernel/Awaiter.h"
#include "kernel/Async.h"
#include "kernel/Event.h"
#include "kernel/EventEngine.h"
#include "kernel/Scheduler.h"
#include "kernel/Strategy.h"
#include "kernel/WaitAction.h"

//protocol
#include "protocol/http.h"
#include "protocol/smtp.h"
#include "protocol/dns.h"

//util
#include "util/parser.h"
#include "util/io.h"
#include "util/stringtools.h"
#include "util/typename.h"
#include "util/random.h"
#include "util/ratelimiter.h"
#include "util/tree.h"
#include "util/thread.h"


#endif