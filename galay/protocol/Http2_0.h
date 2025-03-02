#ifndef GALAY_HTTP2_H
#define GALAY_HTTP2_H

#include "Protocol.h"
#include <vector>
#include <cstdint>
#include <memory>
#include <map>
#include <utility>
#include <stdexcept>

namespace galay::http2
{

// 帧类型枚举
enum class FrameType : uint8_t {
    DATA = 0x00,
    HEADERS = 0x01,
    PRIORITY = 0x02,
    RST_STREAM = 0x03,
    SETTINGS = 0x04,
    PUSH_PROMISE = 0x05,
    PING = 0x06,
    GOAWAY = 0x07,
    WINDOW_UPDATE = 0x08,
    CONTINUATION = 0x09
};

// 帧头结构
struct FrameHeader {
    uint32_t length;        // 24位长度
    FrameType type;         // 帧类型
    uint8_t flags;          // 标志位
    uint32_t stream_id;     // 流ID（31位）
};

// 基类帧
class Frame {
public:
    explicit Frame(FrameHeader header) : m_header(std::move(header)) {}
    virtual ~Frame() = default;

    // 通用访问接口
    FrameType Type() const { return m_header.type; }
    uint8_t Flags() const { return m_header.flags; }
    uint32_t StreamId() const { return m_header.stream_id; }
    uint32_t Length() const { return m_header.length; }

    // 虚方法用于解析payload
    virtual bool ParsePayload(std::string_view data) = 0;

protected:
    FrameHeader m_header;
};

// DATA帧实现（类型0x0）
// flags:
// 0x01: END_STREAM
// 0x08: PADDED
class DataFrame : public Frame {
public:
    using Frame::Frame;
    
    bool ParsePayload(std::string_view data) override;
    std::string_view Data() const { return m_payload; }
    size_t DataLength() const { return m_data_length; }
    uint8_t PaddingLength() const { return m_padding_length; }

private:
    std::string m_payload ;
    size_t m_data_length = 0;
    uint8_t m_padding_length = 0;
};

// SETTINGS帧实现（类型0x4）
/*
​客户端初始化连接时协商参数​

plaintext
客户端 → 服务器：
  SETTINGS帧（flags=0x00，参数列表）
服务器 → 客户端：
  SETTINGS帧（flags=0x01，空负载）  // ACK
​服务器主动更新参数​

plaintext
服务器 → 客户端：
  SETTINGS帧（flags=0x00，参数列表）
客户端 → 服务器：
  SETTINGS帧（flags=0x01，空负载）  // ACK
*/
// flags:
// 0x01: ACK 客户端发送：SETTINGS帧（参数） → 服务器;服务器回复：SETTINGS帧（flags=0x01，无参数） → 客户端


//settings:
/*
​SETTINGS_HEADER_TABLE_SIZE​	    0x1	    ≥0	                4096字节	控制HPACK头部压缩表的最大字节数​（双方独立维护各自的表）。
​SETTINGS_ENABLE_PUSH​	            0x2	    0或1	            1（启用）	是否允许服务器推送（Server Push）。0表示禁用，1表示启用。
​SETTINGS_MAX_CONCURRENT_STREAMS​	0x3	    ≥0	                无限制	    接收方允许的最大并发流数量​（Stream ID为奇数的流由服务器发起，偶数为客户端）。
​SETTINGS_INITIAL_WINDOW_SIZE​	    0x4	    0–2^31-1	        65535字节	流的初始流量控制窗口大小​（影响所有已打开的流）。
​SETTINGS_MAX_FRAME_SIZE​	        0x5	    16384–16777215	    16384字节	接收方愿意接受的最大单帧载荷大小​（以字节为单位）。
​SETTINGS_MAX_HEADER_LIST_SIZE​	    0x6	    ≥0	                无限制	    接收方允许的请求/响应头部列表的最大字节数​（未压缩前的原始大小）。
*/
class SettingsFrame : public Frame {
public:
    using Frame::Frame;

    bool ParsePayload(std::string_view data) override;
    const std::map<uint16_t, uint32_t>& settings() const { return m_settings; }

private:
    std::map<uint16_t, uint32_t> m_settings;
};


class Http2_0 final : public Request, public Response
{
public:
    Http2_0() = default;
    virtual ~Http2_0();

    virtual std::pair<bool,size_t> DecodePdu(const std::string_view &buffer) override;
    virtual std::string EncodePdu() const override;
    virtual bool HasError() const override;
    virtual int GetErrorCode() const override;
    virtual std::string GetErrorString() override;
    virtual void Reset() override;


    Frame* GetStreamFrame() {
        Frame* frame = m_frame;
        m_frame = nullptr;
        return frame;
    }
private:
    Frame* m_frame;
};




}


#endif