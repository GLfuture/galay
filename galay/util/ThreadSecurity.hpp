#ifndef __GALAY_THREAD_SECURITY_HPP__
#define __GALAY_THREAD_SECURITY_HPP__

#include <list>
#include <unordered_map>
#include <atomic>
#include <memory>
#include <mutex>
#include <shared_mutex>

namespace galay::thread::security
{


template <typename T>
class SecurityList {
public:
    class ListNodePos
    {
        friend class SecurityList;
    public:
        ListNodePos() :m_valid(std::make_shared<bool>(false)) {}
        ListNodePos(typename std::list<T>::iterator pos) :m_valid(std::make_shared<bool>(true)), m_pos(pos) {}
        
        std::any operator*()
        {
            if( *m_valid == false ) return std::any();
            return *m_pos;
        }

        bool operator==(const ListNodePos& other) const
        {
            if ( (*m_valid) ^ (*other.m_valid)) return false;
            if ( *m_valid == false ) return true;
            if ( m_pos != other.m_pos ) return false; 
            return true;
        }
    private:
        std::shared_ptr<bool> m_valid;
        typename std::list<T>::iterator m_pos;
    };
    
    static ListNodePos npos;
    ListNodePos PushBack(T&& value)
    {
        std::unique_lock lock(m_mutex);
        m_list.emplace_back(std::forward<T>(value));
        ListNodePos pos(std::prev(m_list.end()));
        lock.unlock();
        return pos;
    }

    ListNodePos PushBack(const T& value)
    {
        std::unique_lock lock(m_mutex);
        m_list.emplace_back(value);
        ListNodePos pos(std::prev(m_list.end()));
        lock.unlock();
        return pos;
    }
    
    ListNodePos PushFront(T&& value)
    {
        std::unique_lock lock(m_mutex);
        m_list.emplace_front(std::forward<T>(value));
        ListNodePos pos(m_list.begin());
        lock.unlock();
        return pos;
    }

    ListNodePos PushFront(const T& value)
    {
        std::unique_lock lock(m_mutex);
        m_list.emplace_front(value);
        ListNodePos pos(m_list.begin());
        lock.unlock();
        return pos;
    }

    ListNodePos PopFront()
    {
        std::unique_lock lock(m_mutex);
        if(m_list.empty()) return npos;
        auto it = m_list.begin();
        ListNodePos pos(it);
        m_list.pop_front();
        return pos;
    }

    ListNodePos PopBack()
    {
        std::unique_lock lock(m_mutex);
        if(m_list.empty()) return npos;
        auto it = std::prev(m_list.end());
        ListNodePos pos(it);
        m_list.pop_back();
        return pos;
    }

    void Clear()
    {
        std::unique_lock lock(m_mutex);
        m_list.clear();
    }

    ListNodePos Front()
    {
        std::shared_lock lock(m_mutex);
        ListNodePos pos(m_list.begin());
        return pos;
    }

    ListNodePos Back()
    {
        std::shared_lock lock(m_mutex);
        ListNodePos pos(std::prev(m_list.end()));
        return pos;
    }

    ListNodePos Begin()
    {
        std::shared_lock lock(m_mutex);
        ListNodePos pos(m_list.begin());
        return pos;
    }

    ListNodePos End()
    {
        std::shared_lock lock(m_mutex);
        ListNodePos pos(m_list.end());
        return pos;
    }

    bool Empty()
    {
        std::shared_lock lock(m_mutex);
        return m_list.empty();
    }

    size_t Size()
    {
        std::shared_lock lock(m_mutex);
        return m_list.size();
    }

    bool Erase(ListNodePos pos)
    {
        std::unique_lock lock(m_mutex);
        if (!*(pos.m_valid) || pos.m_pos == m_list.end())
        {
            return false;
        }
        *(pos.m_valid) = false;
        m_list.erase(pos.m_pos);
        return  true;
    }
    
private:
    std::shared_mutex m_mutex;
    std::list<T> m_list;
};

template <typename T>
typename SecurityList<T>::ListNodePos SecurityList<T>::npos = SecurityList<T>::ListNodePos();

template<typename Key, typename Value>
class SecurityHashMap
{
public:
    void Insert(Key&& key, Value&& value)
    {
        std::unique_lock lock(m_mutex);
        m_hansh_map.emplace(std::forward<Key>(key), std::forward<Value>(value));
        lock.unlock();
    }

    void Insert(const Key& key, const Value& value)
    {
        std::unique_lock lock(m_mutex);
        m_hansh_map.emplace(key, value);
        lock.unlock();
    }
    
    std::unordered_map<Key,Value>::iterator Find(const Key& key)
    {
        std::shared_lock lock(m_mutex);
        return m_hansh_map.find(key);
    }
    
    std::unordered_map<Key,Value>::iterator Begin()
    {
        std::shared_lock lock(m_mutex);
        return m_hansh_map.begin();
    }
    
    std::unordered_map<Key,Value>::iterator End()
    {
        std::shared_lock lock(m_mutex);
        return m_hansh_map.end();
    }

    bool Empty()
    {
        std::shared_lock lock(m_mutex);
        return m_hansh_map.empty();
    }

    bool Contains(const Key& key)
    {
        std::shared_lock lock(m_mutex);
        return m_hansh_map.contains(key);
    }

    void Erase(const Key& key)
    {
        std::unique_lock lock(m_mutex);
        m_hansh_map.erase(key);
    }

    Value& operator[](const Key& key)
    {
        std::shared_lock lock(m_mutex);
        return m_hansh_map[key];
    }
    
    
private:
    std::shared_mutex m_mutex;
    std::unordered_map<Key, Value> m_hansh_map;
};

}


#endif