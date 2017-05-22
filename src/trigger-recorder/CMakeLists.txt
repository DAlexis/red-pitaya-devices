cmake_minimum_required(VERSION 2.8)

project("rp-trigger-recorder")

find_package (Boost COMPONENTS date_time program_options REQUIRED)
find_package (Threads)

set(EXE_SOURCES
    main.cpp
    trigger-recorder.cpp
    trigger-recorder.hpp
)

include_directories(
    
)

add_executable(${PROJECT_NAME} ${EXE_SOURCES} rp-trigger-recorder.conf)

target_link_libraries (${PROJECT_NAME}
    rp
    ${CMAKE_THREAD_LIBS_INIT}
    ${Boost_PROGRAM_OPTIONS_LIBRARY} # For command line options parsing for example
)

set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 11)

configure_file(rp-trigger-recorder.conf rp-trigger-recorder.conf COPYONLY)

install(TARGETS ${PROJECT_NAME}
        RUNTIME DESTINATION /opt/rp-trigger-recorder/bin
        LIBRARY DESTINATION /opt/rp-trigger-recorder/lib
        ARCHIVE DESTINATION /opt/rp-trigger-recorder/lib)

install(FILES rp-trigger-recorder.conf DESTINATION /etc COMPONENT config)
install(FILES trigger-recorder.service DESTINATION /etc/systemd/system/ COMPONENT config)
install(FILES trigger-recorder-launcher.sh DESTINATION /opt/rp-trigger-recorder/bin COMPONENT config)


add_custom_target(uninstall_${PROJECT_NAME}
	    	COMMAND rm -rf /etc/rp-trigger-recorder.conf /opt/rp-trigger-recorder /etc/systemd/system/trigger-recorder.service
	    	WORKING_DIRECTORY "${CMAKE_BINARY_DIR}")