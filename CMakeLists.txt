cmake_minimum_required(VERSION 3.7)
project(top2csv)
set(CMAKE_CXX_STANDARD 14)
if(CMAKE_CROSSCOMPILING)
  set(CMAKE_FIND_LIBRARY_SUFFIXES ".a")
  set(BUILD_SHARED_LIBRARIES OFF)
  set(CMAKE_EXE_LINKER_FLAGS "-static")
  set(Boost_USE_STATIC_LIBS    ON)
  set(Boost_USE_MULTITHREADED  ON)
endif()
find_package(Boost 1.63.0 REQUIRED COMPONENTS program_options filesystem system)
if(Boost_FOUND)
  include_directories(${Boost_INCLUDE_DIRS})
  add_executable(top2csv top2csv.cpp)
  target_link_libraries(top2csv ${Boost_LIBRARIES})
endif()
