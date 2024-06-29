#ifndef GALAY_REFLECTION_H
#define GALAY_REFLECTION_H

#include <functional>
#include <unordered_map>
#include <cxxabi.h>
#include <iostream>

namespace galay
{
    namespace protocol{
        class GY_Request;
        class GY_Response;
    }

    template <typename... Targs>
    class GY_RequestFactory
    {
    public:
        using ptr = std::shared_ptr<GY_RequestFactory>;
        using uptr = std::unique_ptr<GY_RequestFactory>;
        using wptr = std::weak_ptr<GY_RequestFactory>;
        static GY_RequestFactory<Targs...>* GetInstance();
        bool Regist(const std::string &typeName, std::function<std::shared_ptr<galay::protocol::GY_Request>(Targs &&...args)> func);
        std::shared_ptr<galay::protocol::GY_Request> Create(const std::string &typeName, Targs &&...args);
        virtual ~GY_RequestFactory() = default;
    private:
        GY_RequestFactory() = default;

    private:
        static GY_RequestFactory<Targs...>* m_reqFactory;
        std::unordered_map<std::string, std::function<std::shared_ptr<galay::protocol::GY_Request>(Targs &&...)>> m_mapCreateFunction;
    };

    

    template <typename... Targs>
    class GY_ResponseFactory
    {
    public:
        using ptr = std::shared_ptr<GY_ResponseFactory>;
        using uptr = std::unique_ptr<GY_ResponseFactory>;
        using wptr = std::weak_ptr<GY_ResponseFactory>;
        static GY_ResponseFactory<Targs...>* GetInstance();
        bool Regist(const std::string &typeName, std::function<std::shared_ptr<galay::protocol::GY_Response>(Targs &&...args)> func);
        std::shared_ptr<galay::protocol::GY_Response> Create(const std::string &typeName, Targs &&...args);
        virtual ~GY_ResponseFactory() = default;
    private:
        GY_ResponseFactory() = default;

    private:
        static GY_ResponseFactory<Targs...>* m_respFactory;
        std::unordered_map<std::string, std::function<std::shared_ptr<galay::protocol::GY_Response>(Targs &&...)>> m_mapCreateFunction;
    };

    template <typename... Targs>
    GY_ResponseFactory<Targs...>* GY_ResponseFactory<Targs...>::m_respFactory = nullptr;


    template<FactoryType type, typename T,typename... Targs>
    class GY_DynamicCreator
    {
    public:
        struct Register
        {
            Register();
            inline void do_nothing()const { };
        };
        
        GY_DynamicCreator();
        static std::shared_ptr<T> CreateObject(Targs &&...args);
        static const std::string GetTypeName();
        virtual ~GY_DynamicCreator();
    private:
        static Register m_register;
        static std::string m_typeName;
    };

    

    #include "reflection.inl"

}

#endif