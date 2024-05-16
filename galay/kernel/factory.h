#ifndef GALAY_FACTORY_H
#define GALAY_FACTORY_H
#include "base.h"
#include "../protocol/basic_protocol.h"
#include <memory>


namespace galay{

    class GY_Factory{
    public:
        using ptr = std::shared_ptr<GY_Factory>;
        using uptr = std::unique_ptr<GY_Factory>;
        using wptr = std::weak_ptr<GY_Factory>;

        virtual ~GY_Factory() = default;
    };

    class GY_TcpProtocolFactoryBase{
    public:
        using ptr = std::shared_ptr<GY_TcpProtocolFactoryBase>;
        using uptr = std::unique_ptr<GY_TcpProtocolFactoryBase>;
        using wptr = std::weak_ptr<GY_TcpProtocolFactoryBase>;
        virtual protocol::GY_TcpRequest::ptr CreateRequest() = 0;
        virtual protocol::GY_TcpResponse::ptr CreateResponse() = 0;
        virtual ~GY_TcpProtocolFactoryBase() = default;
    };

    template<Tcp_Request REQ, Tcp_Response RESP>
    class GY_TcpProtocolFactory: public GY_TcpProtocolFactoryBase
    {
    public:
        GY_TcpProtocolFactory() = default;
        using ptr = std::shared_ptr<GY_TcpProtocolFactory>;
        using uptr = std::unique_ptr<GY_TcpProtocolFactory>;
        using wptr = std::weak_ptr<GY_TcpProtocolFactory>;
        virtual protocol::GY_TcpRequest::ptr CreateRequest();
        virtual protocol::GY_TcpResponse::ptr CreateResponse();
    };

    #include "factory.inl"
}


#endif