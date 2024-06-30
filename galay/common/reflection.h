#ifndef GALAY_REFLECTION_H
#define GALAY_REFLECTION_H

#include <functional>
#include <unordered_map>
#include <list>
#include "../protocol/basic_protocol.h"
#include "../util/typename.h"

namespace galay
{

    class GY_Base
    {
    public:
        virtual ~GY_Base() = default;
    };

    namespace protocol{
        class GY_Request;
        class GY_Response;
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
        class GY_RequestFactory
        {
        public:
            using ptr = std::shared_ptr<GY_RequestFactory>;
            using uptr = std::unique_ptr<GY_RequestFactory>;
            using wptr = std::weak_ptr<GY_RequestFactory>;
            static GY_RequestFactory<Targs...> *GetInstance();
            bool Regist(const std::string &typeName, std::function<std::shared_ptr<galay::protocol::GY_Request>(Targs &&...args)> func);
            std::shared_ptr<galay::protocol::GY_Request> Create(const std::string &typeName, Targs &&...args);

            virtual ~GY_RequestFactory() = default;
        private:
            GY_RequestFactory() = default;

        private:
            static GY_RequestFactory* m_reqFactory;
            std::unordered_map<std::string, std::function<std::shared_ptr<galay::protocol::GY_Request>(Targs &&...)>> m_mapCreateFunction;
        };

        template <typename... Targs>
        class GY_ResponseFactory
        {
        public:
            using ptr = std::shared_ptr<GY_ResponseFactory>;
            using uptr = std::unique_ptr<GY_ResponseFactory>;
            using wptr = std::weak_ptr<GY_ResponseFactory>;
            static GY_ResponseFactory<Targs...> *GetInstance();
            bool Regist(const std::string &typeName, std::function<std::shared_ptr<galay::protocol::GY_Response>(Targs &&...args)> func);
            std::shared_ptr<galay::protocol::GY_Response> Create(const std::string &typeName, Targs &&...args);
            virtual ~GY_ResponseFactory() = default;
        private:
            GY_ResponseFactory() = default;

        private:
            static GY_ResponseFactory* m_respFactory;
            std::unordered_map<std::string, std::function<std::shared_ptr<galay::protocol::GY_Response>(Targs &&...)>> m_mapCreateFunction;
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
            GY_UserFactory() = default;
            

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
        class Register<protocol::GY_Request,T,Targs...>
        {
        public:
            explicit Register();
            inline void do_nothing() const {};
        };

        template <typename T, typename... Targs>
        class Register<protocol::GY_Response,T,Targs...>
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