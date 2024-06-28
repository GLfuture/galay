#ifndef GALAY_REFLECION_INL
#define GALAY_REFLECION_INL

template <typename... Targs>
galay::GY_RequestFactory<Targs...>* GY_RequestFactory<Targs...>::m_reqFactory = nullptr;

template <typename... Targs>
galay::GY_RequestFactory<Targs...>* 
galay::GY_RequestFactory<Targs...>::GetInstance()
{
    if (m_reqFactory == nullptr)
    {
        m_reqFactory = new GY_RequestFactory;
    }
    return m_reqFactory;
}

template <typename... Targs>
bool 
galay::GY_RequestFactory<Targs...>::Regist(const std::string &typeName, std::function<std::shared_ptr<galay::protocol::GY_Request>(Targs &&...args)> func)
{
    if (nullptr == func)
        return false;
    m_mapCreateFunction.insert(std::make_pair(typeName, func));
    return true;
}

template <typename... Targs>
std::shared_ptr<galay::protocol::GY_Request>
galay::GY_RequestFactory<Targs...>::Create(const std::string &typeName, Targs &&...args)
{
    if (m_mapCreateFunction.contains(typeName))
    {
        return m_mapCreateFunction[typeName](std::forward<Targs>(args)...);
    }
    return nullptr;
}



//response factory
template <typename... Targs>
galay::GY_ResponseFactory<Targs...>*
galay::GY_ResponseFactory<Targs...>::GetInstance()
{
    if (m_respFactory == nullptr)
    {
        m_respFactory = new GY_ResponseFactory();
    }
    return m_respFactory;
}

template <typename... Targs>
bool 
galay::GY_ResponseFactory<Targs...>::Regist(const std::string &typeName, std::function<std::shared_ptr<galay::protocol::GY_Response>(Targs &&...args)> func)
{
    if (nullptr == func)
        return false;
    m_mapCreateFunction.insert(std::make_pair(typeName, func));
    return true;
}

template <typename... Targs>
std::shared_ptr<galay::protocol::GY_Response>
galay::GY_ResponseFactory<Targs...>::Create(const std::string &typeName, Targs &&...args)
{
    if (m_mapCreateFunction.contains(typeName))
    {
        return m_mapCreateFunction[typeName](std::forward<Targs>(args)...);
    }
    return nullptr;
}


//DynamicCreator

template <FactoryType type, typename T, typename... Targs>
typename GY_DynamicCreator<type, T, Targs...>::Register GY_DynamicCreator<type, T, Targs...>::m_register;

template <FactoryType type, typename T, typename... Targs>
typename std::string GY_DynamicCreator<type, T, Targs...>::m_typeName;

template <FactoryType type, typename T, typename... Targs>
GY_DynamicCreator<type, T, Targs...>::Register::Register()
{
    char *szDemangleName = nullptr;
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
        GY_RequestFactory<Targs...>::GetInstance()->Regist(m_typeName, CreateObject);
        break;
    case kResponseFactory:
        GY_ResponseFactory<Targs...>::GetInstance()->Regist(m_typeName, CreateObject);
        break;
    default:
        break;
    }
}

template <FactoryType type, typename T, typename... Targs>
GY_DynamicCreator<type, T, Targs...>::GY_DynamicCreator()
{
    m_register.do_nothing();
}

template <FactoryType type, typename T, typename... Targs>
std::shared_ptr<T> 
GY_DynamicCreator<type, T, Targs...>::CreateObject(Targs &&...args)
{
    return std::make_shared<T>(std::forward<Targs>(args)...);
}

template <FactoryType type, typename T, typename... Targs>
std::string 
GY_DynamicCreator<type, T, Targs...>::GetTypeName()
{
    return m_typeName;
}

template <FactoryType type, typename T, typename... Targs>
GY_DynamicCreator<type, T, Targs...>::~GY_DynamicCreator()
{
    m_register.do_nothing();
}

#endif