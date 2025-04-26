#ifndef GALAY_DISTRIBUTED_HPP
#define GALAY_DISTRIBUTED_HPP 

#include <map>
#include <vector>
#include <string>
#include <mutex>
#include <atomic>
#include <algorithm>
#include <functional>
#include <shared_mutex>
#include "galay/algorithm/MurmurHash3.hpp"

namespace galay::utils
{

struct NodeStatus {
    NodeStatus(const NodeStatus & status){
        is_healthy.store(status.is_healthy.load());
        request_count.store(status.request_count.load());
    }

    NodeStatus() = default;

    NodeStatus& operator=(const NodeStatus& status){
        if (this != &status) {
            is_healthy.store(status.is_healthy.load());
            request_count.store(status.request_count.load());
        }
        return *this;
    }

    std::atomic<bool> is_healthy{true};
    std::atomic<uint64_t> request_count{0};
};

// 节点配置
struct NodeConfig {
    std::string id;
    std::string endpoint;
    int weight = 1;  // 默认权重为1
};

struct PhysicalNode {
    NodeConfig config;
    NodeStatus status;
};

class ConsistentHash {
public:
    // 节点状态（对外暴露）

    explicit ConsistentHash(size_t virtual_nodes = 1000, uint32_t seed = 0)
        : virtual_nodes_(virtual_nodes), hash_seed_(seed) {}

    // 添加节点（线程安全）
    void AddNode(const NodeConfig& config);

    // 删除节点（线程安全）
    bool RemoveNode(const std::string& node_id);

    // 获取节点状态（线程安全）
    NodeStatus GetNodeStatus(const std::string& node_id);

    // 更新节点健康状态（线程安全）
    void UpdateNodeHealth(const std::string& node_id, bool healthy);
    // 查找节点（带健康检查重试）
    std::string GetNode(const std::string& key, int max_retry = 3);
private:
    // 哈希计算分发
    uint32_t CalculateHash(const std::string& key);
    void AddVirtualNodes(PhysicalNode& node);
    void RemoveVirtualNodes(const PhysicalNode& node);

private:
    std::map<uint32_t, PhysicalNode*> ring_;  // 哈希环
    std::vector<PhysicalNode> nodes_;         // 物理节点存储
    uint32_t hash_seed_;                        //hash 种子
    size_t virtual_nodes_;                   // 每个权重单位的虚拟节点数
    std::shared_mutex mutex_;                // 全局读写
};
}


#endif