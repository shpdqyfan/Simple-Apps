/*
    Date: 2017/11/05
    Author: shpdqyfan
    profile: easy timer based on boost asio
*/

#include <iostream>
#include <string>

#include "timer/EasyTimer.h"

void timerTriggerCallback(const std::string& id)
{
    std::cout<<"timerTriggerCallback, id="<<id<<std::endl;
}

typedef std::unique_ptr<EasyTimer> EasyTimerPtr;

int main()
{
    EasyTimer* ptr = new EasyTimer();
    EasyTimerPtr myTimerPtr = std::unique_ptr<EasyTimer>(ptr);
    
    myTimerPtr->start();
    do
    {
        std::cout<<"EasyTimerPtr starting ..."<<std::endl;
    }while(!myTimerPtr->getRunningState());

    std::cout<<"EasyTimerPtr start complete now"<<std::endl;

    myTimerPtr->addTimer("t1", 2000, timerTriggerCallback);
    myTimerPtr->addTimer("t2", 5000, timerTriggerCallback);

    sleep(8);

    myTimerPtr->cancelTimer("t1");
    myTimerPtr->cancelTimer("t2");

    sleep(9);

    myTimerPtr->addTimer("t3", 3000, timerTriggerCallback);
    myTimerPtr->addTimer("t4", 6000, timerTriggerCallback);

    sleep(10);

    myTimerPtr->stop();
    
    return 0;
}

