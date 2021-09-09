#ifndef TIME_QUEUE_H
#define TIME_QUEUE_H

#include <queue>
#include <functional>
#include <memory>

struct Timer {
public:
    bool used;
    std::function<void()> task;
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
    std::shared_ptr<Timer> addTimer(const uint32_t time, std::function<void()> function);
    void delTimer(std::shared_ptr<Timer> timer);
    void tick(const uint32_t time);
private:
    std::priority_queue<std::shared_ptr<Timer>, std::vector<std::shared_ptr<Timer>>, TimerCmp> timer_queue_;
};

#endif /* TIME_QUEUE_H */