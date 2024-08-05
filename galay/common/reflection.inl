#ifndef GALAY_REFLECION_INL
#define GALAY_REFLECION_INL


template <typename... Targs>
galay::common::GY_SRequestFactory<Targs...>* galay::common::GY_SRequestFactory<Targs...>::m_sReqFactory = nullptr;

template <typename... Targs>
galay::common::GY_SRequestFactory<Targs...>* 
galay::common::GY_SRequestFactory<Targs...>::GetInstance()
{
    if (m_sReqFactory == nullptr)
    {
        m_sReqFactory =  new GY_SRequestFactory;
        GY_FactoryManager::AddReleaseFunc([](){
            if(m_sReqFactory){
                delete m_sReqFactory;
                m_sReqFactory = nullptr;
            }
        });
    }
    return m_sReqFactory;
}

template <typename... Targs>
bool 
galay::common::GY_SRequestFactory<Targs...>::Regist(const std::string &typeName, std::function<std::shared_ptr<galay::protocol::GY_SRequest>(Targs &&...args)> func)
{
    if (nullptr == func)
        return false;
    m_mapCreateFunction.insert(std::make_pair(typeName, func));
    return true;
}

template <typename... Targs>
std::shared_ptr<galay::protocol::GY_SRequest>
galay::common::GY_SRequestFactory<Targs...>::Create(const std::string &typeName, Targs &&...args)
{
    if (m_mapCreateFunction.contains(typeName))
    {
        return m_mapCreateFunction[typeName](std::forward<Targs>(args)...);
    }
    return nullptr;
}


//response factory
template <typename... Targs>
galay::common::GY_SResponseFactory<Targs...> *galay::common::GY_SResponseFactory<Targs...>::m_sRespFactory = nullptr;


template <typename... Targs>
galay::common::GY_SResponseFactory<Targs...>*
galay::common::GY_SResponseFactory<Targs...>::GetInstance()
{
    if (m_sRespFactory == nullptr)
    {
        m_sRespFactory = new GY_SResponseFactory ;
        GY_FactoryManager::AddReleaseFunc([](){
            if(m_sRespFactory){
                delete m_sRespFactory;
                m_sRespFactory = nullptr;
            }
        });
    }
    return m_sRespFactory;
}

template <typename... Targs>
bool 
galay::common::GY_SResponseFactory<Targs...>::Regist(const std::string &typeName, std::function<std::shared_ptr<galay::protocol::GY_SResponse>(Targs &&...args)> func)
{
    if (nullptr == func)
        return false;
    m_mapCreateFunction.insert(std::make_pair(typeName, func));
    return true;
}

template <typename... Targs>
std::shared_ptr<galay::protocol::GY_SResponse>
galay::common::GY_SResponseFactory<Targs...>::Create(const std::string &typeName, Targs &&...args)
{
    if (m_mapCreateFunction.contains(typeName))
    {
        return m_mapCreateFunction[typeName](std::forward<Targs>(args)...);
    }
    return nullptr;
}

//crequest factory
template <typename... Targs>
galay::common::GY_CRequestFactory<Targs...>* galay::common::GY_CRequestFactory<Targs...>::m_cReqFactory = nullptr;

template <typename... Targs>
galay::common::GY_CRequestFactory<Targs...>*
galay::common::GY_CRequestFactory<Targs...>::GetInstance()
{
    if (m_cReqFactory == nullptr)
    {
        m_cReqFactory = new GY_CRequestFactory;
        GY_FactoryManager::AddReleaseFunc([](){
            if(m_cReqFactory){
                delete m_cReqFactory;
                m_cReqFactory = nullptr;
            }
        });
    }
    return m_cReqFactory;
}

template <typename... Targs>
bool 
galay::common::GY_CRequestFactory<Targs...>::Regist(const std::string &typeName, std::function<std::shared_ptr<galay::protocol::GY_CRequest>(Targs &&...args)> func)
{
    if( nullptr == func)
        return false;
    m_mapCreateFunction.insert(std::make_pair(typeName, func));
    return true;
}

template <typename... Targs>
std::shared_ptr<galay::protocol::GY_CRequest> 
galay::common::GY_CRequestFactory<Targs...>::Create(const std::string &typeName, Targs &&...args)
{
    if (m_mapCreateFunction.contains(typeName))
    {
        return m_mapCreateFunction[typeName](std::forward<Targs>(args)...);
    }
    return nullptr;
}

//cresponse factory

template <typename... Targs>
galay::common::GY_CResponseFactory<Targs...>* galay::common::GY_CResponseFactory<Targs...>::m_cRespFactory = nullptr;

template <typename... Targs>
galay::common::GY_CResponseFactory<Targs...>*
galay::common::GY_CResponseFactory<Targs...>::GetInstance()
{
    if (m_cRespFactory == nullptr)
    {
        m_cRespFactory = new GY_CResponseFactory;
        GY_FactoryManager::AddReleaseFunc([](){
            if(m_cRespFactory){
                delete m_cRespFactory;
                m_cRespFactory = nullptr;
            }
        });
    }
    return m_cRespFactory;
}

template <typename... Targs>
bool 
galay::common::GY_CResponseFactory<Targs...>::Regist(const std::string &typeName, std::function<std::shared_ptr<galay::protocol::GY_CResponse>(Targs &&...args)> func)
{
    if( nullptr == func)
        return false;
    m_mapCreateFunction.insert(std::make_pair(typeName, func));
    return true;
}

template <typename... Targs>
std::shared_ptr<galay::protocol::GY_CResponse> 
galay::common::GY_CResponseFactory<Targs...>::Create(const std::string &typeName, Targs &&...args)
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
}

template <typename T, typename... Targs>
galay::common::Register<galay::protocol::GY_SRequest,T,Targs...>::Register()
{
    std::string typeName = util::GetTypeName<T>();
    galay::common::GY_SRequestFactory<Targs...>::GetInstance()->Regist(typeName, galay::common::GY_DynamicCreator<galay::protocol::GY_SRequest, T, Targs...>::CreateObject);
}

template <typename T, typename... Targs>
galay::common::Register<galay::protocol::GY_SResponse,T,Targs...>::Register()
{
    std::string typeName = util::GetTypeName<T>();
    galay::common::GY_SResponseFactory<Targs...>::GetInstance()->Regist(typeName, galay::common::GY_DynamicCreator<galay::protocol::GY_SResponse, T, Targs...>::CreateObject);
}

template <typename T, typename... Targs>
galay::common::Register<galay::protocol::GY_CRequest,T,Targs...>::Register()
{
    std::string typeName = util::GetTypeName<T>();
    galay::common::GY_CRequestFactory<Targs...>::GetInstance()->Regist(typeName, galay::common::GY_DynamicCreator<galay::protocol::GY_CRequest, T, Targs...>::CreateObject);
}

template <typename T, typename... Targs>
galay::common::Register<galay::protocol::GY_CResponse,T,Targs...>::Register()
{
    std::string typeName = util::GetTypeName<T>();
    galay::common::GY_CResponseFactory<Targs...>::GetInstance()->Regist(typeName, galay::common::GY_DynamicCreator<galay::protocol::GY_CResponse, T, Targs...>::CreateObject);
}

template <typename T, typename... Targs>
galay::common::Register<galay::GY_Base,T,Targs...>::Register()
{
    std::string typeName = util::GetTypeName<T>();
    galay::common::GY_UserFactory<Targs...>::GetInstance()->Regist(typeName, galay::common::GY_DynamicCreator<galay::GY_Base, T, Targs...>::CreateObject);
}

#endif