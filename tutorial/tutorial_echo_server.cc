// #include "galay/galay.hpp"
// #include <iostream>

// class Handler {
// public:
//     static galay::Coroutine<void> GetHelloWorldHandler(galay::RoutineCtx ctx, galay::http::HttpStream::ptr session) {
//         auto resp = session->GetResponse();
//         resp->SetContent("html", "<html>Hello World</html>");
//         session->Close();
//         co_return;
//     }
// };

// int main(int argc, const char* argv[])
// {
//     galay::args::App app("tutorial_echo_server");
//     app.Help("./tutorial_echo_server [OPTION] COMMAND\nOptions:\n  -p,\t--port number\tremote port.");
//     galay::args::Arg* port_arg = galay::args::Arg::Create("port"), *ip_arg = galay::args::Arg::Create("ip");
//     port_arg->Short('p').Required(true).Input(true).IsInt();
//     ip_arg->Input(true);
//     app.AddArg(port_arg, true).AddArg(ip_arg, true);
//     if(!app.Parse(argc, argv)) {
//         app.ShowHelp();
//         return -1;
//     }
//     uint32_t port;
//     try
//     {
//         port = port_arg->Value().ConvertTo<uint32_t>();
//     }
//     catch(const std::exception& e)
//     {
//         std::cout << e.what() << std::endl;
//         return -1;
//     }
//     galay::GalayEnv env({});
//     auto config = galay::HttpServerConfig::Create();
//     galay::http::HttpServer<galay::AsyncTcpSocket> server(config);
//     server.RouteHandler<galay::http::GET>("/", Handler::GetHelloWorldHandler);
//     server.Start({"", port});
//     getchar();
//     server.Stop();
//     return 0;
// }

int main()
{
}