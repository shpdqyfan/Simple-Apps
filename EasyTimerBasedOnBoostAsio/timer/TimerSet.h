/*
    Date: 2017/11/05
    Author: shpdqyfan
    profile: timer set
*/

#ifndef TIMER_SET_H
#define TIMER_SET_H

#include <map>
#include <set>
#include <vector>
#include <string>
#include <chrono>
#include <mutex>
#include <memory>
#include <functional>
#include <boost/operators.hpp>

namespace SHPDQYFAN{
namespace APP{
namespace TIMER{
            
//////////////////////////////////////////////////////////////////
class TimeStamp : public boost::less_than_comparable<TimeStamp>
{
public:
    TimeStamp()
        : millisecondSinceEpoch2Expire(0) {}

    TimeStamp(int64_t millisecond)
        : millisecondSinceEpoch2Expire(millisecond) {}

    int64_t getMillisecondSinceEpoch2Expire() const
    {
        return millisecondSinceEpoch2Expire;
    }

    bool valid() const
    {
        return (millisecondSinceEpoch2Expire > 0);
    }

    static int64_t expire(uint64_t interval)
    {
        std::chrono::milliseconds ms(interval);    
        return (now()+ms.count());
    }

    static int64_t now()
    {
        std::chrono::time_point<std::chrono::system_clock> timePoint 
            = std::chrono::system_clock::now();
        std::chrono::milliseconds ms 
            = std::chrono::duration_cast<std::chrono::milliseconds>(timePoint.time_since_epoch());
            
        return (ms.count());
    }

public:
    static const int UNIT_THOUSAND = 1000;
    static const int UNIT_MILLION = 1000000;

private:
    int64_t millisecondSinceEpoch2Expire;
};

inline bool operator<(const TimeStamp& lts, const TimeStamp& rts)
{
    return (lts.getMillisecondSinceEpoch2Expire() < rts.getMillisecondSinceEpoch2Expire());
}

inline bool operator==(const TimeStamp& lts, const TimeStamp& rts)
{
    return (lts.getMillisecondSinceEpoch2Expire() == rts.getMillisecondSinceEpoch2Expire());
}

//////////////////////////////////////////////////////////////////
typedef std::function<void(const std::string&)> TimerEventHandleCb;

class Timer
{
public:
    Timer(const std::string& id, uint64_t interval, const TimeStamp& expire, 
        const TimerEventHandleCb& cb)
        : timerId(id)
        , myInterval(interval)
        , nextExpire(expire)
        , myCb(cb)
        , repeat(true) {}

    void timeout()
    {
        myCb(timerId);
    }

    bool getRepeat() const
    {
        return repeat;
    }

    TimeStamp getNextExpire() const
    {
        return nextExpire;
    }

    uint64_t getInterval() const
    {
        return myInterval;
    }

    const std::string& id() const
    {
        return timerId;
    }

    void restart()
    {
        nextExpire = TimeStamp(TimeStamp::expire(myInterval));
    }

    void restart(const TimeStamp& expire)
    {
        nextExpire = expire;
    }

    void restart(uint64_t newInterval, const TimeStamp& expire)
    {
        myInterval = newInterval;
        nextExpire = expire;
    }

private:
    std::string timerId;
    uint64_t myInterval;//unit: millisecond
    TimeStamp nextExpire;
    const TimerEventHandleCb myCb;
    bool repeat;
};

//////////////////////////////////////////////////////////////////
class TimerSet
{
public:
    typedef std::shared_ptr<Timer> TimerPtr;
    typedef std::pair<TimeStamp, TimerPtr> SpecifiedTimer;
    typedef std::set<SpecifiedTimer> AllTimers;
    typedef std::vector<SpecifiedTimer> ExpiredTimers;

    TimerSet();
    ~TimerSet();

    void addTimer(const std::string& id, uint64_t interval, const TimerEventHandleCb& cb, 
        const TimeStamp& nextExpire, bool& earliestChanged);
    void updateTimer(const std::string& id, uint64_t newInterval, const TimeStamp& nextExpire, 
        bool& earliestChanged);
    void restartTimer(const std::string& id, TimeStamp& nextExpire, bool& earliestChanged);
    void cancelTimer(const std::string& id);
    bool empty() const;
    ExpiredTimers getExpiredTimers(const TimeStamp& curTs);
    void resetTimers(ExpiredTimers& timers, TimeStamp& nextExpire);

private:
    AllTimers::iterator findTimerById(const std::string& id);
    bool insertTimer(std::shared_ptr<Timer> timer);

    AllTimers myTimerSet;
    mutable std::recursive_mutex myMutex;
};
}
}
}

#endif

