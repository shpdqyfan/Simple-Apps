/*
    Date: 2017/01/09
    Author: shpdqyfan
    profile: easy timer
*/

#ifndef TIMER_POLLER_H
#define TIMER_POLLER_H

#include <map>
#include <mutex>
#include <vector>
#include <memory>
#include <functional>
#include <sys/epoll.h>

namespace SHPDQYFAN{
namespace APP{
namespace TIMERPOLLER{

//////////////////////////////////////////////////////////////////
class FdHandler
{
public:
    typedef std::function<void(void)> ReadCb;
    typedef std::function<void(void)> WriteCb;

    FdHandler(int fd) 
        : myFd(fd) 
        , events(0)
        , revents(0) {}

    void setReadCb(const ReadCb& cb) {rCb = cb;}
    void setWriteCb(const WriteCb& cb) {wCb = cb;}
    void enableRead() {events |= EPOLLIN;}
    void enableWrite() {events |= EPOLLOUT;}
    void disableRead() {events &= ~EPOLLIN;}
    void disableWrite() {events &= ~EPOLLOUT;}
    void disableAll() {events = 0;}
    void setRevents(int re) {revents = re;}
    int getEvents() const {return events;}
    int getFd() const {return myFd;}
    void handle();

private:
    int myFd;
    int events;
    int revents;
    ReadCb rCb;
    WriteCb wCb;
};

//////////////////////////////////////////////////////////////////
class TimerFdPoller
{
public:
    TimerFdPoller();
    ~TimerFdPoller();
    void polling();
    void addWakeup(int fd, std::shared_ptr<FdHandler> h);
    void add(int fd, std::shared_ptr<FdHandler> h);
    void mod(int fd);
    void del(int fd);

public:
    static const int MAX_EVENT_SIZE = 2;
    
private:
    void ctl(int fd, int op);
    
    int epFd;
    int wakeupFd;
    std::recursive_mutex myMutex;
    std::vector<struct epoll_event> myEventV;
    std::map<int, std::shared_ptr<FdHandler>> myHandlerMap;
};
}
}
}

#endif
