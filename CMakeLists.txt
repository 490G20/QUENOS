cmake_minimum_required(VERSION 3.8)
project(4thYearProjectGroup20cmpe498)

set(CMAKE_CXX_STANDARD 11)

set(SOURCE_FILES
        DBug12.h
        qcore.c
        qcore.h
        qinit.c
        qinit.h
        qmain.c
        qrequest.c
        qrequest.h
        queue.c
        queue.h
        quser.c
        quser.h
        qrequest.s
        )

add_executable(4thYearProjectGroup20cmpe498 ${SOURCE_FILES})