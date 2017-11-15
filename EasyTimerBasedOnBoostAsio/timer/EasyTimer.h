/*
    Date: 2017/11/05
    Author: shpdqyfan
    profile: easy timer based on boost asio
*/

#ifndef EASY_TIMER_H
#define EASY_TIMER_H

#include <string>
#include <boost/scoped_ptr.hpp>
#include "boost/asio.hpp"  
#include "boost/date_time/posix_time/posix_time.hpp"

#include "TimerSet.h"
#include "thread/Thread.h"

using namespace SHPDQYFAN::APP::TIMER;

class EasyTimer : public MyThread
{
public:
    EasyTimer();
    ~EasyTimer();

    void addTimer(const std::string& id, uint64_t interval, const TimerEventHandleCb& cb);
    void updateTimer(const std::string& id, uint64_t newInterval);
    void restartTimer(const std::string& id);
    void cancelTimer(const std::string& id);
    void handleExpiredTimersCb(const boost::system::error_code& error);
    void handleKeepAliveTimerCb(const boost::system::error_code& error);
    void stop();

protected:
    void run();

private:
    void timerIoObjResetExpire(const TimeStamp& expire);
    void timerIoObjKeepAlive();
	
    bool polling;
    boost::asio::io_service ios;
    boost::asio::deadline_timer timerIoObj;
    boost::scoped_ptr<TimerSet> timerSetPtr;
};

#endif

