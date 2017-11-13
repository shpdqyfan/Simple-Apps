/*
    Date: 2017/11/05
    Author: shpdqyfan
    profile: easy timer based on boost asio
*/

#include <iostream>
#include "boost/bind.hpp"  
#include "boost/function.hpp"

#include "EasyTimer.h"

//////////////////////////////////////////////////////////////////
int64_t howMuchTimeFromNow(TimeStamp expire)
{
    int64_t milliseconds = expire.getMillisecondSinceEpoch2Expire() - TimeStamp::now();
    if(0 > milliseconds)
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
	
    bool earliestChanged = false;
    TimeStamp expire(TimeStamp::expire(interval));
    timerSetPtr->addTimer(id, interval, cb, expire, earliestChanged);
    if(earliestChanged)
    {
        resetExpireForDeadlineTimer(expire);
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
        resetExpireForDeadlineTimer(expire);
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
        resetExpireForDeadlineTimer(expire);
    }
}

void EasyTimer::cancelTimer(const std::string& id)
{
    std::cout<<"cancelTimer, id="<<id<<std::endl;
	
    timerSetPtr->cancelTimer(id);
    if(timerSetPtr->empty())
    {
        //TODO:
    }
}

void EasyTimer::handleExpiredTimersCb(const boost::system::error_code& error)
{
    std::cout<<"handleExpiredTimersCb, error="<<error<<std::endl;

    if(error != boost::asio::error::operation_aborted)
    {
	TimeStamp curTs(TimeStamp::now());
        TimerSet::ExpiredTimers expiredTimers = timerSetPtr->getExpiredTimers(curTs);
        if(expiredTimers.empty())
        {
	    return;
        }

        TimeStamp nextExpire;
        timerSetPtr->resetTimers(expiredTimers, nextExpire);
        resetExpireForDeadlineTimer(nextExpire);

        for(auto expired : expiredTimers)
        {
	    (expired.second)->timeout();
        }
    }
}

void EasyTimer::stop()
{
    std::cout<<"EasyTimer, stop ......"<<std::endl;
	
    polling = false;
    MyThread::join();
}

void EasyTimer::run()
{
    std::cout<<"EasyTimer, run ......"<<std::endl;
    setRunningState(RUNNING);
    
    polling = true;
    while(polling)
    {
        ios.run();
    }
    std::cout<<"EasyTimer, shutdown ......"<<std::endl;
}

void EasyTimer::resetExpireForDeadlineTimer(const TimeStamp& expire)
{
    int64_t millisec = howMuchTimeFromNow(expire);
    std::cout<<"resetExpireForDeadlineTimer, in millisecond="<<millisec<<"will be expired"<<std::endl;

    timerIoObj.expires_from_now(boost::posix_time::millisec(millisec));
    timerIoObj.async_wait(boost::bind(&EasyTimer::handleExpiredTimersCb,
	this, boost::asio::placeholders::error));
}

