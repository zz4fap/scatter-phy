cmake_minimum_required(VERSION 2.8)

add_definitions(-std=c++11)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")

include_directories("${PROJECT_BINARY_DIR}")
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories("/usr/local/include")
include_directories("/usr/local/lib")
include_directories("/usr/include")
include_directories("/usr/include/c++")
include_directories("/usr/include/c++/4.8")
include_directories("/usr/include/boost")
include_directories("/usr/include/boost/thread")
include_directories("/usr/local/include/google/protobuf")
include_directories("/usr/local/include/google/")

add_subdirectory(cpp)

set(communicator_srcs $<TARGET_OBJECTS:communicator_cpp>)

add_library(communicator SHARED ${communicator_srcs})
target_link_libraries(communicator ${LIBS})

#Set the location for library installation -- i.e., /usr/lib in this case.
install(TARGETS communicator DESTINATION /usr/local/lib)
file(GLOB communicator_headers "cpp/*.h")
install(FILES ${communicator_headers} DESTINATION /usr/local/include/communicator)
#install(FILES ./cpp/LayerCommunicator.h ./cpp/interf.pb.h ./cpp/IPTOS.h ./cpp/utils.h ./cpp/Maybe.h ./cpp/interAI.pb.h ./cpp/CommManager.h ./cpp/Message.h ./cpp/SafeQueue.h ./cpp/communicator_wrapper.h ./cpp/SlidingWindow.h DESTINATION /usr/local/include/communicator)
set_target_properties(communicator PROPERTIES COMPILE_FLAGS -fPIC)
