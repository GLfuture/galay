#ifndef GALAY_HTTP_PROTOC_HPP
#define GALAY_HTTP_PROTOC_HPP

#include "HttpCommon.hpp"
#include "galay/common/Error.h"
#include <map>
#include <sstream>

namespace galay::http 
{

class HeaderPair
{
public:
    bool HasKey(const std::string& key) const;
    std::string GetValue(const std::string& key);
    error::HttpErrorCode RemoveHeaderPair(const std::string& key);
    error::HttpErrorCode AddHeaderPair(const std::string& key, const std::string& value);
    error::HttpErrorCode SetHeaderPair(const std::string& key, const std::string& value);
    std::string ToString();
    void Clear();
    HeaderPair& operator=(const HeaderPair& headerPair);
private:
    std::ostringstream m_stream;
    std::map<std::string, std::string> m_headerPairs;
};

class HttpRequestHeader
{
public:
    using ptr = std::shared_ptr<HttpRequestHeader>;
    HttpRequestHeader() = default;
    HttpMethod& Method();
    std::string& Uri();
    HttpVersion& Version();
    std::map<std::string,std::string>& Args();
    HeaderPair& HeaderPairs();
    std::string ToString();
    error::HttpErrorCode FromString(HttpDecodeStatus& status, std::string_view str, size_t& next_index);
    void CopyFrom(const HttpRequestHeader::ptr& header);
    void Reset();
private:
    void ParseArgs(std::string uri);
    std::string ConvertFromUri(std::string&& url, bool convert_plus_to_space);
    std::string ConvertToUri(std::string&& url);
    bool IsHex(char c, int &v);
    size_t ToUtf8(int code, char *buff);
    bool FromHexToI(const std::string &s, size_t i, size_t cnt, int &val);
private:
    std::ostringstream m_stream;
    HttpMethod m_method;
    std::string m_uri = "/";                                    // uri
    HttpVersion m_version;                                      // 版本号
    std::map<std::string, std::string> m_argList;               // 参数
    HeaderPair m_headerPairs;                                   // 字段
};

class HttpRequest
{
public:
    using ptr = std::shared_ptr<HttpRequest>;
    using wptr = std::weak_ptr<HttpRequest>;
    using uptr = std::unique_ptr<HttpRequest>;
    
    HttpRequest();
    HttpRequestHeader::ptr Header();
    void SetContent(std::string&& content, const std::string& type = "");
    std::string_view GetContent();
    bool ParseHeader(const std::string_view &buffer);
    bool ParseBody(const std::string_view &buffer);
    std::string EncodePdu() const;
    bool HasError() const;
    int GetErrorCode() const;
    std::string GetErrorString();

    size_t GetNextIndex() const; 

    void Reset();
    //chunk
    bool StartChunk();
    std::string ToChunkData(std::string&& buffer);
    std::string EndChunk();
private:
    bool GetHttpBody(const std::string_view& buffer);
    bool GetChunkBody(const std::string_view& buffer);
private:
    HttpDecodeStatus m_status;
    size_t m_next_index;
    std::string m_body;
    error::HttpError::ptr m_error;
    HttpRequestHeader::ptr m_header;
};

class HttpResponseHeader
{
public:
    using ptr = std::shared_ptr<HttpResponseHeader>;
    HttpResponseHeader() = default;
    HttpVersion& Version();
    HttpStatusCode& Code();
    HeaderPair& HeaderPairs();
    std::string ToString();
    error::HttpErrorCode FromString(HttpDecodeStatus& status, std::string_view str, size_t& next_index);
private:
    std::ostringstream m_stream;
    HttpStatusCode m_code;
    HttpVersion m_version;
    HeaderPair m_headerPairs;
};

class HttpResponse
{
public:
    using ptr = std::shared_ptr<HttpResponse>;
    using wptr = std::weak_ptr<HttpResponse>;
    using uptr = std::unique_ptr<HttpResponse>;
    HttpResponse();
    HttpResponseHeader::ptr Header();
    void SetContent(const std::string& type, std::string&& content);
    std::string_view GetContent();
    size_t GetNextIndex() const;
    std::string EncodePdu() const;
    bool ParseHeader(const std::string_view &buffer);
    bool ParseBody(const std::string_view &buffer);
    bool HasError() const;
    int GetErrorCode() const;
    std::string GetErrorString();
    void Reset();
    
    //chunck
    bool StartChunck();
    std::string ToChunckData(std::string&& buffer);
    std::string EndChunck();
private:
    bool GetHttpBody(const std::string_view& buffer);
    bool GetChunkBody(const std::string_view& buffer);
private:
    HttpDecodeStatus m_status;
    HttpResponseHeader::ptr m_header;
    std::string m_body;
    size_t m_next_index;
    error::HttpError::ptr m_error;
};
    


}

#endif