#ifndef GALAY_HTTP2_H
#define GALAY_HTTP2_H

#include "Protocol.h"
#include <vector>
#include <cstdint>
#include <memory>
#include <queue>
#include <map>
#include <utility>
#include <stdexcept>
#include <unordered_set>
#include <sstream>
#include <iomanip>

	

namespace galay::http2
{

#define SETTINGS_HEADER_TABLE_SIZE          0x1	        //	                   4096字节	    控制HPACK头部压缩表的最大字节数​（双方独立维护各自的表）。
#define SETTINGS_ENABLE_PUSH	            0x2	        //0或1	                1（启用）	  是否允许服务器推送（Server Push）。0表示禁用，1表示启用。
#define SETTINGS_MAX_CONCURRENT_STREAMS	    0x3	        //≥0	                无限制	     接收方允许的最大并发流数量​（Stream ID为奇数的流由服务器发起，偶数为客户端）。
#define SETTINGS_INITIAL_WINDOW_SIZE	    0x4	        //0–2^31-1	            65535字节	流的初始流量控制窗口大小​（影响所有已打开的流）。
#define SETTINGS_MAX_FRAME_SIZE             0x5	        //16384–16777215	    16384字节	接收方愿意接受的最大单帧载荷大小​（以字节为单位）。
#define SETTINGS_MAX_HEADER_LIST_SIZE       0x6         //≥0	                无限制	    接收方允许的请求/响应头部列表的最大字节数​（未压缩前的原始大小）。




class DataFrame;
class HeadersFrame;
class PriorityFrame;
class RstStreamFrame;
class SettingsFrame;
class PushPromiseFrame;
class PingFrame;
class GoAwayFrame;
class WindowUpdateFrame;
class ContinuationFrame;

class FrameFactory
{
public:
    static DataFrame* CreateDataFrame(uint32_t stream_id, bool end_stream, std::string&& data, uint32_t max_payload_length,\
        uint8_t padding_length = 0);
    static HeadersFrame* CreateHeadersFrame(uint32_t stream_id, bool end_stream, std::string&& header_block_fragment, uint32_t max_payload_length,\
        bool priority = false, bool exclusive = false, uint32_t stream_dependency = 0, uint8_t weight = 0, uint8_t padding_length = 0);
    static PriorityFrame* CreatePriorityFrame(uint32_t stream_id, bool exclusive, uint32_t stream_dependency_id, uint8_t weight);
    static RstStreamFrame* CreateRstStreamFrame(uint32_t stream_id, ErrorCode error_code);
    static SettingsFrame* CreateSettingsFrame(uint32_t stream_id, bool ack = false);
    //一次push header包含完毕置end_headers为true
    static PushPromiseFrame* CreatePushPromiseFrame(uint32_t stream_id, uint32_t promised_stream_id, std::string&& header_block_fragment,\
        uint32_t max_payload_length, uint8_t padding_length = 0);
    static PingFrame* CreatePingFrame(uint32_t stream_id, bool ack, std::array<uint8_t, 8> payload);
    static GoAwayFrame* CreateGoAwayFrame(uint32_t stream_id, ErrorCode error_code, std::string&& debug_data);
    static WindowUpdateFrame* CreateWindowUpdateFrame(uint32_t stream_id, uint32_t window_size_increment);
    static ContinuationFrame* CreateContinuationFrame(uint32_t stream_id, bool end_headers, std::string&& header_block_fragment,\
        uint32_t max_payload_length);
};

/*
​**END_STREAM**​	0x01 	DATA, HEADERS	表示当前帧是流的最后一个数据帧（流结束）。
​**END_HEADERS**​	0x04 	HEADERS, CONTINUATION, PUSH_PROMISE	表示当前帧包含完整的头信息块（Header Block），后续无延续帧。
​**PADDED**​	    0x08 	DATA, HEADERS, PUSH_PROMISE	表示帧末尾有填充（Padding），用于混淆数据长度或防止特定攻击。
​**PRIORITY**​	    0x20 	HEADERS	表示帧内包含优先级信息（Stream Dependency 和 Weight）。
​**ACK**​	        0x01 	SETTINGS, PING	确认帧（如 SETTINGS 的 ACK 表示已确认参数生效，PING 的 ACK 表示响应）。
​**COMPRESSED**​	0x01 	ALTSVC（扩展帧）	表示负载经过压缩（非标准帧，部分实现支持）。
*/

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

enum class ErrorCode : uint32_t {
    NO_ERROR          = 0x0,
    PROTOCOL_ERROR    = 0x1,
    INTERNAL_ERROR    = 0x2,
    FLOW_CONTROL_ERROR= 0x3,
    SETTINGS_TIMEOUT  = 0x4,
    STREAM_CLOSED     = 0x5,
    FRAME_SIZE_ERROR  = 0x6,
    REFUSED_STREAM    = 0x7,
    CANCEL            = 0x8,
    COMPRESSION_ERROR = 0x9,
    CONNECT_ERROR     = 0xA,
    ENHANCE_YOUR_CALM = 0xB,
    INADEQUATE_SECURITY=0xC,
    HTTP_1_1_REQUIRED = 0xD
};

enum class SettingIdentifier : uint16_t {
    HEADER_TABLE_SIZE = 0x1,
    ENABLE_PUSH = 0x2,
    MAX_CONCURRENT_STREAMS = 0x3,
    INITIAL_WINDOW_SIZE = 0x4,
    MAX_FRAME_SIZE = 0x5,
    MAX_HEADER_LIST_SIZE = 0x6
};

// 帧头结构
struct FrameHeader {
    uint32_t length;        // 24位长度
    FrameType type;         // 帧类型
    uint8_t flags;          // 标志位
    uint32_t stream_id;     // 流ID（31位）

