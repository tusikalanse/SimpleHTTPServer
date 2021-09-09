#include "timequeue.h"

std::shared_ptr<Timer> TimerManger::addTimer(const uint32_t time, std::function<void()> function) {
    Timer timer;
    timer.used = false;
    timer.expire_time = time;
    timer.task = function;
    std::shared_ptr<Timer> t = std::make_shared<Timer>(timer);
    timer_queue_.push(t);
    return t;
}

void TimerManger::delTimer(std::shared_ptr<Timer> timer) {
    timer->used = true;
}

void TimerManger::tick(const uint32_t time) {
    while (!timer_queue_.empty()) {
        std::shared_ptr<Timer> timer = timer_queue_.top();
        if (timer->expire_time > time)
            break;
        timer_queue_.pop();
        if (timer->used)
            continue;
        timer->task();
    }
}

