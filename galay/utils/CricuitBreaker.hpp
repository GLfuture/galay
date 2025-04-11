#ifndef GALAY_CRICUITBREAKER_H
#define GALAY_CRICUITBREAKER_H

#include <atomic>
#include <chrono>
#include <cmath>
#include <mutex>
#include <shared_mutex>
#include <vector>
#include <thread>

namespace galay::utils 
{

class CircuitBreaker 
{
public:
    enum class State { CLOSED, OPEN, HALF_OPEN };

    struct Config {
        uint32_t failure_threshold = 5;      // 熔断触发失败次数
        uint32_t success_threshold = 3;      // 半开状态成功次数
        std::chrono::milliseconds reset_timeout = std::chrono::seconds(30); // 熔断恢复时间
        double slow_threshold = 1.0;         // 慢调用阈值（秒）
        uint32_t sliding_window_size = 10;   // 滑动窗口大小
    };

    explicit CircuitBreaker(Config config) 
        : config_(std::move(config)),
            state_(State::CLOSED),
            failure_count_(0),
            success_count_(0),
            last_failure_time_(TimePoint::min()),
            slow_calls_(0) {}

    bool allowRequest() {
        const auto now = currentTime();
        
        std::shared_lock lock(mutex_);
        if (state_ == State::OPEN) {
            return now - last_failure_time_ > config_.reset_timeout;
        }
        return true;
    }

    void onSuccess() {
        std::unique_lock lock(mutex_);
        if (state_ == State::HALF_OPEN) {
            if (++success_count_ >= config_.success_threshold) {
                resetToClosed();
            }
        }
        recordMetric(true, 0);
    }

    void onFailure() {
        std::unique_lock lock(mutex_);
        if (state_ == State::HALF_OPEN) {
            tripToOpen();
            return;
        }
        recordMetric(false, 0);
        if (shouldTrip()) {
            tripToOpen();
        }
    }

    void onSlowCall(double duration) {
        std::unique_lock lock(mutex_);
        if (duration > config_.slow_threshold) {
            slow_calls_++;
        }
        recordMetric(true, duration);
        if (shouldTrip()) {
            tripToOpen();
        }
    }

private:
    using Clock = std::chrono::steady_clock;
    using TimePoint = std::chrono::time_point<Clock>;

    struct Metric {
        bool success;
        double duration;
        TimePoint timestamp;
    };

    Config config_;
    State state_;
    uint32_t failure_count_;
    uint32_t success_count_;
    uint32_t slow_calls_;
    TimePoint last_failure_time_;
    std::vector<Metric> metrics_;
    mutable std::shared_mutex mutex_;

    static TimePoint currentTime() { return Clock::now(); }

    void resetToClosed() {
        state_ = State::CLOSED;
        failure_count_ = 0;
        success_count_ = 0;
        slow_calls_ = 0;
        metrics_.clear();
    }

    void tripToOpen() {
        state_ = State::OPEN;
        last_failure_time_ = currentTime();
        std::this_thread::sleep_for(config_.reset_timeout); // 自动恢复定时
        std::unique_lock lock(mutex_);
        state_ = State::HALF_OPEN;
        success_count_ = 0;
    }

    bool shouldTrip() const {
        if (failure_count_ >= config_.failure_threshold) {
            return true;
        }

        const double failure_rate = calculateFailureRate();
        return failure_rate > 0.5 || 
                slow_calls_ > config_.failure_threshold;
    }

    double calculateFailureRate() const {
        if (metrics_.empty()) return 0.0;
        
        const auto window_start = currentTime() - config_.reset_timeout;
        uint32_t failures = 0;
        uint32_t total = 0;

        for (const auto& m : metrics_) {
            if (m.timestamp > window_start) {
                total++;
                if (!m.success) failures++;
            }
        }

        return total > 0 ? static_cast<double>(failures) / total : 0.0;
    }

    void recordMetric(bool success, double duration) {
        metrics_.emplace_back(Metric{success, duration, currentTime()});
        if (!success) failure_count_++;
        
        // 维护滑动窗口
        if (metrics_.size() > config_.sliding_window_size) {
            metrics_.erase(metrics_.begin());
        }
    }
};

}

#endif