    void DeSerialize(std::string_view data);
    std::string Serialize();
};

// 基类帧
class Frame {
public:
    Frame(FrameHeader header): m_header(header) {};
    virtual ~Frame() = default;

    // 通用访问接口
    FrameType Type() const { return m_header.type; }
    uint8_t Flags() const { return m_header.flags; }
    uint32_t StreamId() const { return m_header.stream_id; }
    uint32_t Length() const { return m_header.length; }

    // 虚方法用于解析payload（不包括帧头）
    virtual bool DeSerialize(std::string_view data) = 0;
    //包括帧头
    virtual std::string Serialize() = 0;
protected:
    FrameHeader m_header;
};

// DATA帧实现（类型0x0）
// flags:
// 0x01: END_STREAM
// 0x08: PADDED
class DataFrame : public Frame {
    friend DataFrame* FrameFactory::CreateDataFrame(uint32_t stream_id, bool end_stream, std::string&& data, uint32_t max_payload_length, \
        uint8_t padding_length);
public:
    using Frame::Frame;
    
    bool DeSerialize(std::string_view data) override;
    std::string Serialize() override;
    bool IsEndStream() const { return m_header.flags & 0x01; }
    bool IsPadded() const { return m_header.flags & 0x08; }
    std::string_view Data() const { return m_data; }
    size_t DataLength() const { return m_data.length(); }
    uint8_t PaddingLength() const { return m_padding_length; }

private:
    uint32_t m_max_payload_length = 16384;

    std::string m_data;
    uint8_t m_padding_length = 0;
};

class HeadersFrame final : public Frame {
    friend HeadersFrame* FrameFactory::CreateHeadersFrame(uint32_t stream_id, bool end_stream, std::string&& data,\
        uint32_t max_payload_length, bool priority, bool exclusive, uint32_t stream_dependency, uint8_t weight, uint8_t padding_length);
public:
    using Frame::Frame;

    bool DeSerialize(std::string_view data) override;
    std::string Serialize() override;

    // 验证流ID非0（RFC 7540 Section 6.2）
    bool Validate() const { return StreamId() != 0; }
    bool IsEndStream() const { return m_header.flags & 0x01; }
    bool IsPadded() const { return m_header.flags & 0x08; }
    bool IsPriority() const { return m_header.flags & 0x20; }
    bool IsEndHeaders() const { return m_header.flags & 0x04; }
    // 访问器
    uint8_t GetPadLength() const { return m_padding_length; }
    bool IsExclusive() const { return m_exclusive; }
    uint32_t GetStreamDependency() const { return m_stream_dependency; }
    uint8_t GetWeight() const { return m_weight; }
    std::string_view GetHeaderBlockFragment() const { return m_header_block_fragment; }
private:
    // 解析31位无符号整数（最高位保留）
    static uint32_t Parse31Bit(const uint8_t* data) {
        return (data[0] << 24) | (data[1] << 16) | 
               (data[2] << 8) | data[3];
    }
private:
    uint32_t m_max_payload_length = 16384;
    uint8_t m_padding_length = 0;
    bool m_exclusive = false;
    uint32_t m_stream_dependency = 0;
    uint8_t m_weight = 0;
    std::string m_header_block_fragment;
};

class PriorityFrame : public Frame {
    friend PriorityFrame* FrameFactory::CreatePriorityFrame(uint32_t stream_id, bool exclusive, \
        uint32_t stream_dependency, uint8_t weight);
public:
    using Frame::Frame;

