#ifndef TIME_QUEUE_H
#define TIME_QUEUE_H

#include <queue>
#include <functional>
#include <memory>

// 定时器类
struct Timer {
public:
    // 删除标记
    bool used;
    // 执行任务
    std::function<void()> task;
    // 定时器过期时间
    uint32_t expire_time;
    bool operator<(const Timer& rhs) const {
        return expire_time < rhs.expire_time;
    }
};

struct TimerCmp {
    bool operator()(std::shared_ptr<Timer> a, std::shared_ptr<Timer> b) {
        return *b < *a;
    }
};

class TimerManger {
public:
    TimerManger() = default;
    ~TimerManger() = default;
    // 新增定时器
    std::shared_ptr<Timer> addTimer(const uint32_t time, std::function<void()> function);
    // 删除定时器（为延时删除）
    void delTimer(std::shared_ptr<Timer> timer);
    // tick一次，处理超时时间在time前的所有定时器
    void tick(const uint32_t time);
private:
    std::priority_queue<std::shared_ptr<Timer>, std::vector<std::shared_ptr<Timer>>, TimerCmp> timer_queue_;
};

#endif /* TIME_QUEUE_H */