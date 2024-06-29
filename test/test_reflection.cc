#include "../galay/galay.h"
#include <iostream>

class Base: public galay::GY_Base, public galay::common::GY_DynamicCreator<galay::GY_Base,Base>
{
public:
    virtual void print()
    {
        std::cout << "Base" << std::endl;
    }
    
    virtual ~Base() {}
};

class Child:public Base,galay::common::GY_DynamicCreator<galay::GY_Base,Child>
{
public:
    virtual void print() override
    {
        std::cout << "Child" << std::endl;
    }

    virtual ~Child() {}
};


int main()
{
    std::string buffer = "GET /path HTTP/1.1\r\n\r\n";
    galay::protocol::GY_Request::ptr req = galay::common::GY_RequestFactory<>::GetInstance()->Create("galay::protocol::http::Http1_1_Request");
    if(req){
        req->DecodePdu(buffer);
        std::cout << "create success" << std::endl;
        std::cout << std::dynamic_pointer_cast<galay::protocol::http::Http1_1_Request>(req)->GetMethod() << '\n';
    }else{
        std::cout << "create failed" << std::endl;
    }
    std::shared_ptr<galay::GY_Base> ptr = galay::common::GY_UserFactory<>::GetInstance()->Create(galay::util::GetTypeName<Child>());
    if(ptr) std::dynamic_pointer_cast<Base>(ptr)->print();
    return 0;
}