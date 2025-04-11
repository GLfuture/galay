#include "Http2Hpack.hpp"

namespace galay::http2
{
std::unordered_map<std::string, std::string> HPack::decodeHeaderBlock(const std::vector<uint8_t> &data)
{
    std::unordered_map<std::string, std::string> headers;
    size_t offset = 0;
    while (offset < data.size()) {
        auto [header, newOffset] = decodeHeaderField(data, offset);
        headers.insert(header);
        offset = newOffset;
    }
    return headers;
}

std::string HPack::encodeHeaderBlock(const std::unordered_map<std::string, std::string> &headers)
{
    std::string output;
    for (const auto& [name, value] : headers) {
        if (auto index = FindDynamicTableIndex(name, value)) {
            encodeIndexedField(index, output);
        } else if (auto staticIndex = GetIndexFromKey(name, value)) {
            encodeIndexedField(staticIndex, output);
        } else {
            encodeLiteralField(name, value, output);
        }
    }
    return output;
}

std::pair<std::pair<std::string, std::string>, size_t> HPack::decodeHeaderField(const std::vector<uint8_t> &data, size_t offset)
{
    const uint8_t firstByte = data[offset];
    // 索引字段 (RFC 7541 6.1)
    if (firstByte & 0x80) {
        uint32_t index = decodeInteger(data, offset, 7);
        offset++;
        std::pair<std::string, std::string> header;
        if (index >= 1 && index <= 61) {
            // 静态表查找
            header = getStaticIndexedHeader(index);
        } else if (index >= 62) {
            // 动态表查找（索引从62开始）
            size_t dynamicIndex = index - 62;
            if (dynamicIndex >= dynamicTable_.size()) {
                throw std::runtime_error("Dynamic table index out of range");
            }
            header = {dynamicTable_[dynamicIndex].name, dynamicTable_[dynamicIndex].value};
        } else {
            throw std::runtime_error("Invalid index value");
        }
        if (header.first.empty() || header.second.empty()) { 
            throw std::runtime_error("Invalid indexed header (key-only entry)");
        }
        return {header, offset};
    }

    // 处理字面量字段
    bool addToTable = (firstByte & 0x40) != 0;
    uint8_t prefixBits = (firstByte & 0x40) ? 6 : 4;

    auto [name, nameIndex] = decodeName(data, offset, firstByte, prefixBits);
    auto value = decodeValue(data, offset);
    if (addToTable) {
        updateDynamicTable(name, value);
    }

    return {{name, value}, offset};
}

std::pair<std::string, bool> HPack::decodeName(const std::vector<uint8_t> &data, size_t &offset, uint8_t firstByte, uint8_t prefixBits)
{
    bool isIndexed = (firstByte & (1 << (prefixBits - 1))) == 0;
    if (isIndexed) {
        uint32_t index = decodeInteger(data, offset, prefixBits);
        if (index == 0) {
            return {decodeString(data, offset), false};
        }
        std::string name = getIndexedName(index);
        return {std::move(name), true};
    }
    
    return {decodeString(data, offset), false};
}


uint32_t HPack::decodeInteger(const std::vector<uint8_t> &data, size_t &offset, uint8_t prefixBits)
{
    if (offset >= data.size()) {
        throw std::runtime_error("HPACK integer decode overflow (initial offset)");
    }

    uint8_t firstByte = data[offset];
    uint32_t value = (firstByte & ((1 << prefixBits) - 1)); // 初始前缀
    bool isMultiByte = (value == (1 << prefixBits) - 1);

    if (!isMultiByte) {
        offset++;
        return value;
    }

    // 多字节处理
    value = 0;
    uint32_t m = 0;
    uint8_t b;
    do {
        offset++; // 移动到下一个字节
        if (offset >= data.size()) throw "HPACK: offset out of range";
        b = data[offset];
        value += (b & 0x7F) << m;
        m += 7;
    } while (b & 0x80);

    if (m > 28) throw std::overflow_error("HPACK integer overflow");

    offset++; // 处理完最后一个字节后递增
    return value + ((1 << prefixBits) - 1); // 加上初始前缀
}


std::string HPack::decodeString(const std::vector<uint8_t> &data, size_t &offset)
{
    bool isHuffman = (data[offset] & 0x80) != 0;
    uint32_t length = decodeInteger(data, offset, 7);
    
    if (offset + length > data.size()) {
        throw std::runtime_error("HPACK string length overflow");
    }

    std::string rawData(data.begin() + offset, 
                                data.begin() + offset + length);
    offset += length;

    if (isHuffman) {
        std::string res = "";
        auto vec = huffmanDecoder.decode(utils::HuffmanString(std::move(rawData)));
        for (auto& c : vec) {
            res += static_cast<uint8_t>(c);
        }
        return res;
    }
    return std::string(rawData.begin(), rawData.end());
}

void HPack::updateDynamicTable(const std::string &name, const std::string &value)
{
    DynamicTableEntry entry(name, value);
    
    // 淘汰旧条目
    while (!dynamicTable_.empty() && 
            (dynamicTableCurrentSize_ + entry.size > dynamicTableMaxSize_)) {
        dynamicTableCurrentSize_ -= dynamicTable_.back().size;
        dynamicTable_.pop_back();
    }

    if (entry.size > dynamicTableMaxSize_) return;
    dynamicTable_.push_front(entry);
    dynamicTableCurrentSize_ += entry.size;
}


void HPack::encodeLiteralField(const std::string &name, const std::string &value, std::string &output)
{
    // 使用Literal Header Field with Incremental Indexing
    encodeInteger(0, 6, 0x40, output);
    encodeString(name, output);
    encodeString(value, output);
    updateDynamicTable(name, value);
}

void HPack::encodeString(const std::string &str, std::string &output)
{
    bool useHuffman = shouldUseHuffman();
    std::vector<uint8_t> encoded = useHuffman ? 
        huffmanEncoder.encode(std::vector<uint32_t>(str.begin(), str.end())).toVec() : 
        std::vector<uint8_t>(str.begin(), str.end());

    // 合并 Huffman 标志位和长度编码
    uint8_t huffmanFlag = useHuffman ? 0x80 : 0x00;
    encodeInteger(encoded.size(), 7, huffmanFlag, output); // 新增参数传递标志位
    output.insert(output.end(), encoded.begin(), encoded.end());
}

void HPack::encodeInteger(uint32_t value, uint8_t prefixBits, uint8_t flags, std::string &output)
{
    uint8_t prefixMask = (1 << prefixBits) - 1;
    if (value < prefixMask) {
        output.push_back(flags | value);
        return;
    }
    // 多字节处理
    output.push_back(flags | prefixMask);
    value -= prefixMask;
    while (value >= 0x80) {
        output.push_back((value & 0x7F) | 0x80);
        value >>= 7;
    }
    output.push_back(value);
}


uint32_t HPack::FindDynamicTableIndex(const std::string& key, const std::string& value) {
    uint32_t baseIndex = 62;
    for (size_t i = 0; i < dynamicTable_.size(); ++i) {
        const DynamicTableEntry& entry = dynamicTable_[i];
        if (entry.name == key && entry.value == value) {
            return static_cast<uint32_t>(baseIndex + i);
        }
    }

    // 未找到匹配项
    return 0;
}
}