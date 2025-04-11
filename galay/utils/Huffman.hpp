#ifndef GALAY_HUFFMAN_HPP
#define GALAY_HUFFMAN_HPP

#include <vector>
#include <string>
#include <cstdint>
#include <concepts>
#include <stdexcept>
#include <unordered_map>


namespace galay::utils
{
    
template <typename T>
concept Comparable = requires(T a, T b) {
    { a == b } -> std::same_as<bool>;
    { a != b } -> std::same_as<bool>;
    { a < b } -> std::same_as<bool>;
    { a <= b } -> std::same_as<bool>;
    { a > b } -> std::same_as<bool>;
    { a >= b } -> std::same_as<bool>;
};

template <Comparable T>
class HuffmanTable {
public:
    using CodeHashMap = std::unordered_map<T, std::string>;

    struct CodeInfo {
        uint64_t code;
        uint8_t length;
    };

    HuffmanTable() {}
    HuffmanTable(CodeHashMap code_map) {
        for(auto& [symbol, codeStr] : code_map) {
            addEntry(symbol, codeStr);
        }
    }


    void addEntry(T symbol, const std::string& codeStr) {
        if (codeStr.empty()) {
            throw std::invalid_argument("Code string cannot be empty");
        }
        if (codeStr.length() > 255) { // 显式长度检查
            throw std::invalid_argument("Code length exceeds 255 bits");
        }

        CodeInfo info;
        info.length = codeStr.length();
        info.code = 0;
        
        for (char c : codeStr) {
            info.code <<= 1;
            if (c == '1') info.code |= 1;
            else if (c != '0') {
                throw std::invalid_argument("Invalid code character");
            }
        }

        symbolToCode[symbol] = info;
        codeToSymbol[info.length][info.code] = symbol;
    }

    std::string getCodeStr(const T& symbol) const {
        auto it = symbolToCode.find(symbol);
        if (it == symbolToCode.end()) {
            throw std::invalid_argument("Symbol not found in Huffman table");
        }
        const CodeInfo& info = it->second;
        std::string codeStr;
        codeStr.reserve(info.length); // 预分配内存，提高效率
        for (int i = info.length - 1; i >= 0; --i) {
            uint64_t mask = 1ULL << i;
            codeStr += (info.code & mask) ? '1' : '0';
        }
        return codeStr;
    }

    void validate() const {
        using CodeKey = std::pair<uint8_t, uint64_t>;
        std::unordered_map<CodeKey, T> codeCheck;

        for (const auto& [symbol, info] : symbolToCode) {
            CodeKey key{info.length, info.code};
            if (codeCheck.find(key) != codeCheck.end()) {
                throw std::logic_error(
                    "Ambiguous code detected: symbols " + 
                    std::to_string(symbol) + " and " + 
                    std::to_string(codeCheck[key]) + 
                    " have the same code"
                );
            }
            codeCheck[key] = symbol;

            // 额外检查：是否某个编码是另一个编码的前缀
            for (const auto& [existingKey, existingSymbol] : codeCheck) {
                if (existingKey == key) continue;
                
                uint8_t minLen = std::min(existingKey.first, info.length);
                uint64_t mask = (1ULL << minLen) - 1;
                uint64_t existingPrefix = existingKey.second >> (existingKey.first - minLen);
                uint64_t currentPrefix = info.code >> (info.length - minLen);
                
                if (existingPrefix == currentPrefix) {
                    throw std::logic_error(
                        "Prefix collision: symbol " + std::to_string(symbol) + 
                        " and " + std::to_string(existingSymbol) + 
                        " share a prefix"
                    );
                }
            }
        }
    }

    const CodeInfo* getCodeInfo(T symbol) const {
        auto it = symbolToCode.find(symbol);
        return it != symbolToCode.end() ? &it->second : nullptr;
    }

    const std::unordered_map<uint64_t, T>& getCodesByLength(uint8_t length) const {
        static const std::unordered_map<uint64_t, T> empty;
        auto it = codeToSymbol.find(length);
        return it != codeToSymbol.end() ? it->second : empty;
    }

private:
    std::unordered_map<T, CodeInfo> symbolToCode;
    std::unordered_map<uint8_t, std::unordered_map<uint64_t, T>> codeToSymbol;
};

class HuffmanString {
public:
    explicit HuffmanString(std::string&& str) {
        m_str = std::move(str);
    }

    std::string_view getHuffmanString() const {
        return m_str;
    }

    std::vector<uint8_t> toVec() const {
        std::vector<uint8_t> res;
        for (auto& c : m_str) {
            res.push_back(c);
        }
        return res;
    }
private:
    std::string m_str;
};

template<Comparable T>
class HuffmanEncoder {
public:
    explicit HuffmanEncoder(const HuffmanTable<T>& table) : table(table){}

    HuffmanString encode(const std::vector<uint32_t>& symbols) const {
        std::string output;
        uint64_t bitBuffer = 0;
        uint8_t bitCount = 0;

        for (auto symbol : symbols) {
            const auto* info = table.getCodeInfo(symbol);
            if (!info) {
                throw std::invalid_argument("Symbol not in Huffman table");
            }

            bitBuffer = (bitBuffer << info->length) | info->code;
            bitCount += info->length;

            while (bitCount >= 8) {
                output.push_back(static_cast<uint8_t>(bitBuffer >> (bitCount - 8)));
                bitCount -= 8;
            }
        }

        if (bitCount > 0) {
            uint8_t padding = (1 << (8 - bitCount)) - 1;
            bitBuffer = (bitBuffer << (8 - bitCount)) | padding;
            output.push_back(static_cast<uint8_t>(bitBuffer));
        }

        return HuffmanString(std::move(output));
    }

private:
    const HuffmanTable<T>& table;
};

template<Comparable T>
class HuffmanDecoder {
public:
    explicit HuffmanDecoder(const HuffmanTable<T>& table, uint8_t min_bit, uint8_t max_bit) 
        : table(table), minCodeLength(min_bit), maxCodeLength(max_bit)  {}

    std::vector<T> decode(const HuffmanString& data) const {
        std::vector<T> symbols;
        uint64_t bitBuffer = 0;
        uint8_t bitCount = 0;

        for (uint8_t byte : data.getHuffmanString()) {
            bitBuffer = (bitBuffer << 8) | byte;
            bitCount += 8;

            while (bitCount >= minCodeLength) {
                bool found = false;
                for (uint8_t len = maxCodeLength; len >= minCodeLength; --len) {
                    if (bitCount < len) continue;

                    uint64_t code = (bitBuffer >> (bitCount - len)) & ((1ULL << len) - 1);
                    const auto& codes = table.getCodesByLength(len);
                    if (auto it = codes.find(code); it != codes.end()) {
                        symbols.push_back(it->second);
                        bitCount -= len;
                        found = true;
                        break;
                    }
                }
                
                if (!found) {
                    if (bitCount > maxCodeLength) {
                        throw std::runtime_error("Invalid Huffman code sequence");
                    }
                    break;
                }
            }
        }
        return symbols;
    }

private:
    const HuffmanTable<T>& table;
    uint8_t minCodeLength = 1;
    uint8_t maxCodeLength = 32;
};

}

#endif