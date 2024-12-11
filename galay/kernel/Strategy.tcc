#ifndef GALAY_STRATEGY_TCC
#define GALAY_STRATEGY_TCC

#include "Strategy.hpp"

namespace galay::details
{

template<typename Type>
Type* RoundRobinLoadBalancer<Type>::Select()
{
    if(m_nodes.empty()) return nullptr;
    uint32_t index = m_index.load(std::memory_order_relaxed);
    if( !m_index.compare_exchange_strong(index, (index + 1) % m_nodes.size())) return nullptr;
    return m_nodes[index];
}

template<typename Type>
size_t RoundRobinLoadBalancer<Type>::Size() const
{
    return m_nodes.size();
}

template<typename Type>
void RoundRobinLoadBalancer<Type>::Append(Type* node)
{
    m_nodes.emplace_back(std::move(node));
}

template<typename Type>
WeightRoundRobinLoadBalancer<Type>::WeightRoundRobinLoadBalancer(const std::vector<Type*>& nodes,const std::vector<uint32_t>& weights)
{
    m_index.store(0);
    m_nodes.reserve(nodes.size());
    for(size_t i = 0; i < nodes.size(); ++i) {
        m_nodes[i] = {std::move(nodes[i]), weights[i], 0};
    }
}

template<typename Type>
Type* WeightRoundRobinLoadBalancer<Type>::Select()
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

template<typename Type>
size_t WeightRoundRobinLoadBalancer<Type>::Size() const
{
    return m_nodes.size();
}

template<typename Type>
void WeightRoundRobinLoadBalancer<Type>::Append(Type* node, uint32_t weight)
{
    m_nodes.emplace_back(std::move(node), weight, 0);
}

template<typename Type>
RandomLoadBalancer<Type>::RandomLoadBalancer(const std::vector<Type*>& nodes)
    : m_nodes(nodes) 
{
    m_random = std::mt19937(std::random_device()());
}

template<typename Type>
Type* RandomLoadBalancer<Type>::Select()
{
    return m_nodes[std::uniform_int_distribution<size_t>(0, m_nodes.size() - 1)(m_random)];
}

template<typename Type>
size_t RandomLoadBalancer<Type>::Size() const
{
    return m_nodes.size();
}

template<typename Type>
void RandomLoadBalancer<Type>::Append(Type* node)
{
    m_nodes.emplace_back(std::move(node));
}

template<typename Type>
WeightedRandomLoadBalancer<Type>::WeightedRandomLoadBalancer(const std::vector<Type*>& nodes, const std::vector<uint32_t>& weights)
{
    m_nodes.reserve(nodes.size());
    m_random = std::mt19937(std::random_device()());
    for(size_t i = 0; i < nodes.size(); ++i) {
        m_nodes[i] = {std::move(nodes[i]), weights[i], 0};
        m_total_weight += weights[i];
    }
}

template<typename Type>
Type* WeightedRandomLoadBalancer<Type>::Select()
{
    uint32_t random_weight = std::uniform_int_distribution<uint32_t>(0, m_total_weight)(m_random);
    for(size_t i = 0; i < m_nodes.size(); ++i) {
        random_weight -= m_nodes[i].weight;
        if(random_weight <= 0) return m_nodes[i].node;
    }
    return nullptr;
}

template<typename Type>
size_t WeightedRandomLoadBalancer<Type>::Size() const
{
    return m_nodes.size();
}

template<typename Type>
void WeightedRandomLoadBalancer<Type>::Append(Type* node, uint32_t weight)
{
    m_nodes.emplace_back(std::move(node), weight);
    m_total_weight += weight;
}


}

#endif