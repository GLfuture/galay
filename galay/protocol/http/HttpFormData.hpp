#ifndef GALAY_HTTP_FORM_DATA_HPP
#define GALAY_HTTP_FORM_DATA_HPP

#include "HttpProtoc.hpp"
#include <vector>
#include <variant>

namespace galay::http
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
    static bool IsFormData(HttpRequest::ptr request);
    static bool ParseFormData(HttpRequest::ptr request, std::vector<FormDataValue>& values);
    static void FormDataToString(HttpRequest::ptr request, const std::string& boundary, const std::vector<FormDataValue>& values);
};
    


}

#endif