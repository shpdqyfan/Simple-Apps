/*
    Date: 2017/01/09
    Author: shpdqyfan
    profile: easy timer
*/

#ifndef EASY_TIMER_H
#define EASY_TIMER_H

#include <string>
#include <memory>
#include <boost/scoped_ptr.hpp>

#include "TimerSet.h"
#include "TimerPoller.h"
#include "thread/Thread.h"

using namespace SHPDQYFAN::APP::TIMER;
using namespace SHPDQYFAN::APP::TIMERPOLLER;

class EasyTimer : public MyThread
{
public:
    EasyTimer();
    ~EasyTimer();

    void addTimer(const std::string& id, uint64_t interval, const TimerEventHandleCb& cb, bool rep = true);
    void updateTimer(const std::string& id, uint64_t newInterval);
    void restartTimer(const std::string& id);
    void cancelTimer(const std::string& id);
    void handleExpiredTimers();
    void wakeup();
    void stop();

protected:
    void run();

private:
    int timerFd;
    int wakeupFd;
    bool polling;
    std::shared_ptr<FdHandler> timerHandlerPtr;
    std::shared_ptr<FdHandler> wakeupHandlerPtr;
    boost::scoped_ptr<TimerFdPoller> timerPollerPtr;
    boost::scoped_ptr<TimerSet> timerSetPtr;
};

#endif
