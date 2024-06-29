#ifndef GALAY_REFLECION_INL
#define GALAY_REFLECION_INL


template <typename... Targs>
galay::common::GY_RequestFactory<Targs...>* galay::common::GY_RequestFactory<Targs...>::m_reqFactory = nullptr;

template <typename... Targs>
galay::common::GY_RequestFactory<Targs...>* 
galay::common::GY_RequestFactory<Targs...>::GetInstance()
{
    if (m_reqFactory == nullptr)
    {
        m_reqFactory =  new GY_RequestFactory;
        GY_FactoryManager::AddReleaseFunc([](){
            if(m_reqFactory){
                delete m_reqFactory;
                m_reqFactory = nullptr;
            }
        });
    }
    return m_reqFactory;
}

template <typename... Targs>
bool 
galay::common::GY_RequestFactory<Targs...>::Regist(const std::string &typeName, std::function<std::shared_ptr<galay::protocol::GY_Request>(Targs &&...args)> func)
{
    if (nullptr == func)
        return false;
    m_mapCreateFunction.insert(std::make_pair(typeName, func));
    return true;
}

template <typename... Targs>
std::shared_ptr<galay::protocol::GY_Request>
galay::common::GY_RequestFactory<Targs...>::Create(const std::string &typeName, Targs &&...args)
{
    if (m_mapCreateFunction.contains(typeName))
    {
        return m_mapCreateFunction[typeName](std::forward<Targs>(args)...);
    }
    return nullptr;
}


//response factory
template <typename... Targs>
galay::common::GY_ResponseFactory<Targs...> *galay::common::GY_ResponseFactory<Targs...>::m_respFactory = nullptr;


template <typename... Targs>
galay::common::GY_ResponseFactory<Targs...>*
galay::common::GY_ResponseFactory<Targs...>::GetInstance()
{
    if (m_respFactory == nullptr)
    {
        m_respFactory = new GY_ResponseFactory ;
        GY_FactoryManager::AddReleaseFunc([](){
            if(m_respFactory){
                delete m_respFactory;
                m_respFactory = nullptr;
            }
        });
    }
    return m_respFactory;
}

template <typename... Targs>
bool 
galay::common::GY_ResponseFactory<Targs...>::Regist(const std::string &typeName, std::function<std::shared_ptr<galay::protocol::GY_Response>(Targs &&...args)> func)
{
    if (nullptr == func)
        return false;
    m_mapCreateFunction.insert(std::make_pair(typeName, func));
    return true;
}

template <typename... Targs>
std::shared_ptr<galay::protocol::GY_Response>
galay::common::GY_ResponseFactory<Targs...>::Create(const std::string &typeName, Targs &&...args)
{
    if (m_mapCreateFunction.contains(typeName))
    {
        return m_mapCreateFunction[typeName](std::forward<Targs>(args)...);
    }
    return nullptr;
}


//user factory
template <typename... Targs>
galay::common::GY_UserFactory<Targs...> *galay::common::GY_UserFactory<Targs...>::m_userFactory = nullptr;


template <typename... Targs>
galay::common::GY_UserFactory<Targs...>*
galay::common::GY_UserFactory<Targs...>::GetInstance()
{
    if (m_userFactory == nullptr)
    {
        m_userFactory = new GY_UserFactory;
        GY_FactoryManager::AddReleaseFunc([](){
            if(m_userFactory){
                delete m_userFactory;
                m_userFactory = nullptr;
            }
        });
    }
    return m_userFactory;
}

template <typename... Targs>
bool 
galay::common::GY_UserFactory<Targs...>::Regist(const std::string &typeName, std::function<std::shared_ptr<galay::GY_Base>(Targs &&...args)> func)
{
    if (nullptr == func)
        return false;
    m_mapCreateFunction.insert(std::make_pair(typeName, func));
    return true;
}

template <typename... Targs>
std::shared_ptr<galay::GY_Base>
galay::common::GY_UserFactory<Targs...>::Create(const std::string &typeName, Targs &&...args)
{
    if (m_mapCreateFunction.contains(typeName))
    {
        return m_mapCreateFunction[typeName](std::forward<Targs>(args)...);
    }
    return nullptr;
}

//DynamicCreator

template <typename BaseClass, typename T, typename... Targs>
typename galay::common::Register<BaseClass,T, Targs...> galay::common::GY_DynamicCreator<BaseClass, T, Targs...>::m_register;

template <typename BaseClass, typename T, typename... Targs>
typename std::string galay::common::GY_DynamicCreator<BaseClass, T, Targs...>::m_typeName;

template <typename BaseClass, typename T, typename... Targs>
galay::common::GY_DynamicCreator<BaseClass, T, Targs...>::GY_DynamicCreator()
{
    m_register.do_nothing();
}

template <typename BaseClass, typename T, typename... Targs>
std::shared_ptr<T> 
galay::common::GY_DynamicCreator<BaseClass, T, Targs...>::CreateObject(Targs &&...args)
{
    return std::make_shared<T>(std::forward<Targs>(args)...);
}

template <typename BaseClass, typename T, typename... Targs>
const std::string 
galay::common::GY_DynamicCreator<BaseClass, T, Targs...>::GetTypeName()
{
    return m_typeName;
}

template <typename BaseClass, typename T, typename... Targs>
galay::common::GY_DynamicCreator<BaseClass, T, Targs...>::~GY_DynamicCreator()
{
    m_register.do_nothing();
}


// Register

 template <typename BaseClass, typename T, typename... Targs>
 galay::common::Register<BaseClass,T,Targs...>::Register()
{
    std::string typeName = util::GetTypeName<T>();
    galay::common::GY_RequestFactory<Targs...>::GetInstance()->Regist(typeName, galay::common::GY_DynamicCreator<BaseClass, T, Targs...>::CreateObject);
}

template <typename T, typename... Targs>
galay::common::Register<galay::protocol::GY_Request,T,Targs...>::Register()
{
    std::string typeName = util::GetTypeName<T>();
    galay::common::GY_RequestFactory<Targs...>::GetInstance()->Regist(typeName, galay::common::GY_DynamicCreator<galay::protocol::GY_Request, T, Targs...>::CreateObject);
}

template <typename T, typename... Targs>
galay::common::Register<galay::protocol::GY_Response,T,Targs...>::Register()
{
    std::string typeName = util::GetTypeName<T>();
    galay::common::GY_RequestFactory<Targs...>::GetInstance()->Regist(typeName, galay::common::GY_DynamicCreator<galay::protocol::GY_Response, T, Targs...>::CreateObject);
}

template <typename T, typename... Targs>
galay::common::Register<galay::GY_Base,T,Targs...>::Register()
{
    std::string typeName = util::GetTypeName<T>();
    galay::common::GY_UserFactory<Targs...>::GetInstance()->Regist(typeName, galay::common::GY_DynamicCreator<galay::GY_Base, T, Targs...>::CreateObject);
}

#endif