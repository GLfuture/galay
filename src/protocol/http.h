#ifndef GALAY_HTTP_H
#define GALAY_HTTP_H

#include "tcp.h"
#include <vector>
#include <unordered_map>

namespace galay
{
    class Http_Protocol
    {
    public:
        using Ptr = std::shared_ptr<Http_Protocol>;
        std::string& get_version()
        {
            return this->m_version;
        }   

        std::string& get_body()
        {
            return this->m_body;
        }

    protected:



    protected:
        std::string m_version;                                      // 版本号
        std::unordered_map<std::string,std::string> m_filed_list;   // 字段
        std::string m_body = "";                                    // body
    };
    


    class Http_Request:public Http_Protocol,public Request_Base
    {
    public:
        using ptr = std::shared_ptr<Http_Request>;

        std::string get_http_value(const std::string& key)
        {
            auto it = this->m_arg_list.find(key);
            if(it == this->m_arg_list.end()) return "";
            return it->second;
        }
        
        std::string get_type()
        {
            return this->m_type;
         }

        std::string get_url()
        {
            return this->m_url;
        }

        //for server
        int decode(const std::string& buffer,int &state) override
        {

        }

        //for  client
        std::string encode()
        {

        }

    protected:
        std::string m_type;                                         // http协议类型
        std::string m_url;                                          // url
        std::unordered_map<std::string,std::string> m_arg_list;     // 参数
    };

    class Http_Response:public Http_Protocol,public Response_Base
    {
    public:
        using ptr = std::shared_ptr<Http_Response>;

        void set_http_pair(std::pair<std::string,std::string>&& http_apir)
        {
            this->m_filed_list.emplace(http_apir);
        }

        void set_http_pair(std::string&& key,std::string&& value)
        {
            this->m_filed_list.emplace(std::make_pair(key,value));
        }

        uint16_t& get_status()
        {
            return this->m_status;
        }

        //for server
        std::string encode()
        {
            
        }

        //for client
        int decode(const std::string &buffer,int &state)
        {

        }

    protected:
        uint16_t m_status;                                          // 状态
    };

}

#endif