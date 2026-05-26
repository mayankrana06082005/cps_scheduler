cmake_minimum_required(VERSION 3.10)
project(CPSScheduler)

# Require C++17 for smart pointers and modern language features
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Add all source files to the executable
add_executable(simulator 
    src/main.cpp 
    src/Scheduler.cpp
)