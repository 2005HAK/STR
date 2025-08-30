cmake_minimum_required(VERSION 3.10)
project(jantarDosFilosofosVOrdemFixa)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(src)

set(SOURCES
    src/problemaDosFilosofos.cpp
)

add_executable(run ${SOURCES})

target_link_libraries(run
    pthread
)
