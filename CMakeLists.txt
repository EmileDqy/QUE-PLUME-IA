cmake_minimum_required(VERSION 3.0.0)
project(ProjetIA VERSION 0.1.0)

include(CTest)
enable_testing()

include_directories( ${OpenCV_INCLUDE_DIRS} )
find_package(OpenCV)

find_library(wiringPi_LIB wiringPi)

message(STATUS "OpenCV_INCLUDE_DIRS = ${OpenCV_INCLUDE_DIRS}")
message(STATUS "OpenCV_LIBS = ${OpenCV_LIBS}")

include_directories(include)
file(GLOB SOURCES "src/*.cpp")

link_directories(/usr/local/lib)
add_executable(ProjetIA ${SOURCES})
target_link_libraries( ProjetIA ${OpenCV_LIBS} ${wiringPi_LIB} )

# Copy
configure_file("./src/frozen_graph.pb" "./frozen_graph.pb" COPYONLY)
configure_file("./src/datapush.py" "./datapush.py" COPYONLY)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
