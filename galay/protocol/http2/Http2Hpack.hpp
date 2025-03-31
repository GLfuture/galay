#ifndef GALAY_HTTP2_HPACK_HPP
#define GALAY_HTTP2_HPACK_HPP

#include "Http2HpackHuffman.hpp"
#include "Http2HpackStaticTable.hpp"
#include "galay/utils/Huffman.hpp"
#include "galay/utils/String.h"
#include <deque>
#include <optional>
#include <cstdint>
#include <stdexcept>

namespace galay::http2
{
static utils::HuffmanTable<uint32_t> huffmanTable(HPACK_HUFFMAN_TABLE);
static utils::HuffmanEncoder<uint32_t> huffmanEncoder(huffmanTable);
static utils::HuffmanDecoder<uint32_t> huffmanDecoder(huffmanTable, 5, 30);

class HPack {
public:
    // 构造时初始化动态表最大尺寸
    explicit HPack(size_t dynamicTableSize = 4096) 
        : dynamicTableMaxSize_(dynamicTableSize) {}

    // 主解码接口
    std::unordered_map<std::string, std::string> decodeHeaderBlock(const std::vector<uint8_t>& data);
    // 主编码接口
    std::string encodeHeaderBlock(const std::unordered_map<std::string, std::string>& headers);

    /*
        决定是否哈夫曼编码不在动态表和静态表的header字段
    */
    void setEncodeWithHuffman(bool useHuffman) {
        encodeWithHuffman_ = useHuffman;
    }
private:
    // 动态表条目
    struct DynamicTableEntry {
        std::string name;
        std::string value;
        size_t size;

        DynamicTableEntry(std::string n, std::string v)
            : name(std::move(n)), value(std::move(v)), 
              size(name.size() + value.size() + 32) {}
    };

    

    // 解码单个头字段
    std::pair<std::pair<std::string, std::string>, size_t> decodeHeaderField(const std::vector<uint8_t>& data, size_t offset);

    // 解析字段名
    std::pair<std::string, bool> decodeName(const std::vector<uint8_t>& data,
                                           size_t& offset,
                                           uint8_t firstByte,
                                           uint8_t prefixBits);

    // 解析字段值
    std::string decodeValue(const std::vector<uint8_t>& data, size_t& offset) {
        return decodeString(data, offset);
    }

    // 处理HPACK整数编码 (RFC 7541 5.1)
    uint32_t decodeInteger(const std::vector<uint8_t>& data,
                         size_t& offset,
                         uint8_t prefixBits);

    // 处理字符串解码 (RFC 7541 5.2)
    std::string decodeString(const std::vector<uint8_t>& data, size_t& offset);

    // 更新动态表
    void updateDynamicTable(const std::string& name, const std::string& value);

    // 编码方法实现
    void encodeIndexedField(uint32_t index, std::string& output) {
        output.push_back(0x80 | (index & 0x7F));
    }

    void encodeLiteralField(const std::string& name,
                           const std::string& value,
                           std::string& output);

    void encodeString(const std::string& str, 
                     std::string& output);

    void encodeInteger(uint32_t value, uint8_t prefixBits, uint8_t flags, std::string &output);

    // 工具函数代理
    std::string getIndexedName(uint32_t index) {
        return GetKeyValueFromIndex(static_cast<HpackStaticHeaderKey>(index)).first;
    }

    std::pair<std::string, std::string> getStaticIndexedHeader(uint32_t index) {
        return GetKeyValueFromIndex(static_cast<HpackStaticHeaderKey>(index));
    }

    bool shouldUseHuffman() {
        return encodeWithHuffman_;
    }

    uint32_t FindDynamicTableIndex(const std::string& key, const std::string& value);
private:
    // 动态表管理
    std::deque<DynamicTableEntry> dynamicTable_;
    size_t dynamicTableMaxSize_;
    size_t dynamicTableCurrentSize_ = 0;
    bool encodeWithHuffman_ = true;

};


}


#endif