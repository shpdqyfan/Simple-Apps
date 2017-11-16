/*
    Date: 2017/01/09
    Author: shpdqyfan
    profile: easy timer
*/

#include <iostream>
#include <sys/timerfd.h>
#include <sys/eventfd.h>
#include <unistd.h>
#include <string.h>
#include <time.h>

#include "EasyTimer.h"

//////////////////////////////////////////////////////////////////
int createTimerfd()
{
    return timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
}

struct timespec howMuchTimeFromNow(TimeStamp expire)
{
    int64_t milliseconds = expire.getMillisecondSinceEpoch2Expire() - TimeStamp::now();
    if(0 >= milliseconds)
    {
        milliseconds = 1;
    }

    struct timespec ts;
    ts.tv_sec = static_cast<time_t>(milliseconds / TimeStamp::UNIT_THOUSAND);
    ts.tv_nsec = static_cast<long>((milliseconds % TimeStamp::UNIT_THOUSAND) * TimeStamp::UNIT_MILLION);
    return ts;
}

void resetTimerfd(int timerfd, TimeStamp expire)
{
    std::cout<<"resetTimerfd, next expire timestamp="<<expire.getMillisecondSinceEpoch2Expire()<<std::endl;
    struct itimerspec newValue;
    struct itimerspec oldValue;
    memset(&newValue, 0, sizeof newValue);
    memset(&oldValue, 0, sizeof oldValue);

    newValue.it_value = howMuchTimeFromNow(expire);
    if(0 > timerfd_settime(timerfd, 0, &newValue, &oldValue))
    {
        std::cout<<"resetTimerfd="<<timerfd<<" error"<<std::endl;
    }
}

void stopTimerfd(int timerfd)
{
    struct itimerspec newValue;
    memset(&newValue, 0, sizeof newValue);

    std::cout<<"stopTimerfd, timerfd="<<timerfd<<std::endl;
    if(0 > timerfd_settime(timerfd, 0, &newValue, NULL))
    {
        std::cout<<"stopTimerfd="<<timerfd<<" error"<<std::endl;
    }
}

int createEventFd()
{
    return eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
}

//////////////////////////////////////////////////////////////////
EasyTimer::EasyTimer()
    : timerFd(createTimerfd())
    , wakeupFd(createEventFd())
    , polling(false)
    , timerHandlerPtr(new FdHandler(timerFd)) 
    , wakeupHandlerPtr(new FdHandler(wakeupFd))
    , timerPollerPtr(new TimerFdPoller())
    , timerSetPtr(new TimerSet())
{
    std::cout<<"EasyTimer, construct"<<std::endl;

    timerHandlerPtr->setReadCb(std::bind(&EasyTimer::handleExpiredTimers, this));
    timerHandlerPtr->enableRead();
    timerPollerPtr->add(timerFd, timerHandlerPtr);

    wakeupHandlerPtr->enableRead();
    timerPollerPtr->addWakeup(wakeupFd, wakeupHandlerPtr);
}

EasyTimer::~EasyTimer()
{
    std::cout<<"EasyTimer, deconstruct"<<std::endl;
    
    timerHandlerPtr->disableAll();
    timerPollerPtr->del(timerFd);
    close(timerFd);

    wakeupHandlerPtr->disableAll();
    timerPollerPtr->del(wakeupFd);
    close(wakeupFd);
}

void EasyTimer::addTimer(const std::string& id, uint64_t interval, const TimerEventHandleCb& cb)
{
    std::cout<<"addTimer, id="<<id<<", interval="<<interval<<std::endl;
    bool earliestChanged = false;
    TimeStamp expire(TimeStamp::expire(interval));
    timerSetPtr->addTimer(id, interval, cb, expire, earliestChanged);
    if(earliestChanged)
    {
        resetTimerfd(timerFd, expire);
    }
}

void EasyTimer::updateTimer(const std::string& id, uint64_t newInterval)
{
    std::cout<<"updateTimer, id="<<id<<", new interval="<<newInterval<<std::endl;
    bool earliestChanged = false;
    TimeStamp expire(TimeStamp::expire(newInterval));
    timerSetPtr->updateTimer(id, newInterval, expire, earliestChanged);
    if(earliestChanged)
    {
        resetTimerfd(timerFd, expire);
    }
}

void EasyTimer::restartTimer(const std::string& id)
{
    std::cout<<"restartTimer, id="<<id<<std::endl;
    bool earliestChanged = false;
    TimeStamp expire;
    timerSetPtr->restartTimer(id, expire, earliestChanged);
    if(earliestChanged)
    {
        resetTimerfd(timerFd, expire);
    }
}

void EasyTimer::cancelTimer(const std::string& id)
{
    std::cout<<"cancelTimer, id="<<id<<std::endl;
    timerSetPtr->cancelTimer(id);
    if(timerSetPtr->empty())
    {
        stopTimerfd(timerFd);
    }
}

void EasyTimer::handleExpiredTimers()
{
    std::cout<<"handleExpiredTimers"<<std::endl;
    
    TimeStamp curTs(TimeStamp::now());
    TimerSet::ExpiredTimers expiredTimers = timerSetPtr->getExpiredTimers(curTs);
    if(expiredTimers.empty())
    {
        return;
    }

    TimeStamp nextExpire;
    timerSetPtr->resetTimers(expiredTimers, nextExpire);
    resetTimerfd(timerFd, nextExpire);
    
    for(auto expired : expiredTimers)
    {
        (expired.second)->timeout();
    }
}

void EasyTimer::wakeup()
{
    std::cout<<"wakeup ......"<<std::endl;
    eventfd_t writeSth = 1;
    if(0 > eventfd_write(wakeupFd, writeSth))
    {
        std::cout<<"wakeup write error"<<std::endl;
    }
}

void EasyTimer::stop()
{
    std::cout<<"EasyTimer, stop ......"<<std::endl;
    polling = false;
    wakeup();
    MyThread::join();
}

void EasyTimer::run()
{
    std::cout<<"EasyTimer, run ......"<<std::endl;
    setRunningState(RUNNING);
    
    polling = true;
    while(polling)
    {
        timerPollerPtr->polling();
    }
    std::cout<<"EasyTimer, shutdown ......"<<std::endl;
}
