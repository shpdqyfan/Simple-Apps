/*
    Date: 2017/11/05
    Author: shpdqyfan
    profile: timer set
*/

#include <iostream>

#include "TimerSet.h"

using namespace SHPDQYFAN::APP::TIMER;

TimerSet::TimerSet() 
{
    std::cout<<"TimerSet, construct"<<std::endl;
}

TimerSet::~TimerSet() 
{
    std::cout<<"TimerSet, deconstruct"<<std::endl;;
}

void TimerSet::addTimer(const std::string& id, uint64_t interval, const TimerEventHandleCb& cb, 
        const TimeStamp& nextExpire, bool& earliestChanged)
{
    std::unique_lock<std::recursive_mutex> guard(myMutex);

    TimerPtr tp(new Timer(id, interval, nextExpire, cb));
    earliestChanged = insertTimer(tp);
}

void TimerSet::updateTimer(const std::string& id, uint64_t newInterval, const TimeStamp& nextExpire, 
        bool& earliestChanged)
{
    std::unique_lock<std::recursive_mutex> guard(myMutex);
    
    AllTimers::iterator it = findTimerById(id);
    if(it != myTimerSet.end())
    {
        Timer newTimer(*(it->second));
        newTimer.restart(newInterval, nextExpire);
        myTimerSet.erase(it);
        TimerPtr tp = std::make_shared<Timer>(newTimer);
        earliestChanged = insertTimer(tp);
    }
    else
    {
        std::cout<<"updateTimer, can't find timer id="<<id<<std::endl;
    }
}

void TimerSet::restartTimer(const std::string& id, TimeStamp& nextExpire, bool& earliestChanged)
{
    std::unique_lock<std::recursive_mutex> guard(myMutex);

    AllTimers::iterator it = findTimerById(id);
    if(it != myTimerSet.end())
    {
        TimeStamp expire(TimeStamp::expire((it->second)->getInterval()));
        nextExpire = expire;
        
        Timer newTimer(*(it->second));
        newTimer.restart(nextExpire);
        myTimerSet.erase(it);
        TimerPtr tp = std::make_shared<Timer>(newTimer);
        earliestChanged = insertTimer(tp);
    }
    else
    {
        std::cout<<"restartTimer, can't find timer id="<<id<<std::endl;
    }
}

void TimerSet::cancelTimer(const std::string& id)
{
    std::unique_lock<std::recursive_mutex> guard(myMutex);
    
    AllTimers::iterator it = findTimerById(id);
    if(it != myTimerSet.end())
    {
        myTimerSet.erase(it);
    }
    else
    {
        std::cout<<"cancelTimer, can't find timer id="<<id<<std::endl;
    }
}

bool TimerSet::empty() const
{  
    std::unique_lock<std::recursive_mutex> guard(myMutex);
    
    return myTimerSet.empty();
}

TimerSet::ExpiredTimers TimerSet::getExpiredTimers(const TimeStamp& curTs)
{
    std::unique_lock<std::recursive_mutex> guard(myMutex);
    
    std::cout<<"getExpiredTimers, all added timers:"<<std::endl;
    for(auto timer : myTimerSet)
    {
        std::cout<<"id="<<(timer.second)->id()
            <<", expire timestamp="<<(timer.first).getMillisecondSinceEpoch2Expire()
            <<", interval="<<(timer.second)->getInterval()<<std::endl;
    }
    
    TimerSet::ExpiredTimers expireds;
    SpecifiedTimer t(curTs, NULL);
    AllTimers::iterator firstOneWhichNotLessThanCurTs = myTimerSet.lower_bound(t);

    if(firstOneWhichNotLessThanCurTs == myTimerSet.end())
    {
        std::copy(myTimerSet.begin(), myTimerSet.end(), std::back_inserter(expireds));
        myTimerSet.erase(myTimerSet.begin(), myTimerSet.end());
    }
    else if(firstOneWhichNotLessThanCurTs->first > curTs)
    {
        std::copy(myTimerSet.begin(), firstOneWhichNotLessThanCurTs, std::back_inserter(expireds));
        myTimerSet.erase(myTimerSet.begin(), firstOneWhichNotLessThanCurTs);
    }
    else if(firstOneWhichNotLessThanCurTs->first == curTs)
    {
        AllTimers::iterator it;
        for(it = myTimerSet.begin();it != myTimerSet.end();++it)
        {
            if((it->first < curTs) || (it->first == curTs))
            {
                expireds.push_back(*it);
            }
            else
            {
                break;
            }
        }
        myTimerSet.erase(myTimerSet.begin(), it);
    }

    std::cout<<"getExpiredTimers, all expired timers:"<<std::endl;
    for(auto timer : expireds)
    {
        std::cout<<"id="<<(timer.second)->id()
            <<", expire timestamp="<<(timer.first).getMillisecondSinceEpoch2Expire()
            <<", interval="<<(timer.second)->getInterval()<<std::endl;
    }

    return expireds;
}

void TimerSet::resetTimers(ExpiredTimers& timers, TimeStamp& nextExpire)
{
    std::cout<<"resetTimers, all expired timers:"<<std::endl;
    for(auto timer : timers)
    {
        std::cout<<"id="<<(timer.second)->id()<<std::endl;
    }
    
    std::unique_lock<std::recursive_mutex> guard(myMutex);
    
    ExpiredTimers::iterator it;
    for(it = timers.begin();it != timers.end();++it)
    {
        if(it->second->getRepeat())
        {
            it->second->restart();
            insertTimer(it->second);
        }
    }

    if(!myTimerSet.empty())
    {
        nextExpire = myTimerSet.begin()->first;
    }
}

TimerSet::AllTimers::iterator TimerSet::findTimerById(const std::string& id)
{
    AllTimers::iterator it;
    for(it = myTimerSet.begin();it != myTimerSet.end();++it)
    {
        if(id == it->second->id())
        {
            return it;
        }
    }

    return myTimerSet.end();
}

bool TimerSet::insertTimer(std::shared_ptr<Timer> timer)
{
    bool earliestChanged = false;
    AllTimers::iterator it = myTimerSet.begin();
    if(it == myTimerSet.end())
    {
        earliestChanged = true;
    }
    else if(timer->getNextExpire() < it->first)
    {
        earliestChanged = true;
    }

    SpecifiedTimer t(timer->getNextExpire(), timer);
    myTimerSet.insert(t);

    return earliestChanged;
}


