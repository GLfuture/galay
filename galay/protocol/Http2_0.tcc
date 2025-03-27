#ifndef HTTP2_0_TCC
#define HTTP2_0_TCC

#include "Http2_0.hpp"

namespace galay::http2 
{

template <FrameBaseTypeConcept T>
inline T *Frame::ConvertTo()
{
    assert(m_header.type == T::flag_type);
    return static_cast<T*>(this);
}

template <typename SocketType>
template <typename CoRtn>
inline AsyncResult<Coroutine<ErrorCode>::ptr, CoRtn> Http2Stream<SocketType>::SendHeader(std::string&& header_block, bool end_headers\
    , Priority priority, uint8_t padding_length)
{
    if(m_manager->m_co_scheduler) {
        return this_coroutine::WaitAsyncRtnExecute<Coroutine<ErrorCode>::ptr, CoRtn>(sendFrame(\
            RoutineCtx::Create(m_manager->m_scheduler, m_manager->m_co_scheduler),
            m_manager->m_connection,
            FrameFactory::CreateHeadersFrame(m_stream_id, end_headers, std::move(header_block), m_max_frame_size, priority, padding_length)
        ));
    }
    return this_coroutine::WaitAsyncRtnExecute<Coroutine<ErrorCode>::ptr, CoRtn>(sendFrame(RoutineCtx::Create(m_manager->m_scheduler), 
        m_manager->m_connection,
        FrameFactory::CreateHeadersFrame(m_stream_id, end_headers, std::move(header_block), m_max_frame_size, priority, padding_length)
    ));
}

template <typename SocketType>
template <typename CoRtn>
inline AsyncResult<Coroutine<ErrorCode>::ptr, CoRtn> Http2Stream<SocketType>::SendData(std::string&& data, bool end_stream, uint8_t padding_length)
{
    if(m_manager->m_co_scheduler) {
        return this_coroutine::WaitAsyncRtnExecute<Coroutine<ErrorCode>::ptr, CoRtn>(sendFrame(\
            RoutineCtx::Create(m_manager->m_scheduler, m_manager->m_co_scheduler),
            m_manager->m_connection,
            FrameFactory::CreateDataFrame(m_stream_id, end_stream, std::move(data), m_max_frame_size, padding_length)
        ));
    }
    return this_coroutine::WaitAsyncRtnExecute<Coroutine<ErrorCode>::ptr, CoRtn>(sendFrame(RoutineCtx::Create(m_manager->m_scheduler), 
        m_manager->m_connection,
        FrameFactory::CreateDataFrame(m_stream_id, end_stream, std::move(data), m_max_frame_size, padding_length)
    ));
}

template <typename SocketType>
template <typename CoRtn>
inline AsyncResult<Coroutine<ErrorCode>::ptr, CoRtn> Http2Stream<SocketType>::WindowUpdate(uint32_t window_size)
{
    InCreaseThisWindowSize(window_size);
    if(m_manager->m_co_scheduler) {
        return this_coroutine::WaitAsyncRtnExecute<Coroutine<ErrorCode>::ptr, CoRtn>(sendFrame(\
            RoutineCtx::Create(m_manager->m_scheduler, m_manager->m_co_scheduler),
            m_manager->m_connection,
            FrameFactory::CreateWindowUpdateFrame(m_stream_id, window_size)
        ));
    }
    return this_coroutine::WaitAsyncRtnExecute<Coroutine<ErrorCode>::ptr, CoRtn>(sendFrame(RoutineCtx::Create(m_manager->m_scheduler), 
        m_manager->m_connection,
        FrameFactory::CreateWindowUpdateFrame(m_stream_id, window_size)
    ));
}

template <typename SocketType>
template <typename CoRtn>
inline AsyncResult<Coroutine<ErrorCode>::ptr, CoRtn> Http2Stream<SocketType>::Close()
{
    RstStreamFrame* frame = FrameFactory::CreateRstStreamFrame(m_stream_id, ErrorCode::CANCEL);
    m_state = StreamState::CLOSED;
}

template <typename SocketType>
inline Http2StreamInitialRole Http2Stream<SocketType>::GetRole() const
{
    return m_role;
}

template <typename SocketType>
inline uint32_t Http2Stream<SocketType>::GetStreamId() const
{
    return m_stream_id;
}

template <typename SocketType>
inline uint32_t Http2Stream<SocketType>::GetDependencyStreamId() const
{
    return m_dependency_stream_id;
}

template <typename SocketType>
inline uint32_t Http2Stream<SocketType>::GetWeight() const
{
    return m_weight;
}

template <typename SocketType>
inline uint32_t Http2Stream<SocketType>::GetPeerWindowSize() const
{
    return m_peer_window_size;
}

template <typename SocketType>
inline uint32_t Http2Stream<SocketType>::GetThisWindowSize() const
{
    return m_this_window_size;
}

template <typename SocketType>
inline void Http2Stream<SocketType>::InCreasePeerWindowSize(uint32_t size)
{
    m_peer_window_size += size;
}

template <typename SocketType>
inline void Http2Stream<SocketType>::InCreaseThisWindowSize(uint32_t size)
{
    m_this_window_size += size;
}

template <typename SocketType>
inline bool Http2Stream<SocketType>::DeCreasePeerWindowSize(uint32_t size)
{
    if( m_peer_window_size < size ) return false;
    m_peer_window_size -= size;
    return true;
}

template <typename SocketType>
inline bool Http2Stream<SocketType>::DeCreaseThisWindowSize(uint32_t size)
{
    if( m_this_window_size < size ) return false;
    m_this_window_size -= size;
    return true;
}

template <typename SocketType>
inline bool Http2Stream<SocketType>::IsExclusive() const
{
    return m_exclusive;
}


template <typename SocketType>
inline DataFrame::uptr Http2Stream<SocketType>::ConsumeData()
{
    auto ptr = m_to_consume_frames.front();
    m_to_consume_frames.pop_front();
    return std::move(ptr);
}


template <typename SocketType>
inline bool Http2Stream<SocketType>::ProcessStreamFrames(Frame *frame)
{
    bool res = false;
    switch (frame->Type())
    {
    case FrameType::HEADERS:
        res = ProcessHeadersFrame(frame->ConvertTo<HeadersFrame>());
        break;
    case FrameType::CONTINUATION:
        res = ProcessContinuationFrame(frame->ConvertTo<ContinuationFrame>());
        break;
    case FrameType::DATA:
        res = ProcessDataFrame(frame->ConvertTo<DataFrame>());
        break;
    case FrameType::RST_STREAM:
        res = ProcessRstStreamFrame(frame->ConvertTo<RstStreamFrame>());
        break;
    case FrameType::PUSH_PROMISE:
        res = ProcessPushPromiseFrame(frame->ConvertTo<PushPromiseFrame>());
        break;
    default:
        break;
    }
    return res;
}

template <typename SocketType>
inline bool Http2Stream<SocketType>::ProcessHeadersFrame(HeadersFrame *frame)
{
    std::unique_ptr<HeadersFrame> frame_ptr(frame);
    m_header_block_fragment.clear();
    if(frame->IsEndHeaders()) {
        m_headers = m_hpack.decodeHeaderBlock(stringviewToUint8(frame->GetHeaderBlockFragment()));
    } else {
        m_header_block_fragment += frame->GetHeaderBlockFragment();
    }
    return true;
}

template <typename SocketType>
inline bool Http2Stream<SocketType>::ProcessContinuationFrame(ContinuationFrame *frame)
{
    std::unique_ptr<HeadersFrame> frame_ptr(frame);
    if(frame->IsEndHeader()) {
        m_header_block_fragment += frame->GetHeaderBlockFragment();
        m_headers = m_hpack.decodeHeaderBlock(stringToUint8(m_header_block_fragment));
    } else {
        m_header_block_fragment += frame->GetHeaderBlockFragment();
    }
    return true;
}

template <typename SocketType>
inline bool Http2Stream<SocketType>::ProcessDataFrame(DataFrame *frame)
{
    std::unique_ptr<DataFrame> frame_ptr(frame);
    m_to_consume_frames.push_back(std::move(frame_ptr));
    return true;
}

template <typename SocketType>
inline bool Http2Stream<SocketType>::ProcessRstStreamFrame(RstStreamFrame *frame)
{
    return false;
}

template <typename SocketType>
inline bool Http2Stream<SocketType>::ProcessPushPromiseFrame(PushPromiseFrame *frame)
{
    return false;
}

template <typename SocketType>
inline Http2PriorityTree<SocketType>::Node *Http2PriorityTree<SocketType>::FindNode(uint32_t stream_id)
{
    auto it = m_nodes.find(stream_id);
    return it != m_nodes.end() ? it->second : nullptr;
}

template <typename SocketType>
inline bool Http2PriorityTree<SocketType>::IsAncestor(Node *node, Node *target)
{
    while (target != &m_root) {
        if (target == node) return true;
        target = target->parent;
    }
    return false;
}


template <typename SocketType>
inline void Http2PriorityTree<SocketType>::ReparentNode(Node *node, Node *new_parent)
{
    if (node->parent) { node->parent->children.erase(node); }
    node->parent = new_parent;
    new_parent->children.insert(node).second;
}

template <typename SocketType>
inline Http2PriorityTree<SocketType>::Node *Http2PriorityTree<SocketType>::GetEffectiveParent(Node *parent, bool exclusive)
{
    if (exclusive) return parent;
    for (auto child : parent->children) {
        if (child->exclusive) return GetEffectiveParent(child, false);
    }
    return parent;
}

template <typename SocketType>
inline Http2PriorityTree<SocketType>::Http2PriorityTree()
{
    m_nodes.emplace(0, &m_root);
}

template <typename SocketType>
inline Http2PriorityTree<SocketType>::~Http2PriorityTree()
{
    for (auto& [id, node] : m_nodes) {
        if(id != 0u) delete node;
    }
}

template <typename SocketType>
inline Http2PriorityTree<SocketType>::Node *Http2PriorityTree<SocketType>::GetNode(uint32_t stream_id)
{
    auto it = m_nodes.find(stream_id);
    return it != m_nodes.end() ? it->second : nullptr;
}

template <typename SocketType>
inline bool Http2PriorityTree<SocketType>::AddStream(Http2Stream<SocketType> *stream, uint32_t parent_id, uint32_t weight, bool exclusive)
{
    if (m_nodes.count(stream->m_stream_id)) return false;
    Node* parent = FindNode(parent_id);
    if (!parent) return false;

    parent = GetEffectiveParent(parent, exclusive);

    Node* newNode = new Node(stream, weight, exclusive);
    m_nodes[stream->m_stream_id] = newNode;

    if (exclusive) {
        std::vector<Node*> old_children(parent->children.begin(), parent->children.end());
        for (Node* child : old_children) {
            ReparentNode(child, newNode);
        }
    }
    ReparentNode(newNode, parent);
    return true;
}

template <typename SocketType>
inline bool Http2PriorityTree<SocketType>::UpdatePriority(uint32_t stream_id, uint32_t new_parent_id, uint32_t new_weight, bool exclusive)
{
    Node* node = FindNode(stream_id);
    Node* new_parent = FindNode(new_parent_id);
    if (!node || !new_parent) return false;
    if (IsAncestor(node, new_parent)) return false;
    new_parent = GetEffectiveParent(new_parent, exclusive);
    node->parent->children.erase(node);
    node->weight = std::clamp(new_weight, 1u, 256u);
    node->exclusive = exclusive;
    if (exclusive) {
        std::vector<Node*> old_children(new_parent->children.begin(), new_parent->children.end());
        for (Node* child : old_children) {
            ReparentNode(child, node);  // 将新父节点的子节点移动到当前节点下
        }
    }
    ReparentNode(node, new_parent);
    return true;
}

template <typename SocketType>
inline bool Http2PriorityTree<SocketType>::RemoveStream(uint32_t stream_id)
{
    auto it = m_nodes.find(stream_id);
    if (it == m_nodes.end()) return false;

    Node* node = it->second;
    std::vector<Node*> children(node->children.begin(), node->children.end());
    for (auto* child : children) {
        ReparentNode(child, node->parent);
    }
    node->parent->children.erase(node);
    m_nodes.erase(it);
    delete node;
    return true;
}

template <typename SocketType>
inline bool Http2PriorityTree<SocketType>::SetActive(uint32_t stream_id, bool active)
{
    auto it = m_nodes.find(stream_id);
    if (it == m_nodes.end()) return false;
    it->second->active = active;
    return true;
}

template <typename SocketType>
inline std::list<Http2Stream<SocketType> *> Http2PriorityTree<SocketType>::GetScheduled()
{
    std::list<Http2Stream<SocketType>*> result;
    std::function<void(Node*)> dfs = [&](Node* node) {
        if (node != &m_root && node->active) {
            result.push_back(node->stream);
        }
        for (auto child : node->children) {
            dfs(child);
        }
    };
    dfs(&m_root);
    return result;
}

template <typename SocketType>
inline Http2StreamManager<SocketType>::Http2StreamManager(typename Connection<SocketType>::ptr connection, bool is_client)
    :m_connection(connection)
{
    m_cur_stream_id = (is_client ? 1 : 2);
}

template <typename SocketType>
inline typename Http2Stream<SocketType>::ptr Http2StreamManager<SocketType>::CreateNewStream()
{
    Http2StreamInitialRole role = m_cur_stream_id % 2 == 0 ? Http2StreamInitialRole::SERVER : Http2StreamInitialRole::CLIENT;
    return std::make_shared<Http2Stream<SocketType>>(this, role, GenNewStreamId(), 0, 0, false, 0, 0);
}

template <typename SocketType>
inline std::pair<size_t, std::list<typename Http2Stream<SocketType>::ptr>> Http2StreamManager<SocketType>::ConsumeRecvFrames(const std::string_view &buffer)
{
    size_t total_length = 0;
    std::list<Frame*> frames;
    while(true) {
        std::string_view data = buffer.substr(total_length);
        if (data.length() < 9) break;
        // 解析帧头
        FrameHeader header;
        header.DeSerialize(data.substr(0, 9));

        const size_t frame_length = 9 + header.length;
        if (data.length() < frame_length) break;
        total_length += frame_length;
        // 创建对应帧对象
        Frame* frame = nullptr;
        try {
            switch (header.type) {
                case FrameType::DATA:
                    frame = new DataFrame(header);
                    break;
                case FrameType::HEADERS:
                    frame = new HeadersFrame(header);
                    break;
                case FrameType::PRIORITY:
                    frame = new PriorityFrame(header);
                    break;
                case FrameType::RST_STREAM:
                    frame = new RstStreamFrame(header);
                    break;
                case FrameType::SETTINGS:
                    frame = new SettingsFrame(header);
                    break;
                case FrameType::PUSH_PROMISE:
                    frame = new PushPromiseFrame(header);
                    break;
                case FrameType::PING:
                    frame = new PingFrame(header);
                    break;
                case FrameType::GOAWAY:
                    frame = new GoAwayFrame(header);
                    break;
                case FrameType::WINDOW_UPDATE:
                    frame = new WindowUpdateFrame(header);
                    break;
                case FrameType::CONTINUATION:
                    frame = new ContinuationFrame(header);
                    break;
                default:
                    return {-1, {}}; // 不支持的帧类型
            }
        } catch (...) {
            return {-1, {}};;
        }

        // 解析payload
        if (!frame->DeSerialize(data.substr(9, header.length))) {
            delete frame;
            frame = nullptr;
        } else {
            frames.push_back(frame);
        }
    }
    return {total_length, ProcessFrames(frames)};
}

template <typename SocketType>
inline std::list<typename Http2Stream<SocketType>::ptr>
Http2StreamManager<SocketType>::ProcessFrames(std::list<Frame*> frames)
{
    std::list<typename Http2Stream<SocketType>::ptr> affected_streams;
    for (Frame* frame :frames) {
        // setting帧
        const uint32_t stream_id = frame->StreamId();
        switch (frame->Type()) {
            case FrameType::SETTINGS: {
                if(!ProcessSettingsFrame(frame->ConvertTo<SettingsFrame>())) {
                    delete frame;
                }
                break;
            }
            // PING帧（连接保活）
            case FrameType::PING: {
                if(ProcessPingFrame(frame->ConvertTo<PingFrame>())) {
                    delete frame;
                }
                break;
            }
            // GOAWAY帧（连接终止）
            case FrameType::GOAWAY: {
                if(!ProcessGoAwayFrame(frame->ConvertTo<GoAwayFrame>())) {
                    delete frame;
                }
                if (m_close_callback) m_close_callback();
                break;
            }
            // WINDOW_UPDATE 窗口更新
            case FrameType::WINDOW_UPDATE: {
                if(!ProcessWindowUpdateFrame(frame->ConvertTo<WindowUpdateFrame>())) {
                    delete frame;
                }
                break;
            }
            case FrameType::PRIORITY: {
                if(!ProcessPriorityFrame(frame->ConvertTo<PriorityFrame>())) {
                    delete frame;
                }
                break;
            }
            default: {
                std::shared_ptr<Http2Stream<SocketType>> stream;
                auto stream_it = m_streams.find(stream_id);
                if (stream_it == m_streams.end()) {
                    stream = CreateStreamFromFrame(frame);
                    if( !stream ) {
                        delete frame;
                        break;
                    }
                } else {
                    stream = stream_it->second;
                }
                if(stream->ProcessStreamFrames(frame)) {
                    affected_streams.push_back(stream);
                } 
                break;
            }
        }
    }
    return affected_streams;

}

template <typename SocketType>
inline bool Http2StreamManager<SocketType>::ProcessSettingsFrame(SettingsFrame *frame)
{
    if( frame->StreamId() != 0) {
        SendGoAway(ErrorCode::PROTOCOL_ERROR);
        return false;
    }
    for (auto& setting : frame->Settings()) {
        switch (setting.first) {
            case SettingIdentifier::HEADER_TABLE_SIZE: {
                if (setting.second > std::numeric_limits<int32_t>::max()) {
                    SendGoAway(ErrorCode::FLOW_CONTROL_ERROR);
                    return false;
                }
                m_settings_header_table_size = setting.second;
                break;
            }
            case SettingIdentifier::ENABLE_PUSH: {
                m_settings_enable_push = (setting.second == 1);
                break;
            }
            case SettingIdentifier::MAX_CONCURRENT_STREAMS: {
                if (setting.second > std::numeric_limits<int32_t>::max()) {
                    SendGoAway(ErrorCode::FLOW_CONTROL_ERROR);
                    return false;
                }
                m_settings_max_concurrent_streams = setting.second;
                break;
            }
            case SettingIdentifier::INITIAL_WINDOW_SIZE: {
                if (setting.second > std::numeric_limits<int32_t>::max()) {
                    SendGoAway(ErrorCode::FLOW_CONTROL_ERROR);
                    return false;
                }
                m_settings_initial_peer_window_size = setting.second;
                break;
            }
            case SettingIdentifier::MAX_FRAME_SIZE: {
                if (setting.second > std::numeric_limits<int32_t>::max()) {
                    SendGoAway(ErrorCode::FLOW_CONTROL_ERROR);
                    return false;
                }
                m_settings_max_frame_size = setting.second;
                break;
            }
            case SettingIdentifier::MAX_HEADER_LIST_SIZE: {
                if (setting.second > std::numeric_limits<int32_t>::max()) {
                    SendGoAway(ErrorCode::FLOW_CONTROL_ERROR);
                    return false;
                }
                m_settings_max_header_list_size = setting.second;
                break;
            }
            default: {
                return false;
            }
        }
    }
    return true;
}

template <typename SocketType>
inline bool Http2StreamManager<SocketType>::ProcessPingFrame(PingFrame *frame)
{
    if (frame->StreamId() != 0) { // PING必须在流0
        SendGoAway(ErrorCode::PROTOCOL_ERROR);
        return false;
    }
    auto ping_frame = dynamic_cast<PingFrame*>(frame);
    if (!ping_frame->IsAck()) {
        SendPing(ping_frame, true);
    }
    m_last_ping_time = std::chrono::steady_clock::now();
    return true;
}

template <typename SocketType>
inline bool Http2StreamManager<SocketType>::ProcessGoAwayFrame(GoAwayFrame *frame)
{
    m_cur_stream_id = frame->LastStreamId();
    for (auto& [id, s] : m_streams) {
        if (id > m_cur_stream_id) {
            s->Close();
        }
    }
    m_goaway = true;
    return true;
}

template <typename SocketType>
inline bool Http2StreamManager<SocketType>::ProcessPriorityFrame(PriorityFrame *frame)
{
    auto it = m_streams.find(frame->StreamId());
    if (it == m_streams.end()) {
        SendGoAway(frame->StreamId(), ErrorCode::PROTOCOL_ERROR, "Invalid stream for priority update");
        return false;
    }
    auto stream = it->second;
    stream->m_dependency_stream_id = frame->DependentStreamId();
    stream->m_weight = frame->Weight();
    stream->m_exclusive = frame->IsExclusive();
    if(!m_priority_tree.UpdatePriority(frame->StreamId(), frame->DependentStreamId(), frame->Weight(), frame->IsExclusive())) {
        SendGoAway(frame->StreamId(), ErrorCode::PROTOCOL_ERROR, "Priority tree update failed");
        return false;
    }
    return true;
}

template <typename SocketType>
inline bool Http2StreamManager<SocketType>::ProcessWindowUpdateFrame(WindowUpdateFrame *frame)
{
    if(frame->StreamId() == 0) {
        for(auto& [streamID, stream] : m_streams) {
            stream->InCreasePeerWindowSize(frame->WindowSizeIncrement());
        } 
    } else {
        auto stream_it = m_streams.find(frame->StreamId());
        if(stream_it != m_streams.end()) {
            stream_it.second->InCreasePeerWindowSize(frame->WindowSizeIncrement());
        } else {
            SendGoAway(m_cur_stream_id, ErrorCode::PROTOCOL_ERROR, "Invalid stream for window update");
            return false;
        }
    }
    return true;
}

template <typename SocketType>
inline typename Http2Stream<SocketType>::ptr Http2StreamManager<SocketType>::CreateStreamFromFrame(Frame *frame)
{
    typename Http2Stream<SocketType>::ptr stream;
    uint32_t stream_id = frame->StreamId();
    switch (frame->Type()) {
        case FrameType::HEADERS: { // 客户端发起的流
            auto header_frame = static_cast<HeadersFrame*>(frame);
            stream = std::make_shared<Http2Stream<SocketType>>(this, Http2StreamInitialRole::CLIENT, stream_id, header_frame->GetStreamDependency()\
                , header_frame->GetWeight(), header_frame->IsExclusive(), m_settings_initial_this_window_size, m_settings_initial_peer_window_size
                , header_frame->GetMaxPayloadLength());
            stream->m_state = StreamState::OPEN;
            this->m_streams[stream_id] = stream;
            m_priority_tree.AddStream(stream.get(), header_frame->GetStreamDependency(), header_frame->GetWeight(), header_frame->IsExclusive());
            break;
        }
        case FrameType::PUSH_PROMISE: { // 服务端承诺的流
            auto main_stream_it = m_streams.find(stream_id);
            if (main_stream_it == m_streams.end()) {
                SendGoAway(ErrorCode::PROTOCOL_ERROR);
                return nullptr;
            }
            auto main_stream = main_stream_it->second;
            if (auto pp_frame = dynamic_cast<PushPromiseFrame*>(frame)) {
                const uint32_t promised_id = pp_frame->GetPromisedStreamId();
                if (!ValidateStreamId(promised_id, false)) { // 服务端流需为偶数
                    SendGoAway(ErrorCode::PROTOCOL_ERROR);
                    return nullptr;
                }
                auto stream = std::make_shared<Http2Stream<SocketType>>(this, Http2StreamInitialRole::SERVER, promised_id, main_stream->GetStreamId() \
                    ,stream_id, main_stream->GetWeight(), main_stream->IsExclusive(), main_stream->GetThisWindowSize(), main_stream->GetPeerWindowSize()
                    , m_settings_max_frame_size);
                stream->m_state = StreamState::RESERVED;
                m_streams[promised_id] = stream;
                m_priority_tree.AddStream(stream.get(), stream_id, main_stream->GetWeight(), main_stream->IsExclusive());
            }
            break;
        }
        default: { // 其他帧类型需关联已存在的流
            SendGoAway(ErrorCode::PROTOCOL_ERROR);
            return nullptr;
        }
    }
    return stream;
}


template <typename SocketType>
inline void Http2StreamManager<SocketType>::SendPing(PingFrame *frame, bool ack)
{
    frame->m_header.flags |= (ack ? 0x1 : 0);
    AppendFrame(frame);
}

template <typename SocketType>
inline void Http2StreamManager<SocketType>::SendSettings(SettingsFrame *frame)
{
    if(frame->m_settings.find(SettingIdentifier::INITIAL_WINDOW_SIZE) != frame->m_settings.end()) {
        this->m_settings_initial_this_window_size = frame->m_settings[SettingIdentifier::INITIAL_WINDOW_SIZE];
    }
    AppendFrame(frame);
}

template <typename SocketType>
inline void Http2StreamManager<SocketType>::SendGoAway(uint32_t last_stream_id, ErrorCode code, std::string&& debug_data)
{
    auto frame = FrameFactory::CreateGoAwayFrame(last_stream_id, code, std::move(debug_data));
    AppendFrame(frame);
}

template <typename SocketType>
inline void Http2StreamManager<SocketType>::AppendFrame(Frame *frame)
{
    this->m_outgoing_frames.enqueue(std::unique_ptr<Frame>(frame));
    this->m_send_coroutine.lock()->GetCoScheduler()->ToResumeCoroutine(this->m_send_coroutine);
}


template <typename SocketType>
inline uint32_t Http2StreamManager<SocketType>::GenNewStreamId()
{
    this->m_cur_stream_id += 2;
    return this->m_cur_stream_id;
}

template <typename SocketType>
inline bool Http2StreamManager<SocketType>::ValidateStreamId(uint32_t stream_id, bool is_client_initiated_stream) const
{
    if (stream_id == 0) {
        return false;
    }
    bool is_even = (stream_id % 2 == 0);
    if (is_client_initiated_stream) {
        if (is_even) {
            return false;
        }
    } else {
        if (!is_even) {
            return false;
        }
    }
    if (!m_streams.empty()) {
        if (stream_id <= m_max_stream_id) {
            return false; 
        }
    }

    return true;
}

}


#endif

