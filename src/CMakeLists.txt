SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/build)
SET(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)
#SET(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")
#SET (CMAKE_C_COMPILER "gcc")
#SET (CMAKE_CXX_COMPILER "g++")

SET (CMAKE_BUILD_TYPE "Debug")
SET (CMAKE_CXX_FLAGS "-std=c++11 -Wno-deprecated-declarations" CACHE INTERNAL "compiler options" FORCE)
SET (CMAKE_CXX_FLAGS_DEBUG "-O0 -g" CACHE INTERNAL "compiler options" FORCE)
SET (CMAKE_CXX_FLAGS_RELEASE "-O2" CACHE INTERNAL "compiler options" FORCE)

list(APPEND EXTAR_INCLUDE_HEADER ${CMAKE_CURRENT_SOURCE_DIR}/util
                                 ${CMAKE_CURRENT_SOURCE_DIR}/system 
                                 ${CMAKE_CURRENT_SOURCE_DIR}/db
                                 ${CMAKE_CURRENT_SOURCE_DIR}/storage
                                 ${CMAKE_CURRENT_SOURCE_DIR}/BPTree)

file(GLOB_RECURSE SRC_FILES util/*.cpp
                            system/*.cpp
                            db/*.cpp
                            storage/*.cpp
                            BPTree/*.cpp)

ADD_EXECUTABLE(BPTREE_IMPL ${SRC_FILES})

target_include_directories(BPTREE_IMPL PUBLIC ${EXTAR_INCLUDE_HEADER})
target_link_libraries(BPTREE_IMPL pthread)
