#include "Distributed.hpp"

namespace galay::utils 
{
void ConsistentHash::AddNode(const NodeConfig &config)
{
    std::unique_lock lock(mutex_);
        
    PhysicalNode node {
        .config = config,
        .status = NodeStatus{}
    };
    
    nodes_.emplace_back(std::move(node));
    AddVirtualNodes(nodes_.back());
}

bool ConsistentHash::RemoveNode(const std::string &node_id)
{
    std::unique_lock lock(mutex_);    
    auto it = std::find_if(nodes_.begin(), nodes_.end(),
        [&](const PhysicalNode& n) { return n.config.id == node_id; });
    
    if (it == nodes_.end()) return false;
    
    RemoveVirtualNodes(*it);
    nodes_.erase(it);
    return true;
}

NodeStatus ConsistentHash::GetNodeStatus(const std::string &node_id)
{
    std::shared_lock lock(mutex_);
        
    auto it = std::find_if(nodes_.begin(), nodes_.end(),
        [&](const PhysicalNode& n) { return n.config.id == node_id; });
    
    if (it == nodes_.end()) return NodeStatus{};
    return it->status; 
}

void ConsistentHash::UpdateNodeHealth(const std::string &node_id, bool healthy)
{
    std::unique_lock lock(mutex_);
        
    auto it = std::find_if(nodes_.begin(), nodes_.end(),
        [&](const PhysicalNode& n) { return n.config.id == node_id; });
    
    if (it != nodes_.end()) {
        it->status.is_healthy.store(healthy);
    }
}

std::string ConsistentHash::GetNode(const std::string &key, int max_retry)
{
    std::shared_lock lock(mutex_);
    if (ring_.empty()) return "";
    
    uint32_t base_hash = CalculateHash(key);
    uint32_t current_hash = base_hash;
    int tried = 0;

    while (tried++ < max_retry) {
        auto it = ring_.lower_bound(current_hash);
        if (it == ring_.end()) it = ring_.begin();
        
        PhysicalNode* node = it->second;
        if (node->status.is_healthy.load()) {
            node->status.request_count++;
            return node->config.endpoint;
        }
        
        // 记录当前哈希并推进
        current_hash = it->first + 1;
        if (current_hash < it->first) {  // 处理溢出
            current_hash = 0;
            it = ring_.begin();
        }
    }
    return "";  // 所有尝试失败
}

uint32_t ConsistentHash::CalculateHash(const std::string &key)
{
#if defined(__x86_64__) || defined(_M_X64)
    uint64_t hash[2];
    algorithm::MurmurHash3_x64_128(key.data(), key.size(), hash_seed_, hash);
    return static_cast<uint32_t>(hash[0] ^ hash[1]);
#else
    uint32_t hash;
    algorithm::MurmurHash3_x86_32(key.data(), key.size(), hash_seed_, &hash);
    return hash;
#endif
}

void ConsistentHash::AddVirtualNodes(PhysicalNode &node)
{
    for (size_t i = 0; i < virtual_nodes_ * node.config.weight; ++i) {
        std::string vnode = node.config.id + "#" + std::to_string(i);
        uint32_t hash = CalculateHash(vnode);
        ring_.emplace(hash, &node);
    }
}

void ConsistentHash::RemoveVirtualNodes(const PhysicalNode &node)
{
    auto it = ring_.begin();
    while (it != ring_.end()) {
        if (it->second->config.id == node.config.id) {
            it = ring_.erase(it);
        } else {
            ++it;
        }
    }
}


}