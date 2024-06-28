#include "../galay/galay.h"



int main()
{
    std::string buffer = "GET /path HTTP/1.1\r\n\r\n";
    galay::protocol::GY_Request::ptr req = galay::GY_RequestFactory<>::GetInstance()->Create("galay::protocol::http::Http1_1_Request");
    if(req){
        req->DecodePdu(buffer);
        std::cout << "create success" << std::endl;
        std::cout << std::dynamic_pointer_cast<galay::protocol::http::Http1_1_Request>(req)->GetMethod() << '\n';
    }else{
        std::cout << "create failed" << std::endl;
    }
    return 0;
}