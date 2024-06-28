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

    template <typename... TArgs>
    class GY_RequestFactory
    {
    public:
        using ptr = std::shared_ptr<GY_RequestFactory>;
        using uptr = std::unique_ptr<GY_RequestFactory>;
        using wptr = std::weak_ptr<GY_RequestFactory>;
        static GY_RequestFactory<TArgs...>* Instance()
        {
            if (m_reqFactory == nullptr)
            {
                m_reqFactory = new GY_RequestFactory;
            }
            return m_reqFactory;
        }

        bool Regist(const std::string &typeName, std::function<std::shared_ptr<galay::protocol::GY_Request>(TArgs &&...args)> func)
        {
            if (nullptr == func)
                return false;
            m_mapCreateFunction.insert(std::make_pair(typeName, func));
            return true;
        }

        std::shared_ptr<galay::protocol::GY_Request> Create(const std::string &typeName, TArgs &&...args)
        {
            if (m_mapCreateFunction.contains(typeName))
            {
                return m_mapCreateFunction[typeName](std::forward<TArgs>(args)...);
            }
            return nullptr;
        }

        virtual ~GY_RequestFactory() = default;

    private:
        GY_RequestFactory() = default;

    private:
        static GY_RequestFactory<TArgs...>* m_reqFactory;
        std::unordered_map<std::string, std::function<std::shared_ptr<galay::protocol::GY_Request>(TArgs &&...)>> m_mapCreateFunction;
    };

    template <typename... TArgs>
    GY_RequestFactory<TArgs...>* GY_RequestFactory<TArgs...>::m_reqFactory = nullptr;


    template <typename... TArgs>
    class GY_ResponseFactory
    {
    public:
        using ptr = std::shared_ptr<GY_ResponseFactory>;
        using uptr = std::unique_ptr<GY_ResponseFactory>;
        using wptr = std::weak_ptr<GY_ResponseFactory>;
        static GY_ResponseFactory* Instance()
        {
            if (m_respFactory == nullptr)
            {
                m_respFactory = new GY_ResponseFactory();
            }
            return m_respFactory;
        }

        bool Regist(const std::string &typeName, std::function<std::shared_ptr<galay::protocol::GY_Response>(TArgs &&...args)> func)
        {
            if (nullptr == func)
                return false;
            m_mapCreateFunction.insert(std::make_pair(typeName, func));
            return true;
        }

        std::shared_ptr<galay::protocol::GY_Response> Create(const std::string &typeName, TArgs &&...args)
        {
            if (m_mapCreateFunction.contains(typeName))
            {
                return m_mapCreateFunction[typeName](std::forward<TArgs>(args)...);
            }
            return nullptr;
        }

        virtual ~GY_ResponseFactory() = default;

    private:
        GY_ResponseFactory() = default;

    private:
        static GY_ResponseFactory<TArgs...>* m_respFactory;
        std::unordered_map<std::string, std::function<std::shared_ptr<galay::protocol::GY_Response>(TArgs &&...)>> m_mapCreateFunction;
    };

    template <typename... TArgs>
    GY_ResponseFactory<TArgs...>* GY_ResponseFactory<TArgs...>::m_respFactory = nullptr;


    template<FactoryType type, typename T,typename... TArgs>
    class GY_DynamicCreator
    {
    public:
        struct Register
        {
            Register()
            {
                char* szDemangleName = nullptr;
    #ifdef __GNUC__
                szDemangleName = abi::__cxa_demangle(typeid(T).name(), nullptr, nullptr, nullptr);
    #else
                szDemangleName = abi::__cxa_demangle(typeid(T).name(), nullptr, nullptr, nullptr);
    #endif
                if (nullptr != szDemangleName)
                {
                    m_typeName = szDemangleName;
                    free(szDemangleName);
                }
                switch (type)
                {
                case kRequestFactory:
                    GY_RequestFactory<TArgs...>::Instance()->Regist(m_typeName, CreateObject);
                    break;
                case kResponseFactory:
                    GY_ResponseFactory<TArgs...>::Instance()->Regist(m_typeName, CreateObject);
                    break;
                default:
                    break;
                }
            }
            inline void do_nothing()const { };
        };
        
        GY_DynamicCreator()
        {
            m_register.do_nothing();
        }

        virtual ~GY_DynamicCreator()
        {
            m_register.do_nothing();
        }

        static std::shared_ptr<T> CreateObject(TArgs &&...args)
        {
            return std::make_shared<T>(std::forward<TArgs>(args)...);
        }

        static std::string GetTypeName()
        {
            return m_typeName;
        }

    private:
        static Register m_register;
        static std::string m_typeName;
    };

    template<FactoryType type, typename T, typename ...Targs>
    typename GY_DynamicCreator<type, T, Targs...>::Register GY_DynamicCreator<type, T, Targs...>::m_register;

    template<FactoryType type, typename T, typename ...Targs>
    typename std::string GY_DynamicCreator<type, T, Targs...>::m_typeName;

}

#endif