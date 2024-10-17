#ifndef __GALAY_THREAD_SAFE_H__
#define __GALAY_THREAD_SAFE_H__

#include <atomic>
#include <mutex>
#include <shared_mutex>

namespace galay::thread::safe 
{
    
template<typename T>
struct ListNode
{
    ListNode() 
        :m_prev(nullptr), m_next(nullptr) {}
    ListNode(const T& data)
        :m_prev(nullptr), m_next(nullptr), m_data(data)  {}
    ListNode(T&& data)
        :m_prev(nullptr), m_next(nullptr), m_data(std::forward<T>(data)) {}
    ListNode* m_prev;
    ListNode* m_next;
    T m_data;
};

template<typename T>
class List
{
    static_assert(std::is_default_constructible_v<T>, "T ust be default constructible");
public:
    List()
    {
        m_head = new ListNode<T>();
        m_tail = new ListNode<T>();
        m_head->m_next = m_tail;
        m_tail->m_prev = m_head;
        m_size = 0;
    }

    ListNode<T>* PushFront(const T& data)
    {
        std::unique_lock lock(this->m_mtx);
        ListNode<T>* node = new ListNode<T>(data);
        node->m_next = m_head->m_next;
        node->m_prev = m_head;
        m_head->m_next->m_prev = node;
        m_head->m_next = node;
        m_size.fetch_add(1);
        return node;
    }

    ListNode<T>* PushFront(T&& data)
    {
        std::unique_lock lock(this->m_mtx);
        ListNode<T>* node = new ListNode<T>(std::forward<T>(data));
        node->m_next = m_head->m_next;
        node->m_prev = m_head;
        m_head->m_next->m_prev = node;
        m_head->m_next = node;
        m_size.fetch_add(1);
        return node;
    }

    ListNode<T>* PushBack(const T& data)
    {
        std::unique_lock lock(this->m_mtx);
        ListNode<T>* node = new ListNode<T>(data);
        node->m_next = m_tail;
        node->m_prev = m_tail->m_prev;
        m_tail->m_prev->m_next = node;
        m_tail->m_prev = node;
        m_size.fetch_add(1);
        return node;
    }

    ListNode<T>* PushBack(T&& data)
    {
        std::unique_lock lock(this->m_mtx);
        ListNode<T>* node = new ListNode<T>(std::forward<T>(data));
        node->m_next = m_tail;
        node->m_prev = m_tail->m_prev;
        m_tail->m_prev->m_next = node;
        m_tail->m_prev = node;
        m_size.fetch_add(1);
        return node;
    }

    ListNode<T>* PopFront()
    {
        std::unique_lock lock(this->m_mtx);
        if( m_size.load() == 0 ) return nullptr;
        ListNode<T>* node = m_head->m_next;
        node->m_next->m_prev = m_head;
        m_head->m_next = node->m_next;
        m_size.fetch_sub(1);
        return node;
    }

    ListNode<T>* PopBack()
    {
        std::unique_lock lock(this->m_mtx);
        if( m_size.load() == 0 ) return nullptr;
        ListNode<T>* node = m_tail->m_prev;
        node->m_prev->m_next = m_tail;
        m_tail->m_prev = node->m_prev;
        m_size.fetch_sub(1);
        return node;
    }
    

    bool Remove(ListNode<T>* node)
    {
        std::unique_lock lock(this->m_mtx);
        if( node == nullptr ) return false;
        node->m_prev->m_next = node->m_next;
        node->m_next->m_prev = node->m_prev;
        m_size.fetch_sub(1);
        lock.unlock();
        delete node;
        return true;
    }

    bool Empty()
    {
        return m_size.load() == 0;
    }
    
    uint64_t Size()
    {
        return m_size.load();
    }

    ~List()
    {
        ListNode<T>* node = m_head;
        while(node)
        {
            ListNode<T>* temp = node;
            node = node->m_next;
            delete temp;
        }
    }
private:
    std::atomic_uint64_t m_size;
    ListNode<T>* m_head;
    ListNode<T>* m_tail;
    std::shared_mutex m_mtx;
};

}

#endif
