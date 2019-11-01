add_definitions(-std=c++11)
file(GLOB SOURCES "*.cpp" "*.cc")
add_library(communicator_cpp OBJECT ${SOURCES})
set_target_properties(communicator_cpp PROPERTIES COMPILE_FLAGS -fPIC)
