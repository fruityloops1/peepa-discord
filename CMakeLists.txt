cmake_minimum_required(VERSION 3.5)
project(peepa-discord)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 20)

add_subdirectory(Library/DPP)
add_subdirectory(Library/ELFIO)
add_subdirectory(Library/cpp-httplib)

include_directories(Include)
include_directories(Library/DPP/include)
include_directories(Library/ELFIO)
include_directories(Library/fast-cpp-csv-parser)
include_directories(Library/cpp-httplib)

add_executable(peepa-discord Source/main.cpp Source/util.cpp Source/elfdata.cpp Source/SymbolData.cpp)
target_link_libraries(peepa-discord PRIVATE dpp elfio pthread)

file(COPY Data DESTINATION .)