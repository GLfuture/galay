#ifndef GALAY_HTTPMIDDLEWARE_HPP
#define GALAY_HTTPMIDDLEWARE_HPP

#include <memory>
#include <string>
#include "galay/kernel/Coroutine.hpp"

namespace galay::http {

class HttpContext;

class HttpMiddleware
{
public:
    using ptr = std::shared_ptr<HttpMiddleware>;
    using uptr = std::unique_ptr<HttpMiddleware>;

    virtual void SetNext(HttpMiddleware::uptr next) {
        m_next = std::move(next);
    }

    /*
        @return true: continue to next middleware, false: stop middleware chain and continue to handle next request(must set response)
    */
    virtual Coroutine<bool> HandleRequest(RoutineCtx ctx, HttpContext& context) {
        return HandleRequestImpl(ctx, this, context);
    }
    
    virtual ~HttpMiddleware() = default;
protected:
    static Coroutine<bool> HandleRequestImpl(RoutineCtx ctx, HttpMiddleware* middleware, HttpContext& context);
    
    /*
        处理请求，需要自行发送响应
    */
    virtual Coroutine<void> OnStreamHandle(RoutineCtx ctx, HttpContext& context) = 0;
    /*
        @return true: 责任链继续，false: 责任链停止
    */
    virtual bool StreamHandleSuccess() = 0;
protected:
    HttpMiddleware::uptr m_next;
};


class HttpStaticFileMiddleware final: public HttpMiddleware {
public:
    HttpStaticFileMiddleware(const std::string url_path, const std::string& root_path)
        : m_root_path(root_path), m_url_path(url_path)
    {
    }
    
    Coroutine<void> OnStreamHandle(RoutineCtx ctx, HttpContext& context) override;
    bool StreamHandleSuccess() override;
private:
    static Coroutine<void> OnStreamHandleImpl(RoutineCtx ctx, HttpStaticFileMiddleware* middleware, HttpContext& context);
private:
    std::string m_root_path;
    std::string m_url_path;
    bool m_result = false;
};
    

}

#endif