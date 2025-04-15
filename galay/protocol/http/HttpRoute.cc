#include "HttpRoute.hpp"

namespace galay::http
{

void HttpRouter::AddHandler(const std::string &path, Handler &&handler)
{
    if(IsTemplate(path)) {
        m_template_handlers.emplace(path, std::move(handler));
    } else {
        m_handlers.emplace(path, std::move(handler));
    }
}

bool HttpRouter::Find(const std::string &path, Handler &handler, std::unordered_map<std::string, std::string> &oparams)
{
    std::string normalized = NormalizePath(path);
    // 1. 精确匹配
    if(auto it = m_handlers.find(normalized); it != m_handlers.end()) {
        handler = it->second;
        return true;
    }

    // 2. 模板匹配
    auto pathSegments = SplitSegments(normalized);
    
    for(const auto& [tplPath, tplHandler] : m_template_handlers) {
        auto tplSegments = SplitSegments(tplPath);
        
        if(pathSegments.size() != tplSegments.size()) continue;

        std::unordered_map<std::string, std::string> params;
        bool match = true;

        for(size_t i = 0; i < pathSegments.size(); ++i) {
            const auto& reqSeg = pathSegments[i];
            const auto& tplSeg = tplSegments[i];

            if(IsParamSegment(tplSeg)) {
                auto paramName = tplSeg.substr(1, tplSeg.size() - 2);
                if(paramName.empty()) {
                    if(reqSeg != tplSeg) {
                        match = false;
                        break;
                    }
                } else {
                    params.emplace(paramName, reqSeg);
                }
            } else if(reqSeg != tplSeg) {
                match = false;
                break;
            }
        }

        if(match) {
            handler = tplHandler;
            oparams = std::move(params);
            return true;
        }
    }

    return false;
}

std::string HttpRouter::NormalizePath(const std::string &path)
{
    if(path.empty()) return "/";

    std::string normalized;
    normalized.reserve(path.size());
    bool prevSlash = false;
    
    for(char c : path) {
        if(c == '/') {
            if(!prevSlash) {
                normalized.push_back(c);
                prevSlash = true;
            }
        } else {
            normalized.push_back(c);
            prevSlash = false;
        }
    }
    
    if(normalized.size() > 1 && normalized.back() == '/') {
        normalized.pop_back();
    }
    
    return normalized.empty() ? "/" : normalized;
}

std::vector<std::string> HttpRouter::SplitSegments(const std::string &path)
{
    std::vector<std::string> segments;
    std::istringstream iss(path);
    std::string seg;
    
    while(std::getline(iss, seg, '/')) {
        if(!seg.empty()) {
            segments.emplace_back(seg);
        }
    }
    
    return segments;
}

bool HttpRouter::IsParamSegment(const std::string &seg)
{
    return seg.size() >= 2 && seg.front() == '{' && seg.back() == '}';
}

bool HttpRouter::IsTemplate(const std::string &path)
{
    std::stack<char> braces;
    for(char c : path) {
        if(c == '{') {
            braces.push(c);
        } else if(c == '}') {
            if(braces.empty() || braces.top() != '{') return false;
            braces.pop();
        }
    }
    return braces.empty();
}


}