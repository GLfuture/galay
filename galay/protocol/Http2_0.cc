#include "Http2_0.hpp"

namespace galay::http2 
{

std::unordered_map<uint32_t, std::string> hpackHuffmanMap = {
    {0,   "1111111111000"},
    {1,   "11111111111111111011001"},
    {2,   "1111111111111111111111100010"},
    {3,   "1111111111111111111111100011"},
    {4,   "1111111111111111111111100100"},
    {5,   "1111111111111111111111100101"},
    {6,   "1111111111111111111111100110"},
    {7,   "1111111111111111111111100111"},
    {8,   "1111111111111111111111101000"},
    {9,   "111111111111111111101010"},
    {10,  "111111111111111111111111111100"},
    {11,  "1111111111111111111111101001"},
    {12,  "1111111111111111111111101010"},
    {13,  "111111111111111111111111111101"},
    {14,  "1111111111111111111111101011"},
    {15,  "1111111111111111111111101100"},
    {16,  "1111111111111111111111101101"},
    {17,  "1111111111111111111111101110"},
    {18,  "1111111111111111111111101111"},
    {19,  "1111111111111111111111110000"},
    {20,  "1111111111111111111111110001"},
    {21,  "1111111111111111111111110010"},
    {22,  "111111111111111111111111111110"},
    {23,  "1111111111111111111111110011"},
    {24,  "1111111111111111111111110100"},
    {25,  "1111111111111111111111110101"},
    {26,  "1111111111111111111111110110"},
    {27,  "1111111111111111111111110111"},
    {28,  "1111111111111111111111111000"},
    {29,  "1111111111111111111111111001"},
    {30,  "1111111111111111111111111010"},
    {31,  "1111111111111111111111111011"},
    
    // ASCII 可打印字符
    {32,  "000111"},        // 空格
    {33,  "1111111000"},     // !
    {34,  "1111111001"},     // "
    {35,  "111111111010"},   // #
    {36,  "1111111111001"},  // $
    {37,  "010101"},         // %
    {38,  "11111000"},       // &
    {39,  "11111111010"},    // '
    {40,  "1111111010"},     // (
    {41,  "1111111011"},     // )
    {42,  "11111001"},       // *
    {43,  "11111111011"},    // +
    {44,  "11111010"},       // ,
    {45,  "010110"},         // -
    {46,  "010111"},         // .
    {47,  "011000"},         // /
    
    // 数字 0-9
    {48,  "00000"},          // 0
    {49,  "00001"},          // 1
    {50,  "00010"},          // 2
    {51,  "011001"},         // 3
    {52,  "011010"},         // 4
    {53,  "011011"},         // 5
    {54,  "011100"},         // 6
    {55,  "011101"},         // 7
    {56,  "011110"},         // 8
    {57,  "011111"},         // 9
    
    // 标点符号
    {58,  "1011100"},        // :
    {59,  "11111011"},       // ;
    {60,  "111111111111100"},// <
    {61,  "100000"},         // =
    {62,  "111111111011"},   // >
    {63,  "1111111100"},     // ?
    {64,  "1111111111010"},  // @
    
    // 大写字母 A-Z
    {65,  "100001"},         // A
    {66,  "1011101"},        // B
    {67,  "1011110"},        // C
    {68,  "1011111"},        // D
    {69,  "1100000"},        // E
    {70,  "1100001"},        // F
    {71,  "1100010"},        // G
    {72,  "1100011"},        // H
    {73,  "1100100"},        // I
    {74,  "1100101"},        // J
    {75,  "1100110"},        // K
    {76,  "1100111"},        // L
    {77,  "1101000"},        // M
    {78,  "1101001"},        // N
    {79,  "1101010"},        // O
    {80,  "1101011"},        // P
    {81,  "1101100"},        // Q
    {82,  "1101101"},        // R
    {83,  "1101110"},        // S
    {84,  "1101111"},        // T
    {85,  "1110000"},        // U
    {86,  "1110001"},        // V
    {87,  "1110010"},        // W
    {88,  "11111100"},        // X
    {89,  "1110011"},        // Y
    {90,  "11111101"},        // Z
    
    // 小写字母 a-z
    {97,  "0001100"},        // a
    {98,  "100011"},         // b
    {99,  "0011111"},        // c
    {100, "100100"},         // d
    {101, "001100"},         // e
    {102, "100101"},         // f
    {103, "100110"},         // g
    {104, "100111"},         // h
    {105, "001101"},         // i
    {106, "101000"},         // j
    {107, "101001"},         // k
    {108, "101010"},         // l
    {109, "101011"},         // m
    {110, "001110"},         // n
    {111, "101100"},         // o
    {112, "101101"},         // p
    {113, "101110"},         // q
    {114, "101111"},         // r
    {115, "110000"},         // s
    {116, "110001"},         // t
    {117, "110010"},         // u
    {118, "110011"},         // v
    {119, "110100"},         // w
    {120, "110101"},         // x
    {121, "110110"},         // y
    {122, "110111"},         // z
    {123, "1110100"},    // ASCII 123: {
    {124, "1110101"},    // ASCII 124: |
    {125, "1110110"},    // ASCII 125: }
    {126, "1110111"},    // ASCII 126: ~
    
    // 特殊符号
    {256, "11111111111111111111111111111110"} // EOS
};

DataFrame *FrameFactory::CreateDataFrame(uint32_t stream_id, bool end_stream, std::string &&data, uint32_t max_payload_length, uint8_t padding_length)
{
    FrameHeader header;
    header.flags = end_stream ? 0x1 : 0x0;
    header.length = data.size();
    header.stream_id = stream_id;
    header.type = FrameType::DATA;
    DataFrame *frame = new DataFrame(header);
    frame->m_padding_length = padding_length;
    frame->m_max_payload_length = max_payload_length;
    frame->m_data = std::move(data);
    return frame;
}

HeadersFrame *FrameFactory::CreateHeadersFrame(uint32_t stream_id, bool no_data, std::string&& header_block_fragment, \
    uint32_t max_payload_length, Priority priority, uint8_t padding_length)
{
    FrameHeader header;
    header.stream_id = stream_id;
    header.type = FrameType::HEADERS;
    if (no_data)
    {
        header.flags |= 0x01;
    }
    if (padding_length > 0)
    {
        header.flags |= 0x08;
    }
    if (priority.priority)
    {
        header.flags |= 0x20;
    }
    HeadersFrame* frame = new HeadersFrame(header);
    frame->m_padding_length = padding_length;
    frame->m_priority = priority;
    frame->m_max_payload_length = max_payload_length;
    frame->m_header_block_fragment = std::move(header_block_fragment);
    return frame;
}

PriorityFrame *FrameFactory::CreatePriorityFrame(uint32_t stream_id, Priority priority)
{
    FrameHeader header;
    header.flags = 0;
    header.length = 5;
    header.stream_id = stream_id;
    header.type = FrameType::PRIORITY;
    PriorityFrame* frame = new PriorityFrame(header);
    frame->m_priority = std::move(priority);
    return frame;
}

RstStreamFrame *FrameFactory::CreateRstStreamFrame(uint32_t stream_id, ErrorCode error_code)
{
    FrameHeader header;
    header.flags = 0;
    header.stream_id = stream_id;
    header.type = FrameType::RST_STREAM;
    RstStreamFrame* frame = new RstStreamFrame(header);
    frame->m_error_code = error_code;
    return frame;
}

SettingsFrame *FrameFactory::CreateSettingsFrame(uint32_t stream_id, bool ack)
{
    FrameHeader header;
    header.flags = ack ? 0x1 : 0x0;
    header.stream_id = stream_id;
    header.type = FrameType::SETTINGS;
    SettingsFrame* frame = new SettingsFrame(header);
    return frame;
}

PushPromiseFrame *FrameFactory::CreatePushPromiseFrame(uint32_t stream_id, uint32_t promised_stream_id, std::string &&header_block_fragment,\
     uint32_t max_payload_length, uint8_t padding_length)
{
    FrameHeader header;
    header.stream_id = stream_id;
    header.type = FrameType::PUSH_PROMISE;
    PushPromiseFrame* frame = new PushPromiseFrame(header);
    frame->m_padding_length = padding_length;
    frame->m_max_payload_length = max_payload_length;
    frame->m_promised_stream_id = promised_stream_id;
    frame->m_header_block_fragment = std::move(header_block_fragment);
    return frame;
}

PingFrame *FrameFactory::CreatePingFrame(uint32_t stream_id, bool ack, std::array<uint8_t, 8> payload)
{
    FrameHeader header;
    header.type = FrameType::PING;
    header.stream_id = stream_id;
    header.length = 8;
    header.flags = ack ? 0x1 : 0x0;
    PingFrame *frame = new PingFrame(header);
    frame->OpaqueData(payload);
    return frame;
}

GoAwayFrame *FrameFactory::CreateGoAwayFrame(uint32_t last_stream_id, ErrorCode error_code, std::string &&debug_data)
{
    FrameHeader header;
    header.type = FrameType::GOAWAY;
    header.stream_id = 0;
    header.flags = 0x0;
    GoAwayFrame *frame = new GoAwayFrame(header);
    frame->m_last_stream_id = last_stream_id;
    frame->m_error_code = error_code;
    frame->m_debug_data = std::move(debug_data);
    return frame;
}

WindowUpdateFrame *FrameFactory::CreateWindowUpdateFrame(uint32_t stream_id, uint32_t window_size_increment)
{
    FrameHeader header;
    header.type = FrameType::WINDOW_UPDATE;
    header.stream_id = stream_id;
    header.flags = 0x0;
    WindowUpdateFrame *frame = new WindowUpdateFrame(header);
    frame->m_window_increment = window_size_increment;
    return frame;
}

ContinuationFrame *FrameFactory::CreateContinuationFrame(uint32_t stream_id, bool end_headers, std::string &&header_block_fragment, uint32_t max_payload_length)
{
    FrameHeader header;
    header.type = FrameType::CONTINUATION;
    header.flags = end_headers ? 0x4 : 0x0;
    header.stream_id = stream_id;
    ContinuationFrame *frame = new ContinuationFrame(header);
    frame->m_header_block_fragment = std::move(header_block_fragment);
    return frame;
}

void FrameHeader::DeSerialize(std::string_view data)
{
    length = (data[0] << 16) | (data[1] << 8) | data[2];
    type = static_cast<FrameType>(data[3]);
    flags = data[4];
    stream_id = (data[5] << 24) | (data[6] << 16) 
                        | (data[7] << 8) | data[8];
    stream_id &= 0x7FFFFFFF;
}

std::string FrameHeader::Serialize()
{
    std::string data;
    data.reserve(9);
    // 24位长度 (Big-Endian)
    data.push_back(static_cast<char>((length >> 16) & 0xFF));
    data.push_back(static_cast<char>((length >> 8) & 0xFF));
    data.push_back(static_cast<char>(length & 0xFF));
    // 帧类型
    data.push_back(static_cast<char>(type));
    // 标志位
    data.push_back(flags);
    // 31位流ID (Big-Endian，最高位保留)
    uint32_t stream_id = stream_id & 0x7FFFFFFF;
    data.push_back(static_cast<char>((stream_id >> 24) & 0xFF));
    data.push_back(static_cast<char>((stream_id >> 16) & 0xFF));
    data.push_back(static_cast<char>((stream_id >> 8) & 0xFF));
    data.push_back(static_cast<char>(stream_id & 0xFF));
    return data;
}

bool DataFrame::DeSerialize(std::string_view data)
{
    size_t offset = 0;
    m_padding_length = 0;

    if (Flags() & 0x08) { // PADDED
        if (data.length() < 1) return false;
        m_padding_length = data[0];
        offset = 1;
    }

    const size_t required = offset + m_padding_length;
    if (data.length() < required) return false;

    size_t data_length = m_header.length - (offset + m_padding_length);
    m_data = data.substr(offset, data_length);
    return true;
}

std::string DataFrame::Serialize()
{
    // 检查流ID合法性（必须非0）
    if (m_header.stream_id == 0) {
        throw std::runtime_error("DataFrame: Stream ID cannot be 0");
    }

    std::string payload;

    // 1. 处理填充（PADDED 标志）
    const bool has_padding = (m_padding_length > 0);
    if (has_padding) {
        // 检查填充长度合法性（0~255）
        if (m_padding_length > 255) {
            throw std::runtime_error("DataFrame: Padding length exceeds 255");
        }
        payload.push_back(static_cast<char>(m_padding_length)); // 写入填充长度字段
    }

    // 2. 添加应用数据（HPACK 或其他原始数据）
    payload += m_data;

    // 3. 添加填充字节（全0填充）
    if (has_padding) {
        payload.append(m_padding_length, '\0');
    }

    // 检查总负载长度合法性（24位限制）
    if (payload.size() > m_max_payload_length) {
        throw std::runtime_error("DataFrame: Payload exceeds m_max_payload_length limit");
    }

    m_header.length = static_cast<uint32_t>(payload.size()); // 设置负载长度
    // 序列化：头部 + 负载
    std::string frame_data = m_header.Serialize();          // 假设 FrameHeader 已实现
    frame_data += payload;
    return frame_data;
}

bool HeadersFrame::DeSerialize(std::string_view data) 
{
    size_t offset = 0;
    const uint8_t* ptr = reinterpret_cast<const uint8_t*>(data.data());

    // 处理填充标志（RFC 7540 Section 6.2）
    if (Flags() & 0x08) { // PADDED标志
        if (data.size() < 1) return false;
        m_padding_length = ptr[0];
        offset += 1;
    }

    // 处理优先级标志（RFC 7540 Section 6.3）
    if (Flags() & 0x20) { // PRIORITY标志
        if (data.size() - offset < 5) return false;
        
        // 解析流依赖和权重（RFC 7540 Section 6.2）
        bool exclusive = (ptr[offset] & 0x80) != 0;
        uint32_t stream_dependency = Parse31Bit(ptr + offset) & 0x7FFFFFFF;
        uint16_t weight = ptr[offset + 4] + 1;  // 权重范围1-256

        m_priority = Priority(stream_dependency, weight, exclusive);
        offset += 5;
    }

    // 头块片段（RFC 7541）
    m_header_block_fragment.assign(
        reinterpret_cast<const char*>(ptr) + offset,
        data.size() - offset - m_padding_length
    );
    return true;
}

std::string HeadersFrame::Serialize()
{
    // 验证流ID非0（通过基类方法）
    if (!Validate()) {
        throw std::runtime_error("HeadersFrame: Stream ID cannot be 0");
    }

    std::string payload;

    // 1. 处理填充（PADDED 标志）
    const bool has_padding = (m_padding_length > 0);
    if (has_padding) {
        if (m_padding_length > 255) {
            throw std::runtime_error("HeadersFrame: Pad length exceeds 255");
        }
        payload.push_back(static_cast<char>(m_padding_length)); // 填充长度字段
    }

    // 2. 处理优先级（PRIORITY 标志）
    const bool has_priority = (m_priority.weight >= 1 && m_priority.weight <= 256);
    if (has_priority) {
        // 验证权重和流依赖合法性
        if (m_priority.weight < 1 || m_priority.weight > 256) {
            throw std::runtime_error("HeadersFrame: Invalid weight (1-256)");
        }
        if (m_priority.streamDependency > 0x7FFFFFFF) {
            throw std::runtime_error("HeadersFrame: Stream dependency exceeds 31 bits");
        }

        // 构造 4 字节流依赖（大端序，最高位为独占标志）
        uint32_t stream_dep = m_priority.streamDependency;
        if (m_priority.exclusive) stream_dep |= 0x80000000;
        payload.push_back(static_cast<char>((stream_dep >> 24) & 0xFF));
        payload.push_back(static_cast<char>((stream_dep >> 16) & 0xFF));
        payload.push_back(static_cast<char>((stream_dep >> 8) & 0xFF));
        payload.push_back(static_cast<char>(stream_dep & 0xFF));
        // 写入权重（weight-1 存储为 1 字节）
        payload.push_back(static_cast<char>(m_priority.weight - 1));
    }
    // 更新 Frame 头部标志位和长度
    FrameHeader& header = this->m_header; // 假设基类 Frame 有公有成员 header
    if (payload.size() + m_padding_length + m_header_block_fragment.size() > m_max_payload_length) {
        std::stringstream ss;
        payload += m_header_block_fragment.substr(0, m_max_payload_length - m_padding_length);
        if (has_padding) {
            payload.append(m_padding_length, '\0');
        }
        header.length = static_cast<uint32_t>(payload.size());
        uint32_t offset = m_max_payload_length - m_padding_length;
        ss << header.Serialize();
        ss << payload;
        while (offset < m_header_block_fragment.length())
        {
            bool is_end = (offset + m_max_payload_length >= m_header_block_fragment.size());
            uint32_t chunk_size = std::min(m_max_payload_length, static_cast<uint32_t>(m_header_block_fragment.size() - offset));
            std::unique_ptr<ContinuationFrame> frame(FrameFactory::CreateContinuationFrame(
                m_header.stream_id, 
                is_end,
                m_header_block_fragment.substr(offset, chunk_size),
                chunk_size
            ));
            ss << frame->Serialize();
            offset += chunk_size;
        }
        return ss.str();
    } else {
        //end_header
        m_header.flags |= 0x04;
        // 3. 添加头部块片段（HPACK 压缩数据）
        payload += m_header_block_fragment;
        // 4. 添加填充数据（全0填充）
        if (has_padding) {
            payload.append(m_padding_length, '\0');
        }
        header.length = static_cast<uint32_t>(payload.size());
        std::string frame_data = header.Serialize();
        frame_data += payload;
        return frame_data;
    }
    return "";
}

bool PriorityFrame::DeSerialize(std::string_view data) {
    // 校验负载长度必须为 5 字节
    if (data.size() != 5) {
        return false;  // RFC 规定 PRIORITY 帧负载长度固定为 5 字节
    }

    // 解析前 4 字节（Exclusive 标志和依赖流 ID）
    const auto* bytes = reinterpret_cast<const uint8_t*>(data.data());
    uint32_t e_and_dep_stream = (bytes[0] << 24) | (bytes[1] << 16) | (bytes[2] << 8) | bytes[3];
    bool exclusive = (e_and_dep_stream & 0x80000000) != 0;         // 取最高位
    uint32_t stream_dependency = e_and_dep_stream & 0x7FFFFFFF;      // 取低 31 位

    // 解析第 5 字节（Weight）
    uint16_t weight = bytes[4] + 1;  // 实际权重 = 存储值 +1（RFC 规定）
    m_priority = Priority{stream_dependency, weight, exclusive};

    return true;
}

std::string PriorityFrame::Serialize()
{
    // 验证当前流 ID 非0（RFC 7540 Section 6.3）
    if (m_header.stream_id == 0) {
        throw std::runtime_error("PriorityFrame: Stream ID cannot be 0");
    }

    // 验证依赖流 ID 是31位有效值（0x7FFFFFFF为最大值）
    if (m_priority.streamDependency > 0x7FFFFFFF) {
        throw std::runtime_error("PriorityFrame: Dependent Stream ID exceeds 31 bits");
    }

    // 验证权重范围（实际值 1~256）
    if (m_priority.weight < 1 || m_priority.weight > 256) {
        throw std::runtime_error("PriorityFrame: Weight must be 1~256");
    }

    // 构造负载数据（固定5字节）
    std::string payload(5, 0); 

    // 1. 处理依赖流 ID 和独占标志
    uint32_t stream_dep = m_priority.streamDependency;
    if (m_priority.exclusive) {
        stream_dep |= 0x80000000; // 设置最高位为1（独占标志）
    }
    // 转为大端序（4字节）
    payload[0] = static_cast<char>((stream_dep >> 24) & 0xFF);
    payload[1] = static_cast<char>((stream_dep >> 16) & 0xFF);
    payload[2] = static_cast<char>((stream_dep >> 8) & 0xFF);
    payload[3] = static_cast<char>(stream_dep & 0xFF);

    // 2. 处理权重（存储为 weight-1）
    payload[4] = static_cast<char>(m_priority.weight - 1);
    m_header.length = 5;                                       // 固定负载长度

    // 序列化：头部 + 负载
    std::string frame_data = m_header.Serialize(); // 假设基类 Frame 处理头部序列化
    frame_data += payload;

    return frame_data;
}


bool SettingsFrame::DeSerialize(std::string_view data)
{
    if (data.length() % 6 != 0) return false;
    
    for (size_t i = 0; i < data.length(); i += 6) {
        // 解析ID（大端序转主机序）
        uint16_t id = static_cast<uint8_t>(data[i]) << 8 | static_cast<uint8_t>(data[i + 1]);

        // 解析Value（大端序转主机序）
        uint32_t value = static_cast<uint8_t>(data[i + 2]) << 24 | static_cast<uint8_t>(data[i + 3]) << 16 |
                        static_cast<uint8_t>(data[i + 4]) << 8 | static_cast<uint8_t>(data[i + 5]);

        // 忽略未知参数（RFC 7540规定）
        if (id < 0x1 || id > 0x6) {
            continue;
        }
        SettingIdentifier setting_id = static_cast<SettingIdentifier>(id);
        // 检查参数值合法性
        switch (setting_id) {
            case SettingIdentifier::HEADER_TABLE_SIZE :  // HEADER_TABLE_SIZE
                // 无范围限制（但实现可定义上限）
                break;
            case SettingIdentifier::ENABLE_PUSH :  // ENABLE_PUSH
                if (value != 0 && value != 1) {
                    return false;  // 非法值
                }
                break;
            case SettingIdentifier::MAX_CONCURRENT_STREAMS :  // MAX_CONCURRENT_STREAMS
                // 无范围限制
                break;
            case SettingIdentifier::INITIAL_WINDOW_SIZE :  // INITIAL_WINDOW_SIZE
                if (value > 0x7FFFFFFF) {  // 最大允许2^31-1
                    return false;
                }
                break;
            case SettingIdentifier::MAX_FRAME_SIZE :  // MAX_FRAME_SIZE
                if (value < 16384 || value > 16777215) {
                    return false;
                }
                break;
            case SettingIdentifier::MAX_HEADER_LIST_SIZE :  // MAX_HEADER_LIST_SIZE
                // 无范围限制
                break;
            default:
                break;
        }

        m_settings[setting_id] = value;
    }
    return true;
}

std::string SettingsFrame::Serialize()
{
    std::string payload;

    // 检查是否是ACK，ACK帧的payload必须为空
    if (!(Flags() & 0x01)) { // 非ACK帧才序列化键值对
        for (const auto& [id, value] : m_settings) {

            uint16_t tid = static_cast<uint16_t>(id);
             // ID（2字节大端）
            payload.push_back(static_cast<char>(( tid >> 8) & 0xFF));
            payload.push_back(static_cast<char>(tid & 0xFF));

            // Value（4字节大端）
            payload.push_back(static_cast<char>((value >> 24) & 0xFF));
            payload.push_back(static_cast<char>((value >> 16) & 0xFF));
            payload.push_back(static_cast<char>((value >> 8) & 0xFF));
            payload.push_back(static_cast<char>(value & 0xFF));
        }
    }

    // 构建帧头（长度、类型、标志、流ID）
    m_header.type = FrameType::SETTINGS;
    m_header.length = payload.length();
    return m_header.Serialize() + payload;
}

bool GoAwayFrame::DeSerialize(std::string_view data) 
{
    // 最小有效载荷：Last-Stream-ID(4) + ErrorCode(4) = 8字节
    if (data.length() < 8) return false;

    // 解析最后流ID（31位无符号整数）
    m_last_stream_id = (data[0] << 24) | (data[1] << 16) 
                    | (data[2] << 8) | data[3];
    m_last_stream_id &= 0x7FFFFFFF; // 清除最高位

    // 解析错误码（32位无符号整数）
    m_error_code = static_cast<ErrorCode>(
        (data[4] << 24) | (data[5] << 16) 
        | (data[6] << 8) | data[7]
    );

    // 处理调试数据（剩余字节）
    size_t debug_data_length_ = m_header.length - 8;
    if (data.length() < 8 + debug_data_length_) return false;
    
    m_debug_data = data.substr(8, debug_data_length_);
    return true;
}

std::string GoAwayFrame::Serialize()
{
    // 验证流ID必须为0（作用于整个连接，RFC 7540 Section 6.8）
    if (m_header.stream_id != 0) {
        throw std::runtime_error("GoAwayFrame: Stream ID must be 0");
    }

    // 验证最后流ID是31位无符号整数
    if (m_last_stream_id > 0x7FFFFFFF) {
        throw std::runtime_error("GoAwayFrame: Last Stream ID exceeds 31 bits");
    }

    // 构造负载数据
    std::string payload;

    // 1. 序列化最后流ID（4字节大端序）
    payload.push_back(static_cast<char>((m_last_stream_id >> 24) & 0xFF));
    payload.push_back(static_cast<char>((m_last_stream_id >> 16) & 0xFF));
    payload.push_back(static_cast<char>((m_last_stream_id >> 8) & 0xFF));
    payload.push_back(static_cast<char>(m_last_stream_id & 0xFF));

    // 2. 序列化错误码（4字节大端序）
    uint32_t error_code = static_cast<uint32_t>(m_error_code);
    payload.push_back(static_cast<char>((error_code >> 24) & 0xFF));
    payload.push_back(static_cast<char>((error_code >> 16) & 0xFF));
    payload.push_back(static_cast<char>((error_code >> 8) & 0xFF));
    payload.push_back(static_cast<char>(error_code & 0xFF));

    // 3. 添加调试数据（可选）
    payload += m_debug_data;

    // 检查负载总长度合法性（24位限制）
    if (payload.size() > 0x00FFFFFF) {
        throw std::runtime_error("GoAwayFrame: Payload exceeds 24-bit limit");
    }

    // 更新帧头部
    m_header.type = FrameType::GOAWAY; // 类型为0x7
    m_header.flags = 0;                                      // 无定义标志位
    m_header.length = static_cast<uint32_t>(payload.size()); // 负载长度

    // 序列化：头部 + 负载
    std::string frame_data = m_header.Serialize(); // 假设 FrameHeader 已实现序列化
    frame_data += payload;

    return frame_data;
}

bool RstStreamFrame::DeSerialize(std::string_view data)  
{
    if (data.length() != 4) return false; // 必须正好4字节
    
    m_error_code = static_cast<ErrorCode>(
        (data[0] << 24) | (data[1] << 16) 
        | (data[2] << 8) | data[3]
    );
    return true;
}

std::string RstStreamFrame::Serialize()
{
    // 验证流ID非0（RFC 7540 Section 6.4）
    if (m_header.stream_id == 0) {
        throw std::runtime_error("RstStreamFrame: Stream ID cannot be 0");
    }

    // 验证错误码合法性（假设 ErrorCode 是枚举类型，范围 0x00~0x0D）
    const auto error_value = static_cast<uint32_t>(m_error_code);
    if (error_value > 0x0D) { // 0x0D 是 HTTP_1_1_REQUIRED
        throw std::runtime_error("RstStreamFrame: Invalid error code");
    }

    // 构造负载（4字节错误码，大端序）
    std::string payload(4, 0);
    payload[0] = static_cast<char>((error_value >> 24) & 0xFF);
    payload[1] = static_cast<char>((error_value >> 16) & 0xFF);
    payload[2] = static_cast<char>((error_value >> 8) & 0xFF);
    payload[3] = static_cast<char>(error_value & 0xFF);

    // 更新帧头部
    m_header.type = FrameType::RST_STREAM; // 类型为0x3
    m_header.flags = 0;                                         // 无定义标志位
    m_header.length = 4;                                        // 固定负载长度

    // 序列化：头部 + 负载
    std::string frame_data = m_header.Serialize(); // 假设基类 Frame 处理头部序列化
    frame_data += payload;

    return frame_data;
}

bool PushPromiseFrame::DeSerialize(std::string_view data)  
{
    size_t offset = 0;
    const uint8_t* ptr = reinterpret_cast<const uint8_t*>(data.data());

    // 处理填充标志（RFC 7540 Section 6.6）
    if (Flags() & 0x08) { // PADDED标志
        if (data.size() < 1) return false;
        m_padding_length = ptr[0];
        offset += 1;
    }

    // 解析承诺流ID（RFC 7540 Section 6.6）
    if (data.size() - offset < 4) return false;
    m_promised_stream_id = Parse31Bit(ptr + offset);
    offset += 4;

    // 头块片段（需与Headers帧连续）
    m_header_block_fragment.assign(
        reinterpret_cast<const char*>(ptr) + offset,
        data.size() - offset - m_padding_length
    );
    return true;
}

std::string PushPromiseFrame::Serialize()
{
     // 验证当前流 ID 非0（RFC 7540 Section 6.6）
    if (m_header.stream_id == 0) {
        throw std::runtime_error("PushPromiseFrame: Stream ID cannot be 0");
    }

    // 验证承诺流 ID 合法性（31位无符号整数）
    if (m_promised_stream_id > 0x7FFFFFFF) {
        throw std::runtime_error("PushPromiseFrame: Promised stream ID exceeds 31 bits");
    }

    std::string payload;

    // 1. 处理填充（PADDED 标志）
    const bool has_padding = (m_padding_length > 0);
    if (has_padding) {
        if (m_padding_length > 255) {
            throw std::runtime_error("PushPromiseFrame: Pad length exceeds 255");
        }
        payload.push_back(static_cast<char>(m_padding_length)); // 填充长度字段
    }

    // 2. 序列化承诺流 ID（4字节大端序）
    payload.push_back(static_cast<char>((m_promised_stream_id >> 24) & 0xFF));
    payload.push_back(static_cast<char>((m_promised_stream_id >> 16) & 0xFF));
    payload.push_back(static_cast<char>((m_promised_stream_id >> 8) & 0xFF));
    payload.push_back(static_cast<char>(m_promised_stream_id & 0xFF));

    
    // 检查总负载长度合法性（24位限制）
    if (payload.size() + m_padding_length + m_header_block_fragment.size() > m_max_payload_length) {
        std::stringstream ss;
        payload += m_header_block_fragment.substr(0, m_max_payload_length - m_padding_length);
        if (has_padding) {
            payload.append(m_padding_length, '\0');
        }
        m_header.length = static_cast<uint32_t>(payload.size());
        uint32_t offset = m_max_payload_length - m_padding_length;
        ss << m_header.Serialize();
        ss << payload;
        while (offset < m_header_block_fragment.length())
        {
            bool is_end = (offset + m_max_payload_length >= m_header_block_fragment.size());
            uint32_t chunk_size = std::min(m_max_payload_length, static_cast<uint32_t>(m_header_block_fragment.size() - offset));
            std::unique_ptr<ContinuationFrame> frame(FrameFactory::CreateContinuationFrame(
                m_header.stream_id, 
                is_end,
                m_header_block_fragment.substr(offset, chunk_size),
                chunk_size
            ));
            ss << frame->Serialize();
            offset += chunk_size;
        }
        return ss.str();
    }  else {
        payload += m_header_block_fragment;
        if (has_padding) {
            payload.append(m_padding_length, '\0');
        }
        m_header.flags |= 0x04; //end of headers
        m_header.length = static_cast<uint32_t>(payload.size());      // 负载长度
        std::string frame_data = m_header.Serialize(); // 假设 FrameHeader 已实现序列化
        frame_data += payload;
        return frame_data;
    }
    return "";
}

bool PushPromiseFrame::Validate() const {
    // 流ID必须为偶数且不能为0
    return (m_promised_stream_id != 0) && 
            ((m_promised_stream_id & 1) == 0);
}


bool PingFrame::DeSerialize(std::string_view data) {
    // 校验负载长度必须为 8 字节
    if (data.size() != 8) {
        return false;
    }
    // 拷贝负载数据
    memcpy(m_opaque_data.data(), data.data(), 8);
    return true;
}

std::string PingFrame::Serialize()
{
    // 验证流ID必须为0（RFC 7540 Section 6.7）
    if (m_header.stream_id != 0) {
        throw std::runtime_error("PingFrame: Stream ID must be 0");
    }

    // 验证负载必须为8字节
    if (m_opaque_data.size() != 8) {
        throw std::runtime_error("PingFrame: Opaque data must be 8 bytes");
    }

    // 构造负载（固定8字节）
    std::string payload;
    payload.reserve(8);
    for (const auto& byte : m_opaque_data) {
        payload.push_back(static_cast<char>(byte));
    }

    // 更新帧头部
    m_header.type = FrameType::PING; // 类型为0x6
    m_header.length = 8;                                   // 固定负载长度
    // 序列化：头部 + 负载
    std::string frame_data = m_header.Serialize(); // 假设 FrameHeader 已实现序列化
    frame_data += payload;

    return frame_data;
}

bool WindowUpdateFrame::DeSerialize(std::string_view data) {
    // 校验负载长度必须为 4 字节
    if (data.size() != 4) {
        return false;
    }

    // 解析窗口增量值（忽略最高位的保留位）
    const auto* bytes = reinterpret_cast<const uint8_t*>(data.data());
    uint32_t value = (bytes[0] << 24) | (bytes[1] << 16) | (bytes[2] << 8) | bytes[3];
    m_window_increment = value & 0x7FFFFFFF;  // 取低 31 位

    // 校验窗口增量值必须 >=1
    if (m_window_increment == 0 || m_window_increment > 0x7FFFFFFF) {
        return false;  // 触发 PROTOCOL_ERROR
    }

    return true;
}

std::string WindowUpdateFrame::Serialize()
{
    // 验证窗口增量合法性（RFC 7540 Section 6.9）
    if (m_window_increment < 1 || m_window_increment > 0x7FFFFFFF) {
        throw std::runtime_error("WindowUpdateFrame: Window increment must be 1~2^31-1");
    }

    // 构造负载（4字节大端序的31位无符号整数）
    std::string payload(4, 0);
    const uint32_t value = m_window_increment;
    payload[0] = static_cast<char>((value >> 24) & 0xFF);
    payload[1] = static_cast<char>((value >> 16) & 0xFF);
    payload[2] = static_cast<char>((value >> 8) & 0xFF);
    payload[3] = static_cast<char>(value & 0xFF);

    // 更新帧头部
    m_header.type = FrameType::WINDOW_UPDATE; // 类型为0x8
    m_header.flags = 0;                                            // 无定义标志位
    m_header.length = 4;                                           // 固定负载长度

    // 序列化：头部 + 负载
    std::string frame_data = m_header.Serialize(); // 假设 FrameHeader 已实现序列化
    frame_data += payload;
    return frame_data;
}

bool ContinuationFrame::DeSerialize(std::string_view data) 
{
    m_header_block_fragment = data;
    return true;
}

std::string ContinuationFrame::Serialize()
{
    // 验证流ID非0（必须属于特定流，RFC 7540 Section 6.10）
    if (m_header.stream_id == 0) {
        throw std::runtime_error("ContinuationFrame: Stream ID cannot be 0");
    }

    // 构造负载（仅包含头部块片段）
    std::string payload = m_header_block_fragment;

    // 检查负载长度合法性（24位限制）
    if (payload.size() > 0x00FFFFFF) {
        throw std::runtime_error("ContinuationFrame: Payload exceeds 24-bit limit");
    }

    // 更新帧头部信息
    m_header.type = FrameType::CONTINUATION; // 类型为0x9
    m_header.length = static_cast<uint32_t>(payload.size());       // 负载长度

    // 注意：CONTINUATION 帧的标志位只允许 END_HEADERS (0x4) 和保留位
    // 标志位合法性应在 Validate() 中检查，此处不处理
    // 序列化：头部 + 负载
    std::string frame_data = m_header.Serialize(); // 假设 FrameHeader 已实现序列化
    frame_data += payload;
    return frame_data;
}

bool ContinuationFrame::Validate(Frame* last_frame) const
{
    return StreamId() != 0 && (last_frame->Type() == FrameType::HEADERS || \
            last_frame->Type() == FrameType::CONTINUATION || \
            last_frame->Type() == FrameType::PUSH_PROMISE);
}


}