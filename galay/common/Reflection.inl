#ifndef GALAY_REFLECION_INL
#define GALAY_REFLECION_INL


template <typename... Targs>
galay::common::RequestFactory<Targs...>* galay::common::RequestFactory<Targs...>::m_ReqFactory = nullptr;

template <typename... Targs>
galay::common::RequestFactory<Targs...>* 
galay::common::RequestFactory<Targs...>::GetInstance()
{
    if (m_ReqFactory == nullptr)
    {
        m_ReqFactory =  new RequestFactory;
        FactoryManager::AddReleaseFunc([](){
            if(m_ReqFactory){
                delete m_ReqFactory;
                m_ReqFactory = nullptr;
            }
        });
    }
    return m_ReqFactory;
}

template <typename... Targs>
bool 
galay::common::RequestFactory<Targs...>::Regist(const std::string &typeName, std::function<std::shared_ptr<galay::protocol::Request>(Targs &&...args)> func)
{
    if (nullptr == func)
        return false;
    m_mapCreateFunction.insert(std::make_pair(typeName, func));
    return true;
}

template <typename... Targs>
std::shared_ptr<galay::protocol::Request>
galay::common::RequestFactory<Targs...>::Create(const std::string &typeName, Targs &&...args)
{
    if (m_mapCreateFunction.contains(typeName))
    {
        return m_mapCreateFunction[typeName](std::forward<Targs>(args)...);
    }
    return nullptr;
}


//response factory
template <typename... Targs>
galay::common::ResponseFactory<Targs...> *galay::common::ResponseFactory<Targs...>::m_RespFactory = nullptr;


template <typename... Targs>
galay::common::ResponseFactory<Targs...>*
galay::common::ResponseFactory<Targs...>::GetInstance()
{
    if (m_RespFactory == nullptr)
    {
        m_RespFactory = new ResponseFactory ;
        FactoryManager::AddReleaseFunc([](){
            if(m_RespFactory){
                delete m_RespFactory;
                m_RespFactory = nullptr;
            }
        });
    }
    return m_RespFactory;
}

template <typename... Targs>
bool 
galay::common::ResponseFactory<Targs...>::Regist(const std::string &typeName, std::function<std::shared_ptr<galay::protocol::Response>(Targs &&...args)> func)
{
    if (nullptr == func)
        return false;
    m_mapCreateFunction.insert(std::make_pair(typeName, func));
    return true;
}

template <typename... Targs>
std::shared_ptr<galay::protocol::Response>
galay::common::ResponseFactory<Targs...>::Create(const std::string &typeName, Targs &&...args)
{
    if (m_mapCreateFunction.contains(typeName))
    {
        return m_mapCreateFunction[typeName](std::forward<Targs>(args)...);
    }
    return nullptr;
}


//user factory
template <typename... Targs>
galay::common::UserFactory<Targs...> *galay::common::UserFactory<Targs...>::m_userFactory = nullptr;


template <typename... Targs>
galay::common::UserFactory<Targs...>*
galay::common::UserFactory<Targs...>::GetInstance()
{
    if (m_userFactory == nullptr)
    {
        m_userFactory = new UserFactory;
        FactoryManager::AddReleaseFunc([](){
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
galay::common::UserFactory<Targs...>::Regist(const std::string &typeName, std::function<std::shared_ptr<galay::Base>(Targs &&...args)> func)
{
    if (nullptr == func)
        return false;
    m_mapCreateFunction.insert(std::make_pair(typeName, func));
    return true;
}

template <typename... Targs>
std::shared_ptr<galay::Base>
galay::common::UserFactory<Targs...>::Create(const std::string &typeName, Targs &&...args)
{
    if (m_mapCreateFunction.contains(typeName))
    {
        return m_mapCreateFunction[typeName](std::forward<Targs>(args)...);
    }
    return nullptr;
}

//DynamicCreator

template <typename BaseClass, typename T, typename... Targs>
typename galay::common::Register<BaseClass,T, Targs...> galay::common::DynamicCreator<BaseClass, T, Targs...>::m_register;

template <typename BaseClass, typename T, typename... Targs>
typename std::string galay::common::DynamicCreator<BaseClass, T, Targs...>::m_typeName;

template <typename BaseClass, typename T, typename... Targs>
galay::common::DynamicCreator<BaseClass, T, Targs...>::DynamicCreator()
{
    m_register.do_nothing();
}

template <typename BaseClass, typename T, typename... Targs>
std::shared_ptr<T> 
galay::common::DynamicCreator<BaseClass, T, Targs...>::CreateObject(Targs &&...args)
{
    return std::make_shared<T>(std::forward<Targs>(args)...);
}

template <typename BaseClass, typename T, typename... Targs>
const std::string 
galay::common::DynamicCreator<BaseClass, T, Targs...>::GetTypeName()
{
    return m_typeName;
}

template <typename BaseClass, typename T, typename... Targs>
galay::common::DynamicCreator<BaseClass, T, Targs...>::~DynamicCreator()
{
    m_register.do_nothing();
}


// Register

 template <typename BaseClass, typename T, typename... Targs>
 galay::common::Register<BaseClass,T,Targs...>::Register()
{
    std::string typeName = utils::GetTypeName<T>();
}

template <typename T, typename... Targs>
galay::common::Register<galay::protocol::Request,T,Targs...>::Register()
{
    std::string typeName = utils::GetTypeName<T>();
    galay::common::RequestFactory<Targs...>::GetInstance()->Regist(typeName, galay::common::DynamicCreator<galay::protocol::Request, T, Targs...>::CreateObject);
}

template <typename T, typename... Targs>
galay::common::Register<galay::protocol::Response,T,Targs...>::Register()
{
    std::string typeName = utils::GetTypeName<T>();
    galay::common::ResponseFactory<Targs...>::GetInstance()->Regist(typeName, galay::common::DynamicCreator<galay::protocol::Response, T, Targs...>::CreateObject);
}

template <typename T, typename... Targs>
galay::common::Register<galay::Base,T,Targs...>::Register()
{
    std::string typeName = utils::GetTypeName<T>();
    galay::common::UserFactory<Targs...>::GetInstance()->Regist(typeName, galay::common::DynamicCreator<galay::Base, T, Targs...>::CreateObject);
}

#endif