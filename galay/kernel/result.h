#ifndef GALAY_RESULT_H
#define GALAY_RESULT_H

#include <memory>
#include <any>
#include "../protocol/http.h"
#include "../protocol/smtp.h"

namespace galay{
    namespace common{
        class GY_TcpClient;
        class GY_NetCoroutinePool;
        class GroupAwaiter;
        class WaitGroup;
    }

    namespace kernel{
        class GY_HttpAsyncClient; 
        class GY_SmtpAsyncClient;
        class GY_TcpClient;
        class GY_Connector;

        class TcpResult
        {
            friend class kernel::GY_TcpClient;
            friend class kernel::GY_HttpAsyncClient;
            friend class kernel::GY_Connector;
        public:
            using ptr = std::shared_ptr<TcpResult>;
            using wptr = std::weak_ptr<TcpResult>;
            TcpResult();
            bool Success();
            std::string Error();
            void AddTaskNum(uint16_t taskNum);
            common::GroupAwaiter& Wait();
            void Done();

        protected:
            bool m_success;
            std::string m_errMsg;
            std::any m_resp;    //一次请求(std::string) ,多次请求(std::queue<std::string>)
            std::shared_ptr<common::WaitGroup> m_waitGroup;
        };

        class HttpResult: public TcpResult
        {
            friend class kernel::GY_HttpAsyncClient;
        public:
            using ptr = std::shared_ptr<HttpResult>;
            using wptr = std::weak_ptr<HttpResult>;
            HttpResult();
            protocol::http::HttpResponse GetResponse();
        };

        class SmtpResult: public TcpResult
        {
            friend class kernel::GY_SmtpAsyncClient;
        public:
            using ptr = std::shared_ptr<SmtpResult>;
            using wptr = std::weak_ptr<SmtpResult>;
            SmtpResult();
            std::queue<protocol::smtp::SmtpResponse> GetResponse();
        private:
            
        };
    }
}


#endif