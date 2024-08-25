#ifndef GALAY_HTTP_HELPER_H
#define GALAY_HTTP_HELPER_H

#include "../protocol/http.h"

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
        void SetFile(const std::string& fileName, const std::string& body);
        FormFile ToFile() const;
        void SetString(const std::string& body);
        bool IsString() const;
        std::string ToString() const;
        void SetNumber(int number);
        bool IsNumber() const;
        int ToNumber() const;
        void SetDouble(double number);
        bool IsDouble() const;
        double ToDouble() const;
    private:
        FormDataType m_type;
        FormDataHeader m_header;
        std::string m_name;
        std::any m_body;
    };

    class HttpFormDataHelper
    {
    public:
        static bool IsFormData(protocol::http::HttpRequest::ptr request);
        static bool ParseFormData(protocol::http::HttpRequest::ptr request, std::vector<FormDataValue>& values);
        static void FormDataToString(protocol::http::HttpRequest::ptr request, const std::string& boundary, const std::vector<FormDataValue>& values);
    };
}

#endif