#ifndef GALAY_LOGENTRY_HPP
#define GALAY_LOGENTRY_HPP

#include <string>
#include "galay/kernel/ExternApi.hpp"

namespace galay::raft
{

// class LogEntry
// {
// public:
//     LogEntry(int index, int term, const std::string& command)
//         : index(index), term(term), command(command) {}

//     // 访问器
//     int getIndex() const { return index; }
//     int getTerm() const { return term; }
//     const std::string& getCommand() const { return command; }

//     // 修改器
//     void setIndex(int index) { this->index = index; }
//     void setTerm(int term) { this->term = term; }
//     void setCommand(const std::string& command) { this->command = command; }

// private:
//     int index;              // 日志项的索引
//     int term;               // 日志项被创建时的任期号
//     std::string command;    // 日志项中存储的命令
// };

// class RaftNode
// {
// public:
//     enum class NodeState { Follower, Candidate, Leader };
    
//     RaftNode(int nodeId, int electionTimeout, int heartbeatInterval)
//         : nodeId_(nodeId), electionTimeout_(electionTimeout),
//           heartbeatInterval_(heartbeatInterval), currentTerm_(0),
//           votedFor_(-1), commitIndex_(0), lastApplied_(0),
//           state_(NodeState::Follower), running_(false) {}
          
//     void start() {
//         running_ = true;
//         serverThread_ = std::thread(&RaftNode::run, this);
//     }
    
//     void stop() {
//         running_ = false;
//         if (serverThread_.joinable()) {
//             serverThread_.join();
//         }
//     }
    
//     void requestVote();
//     void appendEntries();
//     void becomeLeader();
//     void becomeFollower(int term);
    
// private:
//     void run();
//     void startElection();
//     void sendHeartbeat();
//     void applyLogEntries();
    
//     const int nodeId_;
//     int currentTerm_;
//     int votedFor_;
//     int commitIndex_;
//     int lastApplied_;
//     std::atomic<NodeState> state_;
//     int electionTimeout_;
//     const int heartbeatInterval_;
//     std::unordered_map<int, THost> peers_;
//     std::vector<std::string> log_;

//     std::mutex lock_;
//     std::atomic<bool> running_;
//     std::thread serverThread_;
//     int votesReceived_ = 0;
// };  

}

#endif