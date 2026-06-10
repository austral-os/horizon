set(LIBRARY_NAME horizon-apt)

find_package(PkgConfig REQUIRED)
pkg_check_modules(SOUP REQUIRED libsoup-3.0)

file(GLOB_RECURSE SOURCES "src/*.cpp")

add_library(${LIBRARY_NAME} SHARED ${SOURCES})

target_include_directories(${LIBRARY_NAME}
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${PROJECT_SOURCE_DIR}/include
        ${SOUP_INCLUDE_DIRS}
)

# Link against apt-pkg and libsoup
target_link_libraries(${LIBRARY_NAME}
    PUBLIC
        horizon
        apt-pkg
        ${SOUP_LIBRARIES}
)

set_target_properties(${LIBRARY_NAME} PROPERTIES POSITION_INDEPENDENT_CODE ON)

install(TARGETS ${LIBRARY_NAME} DESTINATION lib COMPONENT libhorizon-apt)

install(DIRECTORY include/
    DESTINATION include
    COMPONENT libhorizon-apt-dev
)

# Tests
option(BUILD_TESTING "Build tests" ON)
if(BUILD_TESTING)
    enable_testing()
    add_executable(${LIBRARY_NAME}-tests tests/AptManagerTest.cpp)
    target_link_libraries(${LIBRARY_NAME}-tests PRIVATE ${LIBRARY_NAME} apt-pkg ${SOUP_LIBRARIES})
    add_test(NAME ${LIBRARY_NAME}-tests COMMAND ${LIBRARY_NAME}-tests)

    add_executable(${LIBRARY_NAME}-appstore-tests tests/AppStoreClientTest.cpp)
    target_link_libraries(${LIBRARY_NAME}-appstore-tests PRIVATE ${LIBRARY_NAME} apt-pkg ${SOUP_LIBRARIES})
    add_test(NAME ${LIBRARY_NAME}-appstore-tests COMMAND ${LIBRARY_NAME}-appstore-tests)
endif()
