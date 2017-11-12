cmake_minimum_required(VERSION 3.0)
project(EASYTIMER)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

find_package(Boost
             1.57.0
             COMPONENTS 
             REQUIRED)
            
file(GLOB THREAD_SOURCE timer/thread/*.cpp)
set(THREAD_LIB_NAME thread)
add_library(${THREAD_LIB_NAME} SHARED ${THREAD_SOURCE})

file(GLOB TIMER_SOURCE timer/*.cpp)
set(TIMER_LIB_NAME timer)
add_library(${TIMER_LIB_NAME} SHARED ${TIMER_SOURCE})

set(EASYTIMER_MAIN_SOURCE main.cpp) 
add_executable(appEasyTimer ${EASYTIMER_MAIN_SOURCE}) 
target_link_libraries(appEasyTimer ${THREAD_LIB_NAME} ${TIMER_LIB_NAME} pthread)

