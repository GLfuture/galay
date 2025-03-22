#include <cassert>
#include <vector>
#include <string>
#include <utility> // for pair
#include <iostream>
#include "galay/protocol/Http2_0.hpp"

// ------------------------------------------------------------------------
// 辅助工具函数
// ------------------------------------------------------------------------
using namespace galay::http2;
// 比较两个头部列表是否完全一致
bool compareHeaders(
    const std::unordered_map<std::string, std::string>& a,
    const std::unordered_map<std::string, std::string>& b
) {
    if (a.size() != b.size()) return false;
    for (const auto& pair : a) {
        auto it = b.find(pair.first);
        if (it == b.end() || it->second != pair.second) {
            return false;
        }
    }
    return true;
}

std::vector<uint8_t> StringToUint8Vector(const std::string& str) {
    std::vector<uint8_t> result;
    result.reserve(str.size());
    for (char c : str) {
        result.push_back(static_cast<uint8_t>(c));
    }
    return result;
}

// ------------------------------------------------------------------------
// 测试用例
// ------------------------------------------------------------------------

// 测试空头部列表
void testEmptyHeaders() {
    HPack encoder(false); // 不启用Huffman
    std::string encoded = encoder.encodeHeaderBlock({});
    assert(encoded.empty()); // 编码结果应为空

    HPack decoder(false);
    auto decoded = decoder.decodeHeaderBlock(StringToUint8Vector(encoded));
    assert(decoded.empty()); // 解码结果应为空
}

// 测试静态表项（例如 :method: GET）
void testStaticTable() {
    std::unordered_map<std::string, std::string> headers = {{":method", "GET"}};
    
    // 启用Huffman
    HPack encoder_huff;
    encoder_huff.setEncodeWithHuffman(true);
    std::string encoded_huff = encoder_huff.encodeHeaderBlock(headers);
    assert(encoded_huff.length() == 1);
    std::cout << std::hex << static_cast<uint32_t>(encoded_huff[0]) << std::endl;
    assert(static_cast<unsigned char>(encoded_huff[0]) == 0x82);
    HPack decoder_huff;
    decoder_huff.setEncodeWithHuffman(true);
    auto decoded_huff = decoder_huff.decodeHeaderBlock(StringToUint8Vector(encoded_huff));
    assert(compareHeaders(headers, decoded_huff));

    // 不启用Huffman
    HPack encoder_no_huff;
    encoder_no_huff.setEncodeWithHuffman(false);
    std::string encoded_no_huff = encoder_no_huff.encodeHeaderBlock(headers);
    HPack decoder_no_huff;
    decoder_no_huff.setEncodeWithHuffman(false);
    auto decoded_no_huff = decoder_no_huff.decodeHeaderBlock(StringToUint8Vector(encoded_no_huff));
    assert(compareHeaders(headers, decoded_no_huff));
}

// 测试字面量头部（不在静态表中）
void testLiteralHeader() {

    

    std::unordered_map<std::string, std::string> headers = {
        {"x-custom-header", "hello world"},
        {"user-agent", "test-agent/1.0"}
    };
    std::cout << "---------------------------------------------" << std::endl;
    // 测试启用Huffman
    HPack encoder_huff;
    encoder_huff.setEncodeWithHuffman(true);
    std::string encoded_huff = encoder_huff.encodeHeaderBlock(headers);
    for(uint8_t c: encoded_huff) {
      std::cout << std::hex << (int)c << " ";
    }
    std::cout << std::endl;
    std::cout << "---------------------------------------------" << std::endl;
    HPack decoder_huff;
    decoder_huff.setEncodeWithHuffman(true);
    auto decoded = decoder_huff.decodeHeaderBlock(StringToUint8Vector(encoded_huff));
    assert(compareHeaders(headers, decoded));

    // 测试不启用Huffman
    HPack encoder_no_huff;
    encoder_huff.setEncodeWithHuffman(false);
    std::string encoded_no_huff = encoder_no_huff.encodeHeaderBlock(headers);
    HPack decoder_no_huff;
    decoder_no_huff.setEncodeWithHuffman(false);
    auto decoded_no_huff = decoder_no_huff.decodeHeaderBlock(StringToUint8Vector(encoded_no_huff));
    assert(compareHeaders(headers, decoded_no_huff));
}

// 测试动态表重用（同一头部编码两次）
void testDynamicTableReuse() {
    std::unordered_map<std::string, std::string> headers = {{"x-reuse", "value"}};

    HPack encoder(4096); // 启用Huffman，动态表足够大

    // 第一次编码（字面量）
    std::string encoded1 = encoder.encodeHeaderBlock(headers);
    // 第二次编码（应使用动态表索引）
    std::string encoded2 = encoder.encodeHeaderBlock(headers);

    // 确保第二次编码更短
    assert(encoded2.size() < encoded1.size());

    // 解码验证
    HPack decoder;
    auto decoded1 = decoder.decodeHeaderBlock(StringToUint8Vector(encoded1));
    auto decoded2 = decoder.decodeHeaderBlock(StringToUint8Vector(encoded2));
    assert(compareHeaders(headers, decoded1));
    assert(compareHeaders(headers, decoded2));
}

// 测试动态表淘汰策略（插入超过最大尺寸时淘汰旧条目）
void testDynamicTableEviction() {
    // 动态表最大尺寸设置为 100 字节
    const size_t max_size = 100;
    HPack encoder(max_size); // 不启用Huffman
    encoder.setEncodeWithHuffman(false);

    // 条目大小计算：name.size() + value.size() + 32
    // 条目1：name=20字节，value=30字节 → 20+30+32=82字节
    std::unordered_map<std::string, std::string> headers1 = {
        {std::string(20, 'a'), std::string(30, 'a')}
    };
    encoder.encodeHeaderBlock(headers1); // 动态表当前大小 82

    // 条目2：name=20字节，value=30字节 → 82字节，插入后总大小 82+82=164 > 100
    std::unordered_map<std::string, std::string> headers2 = {
        {std::string(20, 'b'), std::string(30, 'b')}
    };
    encoder.encodeHeaderBlock(headers2); // 淘汰条目1，插入条目2（当前大小82）

    // 第三次编码 headers2 应使用动态表索引
    std::string encoded3 = encoder.encodeHeaderBlock(headers2);
    std::cout << "encoded3: " << uint8ToHex(StringToUint8Vector(encoded3)) << std::endl;
    assert(encoded3.size() ==  1); // 索引编码（0x80 | index） + 可能的其他字节

    // 解码验证
    auto decoded = encoder.decodeHeaderBlock(StringToUint8Vector(encoded3));
    assert(compareHeaders(headers2, decoded));
}

// ------------------------------------------------------------------------
// 主测试入口
// ------------------------------------------------------------------------
int main() {
    testEmptyHeaders();
    testStaticTable();
    testLiteralHeader();
    testDynamicTableReuse();
    testDynamicTableEviction();
    return 0;
}