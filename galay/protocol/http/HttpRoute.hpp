#ifndef GALAY_HTTP_ROUTE_HPP
#define GALAY_HTTP_ROUTE_HPP

#include <unordered_map>

namespace galay::http
{

template <typename SocketType>
class HttpRouter
{
public:
    using HttpStreamPtr = typename HttpStreamImpl<SocketType>::ptr;
    using Handler = std::function<Coroutine<void>(RoutineCtx,HttpStreamPtr)>;
    using HandlerMap = std::unordered_map<HttpMethod, std::unordered_map<std::string, Handler>>;


};

    
}


#endif