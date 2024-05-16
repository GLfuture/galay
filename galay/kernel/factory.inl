
#ifndef GALAY_KERNEL_FACTORY_INL
#define GALAY_KERNEL_FACTORY_INL
template <galay::Tcp_Request REQ, galay::Tcp_Response RESP>
galay::protocol::GY_TcpRequest::ptr
galay::GY_TcpProtocolFactory<REQ, RESP>::CreateRequest()
{
    return std::make_shared<REQ>();
}

template <galay::Tcp_Request REQ, galay::Tcp_Response RESP>
galay::protocol::GY_TcpResponse::ptr
galay::GY_TcpProtocolFactory<REQ, RESP>::CreateResponse()
{
    return std::make_shared<RESP>();
}
#endif
