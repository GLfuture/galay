#ifndef GALAY_HTTP_HELPER_H
#define GALAY_HTTP_HELPER_H

#include "galay/protocol/Http.h"
#include <variant>

namespace galay::helper::http
{

    enum FormsType
    {
        kFormsType_NoFrom,
        kFormsType_FormData,
        kFormsType_XWwwFormUrlencoded,
        kFormsType_Json,
        kFormsType_Xml,
        kFormsType_Raw,
        kFormsType_Binary,
        kFormsType_GraphQL,
        kFormsType_MsgPack,
    };

    class FormDataHeader
    {
    public:
        bool HasKey(const std::string& key);
        std::string GetValue(const std::string& key);
        bool RemoveHeaderPair(const std::string& key);
        bool SetHeaderPair(const std::string& key, const std::string& value);
        bool AddHeaderPair(const std::string& key, const std::string& value);
        std::string ToString();
    private:
        std::map<std::string, std::string> m_headerPairs;
    };

    enum FormDataType
    {
        kFormDataType_File,
        kFormDataType_String,
        kFormDataType_Number,
        kFormDataType_Double,
    };

    struct FormFile
    {
        std::string m_fileName;
        std::string m_body;
    };

    class FormDataValue
    {
    public:
        FormDataValue() = default;
        FormDataHeader& Header();
        std::string Name() const;
        void SetName(const std::string& name);
        bool IsFile() const;
        bool IsString() const;
        bool IsNumber() const;
        bool IsDouble() const;

        FormFile ToFile() const;
        std::string ToString() const;
        int ToNumber() const;
        double ToDouble() const;
        
        void SetValue(const FormFile& file);
        void SetValue(const std::string& body);
        void SetValue(int number);
        void SetValue(double number);
    private:
        FormDataType m_type;
        FormDataHeader m_header;
        std::string m_name;
        std::variant<int, double, std::string, FormFile> m_body;
    };

    class HttpFormDataHelper
    {
    public:
        static bool IsFormData(protocol::http::HttpRequest::ptr request);
        static bool ParseFormData(protocol::http::HttpRequest::ptr request, std::vector<FormDataValue>& values);
        static void FormDataToString(protocol::http::HttpRequest::ptr request, const std::string& boundary, const std::vector<FormDataValue>& values);
    };

    class HttpHelper
    {
    public:
        using HttpResponseCode = protocol::http::HttpStatusCode;
        //request
        static bool DefaultGet(protocol::http::HttpRequest* request, const std::string& url, bool keepalive = true);
        static bool DefaultPost(protocol::http::HttpRequest* request, const std::string& url, bool keepalive = true);
        //response
        static bool DefaultRedirect(protocol::http::HttpResponse* response, const std::string& url, HttpResponseCode code);
        static bool DefaultHttpResponse(protocol::http::HttpResponse* response, protocol::http::HttpVersion version, protocol::http::HttpStatusCode code, std::string type, std::string &&body);
    };
    
}

#endif