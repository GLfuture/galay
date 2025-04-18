#ifndef GALAY_HTTP_ROUTE_HPP
#define GALAY_HTTP_ROUTE_HPP

#include <stack>
#include <unordered_map>
#include <functional>
#include "galay/kernel/Coroutine.hpp"


namespace galay::http
{

class HttpContext;

class HttpRouter
{
public:
    using Handler = std::function<Coroutine<void>(RoutineCtx,HttpContext)>;
    using HandlerMap = std::unordered_map<std::string, Handler>;

    void AddHandler(const std::string& path, Handler handler);
    bool Find(const std::string& path, Handler& handler, std::unordered_map<std::string, std::string> &params);
private:
    static std::string NormalizePath(const std::string& path);
    // 分割路径段
    static std::vector<std::string> SplitSegments(const std::string& path);
    // 判断是否为参数段 {param}
    static bool IsParamSegment(const std::string& seg);
    // 验证模板有效性
    bool IsTemplate(const std::string& path);
private:
    HandlerMap m_handlers;
    HandlerMap m_template_handlers;
};

    
}


#endif