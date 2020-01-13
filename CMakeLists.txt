cmake_minimum_required(VERSION 3.10)

# set the project name
LINK_DIRECTORIES(dependencies/cuesdk/lib/x64)
project(CorsairCueNetworkReceiver)

# add the executable
add_executable(CorsairCueNetworkReceiver src/corsairCueNetworkReceiver.cpp)

# specify the C++ standard
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

INCLUDE_DIRECTORIES(dependencies/cuesdk/include)
TARGET_LINK_LIBRARIES(CorsairCueNetworkReceiver CUESDK.x64_2013 Ws2_32)


list(APPEND ExternalSharedLibraries dependencies/cuesdk/redist/x64/CUESDK.x64_2013.dll)

# Where will the executables be located?
if( DEFINED EXE_DIR )
    set( exeDir "${EXE_DIR}" )
else()
    set( exeDir "${CMAKE_CURRENT_BINARY_DIR}" )
endif()

# This will copy any files with differing timestamps
message( STATUS "\tCopying <name> shared libraries." )
file( COPY ${ExternalSharedLibraries} DESTINATION "${exeDir}" NO_SOURCE_PERMISSIONS )