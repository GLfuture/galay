#ifndef GALAY_REFLECTION_H
#define GALAY_REFLECTION_H

#include <functional>
#include <unordered_map>
#include <list>
#include <memory>
#include "galay/utils/TypeName.h"

namespace galay
{
class Request;
class Response;

class Base
{
public:
    virtual ~Base() = default;
};



namespace common
{
    
    class FactoryManager
    {
    public:
        using uptr = std::unique_ptr<FactoryManager>;
        static void AddReleaseFunc(std::function<void()> func);
        virtual ~FactoryManager();
    private:
        static FactoryManager::uptr m_FactoryManager;
        std::list<std::function<void()>> m_ReleaseFunc;
    };

    //必须满足构造函数参数相同且都继承自同一基类
    template <typename... Targs>
    class RequestFactory
    {
    public:
        using ptr = std::shared_ptr<RequestFactory>;
        using uptr = std::unique_ptr<RequestFactory>;
        using wptr = std::weak_ptr<RequestFactory>;
        static RequestFactory<Targs...> *GetInstance();
        bool Regist(const std::string &typeName, std::function<std::shared_ptr<galay::Request>(Targs &&...args)> func);
        std::shared_ptr<galay::Request> Create(const std::string &typeName, Targs &&...args);
        virtual ~RequestFactory() = default;
    private:
        static RequestFactory* m_ReqFactory;
        std::unordered_map<std::string, std::function<std::shared_ptr<galay::Request>(Targs &&...)>> m_mapCreateFunction;
    };

    template <typename... Targs>
    class ResponseFactory
    {
    public:
        using ptr = std::shared_ptr<ResponseFactory>;
        using uptr = std::unique_ptr<ResponseFactory>;
        using wptr = std::weak_ptr<ResponseFactory>;
        static ResponseFactory<Targs...> *GetInstance();
        bool Regist(const std::string &typeName, std::function<std::shared_ptr<galay::Response>(Targs &&...args)> func);
        std::shared_ptr<galay::Response> Create(const std::string &typeName, Targs &&...args);
        virtual ~ResponseFactory() = default;
    private:
        static ResponseFactory* m_RespFactory;
        std::unordered_map<std::string, std::function<std::shared_ptr<galay::Response>(Targs &&...)>> m_mapCreateFunction;
    };

    template <typename... Targs>
    class UserFactory
    {
    public:
        using ptr = std::shared_ptr<UserFactory>;
        using uptr = std::unique_ptr<UserFactory>;
        using wptr = std::weak_ptr<UserFactory>;
        static UserFactory<Targs...> *GetInstance();
        bool Regist(const std::string &typeName, std::function<std::shared_ptr<Base>(Targs &&...args)> func);
        std::shared_ptr<Base> Create(const std::string &typeName, Targs &&...args);
        virtual ~UserFactory() = default;
    private:
        static UserFactory *m_userFactory;
        std::unordered_map<std::string, std::function<std::shared_ptr<Base>(Targs &&...)>> m_mapCreateFunction;
    };

    template <typename BaseClass, typename T, typename... Targs>
    class Register;
    

    template <typename BaseClass, typename T, typename... Targs>
    class DynamicCreator
    {
    public:
        DynamicCreator();
        static std::shared_ptr<T> CreateObject(Targs &&...args);
        static const std::string GetTypeName();
        virtual ~DynamicCreator();

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
    class Register<Request,T,Targs...>
    {
    public:
        explicit Register();
        inline void do_nothing() const {};
    };

    template <typename T, typename... Targs>
    class Register<Response,T,Targs...>
    {
    public:
        explicit Register();
        inline void do_nothing() const {};
    };
    
    
    template <typename T, typename... Targs>
    class Register<Base,T,Targs...>
    {
    public:
        explicit Register();
        inline void do_nothing() const {};
    };


    
    #include "Reflection.inl"
}

}

#endif