    // 实现基类的 ParsePayload 方法
    bool DeSerialize(std::string_view data) override;
    std::string Serialize() override;

    // 访问 PRIORITY 帧的字段
    bool IsExclusive() const { return m_exclusive; }
    uint32_t DependentStreamId() const { return m_dependent_stream_id; }
    uint8_t Weight() const { return m_weight; }

private:
    bool m_exclusive = false;        // Exclusive 标志
    uint32_t m_dependent_stream_id;  // 依赖的流 ID
    uint8_t m_weight;                // 权重（实际值 = 存储值 +1）
};

// RST_STREAM帧实现（展示错误码使用）单个流
class RstStreamFrame : public Frame {
    friend RstStreamFrame* FrameFactory::CreateRstStreamFrame(uint32_t stream_id, ErrorCode error_code);
public:
    using Frame::Frame;

    bool DeSerialize(std::string_view data) override;
    std::string Serialize() override;

    ErrorCode error_code() const { return m_error_code; }

private:
    ErrorCode m_error_code = ErrorCode::NO_ERROR;
};

class SettingsFrame : public Frame {
    friend SettingsFrame* FrameFactory::CreateSettingsFrame(uint32_t stream_id, bool ack);
public:
    using Frame::Frame;

    bool DeSerialize(std::string_view data) override;
    std::string Serialize() override;

    bool IsAck() const { return m_header.flags & 0x1; }
    void SetHeaderTableSize(uint32_t header_table_size) { m_settings[SETTINGS_HEADER_TABLE_SIZE] = header_table_size; }
    void SetEnablePush(bool enable_push) { m_settings[SETTINGS_ENABLE_PUSH] = enable_push ? 1 : 0; }
    void SetMaxConcurrentStreams(uint32_t max_concurrent_streams) { m_settings[SETTINGS_MAX_CONCURRENT_STREAMS] = max_concurrent_streams; }
    void SetInitialWindowSize(uint32_t initial_window_size) { m_settings[SETTINGS_INITIAL_WINDOW_SIZE] = initial_window_size; }
    void SetMaxFrameSize(uint32_t max_frame_size) { m_settings[SETTINGS_MAX_FRAME_SIZE] = max_frame_size; }
    void SetMaxHeaderListSize(uint32_t max_header_list_size) { m_settings[SETTINGS_MAX_HEADER_LIST_SIZE] = max_header_list_size; }
    const std::map<uint16_t, uint32_t>& Settings() const { return m_settings; }

private:
    std::map<uint16_t, uint32_t> m_settings;
};

// --------------------- Push Promise帧（类型0x5）--------------------- 
class PushPromiseFrame final : public Frame {
    friend PushPromiseFrame* FrameFactory::CreatePushPromiseFrame(uint32_t stream_id, uint32_t promised_stream_id, std::string&& data,\
        uint32_t max_payload_length, uint8_t padding_length);
public:
    using Frame::Frame;

    bool DeSerialize(std::string_view data) override;
    std::string Serialize() override;

    bool IsEndHeader() const { return m_header.flags & 0x4; }
    bool IsPadded() const { return m_header.flags & 0x8; }

    // 验证承诺流ID合法性（RFC 7540 Section 6.6）
    bool Validate() const;

    // 访问器
    uint32_t GetPromisedStreamId() const { return m_promised_stream_id; }
    std::string_view GetHeaderBlockFragment() const { 
        return m_header_block_fragment; 
    }
private:
    static uint32_t Parse31Bit(const uint8_t* data) {
        return (data[0] << 24) | (data[1] << 16) | 
               (data[2] << 8) | data[3];
    }

private:
    uint32_t m_max_payload_length = 16384;

    uint8_t m_padding_length = 0;
    uint32_t m_promised_stream_id = 0;
    std::string m_header_block_fragment;
};

class PingFrame : public Frame {
    friend PingFrame* FrameFactory::CreatePingFrame(uint32_t stream_id, bool ack, std::array<uint8_t, 8> data);
public:
    using Frame::Frame;

    // 实现基类方法
    bool DeSerialize(std::string_view data) override;
    std::string Serialize() override;

    // 访问 PING 帧特有字段
    bool IsAck() const { return m_header.flags & 0x01; }  // ACK 标志位
    const std::array<uint8_t, 8>& OpaqueData() const { return m_opaque_data; }
    void OpaqueData(const std::array<uint8_t, 8>& data) { m_opaque_data = data; }

private:
    std::array<uint8_t, 8> m_opaque_data{};  // 固定 8 字节负载
};

// GOAWAY帧实现（类型0x7）整个连接
class GoAwayFrame : public Frame {
    friend GoAwayFrame* FrameFactory::CreateGoAwayFrame(uint32_t stream_id, ErrorCode error_code, std::string&& debug_data);
public:
    using Frame::Frame;

