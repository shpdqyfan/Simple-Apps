/*
    Date: 2017/11/05
    Author: shpdqyfan
    profile: easy timer based on boost asio
*/

#include <iostream>
#include <time.h>
#include "boost/bind.hpp"  
#include "boost/function.hpp"

#include "EasyTimer.h"

//////////////////////////////////////////////////////////////////
int64_t howMuchTimeFromNow(TimeStamp expire)
{
    int64_t milliseconds = expire.getMillisecondSinceEpoch2Expire() - TimeStamp::now();
    if(0 >= milliseconds)
    {
        milliseconds = 1;
    }

    return milliseconds;
}

//////////////////////////////////////////////////////////////////
EasyTimer::EasyTimer()
    : polling(false)
    , timerIoObj(ios)
    , timerSetPtr(new TimerSet())
{
    std::cout<<"EasyTimer, construct"<<std::endl;
}

EasyTimer::~EasyTimer()
{
    std::cout<<"EasyTimer, deconstruct"<<std::endl;
}

void EasyTimer::addTimer(const std::string& id, uint64_t interval, const TimerEventHandleCb& cb)
{
    std::cout<<"addTimer, id="<<id<<", interval="<<interval<<std::endl;

    time_t timeNow;
    time(&timeNow);
    std::cout<<"addTimer, timestamp="<<ctime(&timeNow)<<std::endl;
	
    bool earliestChanged = false;
    TimeStamp expire(TimeStamp::expire(interval));
    timerSetPtr->addTimer(id, interval, cb, expire, earliestChanged);
    if(earliestChanged)
    {
        timerIoObjResetExpire(expire);
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
        timerIoObjResetExpire(expire);
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
        timerIoObjResetExpire(expire);
    }
}

void EasyTimer::cancelTimer(const std::string& id)
{
    std::cout<<"cancelTimer, id="<<id<<std::endl;
	
    timerSetPtr->cancelTimer(id);
    if(timerSetPtr->empty())
    {
        std::cout<<"cancelTimer, timerset is empty now, try to keepalive"<<std::endl;
        timerIoObjKeepAlive();
    }
}

void EasyTimer::handleExpiredTimersCb(const boost::system::error_code& error)
{
    std::cout<<"handleExpiredTimersCb, error="<<error<<std::endl;

    time_t timeNow;
    time(&timeNow);
    std::cout<<"handleExpiredTimersCb, timestamp="<<ctime(&timeNow)<<std::endl;

    if(!error)
    {
        TimeStamp curTs(TimeStamp::now());
        TimerSet::ExpiredTimers expiredTimers = timerSetPtr->getExpiredTimers(curTs);
        if(expiredTimers.empty())
        {
	        return;
        }

        TimeStamp nextExpire;
        timerSetPtr->resetTimers(expiredTimers, nextExpire);
        timerIoObjResetExpire(nextExpire);

        for(auto expired : expiredTimers)
        {
	        (expired.second)->timeout();
        }
    }
    else if(error == boost::asio::error::operation_aborted)
    {
        std::cout<<"handleExpiredTimersCb, timer: operation_aborted"<<std::endl;
    }
}

void EasyTimer::handleKeepAliveTimerCb(const boost::system::error_code& error)
{
    std::cout<<"handleKeepAliveTimerCb, error="<<error<<std::endl;

    time_t timeNow;
    time(&timeNow);
    std::cout<<"handleKeepAliveTimerCb, timestamp="<<ctime(&timeNow)<<std::endl;

    if(!error)
    {
        timerIoObjKeepAlive();
    }
}

void EasyTimer::stop()
{
    std::cout<<"EasyTimer, stop ......"<<std::endl;
	
    timerIoObj.cancel();
    MyThread::join();
}

void EasyTimer::run()
{
    std::cout<<"EasyTimer, run ......"<<std::endl;
    
    setRunningState(RUNNING);
    timerIoObjKeepAlive();
    ios.run();
    
    std::cout<<"EasyTimer, shutdown ......"<<std::endl;
}

void EasyTimer::timerIoObjResetExpire(const TimeStamp& expire)
{
    int64_t millisec = howMuchTimeFromNow(expire);
    std::cout<<"timerIoObjResetExpire, in millisecond="<<millisec<<" will be expired"<<std::endl;

    timerIoObj.expires_from_now(boost::posix_time::millisec(millisec));
    timerIoObj.async_wait(boost::bind(&EasyTimer::handleExpiredTimersCb,
        this, boost::asio::placeholders::error));
}

void EasyTimer::timerIoObjKeepAlive()
{
    std::cout<<"timerIoObjKeepAlive"<<std::endl;

    //This expire time=1000 has no meaning, you can set expire time to <any>.
    //Just keep the timerIoObj alive. Because ios.run() will end if no asynchronous 
    //operations waiting on the timer.
    timerIoObj.expires_from_now(boost::posix_time::millisec(1000));
    timerIoObj.async_wait(boost::bind(&EasyTimer::handleKeepAliveTimerCb,
        this, boost::asio::placeholders::error));
}

