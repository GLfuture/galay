#ifndef GALAY_STRATEGY_HPP
#define GALAY_STRATEGY_HPP

#include <atomic>
#include <vector>
#include <memory>
#include <random>

namespace galay::details 
{

//轮询(线程安全)
template<typename Type>
class RoundRobinLoadBalancer 
{
public:
    using value_type = Type;
    using uptr = std::unique_ptr<RoundRobinLoadBalancer>;
    using ptr = std::shared_ptr<RoundRobinLoadBalancer>;
    RoundRobinLoadBalancer(const std::vector<Type*>& nodes)
        : m_nodes(nodes), m_index(0) {}

    Type* Select();
    size_t Size() const;
    void Append(Type* node);

private:
    std::atomic_uint32_t m_index;
    std::vector<Type*> m_nodes;
};

//加权轮询(非线程安全)，散列均匀
template<typename Type>
class WeightRoundRobinLoadBalancer
{
    struct Node {
        Type* node;
        uint32_t weight;
        int32_t currentWeight;
    };
public:
    using value_type = Type;
    using uptr = std::unique_ptr<WeightRoundRobinLoadBalancer>;
    using ptr = std::shared_ptr<WeightRoundRobinLoadBalancer>;
    WeightRoundRobinLoadBalancer(const std::vector<Type*>& nodes,const std::vector<uint32_t>& weights);
    Type* Select();
    size_t Size() const;
    void Append(Type* node, uint32_t weight);
private:
    std::vector<Node> m_nodes;
    std::atomic_uint32_t m_index;
};

//随机(非线程安全)
template<typename Type>
class RandomLoadBalancer 
{
public:
    using value_type = Type;
    using uptr = std::unique_ptr<RandomLoadBalancer>;
    using ptr = std::shared_ptr<RandomLoadBalancer>;
    RandomLoadBalancer(const std::vector<Type*>& nodes);
    Type* Select();
    size_t Size() const;
    void Append(Type* node);
private:
    std::mt19937 m_random;
    std::vector<Type*> m_nodes;
};

//加权随机(非线程安全)
template<typename Type>
class WeightedRandomLoadBalancer 
{
    struct Node {
        Type* node;
        uint32_t weight;
    };
public:
    using value_type = Type;
    using uptr = std::unique_ptr<WeightedRandomLoadBalancer>;
    using ptr = std::shared_ptr<WeightedRandomLoadBalancer>;
    WeightedRandomLoadBalancer(const std::vector<Type*>& nodes, const std::vector<uint32_t>& weights);
    
    Type* Select();
    size_t Size() const;
    void Append(Type* node, uint32_t weight);

private:
    std::mt19937 m_random;
    std::vector<Node> m_nodes;
    uint32_t m_total_weight = 0;
};

}

#include "Strategy.tcc"

#endif