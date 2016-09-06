# general settings
cmake_minimum_required (VERSION 3.0)
project(DebugPrinter
        VERSION 2015.1110)

enable_testing()

set(FSC_WEBSITE ${PROJECT_SOURCE_DIR}/../../website/frescolino)

# if additional cmake modules are needed
list(APPEND CMAKE_MODULE_PATH 
    /usr/include /usr/local/include
    ~/.local/include ${PROJECT_SOURCE_DIR}/../cmake_extensions)

include(fsc/softinstall)
include(fsc/uninstall)

# compiler & flags
set(CMAKE_CXX_COMPILER "g++")
set(CMAKE_CXX_FLAGS "-std=c++14 -O3 -march=native -Werror -Wall -Wextra -Wpedantic")
#~ set(CMAKE_CXX_COMPILER "clang++")
#~ set(CMAKE_CXX_FLAGS "-std=c++14 -O3 -march=native -Werror -Weverything -Wno-c++98-compat -Wno-c++98-compat-pedantic -Wno-missing-prototypes -Wno-exit-time-destructors -Wno-global-constructors -Wno-implicit-fallthrough -Wno-disabled-macro-expansion -Wno-documentation-unknown-command")

#~ set(CMAKE_EXE_LINKER_FLAGS "-pg")

# define variables

# include directories
include_directories(${PROJECT_SOURCE_DIR}/src)

# add subdirs
add_subdirectory(${PROJECT_SOURCE_DIR}/src)
add_subdirectory(${PROJECT_SOURCE_DIR}/doc)
add_subdirectory(${PROJECT_SOURCE_DIR}/test)
add_subdirectory(${PROJECT_SOURCE_DIR}/example)
