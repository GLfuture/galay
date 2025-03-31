#include "HttpMiddleware.hpp"
#include "Http.hpp"

namespace galay::http 
{

Coroutine<bool> HttpMiddleware::HandleRequestImpl(RoutineCtx ctx, HttpMiddleware *middleware, HttpStream_Ptr stream)
{
    co_await this_coroutine::WaitAsyncExecute<void, bool>(middleware->OnStreamHandle(ctx, stream));
    bool result = middleware->StreamHandleSuccess();
    if(result) {
        if(middleware->m_next.get()) { 
            bool next_res = co_await this_coroutine::WaitAsyncRtnExecute<bool, bool>(HandleRequestImpl(ctx, middleware->m_next.get(), stream));
            co_return std::move(next_res);
        }
        co_return std::move(result);
    }
    co_return std::move(result);
}

Coroutine<void> HttpStaticFileMiddleware::OnStreamHandle(RoutineCtx ctx, HttpStream_Ptr stream)
{
    return OnStreamHandleImpl(ctx, this, stream);
}

bool HttpStaticFileMiddleware::StreamHandleSuccess()
{
    return m_result;
}

Coroutine<void> HttpStaticFileMiddleware::OnStreamHandleImpl(RoutineCtx ctx, HttpStaticFileMiddleware *middleware, HttpStream_Ptr stream)
{
    auto& request = stream->GetRequest();
    auto& response = stream->GetResponse();
    std::string& uri = request->Header()->Uri();
    if(request->Header()->Method() != HttpMethod::Http_Method_Get \
        && !uri.starts_with(middleware->m_url_path)) {
        middleware->m_result = true;
        co_return;
    }
    std::string path = middleware->m_root_path + uri.substr(middleware->m_url_path.length());
    if(std::filesystem::exists(path) && std::filesystem::is_regular_file(path)) {
        unsigned long file_size = std::filesystem::file_size(path);
        HttpHelper::DefaultHttpResponse(response.get(), HttpVersion::Http_Version_1_1, HttpStatusCode::OK_200\
            , MimeType::ConvertToMimeType(path.substr(path.find_last_of('.') + 1)), "");
        response->Header()->HeaderPairs().AddHeaderPair("Content-Length", std::to_string(file_size));
        bool res = co_await stream->SendResponse(ctx);
        if(!res) {
            co_await stream->Close();
            middleware->m_result = false;
            co_return;
        }
        int fd = open(path.c_str(), O_RDONLY | O_NONBLOCK);
        FileDesc desc({fd}, static_cast<off_t>(file_size));
        res = co_await stream->SendFile(ctx, &desc);
        if(!res) {
            co_await stream->Close();
            middleware->m_result = false;
            co_return;
        }
    }
    middleware->m_result = false;
    co_return;
}



}