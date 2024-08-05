#ifndef GALAY_REFLECTION_H
#define GALAY_REFLECTION_H

#include <functional>
#include <unordered_map>
#include <list>
#include <memory>
#include "../util/typename.h"

namespace galay
{

    class GY_Base
    {
    public:
        virtual ~GY_Base() = default;
    };

    namespace protocol{
        class GY_SRequest;
        class GY_SResponse;
        class GY_CRequest;
        class GY_CResponse;
    }

    namespace common
    {
        
        class GY_FactoryManager
        {
        public:
            using uptr = std::unique_ptr<GY_FactoryManager>;
            static void AddReleaseFunc(std::function<void()> func);
            virtual ~GY_FactoryManager();
        private:
            static GY_FactoryManager::uptr m_FactoryManager;
            std::list<std::function<void()>> m_ReleaseFunc;
        };

        //必须满足构造函数参数相同且都继承自同一基类
        template <typename... Targs>
        class GY_SRequestFactory
        {
        public:
            using ptr = std::shared_ptr<GY_SRequestFactory>;
            using uptr = std::unique_ptr<GY_SRequestFactory>;
            using wptr = std::weak_ptr<GY_SRequestFactory>;
            static GY_SRequestFactory<Targs...> *GetInstance();
            bool Regist(const std::string &typeName, std::function<std::shared_ptr<galay::protocol::GY_SRequest>(Targs &&...args)> func);
            std::shared_ptr<galay::protocol::GY_SRequest> Create(const std::string &typeName, Targs &&...args);
            virtual ~GY_SRequestFactory() = default;
        private:
            static GY_SRequestFactory* m_sReqFactory;
            std::unordered_map<std::string, std::function<std::shared_ptr<galay::protocol::GY_SRequest>(Targs &&...)>> m_mapCreateFunction;
        };

        template <typename... Targs>
        class GY_SResponseFactory
        {
        public:
            using ptr = std::shared_ptr<GY_SResponseFactory>;
            using uptr = std::unique_ptr<GY_SResponseFactory>;
            using wptr = std::weak_ptr<GY_SResponseFactory>;
            static GY_SResponseFactory<Targs...> *GetInstance();
            bool Regist(const std::string &typeName, std::function<std::shared_ptr<galay::protocol::GY_SResponse>(Targs &&...args)> func);
            std::shared_ptr<galay::protocol::GY_SResponse> Create(const std::string &typeName, Targs &&...args);
            virtual ~GY_SResponseFactory() = default;
        private:
            static GY_SResponseFactory* m_sRespFactory;
            std::unordered_map<std::string, std::function<std::shared_ptr<galay::protocol::GY_SResponse>(Targs &&...)>> m_mapCreateFunction;
        };

        template <typename... Targs>
        class GY_CRequestFactory
        {
        public:
            using ptr = std::shared_ptr<GY_CRequestFactory>;
            using uptr = std::unique_ptr<GY_CRequestFactory>;
            using wptr = std::weak_ptr<GY_CRequestFactory>;
            static GY_CRequestFactory<Targs...> *GetInstance();
            bool Regist(const std::string &typeName, std::function<std::shared_ptr<galay::protocol::GY_CRequest>(Targs &&...args)> func);
            std::shared_ptr<galay::protocol::GY_CRequest> Create(const std::string &typeName, Targs &&...args);
            virtual ~GY_CRequestFactory() = default;
        private:
            static GY_CRequestFactory* m_cReqFactory;
            std::unordered_map<std::string, std::function<std::shared_ptr<galay::protocol::GY_CRequest>(Targs &&...)>> m_mapCreateFunction;
        };

        template <typename... Targs>
        class GY_CResponseFactory
        {
        public:
            using ptr = std::shared_ptr<GY_CResponseFactory>;
            using uptr = std::unique_ptr<GY_CResponseFactory>;
            using wptr = std::weak_ptr<GY_CResponseFactory>;
            static GY_CResponseFactory<Targs...> *GetInstance();
            bool Regist(const std::string &typeName, std::function<std::shared_ptr<galay::protocol::GY_CResponse>(Targs &&...args)> func);
            std::shared_ptr<galay::protocol::GY_CResponse> Create(const std::string &typeName, Targs &&...args);
            virtual ~GY_CResponseFactory() = default;
        private:
            static GY_CResponseFactory* m_cRespFactory;
            std::unordered_map<std::string, std::function<std::shared_ptr<galay::protocol::GY_CResponse>(Targs &&...)>> m_mapCreateFunction;
        };

        template <typename... Targs>
        class GY_UserFactory
        {
        public:
            using ptr = std::shared_ptr<GY_UserFactory>;
            using uptr = std::unique_ptr<GY_UserFactory>;
            using wptr = std::weak_ptr<GY_UserFactory>;
            static GY_UserFactory<Targs...> *GetInstance();
            bool Regist(const std::string &typeName, std::function<std::shared_ptr<GY_Base>(Targs &&...args)> func);
            std::shared_ptr<GY_Base> Create(const std::string &typeName, Targs &&...args);
            virtual ~GY_UserFactory() = default;
        private:
            static GY_UserFactory *m_userFactory;
            std::unordered_map<std::string, std::function<std::shared_ptr<GY_Base>(Targs &&...)>> m_mapCreateFunction;
        };

        template <typename BaseClass, typename T, typename... Targs>
        class Register;
        

        template <typename BaseClass, typename T, typename... Targs>
        class GY_DynamicCreator
        {
        public:
            GY_DynamicCreator();
            static std::shared_ptr<T> CreateObject(Targs &&...args);
            static const std::string GetTypeName();
            virtual ~GY_DynamicCreator();

        private:
            static Register<BaseClass, T, Targs...> m_register;
            static std::string m_typeName;
        };


        template <typename BaseClass, typename T, typename... Targs>
        class Register
        {
        public:
            explicit Register();
            inline void do_nothing() const {};
        };

        //偏特化
        template <typename T, typename... Targs>
        class Register<protocol::GY_SRequest,T,Targs...>
        {
        public:
            explicit Register();
            inline void do_nothing() const {};
        };

        template <typename T, typename... Targs>
        class Register<protocol::GY_SResponse,T,Targs...>
        {
        public:
            explicit Register();
            inline void do_nothing() const {};
        };

        template <typename T, typename... Targs>
        class Register<protocol::GY_CRequest,T,Targs...>
        {
        public:
            explicit Register();
            inline void do_nothing() const {};
        };

        template <typename T, typename... Targs>
        class Register<protocol::GY_CResponse,T,Targs...>
        {
        public:
            explicit Register();
            inline void do_nothing() const {};
        };
        
        
        template <typename T, typename... Targs>
        class Register<GY_Base,T,Targs...>
        {
        public:
            explicit Register();
            inline void do_nothing() const {};
        };


        
        #include "reflection.inl"
    }

}

#endif