cmake_minimum_required(VERSION 3.0.0)
project(ProjetIA VERSION 0.1.0)

include(CTest)
enable_testing()

include_directories( ${OpenCV_INCLUDE_DIRS} )
find_package(OpenCV)

message(STATUS "OpenCV_INCLUDE_DIRS = ${OpenCV_INCLUDE_DIRS}")
message(STATUS "OpenCV_LIBS = ${OpenCV_LIBS}")

include_directories(include)
file(GLOB SOURCES "src/*.cpp")

link_directories(/usr/local/lib)
add_executable(ProjetIA ${SOURCES})
target_link_libraries( ProjetIA ${OpenCV_LIBS} )

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
