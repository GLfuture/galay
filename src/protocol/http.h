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
        std::string m_version;                                      // 版本号
        std::unordered_map<std::string,std::string> m_filed_list;   // 字段
        std::string m_body = "";                                    // body
    };
    


    class Http_Request:public Http_Protocol,public Request_Base
    {
    public:
        using ptr = std::shared_ptr<Http_Request>;
    protected:
        std::string m_type;                                         // http协议类型
        std::string m_url;                                          // url
        std::unordered_map<std::string,std::string> m_arg_list;     // 参数
    };

    class Http_Response:public Http_Protocol,public Response_Base
    {
    public:
        using ptr = std::shared_ptr<Http_Response>;
    protected:
        uint16_t m_status;                                          // 状态
    };

}

#endif