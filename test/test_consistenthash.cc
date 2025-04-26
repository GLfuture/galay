#include "galay/galay.hpp"


int main()
{
    galay::utils::ConsistentHash hash;
    galay::utils::NodeConfig node1, node2;
    node1.id = 1, node2.id = 2;
    node1.endpoint = "127.0.0.1:8080", node2.endpoint = "127.0.0.1:8081";
    hash.AddNode(node1);
    hash.AddNode(node2);
    std::cout << hash.GetNode("test") << std::endl;
    std::cout << hash.GetNode("dasdasdasads23") << std::endl;

    return 0;
}