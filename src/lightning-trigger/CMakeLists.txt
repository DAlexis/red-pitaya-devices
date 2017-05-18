cmake_minimum_required(VERSION 2.8)

project(lightning-trigger)

find_package (Boost COMPONENTS date_time program_options REQUIRED)

set(EXE_SOURCES
    main.cpp
    lightning-trigger.cpp
    lightning-trigger.hpp
)

include_directories(
    
)

add_executable(${PROJECT_NAME} ${EXE_SOURCES})

target_link_libraries (${PROJECT_NAME}
    rp
    ${Boost_PROGRAM_OPTIONS_LIBRARY} # For command line options parsing for example
)

set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 11)
