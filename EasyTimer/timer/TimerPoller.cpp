/*
    Date: 2017/01/09
    Author: shpdqyfan
    profile: easy timer
*/

#include <unistd.h>
#include <string.h>
#include <iostream>

#include "TimerPoller.h"

using namespace SHPDQYFAN::APP::TIMERPOLLER;

//////////////////////////////////////////////////////////////////
void FdHandler::handle()
{
    if(revents & EPOLLIN)
    {
        rCb();
    }
}

//////////////////////////////////////////////////////////////////
TimerFdPoller::TimerFdPoller()
    : epFd(epoll_create1(EPOLL_CLOEXEC))
    , myEventV(TimerFdPoller::MAX_EVENT_SIZE)
{
    std::cout<<"TimerFdPoller, construct, epFd="<<epFd<<std::endl;
}

TimerFdPoller::~TimerFdPoller()
{
    std::cout<<"TimerFdPoller, deconstruct, epFd="<<epFd<<std::endl;
    close(epFd);
}

void TimerFdPoller::polling()
{
    int num = epoll_wait(epFd, &(*myEventV.begin()), TimerFdPoller::MAX_EVENT_SIZE, -1);
    if(0 > num)
    {
        return;
    }

    std::unique_lock<std::recursive_mutex> guard(myMutex);
    
    for(int i = 0;i < num;i++)
    {
        std::map<int, std::shared_ptr<FdHandler>>::iterator it 
                = myHandlerMap.find(myEventV[i].data.fd);
        if(it != myHandlerMap.end())
        {
            if(wakeupFd == (it->second)->getFd())
            {
                std::cout<<"TimerFdPoller, wakeup from waiting/working to shutdown"<<std::endl;
                (it->second)->setRevents(myEventV[i].events);
                return;
            }
            else
            {
                (it->second)->setRevents(myEventV[i].events);
                (it->second)->handle();
                continue;
            }
        }    
    }
    
    return;
}

void TimerFdPoller::addWakeup(int fd, std::shared_ptr<FdHandler> h)
{
    std::cout<<"addWakeup, fd="<<fd<<std::endl;
    
    std::unique_lock<std::recursive_mutex> guard(myMutex);

    if(myHandlerMap.find(fd) == myHandlerMap.end())
    {
        wakeupFd = fd;
        myHandlerMap.insert(std::make_pair(wakeupFd, h));
        ctl(wakeupFd, EPOLL_CTL_ADD);
    }
}

void TimerFdPoller::add(int fd, std::shared_ptr<FdHandler> h)
{
    std::cout<<"add, fd="<<fd<<std::endl;
    std::unique_lock<std::recursive_mutex> guard(myMutex);

    if(myHandlerMap.find(fd) == myHandlerMap.end())
    {
        myHandlerMap.insert(std::make_pair(fd, h));
        ctl(fd, EPOLL_CTL_ADD);
    }
}

void TimerFdPoller::mod(int fd)
{
    std::unique_lock<std::recursive_mutex> guard(myMutex);

    if(myHandlerMap.find(fd) != myHandlerMap.end())
    {
        ctl(fd, EPOLL_CTL_MOD);
    }
}

void TimerFdPoller::del(int fd)
{
    std::unique_lock<std::recursive_mutex> guard(myMutex);
    
    if(myHandlerMap.find(fd) != myHandlerMap.end())
    {
        ctl(fd, EPOLL_CTL_DEL);
        myHandlerMap.erase(fd);
    }
}

void TimerFdPoller::ctl(int fd, int op)
{
    struct epoll_event event;
    memset(&event, 0, sizeof event);

    std::map<int, std::shared_ptr<FdHandler>>::iterator it = myHandlerMap.find(fd);
    if(it == myHandlerMap.end())
    {
        return;
    }
    
    event.events = (it->second)->getEvents();
    event.data.fd = (it->second)->getFd();

    std::cout<<"ctl, timerfd="<<(it->second)->getFd()
        <<", op="<<op<<", events="<<(it->second)->getEvents()<<std::endl;
    epoll_ctl(epFd, op, (it->second)->getFd(), &event);
}
