#ifndef GALAY_HTTP_1_1_H
#define GALAY_HTTP_1_1_H

#include "basic_protocol.h"
#include <vector>
#include <assert.h>
#include <algorithm>
#include <unordered_set>
#include <string_view>
#include <map>


namespace galay
{
    namespace protocol
    {
        namespace http
        {
            extern std::unordered_set<std::string> m_stdMethods;

            class HttpRequestHeader
            {
            public:
                using ptr = std::shared_ptr<HttpRequestHeader>;
                HttpRequestHeader() = default;
                std::string& Method();
                std::string& Uri();
                std::string& Version();
                std::map<std::string,std::string>& Args();
                std::map<std::string,std::string>& Headers();
                std::string ToString();
                bool FromString(std::string_view str);
                void CopyFrom(HttpRequestHeader::ptr header);
            private:
                void ParseArgs(std::string uri);
                std::string ConvertFromUri(std::string&& url, bool convert_plus_to_space);
                std::string ConvertToUri(std::string&& url);
                bool IsHex(char c, int &v);
                size_t ToUtf8(int code, char *buff);
                bool FromHexToI(const std::string &s, size_t i, size_t cnt, int &val);
            private:
                std::string m_method;
                std::string m_uri;                                          // uri
                std::string m_version;                                      // 版本号
                std::map<std::string, std::string> m_argList;     // 参数
                std::map<std::string, std::string> m_headers;     // 字段
            };

            class HttpRequest : public GY_Request, public GY_Response, public galay::common::GY_DynamicCreator<GY_Request,HttpRequest>
            {
            public:
                using ptr = std::shared_ptr<HttpRequest>;
                using wptr = std::weak_ptr<HttpRequest>;
                using uptr = std::unique_ptr<HttpRequest>;
                HttpRequestHeader::ptr Header();
                std::string& Body();
                galay::protocol::ProtoJudgeType DecodePdu(std::string &buffer) override;
                std::string EncodePdu() override;
                //chunck
                bool StartChunck();
                std::string ChunckStream(std::string&& buffer);
                std::string EndChunck();
            private:
                galay::protocol::ProtoJudgeType GetHttpBody(std::string& buffer);
                galay::protocol::ProtoJudgeType GetChunckBody(std::string& buffer);
            private:
                HttpProStatus m_status = kHttpHeader;
                HttpRequestHeader::ptr m_header;
                std::string m_body;
            };

            class HttpResponseHeader
            {
            public:
                using ptr = std::shared_ptr<HttpResponseHeader>;
                std::string& Version();
                int& Code();
                std::map<std::string, std::string>& Headers();
                std::string ToString();
                bool FromString(std::string_view str);
            private:
                std::string CodeMsg(int status);
            private:
                std::string m_version;
                int m_code;
                std::map<std::string, std::string> m_headers;
            };

            class HttpResponse : public GY_Response, public GY_Request, public galay::common::GY_DynamicCreator<GY_Response,HttpResponse>
            {
            public:
                using ptr = std::shared_ptr<HttpResponse>;
                using wptr = std::weak_ptr<HttpResponse>;
                using uptr = std::weak_ptr<HttpResponse>;
                
                HttpResponseHeader::ptr Header();
                std::string& Body();
                std::string EncodePdu() override;
                galay::protocol::ProtoJudgeType DecodePdu(std::string &buffer) override;
                //chunck
                bool StartChunck();
                std::string ChunckStream(std::string&& buffer);
                std::string EndChunck();
            private:
                galay::protocol::ProtoJudgeType GetHttpBody(std::string& buffer);
                galay::protocol::ProtoJudgeType GetChunckBody(std::string& buffer);
            private:
                HttpProStatus m_status = kHttpHeader;
                HttpResponseHeader::ptr m_header;
                std::string m_body;
            };


            extern HttpRequest::ptr DefaultHttpRequest();
        }
    }

}

#endif