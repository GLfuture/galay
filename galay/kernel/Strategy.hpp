#ifndef GALAY_STRATEGY_HPP
#define GALAY_STRATEGY_HPP

#include <atomic>
#include <vector>
#include <memory>
#include <random>

namespace galay::details 
{

//轮询(线程安全)
template<typename T>
class RoundRobinLoadBalancer 
{
public:
    using value_type = T;
    using uptr = std::unique_ptr<RoundRobinLoadBalancer>;
    using ptr = std::shared_ptr<RoundRobinLoadBalancer>;

    RoundRobinLoadBalancer(const std::vector<T*>& nodes)
        : m_nodes(nodes), m_index(0) {}

    T* Select()
    {
        uint32_t index = m_index.load(std::memory_order_relaxed);
        if( !m_index.compare_exchange_strong(index, (index + 1) % m_nodes.size())) return nullptr;
        return m_nodes[index];
    }

    size_t Size() const
    {
        return m_nodes.size();
    }

    // 非线程安全
    void Append(T* node)
    {
        m_nodes.emplace_back(std::move(node));
    }

private:
    std::atomic_uint32_t m_index;
    std::vector<T*> m_nodes;
};

//加权轮询(非线程安全)，散列均匀
template<typename T>
class WeightRoundRobinLoadBalancer
{
    struct Node {
        T* node;
        uint32_t weight;
        int32_t currentWeight;
    };
public:
    using value_type = T;
    using uptr = std::unique_ptr<WeightRoundRobinLoadBalancer>;
    using ptr = std::shared_ptr<WeightRoundRobinLoadBalancer>;
    WeightRoundRobinLoadBalancer(const std::vector<T*>& nodes,const std::vector<uint32_t>& weights)
    {
        m_index.store(0);
        m_nodes.reserve(nodes.size());
        for(size_t i = 0; i < nodes.size(); ++i) {
            m_nodes[i] = {std::move(nodes[i]), weights[i], 0};
        }
    }

    T* Select()
    {
        uint32_t index = m_index.load(std::memory_order_relaxed);
        int32_t total_weight = 0, max_weight_index = 0;
        for(int i = 0; i < m_nodes.size(); ++i) {
            m_nodes[i].currentWeight += m_nodes[i].weight;
            total_weight += m_nodes[i].weight;
            if(m_nodes[i].currentWeight > m_nodes[max_weight_index].currentWeight) {
                max_weight_index = i;
            }
        }
        m_nodes[max_weight_index].currentWeight -= total_weight;
        return m_nodes[max_weight_index].node;
    }

    size_t Size() const
    {
        return m_nodes.size();
    }

    void Append(T* node, uint32_t weight)
    {
        m_nodes.emplace_back(std::move(node), weight, 0);
    }
private:
    std::vector<Node> m_nodes;
    std::atomic_uint32_t m_index;
};

//随机(非线程安全)
template<typename T>
class RandomLoadBalancer 
{
public:
    using value_type = T;
    using uptr = std::unique_ptr<RandomLoadBalancer>;
    using ptr = std::shared_ptr<RandomLoadBalancer>;
    RandomLoadBalancer(const std::vector<T*>& nodes)
        : m_nodes(nodes) 
    {
        m_random = std::mt19937(std::random_device()());
    }

    T* Select()
    {
        return m_nodes[std::uniform_int_distribution<size_t>(0, m_nodes.size() - 1)(m_random)];
    }

    size_t Size() const
    {
        return m_nodes.size();
    }

    void Append(T* node)
    {
        m_nodes.emplace_back(std::move(node));
    }
private:
    std::mt19937 m_random;
    std::vector<T*> m_nodes;
};

//加权随机(非线程安全)
template<typename T>
class WeightedRandomLoadBalancer 
{
    struct Node {
        T* node;
        uint32_t weight;
    };
public:
    using value_type = T;
    using uptr = std::unique_ptr<WeightedRandomLoadBalancer>;
    using ptr = std::shared_ptr<WeightedRandomLoadBalancer>;
    WeightedRandomLoadBalancer(const std::vector<T*>& nodes, const std::vector<uint32_t>& weights)
    {
        m_nodes.reserve(nodes.size());
        m_random = std::mt19937(std::random_device()());
        for(size_t i = 0; i < nodes.size(); ++i) {
            m_nodes[i] = {std::move(nodes[i]), weights[i], 0};
            m_total_weight += weights[i];
        }
    }
    
    T* Select()
    {
        uint32_t random_weight = std::uniform_int_distribution<uint32_t>(0, m_total_weight)(m_random);
        for(size_t i = 0; i < m_nodes.size(); ++i) {
            random_weight -= m_nodes[i].weight;
            if(random_weight <= 0) return m_nodes[i].node;
       }
       return nullptr;
    }

    size_t Size() const
    {
        return m_nodes.size();
    }

    void Append(T* node, uint32_t weight)
    {
        m_nodes.emplace_back(std::move(node), weight);
        m_total_weight += weight;
    }

private:
    std::mt19937 m_random;
    std::vector<Node> m_nodes;
    uint32_t m_total_weight = 0;
};


}

#endif