cmake_minimum_required(VERSION 3.1.0 FATAL_ERROR)

project("self-organizing-transport-modeller")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0 -g -DDEBUG")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -g -DNDEBUG -funroll-loops")

link_directories("/opt/redpitaya/lib")
include_directories("/opt/redpitaya/include")

# For using define GIT_COMMIT_TAG in code
execute_process(COMMAND git log -1 --format="%H" OUTPUT_VARIABLE git_commit)
# The line above brokes QT visualization compilation. This need some investigation
#add_definitions( -DGIT_COMMIT_TAG=${git_commit} )

# This is to run all tests using 'make run-unit-tests' command.
# Another way is simply run 'ctest -VV', but output is not colorized by default :(
# This project may be a part of other cmake project so this trick may be implemented yet
if (NOT COMMAND add_unit_test)
	add_custom_target(run-unit-tests)
	macro(add_unit_test test_target)
		add_custom_target(${test_target}_runtest
	    	COMMAND ${test_target} 
	    	DEPENDS ${test_target}
	    	WORKING_DIRECTORY "${CMAKE_BINARY_DIR}")
		add_dependencies(run-unit-tests ${test_target}_runtest)
	endmacro()
endif()

add_subdirectory(lightning-trigger)

# To enable ctest usage
enable_testing()

