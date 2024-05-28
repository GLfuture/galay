#include "../galay/kernel/server.h"
#include "../galay/protocol/http1_1.h"
#include <condition_variable>
#include <spdlog/spdlog.h>
#include <mutex>
#include <signal.h>

#define ILLEGAL_NOT_CLOSE 0
#define ILLEGAL_DELAY_CLOSE 1
#define ILLEGAL_IMMEDIATELY_CLOSE 0
galay::GY_TcpCoroutine<galay::CoroutineStatus> test(galay::GY_Controller::wptr ctrl)
{
    auto request = std::dynamic_pointer_cast<galay::protocol::http::Http1_1_Request>(ctrl.lock()->GetRequest());
    auto response = std::make_shared<galay::protocol::http::Http1_1_Response>();
    response->SetStatus(200) ;
    response->SetVersion("1.1");
    response->SetHeadPair({"Content-Type", "text/html"});
    std::string body = "<html><head><meta charset=\"utf-8\"><title>title</title></head><body>hello world!</body></html>";
    response->SetBody(std::move(body));
    ctrl.lock()->PushResponse(response);
    if(request->GetHeadValue("connection").compare("close") == 0){
        ctrl.lock()->Close();
    }
    ctrl.lock()->Done();
    co_return galay::CoroutineStatus::GY_COROUTINE_FINISHED;
}

#if ILLEGAL_NOT_CLOSE
galay::GY_TcpCoroutine<galay::TaskStatus> illegal(std::string& rbuffer,std::string& wbuffer)
{
    wbuffer = "your protocol is illegal , please try again";
    co_return galay::TaskStatus::GY_TASK_SEND;
}
#elif ILLEGAL_DELAY_CLOSE
galay::GY_TcpCoroutine<galay::CoroutineStatus> illegal(std::string& rbuffer,std::string& wbuffer)
{
    wbuffer = "your protocol is illegal , please try again";
    co_return galay::CoroutineStatus::GY_COROUTINE_FINISHED;
}
#elif ILLEGAL_IMMEDIATELY_CLOSE
galay::GY_TcpCoroutine<galay::TaskStatus> illegal(std::string& rbuffer,std::string& wbuffer)
{
    wbuffer = "your protocol is illegal , please try again";
    co_return galay::TaskStatus::GY_TASK_IMMEDIATELY_CLOSE;
}
#endif

galay::GY_TcpServer server;

void signal_handler(int signo)
{
    if (signo == SIGINT)
    {
        server.Stop();
    }
}

int main()
{
    signal(SIGINT,signal_handler);
    spdlog::set_level(spdlog::level::debug);
    galay::GY_TcpServerBuilder<galay::protocol::http::Http1_1_Request,galay::protocol::http::Http1_1_Response>::ptr 
    builder = std::make_shared<galay::GY_TcpServerBuilder<galay::protocol::http::Http1_1_Request,galay::protocol::http::Http1_1_Response>>();
    builder->SetSchedulerType(galay::GY_TcpServerBuilderBase::SchedulerType::SELECT_SCHEDULER);
    builder->SetUserFunction({8082,test});
    builder->SetIllegalFunction(illegal);
    builder->SetThreadNum(1);
    server.Start(builder);
    return 0;
}