#include "HttpCommon.hpp"
#include <vector>

namespace galay::http 
{
std::vector<std::string> 
g_HttpMethods = {
    "GET", "POST", "HEAD", "PUT", "DELETE", "TRACE", "OPTIONS", "CONNECT" , "PATCH", "UNKNOWN"
};

std::vector<std::string> 
g_HttpVersion = {
    "HTTP/1.0",
    "HTTP/1.1",
    "HTTP/2.0",
    "HTTP/3.0",
    "Unknown"
};

std::string HttpVersionToString(HttpVersion version)
{
    return g_HttpVersion[static_cast<int>(version)];
}

HttpVersion StringToHttpVersion(std::string_view str)
{
    for (int i = 0; i < g_HttpVersion.size(); ++i) {
        if (str == g_HttpVersion[i]) {
            return static_cast<HttpVersion>(i);
        }
    }
    return HttpVersion::Http_Version_Unknown;
}

std::string HttpMethodToString(HttpMethod method)
{
    return g_HttpMethods[static_cast<int>(method)];
}

HttpMethod StringToHttpMethod(std::string_view str)
{
    for (int i = 0; i < g_HttpMethods.size(); ++i) {
        if (str == g_HttpMethods[i]) {
            return static_cast<HttpMethod>(i);
        }
    }
    return HttpMethod::Http_Method_Unknown;
}

std::string HttpStatusCodeToString(HttpStatusCode code)
{
    switch (code)
    {
    case HttpStatusCode::Continue_100:
        return "Continue";
    case HttpStatusCode::SwitchingProtocol_101:
        return "Switching Protocol";
    case HttpStatusCode::Processing_102:
        return "Processing";
    case HttpStatusCode::EarlyHints_103:
        return "Early Hints";
    case HttpStatusCode::OK_200:
        return "OK";
    case HttpStatusCode::Created_201:
        return "Created";
    case HttpStatusCode::Accepted_202:
        return "Accepted";
    case HttpStatusCode::NonAuthoritativeInformation_203:
        return "Non-Authoritative Information";
    case HttpStatusCode::NoContent_204:
        return "No Content";
    case HttpStatusCode::ResetContent_205:
        return "Reset Content";
    case HttpStatusCode::PartialContent_206:
        return "Partial Content";
    case HttpStatusCode::MultiStatus_207:
        return "Multi-Status";
    case HttpStatusCode::AlreadyReported_208:
        return "Already Reported";
    case HttpStatusCode::IMUsed_226:
        return "IM Used";
    case HttpStatusCode::MultipleChoices_300:
        return "Multiple Choices";
    case HttpStatusCode::MovedPermanently_301:
        return "Moved Permanently";
    case HttpStatusCode::Found_302:
        return "Found";
    case HttpStatusCode::SeeOther_303:
        return "See Other";
    case HttpStatusCode::NotModified_304:
        return "Not Modified";
    case HttpStatusCode::UseProxy_305:
        return "Use Proxy";
    case HttpStatusCode::Unused_306:
        return "unused";
    case HttpStatusCode::TemporaryRedirect_307:
        return "Temporary Redirect";
    case HttpStatusCode::PermanentRedirect_308:
        return "Permanent Redirect";
    case HttpStatusCode::BadRequest_400:
        return "Bad Request";
    case HttpStatusCode::Unauthorized_401:
        return "Unauthorized";
    case HttpStatusCode::PaymentRequired_402:
        return "Payment Required";
    case HttpStatusCode::Forbidden_403:
        return "Forbidden";
    case HttpStatusCode::NotFound_404:
        return "Not Found";
    case HttpStatusCode::MethodNotAllowed_405:
        return "Method Not Allowed";
    case HttpStatusCode::NotAcceptable_406:
        return "Not Acceptable";
    case HttpStatusCode::ProxyAuthenticationRequired_407:
        return "Proxy Authentication Required";
    case HttpStatusCode::RequestTimeout_408:
        return "Request Timeout";
    case HttpStatusCode::Conflict_409:
        return "Conflict";
    case HttpStatusCode::Gone_410:
        return "Gone";
    case HttpStatusCode::LengthRequired_411:
        return "Length Required";
    case HttpStatusCode::PreconditionFailed_412:
        return "Precondition Failed";
    case HttpStatusCode::PayloadTooLarge_413:
        return "Payload Too Large";
    case HttpStatusCode::UriTooLong_414:
        return "URI Too Long";
    case HttpStatusCode::UnsupportedMediaType_415:
        return "Unsupported Media Type";
    case HttpStatusCode::RangeNotSatisfiable_416:
        return "Range Not Satisfiable";
    case HttpStatusCode::ExpectationFailed_417:
        return "Expectation Failed";
    case HttpStatusCode::ImATeapot_418:
        return "I'm a teapot";
    case HttpStatusCode::MisdirectedRequest_421:
        return "Misdirected Request";
    case HttpStatusCode::UnprocessableContent_422:
        return "Unprocessable Content";
    case HttpStatusCode::Locked_423:
        return "Locked";
    case HttpStatusCode::FailedDependency_424:
        return "Failed Dependency";
    case HttpStatusCode::TooEarly_425:
        return "Too Early";
    case HttpStatusCode::UpgradeRequired_426:
        return "Upgrade Required";
    case HttpStatusCode::PreconditionRequired_428:
        return "Precondition Required";
    case HttpStatusCode::TooManyRequests_429:
        return "Too Many Requests";
    case HttpStatusCode::RequestHeaderFieldsTooLarge_431:
        return "Request Header Fields Too Large";
    case HttpStatusCode::UnavailableForLegalReasons_451:
        return "Unavailable For Legal Reasons";
    case HttpStatusCode::NotImplemented_501:
        return "Not Implemented";
    case HttpStatusCode::BadGateway_502:
        return "Bad Gateway";
    case HttpStatusCode::ServiceUnavailable_503:
        return "Service Unavailable";
    case HttpStatusCode::GatewayTimeout_504:
        return "Gateway Timeout";
    case HttpStatusCode::HttpVersionNotSupported_505:
        return "HTTP Version Not Supported";
    case HttpStatusCode::VariantAlsoNegotiates_506:
        return "Variant Also Negotiates";
    case HttpStatusCode::InsufficientStorage_507:
        return "Insufficient Storage";
    case HttpStatusCode::LoopDetected_508:
        return "Loop Detected";
    case HttpStatusCode::NotExtended_510:
        return "Not Extended";
    case HttpStatusCode::NetworkAuthenticationRequired_511:
        return "Network Authentication Required";
    default:
    case HttpStatusCode::InternalServerError_500:
        return "Internal Server Error";
    }
    return "";
}

std::unordered_map<std::string, std::string> MimeType::mimeTypeMap = {
    {"html", "text/html"},
    {"htm", "text/html"},
    {"shtml", "text/html"},
    {"css", "text/css"},
    {"xml", "text/xml"},
    {"gif", "image/gif"},
    {"jpeg", "image/jpeg"},
    {"jpg", "image/jpeg"},
    {"js", "application/javascript"},
    {"atom", "application/atom+xml"},
    {"rss", "application/rss+xml"},
    {"mml", "text/mathml"},
    {"txt", "text/plain"},
    {"jad", "text/vnd.sun.j2me.app-descriptor"},
    {"wml", "text/vnd.wap.wml"},
    {"htc", "text/x-component"},
    {"png", "image/png"},
    {"tif", "image/tiff"},
    {"tiff", "image/tiff"},
    {"wbmp", "image/vnd.wap.wbmp"},
    {"ico", "image/x-icon"},
    {"jpe", "image/jpeg"},
    {"json", "application/json"},
    {"pdf", "application/pdf"},
    {"zip", "application/zip"},
    {"tar", "application/x-tar"},
    {"gz", "application/gzip"},
    {"bz2", "application/x-bzip2"},
    {"mp3", "audio/mpeg"},
    {"wav", "audio/wav"},
    {"mp4", "video/mp4"},
    {"avi", "video/x-msvideo"},
    {"mov", "video/quicktime"},
    {"flv", "video/x-flv"},
    {"swf", "application/x-shockwave-flash"},
    {"doc", "application/msword"},
    {"docx", "application/vnd.openxmlformats-officedocument.wordprocessingml.document"},
    {"xls", "application/vnd.ms-excel"},
    {"xlsx", "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet"},
    {"ppt", "application/vnd.ms-powerpoint"},
    {"pptx", "application/vnd.openxmlformats-officedocument.presentationml.presentation"},
    {"rtf", "application/rtf"},
    {"ps", "application/postscript"},
    {"eps", "application/postscript"},
    {"svg", "image/svg+xml"},
    {"svgz", "image/svg+xml"},
    {"csv", "text/csv"},
    {"tsv", "text/tab-separated-values"},
    {"md", "text/markdown"},
    {"markdown", "text/markdown"},
    {"log", "text/plain"},
    {"conf", "text/plain"},
    {"ini", "text/plain"},
    {"sh", "application/x-sh"},
    {"bat", "application/x-msdos-program"},
    {"py", "text/x-python"},
    {"pl", "application/x-perl"},
    {"php", "application/x-httpd-php"},
    {"rb", "application/x-ruby"},
    {"java", "text/x-java-source"},
    {"c", "text/x-c"},
    {"cpp", "text/x-c++"},
    {"h", "text/x-c"},
    {"hpp", "text/x-c++"},
    {"cs", "text/x-csharp"},
    {"go", "text/x-go"},
    {"swift", "text/x-swift"},
    {"kt", "text/x-kotlin"},
    {"rs", "text/x-rust"},
    {"dart", "text/x-dart"},
    {"ts", "text/typescript"},
    {"tsx", "text/typescript-jsx"},
    {"jsx", "text/jsx"},
    {"vue", "text/x-vue"},
    {"less", "text/x-less"},
    {"scss", "text/x-scss"},
    {"sass", "text/x-sass"},
    {"styl", "text/x-stylus"},
    {"map", "application/json"},
    {"map.json", "application/json"},
    {"woff", "font/woff"},
    {"woff2", "font/woff2"},
    {"ttf", "font/ttf"},
    {"otf", "font/otf"},
    {"eot", "application/vnd.ms-fontobject"},
    {"oga", "audio/ogg"},
    {"ogv", "video/ogg"},
    {"ogx", "application/ogg"},
    {"webm", "video/webm"},
    {"aac", "audio/aac"},
    {"oga", "audio/ogg"},
    {"opus", "audio/opus"},
    {"webp", "image/webp"},
    {"bmp", "image/bmp"},
    {"weba", "audio/webm"},
    {"mid", "audio/midi"},
    {"midi", "audio/midi"},
    {"mpga", "audio/mpeg"},
    {"mp2", "audio/mpeg"},
    {"mp2a", "audio/mpeg"},
    {"m3u", "audio/x-mpegurl"},
    {"m3u8", "application/x-mpegURL"},
    {"pls", "audio/x-scpls"},
    {"xspf", "application/xspf+xml"},
    {"au", "audio/basic"},
    {"snd", "audio/basic"},
    {"mid", "audio/midi"},
    {"midi", "audio/midi"},
    {"kar", "audio/midi"},
    {"mpga", "audio/mpeg"},
    {"mp2", "audio/mpeg"},
    {"mp2a", "audio/mpeg"},
    {"mp3", "audio/mpeg"},
    {"m2a", "audio/mpeg"},
    {"m3a", "audio/mpeg"},
    {"oga", "audio/ogg"},
    {"ogg", "audio/ogg"},
    {"spx", "audio/ogg"},
    {"opus", "audio/opus"},
    {"ra", "audio/x-realaudio"},
    {"ram", "audio/x-pn-realaudio"},
    {"wav", "audio/x-wav"},
    {"weba", "audio/webm"},
    {"aac", "audio/aac"},
    {"flac", "audio/flac"},
    {"mka", "audio/x-matroska"},
    {"mkv", "video/x-matroska"},
    {"ogv", "video/ogg"},
    {"qt", "video/quicktime"},
    {"mov", "video/quicktime"},
    {"webm", "video/webm"},
    {"rm", "application/vnd.rn-realmedia"},
    {"rpm", "application/x-rpm"},
    {"rar", "application/x-rar-compressed"},
    {"deb", "application/x-debian-package"},
    {"dmg", "application/x-apple-diskimage"},
    {"iso", "application/x-iso9660-image"},
    {"bin", "application/octet-stream"},
    {"dat", "application/octet-stream"},
    {"dump", "application/octet-stream"},
    {"exe", "application/octet-stream"},
    {"img", "application/octet-stream"},
    {"msi", "application/octet-stream"},
    {"msp", "application/octet-stream"},
    {"msu", "application/octet-stream"},
    {"class", "application/java-vm"},
    {"jar", "application/java-archive"},
    {"war", "application/java-archive"},
    {"ear", "application/java-archive"},
    {"jad", "text/vnd.sun.j2me.app-descriptor"},
    {"jardiff", "application/x-java-archive-diff"},
    {"jnlp", "application/x-java-jnlp-file"},
    {"ser", "application/java-serialized-object"},
    {"class", "application/java-vm"},
    {"js", "application/javascript"},
    {"mjs", "application/javascript"},
    {"json", "application/json"},
    {"map", "application/json"},
    {"jsonld", "application/ld+json"},
    {"webmanifest", "application/manifest+json"},
    {"xhtml", "application/xhtml+xml"},
    {"xht", "application/xhtml+xml"},
    {"xslt", "application/xslt+xml"},
    {"xsl", "application/xslt+xml"},
    {"xspf", "application/xspf+xml"},
    {"xul", "application/vnd.mozilla.xul+xml"},
    {"yaml", "text/yaml"},
    {"yml", "text/yaml"},
};


std::string http::MimeType::ConvertToMimeType(const std::string &type)
{
    auto it = mimeTypeMap.find(type);
    if( it == mimeTypeMap.end() ) {
        return "text/plain";
    }
    return it->second;
}

}