    bool DeSerialize(std::string_view data) override;
    std::string Serialize() override;

    uint32_t LastStreamId() const { return m_last_stream_id; }
    ErrorCode Code() const { return m_error_code; }
    std::string_view DebugData() const { return m_debug_data; }

private:
    uint32_t m_last_stream_id = 0;
    ErrorCode m_error_code = ErrorCode::NO_ERROR;
    std::string m_debug_data;
};


class WindowUpdateFrame : public Frame {
    friend WindowUpdateFrame* FrameFactory::CreateWindowUpdateFrame(uint32_t stream_id, uint32_t window_size_increment);
public:
    using Frame::Frame;
    // 实现基类方法
    bool DeSerialize(std::string_view data) override;
    std::string Serialize() override;
    // 访问窗口增量值
    uint32_t WindowSizeIncrement() const { return m_window_increment; }

private:
    uint32_t m_window_increment = 0;  // 窗口增量值
};

// --------------------- Continuation帧（类型0x9）--------------------- 
class ContinuationFrame final : public Frame {
    friend class HeadersFrame;
    friend class PushPromiseFrame;
    friend ContinuationFrame* FrameFactory::CreateContinuationFrame(uint32_t stream_id, bool end_headers,\
        std::string&& header_block_fragment, uint32_t max_payload_length);
public:
    using Frame::Frame;

    bool DeSerialize(std::string_view data) override;
    std::string Serialize() override;
    bool IsEndHeader() const { return m_header.flags & 0x04; };
    // 验证必须属于同一个流且前序帧合法（RFC 7540 Section 6.10）
    bool Validate(Frame* last_frame) const;
    // 访问器
    std::string_view GetHeaderBlockFragment() const { return m_header_block_fragment; }

private:
    std::string m_header_block_fragment;
};


enum class StreamState {
    IDLE,        // 初始状态，未打开
    OPEN,        // 已打开（已发送或接收 HEADERS 帧）
    RESERVED,    // 保留状态（如 PUSH_PROMISE 预创建）
    CLOSED       // 流已关闭（发送或接收 RST_STREAM 或 END_STREAM 标志）
};




struct Http2Stream
{
    using ptr = std::shared_ptr<Http2Stream>;
    explicit Http2Stream(uint32_t stream_id, uint32_t initial_window_size = 65535)
        : m_state(StreamState::IDLE),
          m_stream_id(stream_id),
          m_recv_window_size(initial_window_size),
          m_send_window_size(initial_window_size) {}

    
    StreamState m_state = StreamState::IDLE;
    uint32_t m_stream_id = 0;
    uint32_t m_recv_window_size = 0;
    uint32_t m_send_window_size = 0;
};

class Http2StreamManager
{
    using ptr = std::shared_ptr<Http2StreamManager>;
    Http2StreamManager();
    std::list<Http2Stream::ptr> ProcessFrames(std::list<Frame*> frames);


    void SendGoAway(ErrorCode code);
    void SendRstStream(uint32_t stream_id, ErrorCode code);


private:
    bool ValidateStreamId(uint32_t stream_id, bool is_client_initiated_stream) const;
private:
    bool m_closed = false;
    uint32_t m_settings_header_table_size = 4096;
    bool m_settings_enable_push = true;
    uint32_t m_settings_max_concurrent_streams = 100;
    uint32_t m_settings_initial_window_size = 65535;
    uint32_t m_max_frame_size = 16384;          
    uint32_t m_max_header_list_size = 16384;    //max header 16k

    uint32_t m_max_stream_id = 0;
    std::unordered_map<uint32_t, std::shared_ptr<Http2Stream>> m_streams;
};


class Http2_0 final : public Request, public Response
{
public:
    Http2_0();
    virtual ~Http2_0();

    virtual std::pair<bool,size_t> DecodePdu(const std::string_view &buffer) override;
    virtual std::string EncodePdu() const override;
    virtual bool HasError() const override;
    virtual int GetErrorCode() const override;
    virtual std::string GetErrorString() override;
    virtual void Reset() override;

    std::list<Frame*> GetRecvFrames();
    void AppendSendFrame(Frame* frame);
    void ClearRecvFrames();
    void ClearSendFrames();
private:
    std::list<Frame*> m_recv_frames;
    std::list<Frame*> m_send_frames;
};



}


#endif