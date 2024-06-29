#include "../galay/galay.h"
#include <signal.h>
#include <iostream>
#include <spdlog/spdlog.h>
galay::GY_TcpCoroutine<galay::CoroutineStatus> test(galay::GY_HttpController::wptr ctrl)
{
    auto request = std::dynamic_pointer_cast<galay::protocol::http::Http1_1_Request>(ctrl.lock()->GetRequest());
    auto response = std::make_shared<galay::protocol::http::Http1_1_Response>();
    if(request->GetUri().compare("/smtp") == 0){
        galay::GY_SmtpAsyncClient client("117.135.207.210", 25);
        auto resps1 = co_await client.Auth("bigdata_C1004@163.com", "EPOXVZMINXCXHXUO");
        // g++ 12 bug , 使用该方法赋值时，SendEmail函数编译器会报错
        // galay::protocol::smtp::SmtpMsgInfo msg{
        //     .m_subject = "test",
        //     .m_content = "<html><head>Verify</head><body>111111</body></html>",
        //     .m_content_type = "text/html",
        //     .m_charset = "utf-8",
        // };
        galay::protocol::smtp::SmtpMsgInfo msg;
        msg.m_subject = "test";
        msg.m_content = "<html><head>Verify</head><body>111111</body></html>";
        std::vector<std::string> arr = {"gzj17607270093@163.com", "1790188287@qq.com"};
        auto resp2 = co_await client.SendEmail("bigdata_C1004@163.com", arr , msg);
        auto resps3 = co_await client.Quit();
        response->SetStatus(200);
        response->SetVersion("1.1");
        response->SetHeadPair({"Content-Type", "text/html"});
        std::string body = "<html><head><meta charset=\"utf-8\"><title>title</title></head><body>OK!</body></html>";
        response->SetBody(std::move(body));
        if (request->GetHeadValue("connection").compare("close") == 0)
        {
            ctrl.lock()->Close();
        }
    }else{
        response->SetStatus(404);
        response->SetVersion("1.1");
        response->SetHeadPair({"Content-Type", "text/html"});
        std::string body = "<html><head><meta charset=\"utf-8\"><title>title</title></head><body>Not Found</body></html>";
        response->SetBody(std::move(body));
    }
    ctrl.lock()->PushResponse(response->EncodePdu());
    co_return galay::CoroutineStatus::kCoroutineFinished;
}


galay::GY_TcpServer::ptr server;

void signal_handler(int signum)
{
    std::cout << "signal_handler begin" << std::endl;
    if(signum == SIGINT) {
        server->Stop();
        //signal(signum,SIG_DFL);
        //raise(signum);
    }    
}

int main()
{
    signal(SIGINT,signal_handler);
    spdlog::set_level(spdlog::level::debug);
    auto router = galay::GY_RouterFactory::CreateHttpRouter();
    router->Get("/smtp",test);
    galay::GY_HttpServerBuilder::ptr builder = galay::GY_ServerBuilderFactory::CreateHttpServerBuilder(8080,router);
    server = galay::GY_ServerFactory::CreateHttpServer(builder);
    server->Start();
    return 0;
}