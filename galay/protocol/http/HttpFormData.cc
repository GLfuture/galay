#include "HttpFormData.hpp"
#include "galay/utils/String.h"
#include <regex>

namespace galay::http 
{

bool 
FormDataHeader::HasKey(const std::string& key)
{
    return m_headerPairs.contains(key);
}

std::string 
FormDataHeader::GetValue(const std::string& key)
{
    if(m_headerPairs.contains(key)) return m_headerPairs[key];
    return "";
}

bool 
FormDataHeader::RemoveHeaderPair(const std::string& key)
{
    if(m_headerPairs.contains(key)) {
        m_headerPairs.erase(key);
        return true;
    }
    return false;
}

bool  
FormDataHeader::SetHeaderPair(const std::string& key, const std::string& value)
{
    if(m_headerPairs.contains(key)){
        m_headerPairs[key] = value;
        return true;
    }
    return false;
}

bool 
FormDataHeader::AddHeaderPair(const std::string& key, const std::string& value)
{
    if(!m_headerPairs.contains(key)){
        m_headerPairs[key] = value;
        return true;
    }
    return false;
}

std::string 
FormDataHeader::ToString()
{
    std::string res;
    for(auto &headerPair : m_headerPairs)
    {
        res += headerPair.first + ": " + headerPair.second + "\r\n";
    }
    return res + "\r\n";
}

void 
FormDataValue::SetValue(const FormFile& file)
{
    this->m_body = file;
    this->m_type = kFormDataType_File;
}

bool 
FormDataValue::IsFile() const
{
    return this->m_type == kFormDataType_File;
}

FormFile 
FormDataValue::ToFile() const
{
    return std::get<FormFile>(this->m_body);
}

void 
FormDataValue::SetValue(const std::string& body)
{
    this->m_body = body;
    this->m_type = kFormDataType_String;
}

bool 
FormDataValue::IsString() const
{
    return this->m_type == kFormDataType_String;
}

std::string 
FormDataValue::ToString() const
{
    return std::get<std::string>(this->m_body);
}

void 
FormDataValue::SetValue(int number)
{
    this->m_body = number;
    this->m_type = kFormDataType_Number;
}

bool 
FormDataValue::IsNumber() const
{
    return this->m_type == kFormDataType_Number;
}

void 
FormDataValue::SetValue(double number)
{
    this->m_body = number;
    this->m_type = kFormDataType_Double;
}

int 
FormDataValue::ToNumber() const
{
    return std::get<int>(this->m_body);
}

bool 
FormDataValue::IsDouble() const
{
    return this->m_type == kFormDataType_Double;
}

double 
FormDataValue::ToDouble() const
{
    return std::get<double>(m_body);
}

FormDataHeader& 
FormDataValue::Header()
{
    return this->m_header;
}

std::string 
FormDataValue::Name() const
{
    return this->m_name;
}

void 
FormDataValue::SetName(const std::string& name)
{
    this->m_name = name;
}

bool
HttpFormDataHelper::IsFormData(HttpRequest::ptr request)
{
    std::string contentType = request->Header()->HeaderPairs().GetValue("Content-Type");
    int pos = contentType.find("multipart/form-data");
    if(pos != std::string::npos) return true;
    return false;
}

bool 
HttpFormDataHelper::ParseFormData(HttpRequest::ptr request, std::vector<FormDataValue>& values)
{
    std::string contentType = request->Header()->HeaderPairs().GetValue("Content-Type");
    int begin = contentType.find("boundary=");
    if(begin != std::string::npos)
    {
        std::string boundary = contentType.substr(begin + 9);
        std::string_view body = request->GetContent();
        std::vector<std::string_view> parts = utils::StringSplitter::SpiltWithStr(body, "--" + boundary + "\r\n");
        size_t len = parts[parts.size() - 1].find("--" + boundary + "--");
        parts[parts.size() - 1] = parts[parts.size() - 1].substr(0, len);
        for(auto &part : parts)
        {
            FormDataValue temp;
            size_t eLength = 0;
            std::string_view rowstr;
            //解析头
            do
            {
                //取出一行
                size_t now = part.find("\r\n", eLength);
                rowstr = part.substr(eLength, now - eLength);
                eLength = (now + 2);
                if(rowstr.empty()) break;
                //分离key和value
                size_t des = rowstr.find_first_of(": ");
                std::string_view key = rowstr.substr(0, des);
                std::string_view value = rowstr.substr(des + 2);
                temp.Header().AddHeaderPair(std::string(key.data(), key.length()), std::string(value.data(), value.length()));
            } while (eLength < part.length());
            //解析体
            std::string disposition = temp.Header().GetValue("Content-Disposition");
            if(!disposition.empty())
            {
                size_t namePosBeg = disposition.find("name=");
                size_t namePosEnd = disposition.find('"', namePosBeg + 6);
                temp.SetName(disposition.substr(namePosBeg + 6, namePosEnd - namePosBeg - 6 ));
                size_t pos = disposition.find("filename=");
                //file
                if(pos != std::string::npos)
                {
                    std::string filename = disposition.substr(pos + 10, disposition.length() - pos - 11);
                    std::string_view body = part.substr(eLength, part.length() - eLength - 2);
                    std::string tfile = std::string(filename.data(), filename.length());
                    FormFile file;
                    file.m_fileName = std::move(tfile);
                    file.m_body = std::move(body);
                    temp.SetValue(file);
                }
                else
                {
                    std::string_view x = part.substr(eLength, part.length() - eLength - 2);
                    std::string body = std::string(x.data(), x.length());
                    std::regex numpattern("^[-+]?\\d+$");
                    if(std::regex_match(body, numpattern))
                    {
                        temp.SetValue(std::stoi(body));
                    }
                    else
                    {
                        std::regex doublepattern("^[-+]?\\d+\\.\\d+$");
                        if(std::regex_match(body, doublepattern))
                        {
                            temp.SetValue(std::stod(body));
                        }
                        else
                        {
                            temp.SetValue(body);
                        }
                    }
                }
                values.push_back(temp);
            }
                
        }

    }
    else
    {
        return false;
    }
    return true;
}


void 
HttpFormDataHelper::FormDataToString(HttpRequest::ptr request, const std::string& boundary, const std::vector<FormDataValue>& values)
{
    std::ostringstream stream;
    for(int i = 0 ; i < values.size() ; ++ i)
    {
        stream << "--" << boundary << "\r\n";
        if( values[i].IsFile() )
        {
            auto file = values[i].ToFile();
            stream << "Content-Disposition: form-data; name=\"" << values[i].Name() << "\"; filename=\"" << file.m_fileName << "\"\r\n";
            stream << "Content-Type: application/octet-stream\r\n\r\n";
            stream << file.m_body << "\r\n";
        }
        else if( values[i].IsNumber() )
        {
            stream << "Content-Disposition: form-data; name=\"" << values[i].Name() << "\"\r\n\r\n";
            stream << std::to_string(values[i].ToNumber()) << "\r\n";
        }
        else if( values[i].IsDouble() )
        {
            stream << "Content-Disposition: form-data; name=\"" << values[i].Name() << "\"\r\n\r\n";
            std::string num = std::to_string(values[i].ToDouble());
            stream << num << "\r\n";
        }
        else if( values[i].IsString() )
        {
            stream << "Content-Disposition: form-data; name=\"" << values[i].Name() << "\"\r\n\r\n";
            stream << values[i].ToString() << "\r\n";
        }
    }
    stream << "--" << boundary << "--\r\n\r\n";
    request->SetContent(stream.str());
    if(request->Header()->HeaderPairs().HasKey("Content-Type"))
    {
        request->Header()->HeaderPairs().SetHeaderPair("Content-Type", "multipart/form-data; boundary=" + boundary);
    }
    else
    {
        request->Header()->HeaderPairs().AddHeaderPair("Content-Type", "multipart/form-data; boundary=" + boundary);
    }
}
    
}