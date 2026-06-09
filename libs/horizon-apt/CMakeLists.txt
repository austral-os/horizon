cmake_minimum_required(VERSION 3.10)
project(horizon-apt)

set(CMAKE_CXX_STANDARD 17)

file(GLOB_RECURSE SOURCES "src/*.cpp")

add_library(horizon-apt SHARED ${SOURCES})

target_include_directories(horizon-apt
    PUBLIC
        ${PROJECT_SOURCE_DIR}/include
        ${PROJECT_SOURCE_DIR}/../../include
        ${PROJECT_SOURCE_DIR}/../../src
)

# Link against apt-pkg
target_link_libraries(horizon-apt
    PUBLIC
        horizon
        apt-pkg
)

set_target_properties(horizon-apt PROPERTIES POSITION_INDEPENDENT_CODE ON)

install(TARGETS horizon-apt DESTINATION lib COMPONENT libhorizon-apt)

install(DIRECTORY include/
    DESTINATION include
    COMPONENT libhorizon-apt-dev
)

# Tests
option(BUILD_TESTING "Build tests" ON)
if(BUILD_TESTING)
    enable_testing()
    add_executable(horizon-apt-tests tests/AptManagerTest.cpp)
    target_link_libraries(horizon-apt-tests PRIVATE horizon-apt apt-pkg)
    add_test(NAME horizon-apt-tests COMMAND horizon-apt-tests)
endif()
