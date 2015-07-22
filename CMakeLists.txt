cmake_minimum_required (VERSION 2.8)
project (makeadmin)

ENABLE_TESTING()

set (makeadmin_VERSION_MAJOR 1)
set (makeadmin_VERSION_MINOR 0)

find_package(Threads)

configure_file (
	"${PROJECT_SOURCE_DIR}/makeadmin_config.h.in"
	"${PROJECT_BINARY_DIR}/makeadmin_config.h"
)
include_directories("${PROJECT_BINARY_DIR}")

set(APP_SOURCES
	main.c
)

set(CORE_SOURCES
    sqlite3.c
    sqlite3ext.h
    sqlite3.h
    user.c
    user.h
)

set(TEST_SOURCES
    testmain.c
    CuTest.h
    CuTest.c
)

IF(CMAKE_SYSTEM MATCHES "Linux.*")
  set(PLATFORM_LIBS dl)
ENDIF(CMAKE_SYSTEM MATCHES "Linux.*")

add_executable(makeadmin ${APP_SOURCES} ${CORE_SOURCES})
target_link_libraries(makeadmin ${CMAKE_THREAD_LIBS_INIT} ${PLATFORM_LIBS})

add_executable(usertest ${CORE_SOURCES} ${TEST_SOURCES})
target_link_libraries(usertest ${CMAKE_THREAD_LIBS_INIT} ${PLATFORM_LIBS})
add_test(UnitTest ./usertest)
