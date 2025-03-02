#include "Http2_0.h"

namespace galay::http2 
{

bool DataFrame::ParsePayload(std::string_view data)
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

    m_data_length = m_header.length - (offset + m_padding_length);
    m_payload = data.substr(offset, m_data_length);
    return true;
}


bool SettingsFrame::ParsePayload(std::string_view data)
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

        // 检查参数值合法性
        switch (id) {
            case 0x1:  // HEADER_TABLE_SIZE
                // 无范围限制（但实现可定义上限）
                break;
            case 0x2:  // ENABLE_PUSH
                if (value != 0 && value != 1) {
                    return false;  // 非法值
                }
                break;
            case 0x3:  // MAX_CONCURRENT_STREAMS
                // 无范围限制
                break;
            case 0x4:  // INITIAL_WINDOW_SIZE
                if (value > 0x7FFFFFFF) {  // 最大允许2^31-1
                    return false;
                }
                break;
            case 0x5:  // MAX_FRAME_SIZE
                if (value < 16384 || value > 16777215) {
                    return false;
                }
                break;
            case 0x6:  // MAX_HEADER_LIST_SIZE
                // 无范围限制
                break;
            default:
                break;
        }

        m_settings[id] = value;
    }
    return true;
}


std::pair<bool, size_t> Http2_0::DecodePdu(const std::string_view &data)
{
    if (data.length() < 9) return {false, 0};

    // 解析帧头
    FrameHeader header;
    header.length = (data[0] << 16) | (data[1] << 8) | data[2];
    header.type = static_cast<FrameType>(data[3]);
    header.flags = data[4];
    header.stream_id = (data[5] << 24) | (data[6] << 16) 
                        | (data[7] << 8) | data[8];
    header.stream_id &= 0x7FFFFFFF;

    const size_t total_length = 9 + header.length;
    if (data.length() < total_length) return {false, 0};

    // 创建对应帧对象
    try {
        switch (header.type) {
            case FrameType::DATA:
                m_frame = new DataFrame(header);
                break;
            case FrameType::SETTINGS:
                m_frame = new SettingsFrame(header);
                break;
            case FrameType::GOAWAY:
                m_frame = new GoAwayFrame(header);
                break;
            //to do
            default:
                return {false, 0}; // 不支持的帧类型
        }
    } catch (...) {
        return {false, 0};
    }

    // 解析payload
    if (!m_frame->ParsePayload(data.substr(9, header.length))) {
        delete m_frame;
        m_frame = nullptr;
        return {false, 0};
    }

    return {true, total_length};
}

Http2_0::~Http2_0()
{
    if(m_frame) {
        delete m_frame;
        m_frame = nullptr;
    }
}


}