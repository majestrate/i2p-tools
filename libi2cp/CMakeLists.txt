cmake_minimum_required(VERSION 2.6)
include(CheckIncludeFiles)
project(i2pc C)
set(PROJECT_VERSION "0.0.1")

option(ENABLE_GNUTLS "Enable GnuTLS." ON)

set(CMAKE_INSTALL_PREFIX "/usr/local" CACHE PATH "Installation prefix")
set(PROJECT_BINARY_INSTALL_DIR "bin")
set(PROJECT_INCLUDE_INSTALL_DIR "include")
set(PROJECT_LIBRARY_INSTALL_DIR "lib")

include_directories("${CMAKE_BINARY_DIR}/")
include_directories("${CMAKE_SOURCE_DIR}/includes")
include_directories("${CMAKE_SOURCE_DIR}/src")

# Setup compiler
if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wno-long-long -pedantic -fPIC")
endif()

find_package(PkgConfig)

# find nettle libraries
pkg_check_modules(NETTLE nettle)
link_directories(${NETTLE_LIBRARY_DIRS})
include_directories(${NETTLE_INCLUDE_DIRS})
if(!NETTL_FOUND)
  message( FATAL_ERROR "libnettle is required.")
endif ()


# find optional gnutls
if (ENABLE_GNUTLS)
  pkg_check_modules(GNUTLS gnutls)
  link_directories(${GNUTLS_LIBRARY_DIRS})
  include_directories(${GNUTLS_INCLUDE_DIRS})
  if (GNUTLS_FOUND)
    set(WITH_GNUTLS 1)
  else ()
    message( FATAL_ERROR "You have enabled use of GnuTLS which was not found on your system.")
  endif ()
endif()


set(LIBS ${NETTLE_LIBRARIES} ${HOGWEED_LIBRARIES} ${GNUTLS_LIBRARIES} "-lpthread" "-lz" "-lgmp")

configure_file(config.h.cmake i2cp/config.h)

set(LIBI2CP_SOURCES
  src/i2cp.c
  src/client.c
  src/crypto.c
  src/certificate.c
  src/datagram.c
  src/destination.c
  src/lease.c
  src/session.c
  src/session_config.c
  src/stringmap.c
  src/intmap.c
  src/tcp.c
  src/logger.c
  src/queue.c
  src/config_file.c
  src/version.c
)

#
# Generate libi2cp.pc Package config file
#
configure_file ("libi2cp.pc.in" "libi2cp.pc" @ONLY)
install (FILES "${CMAKE_CURRENT_BINARY_DIR}/libi2cp.pc"
         DESTINATION "${PROJECT_LIBRARY_INSTALL_DIR}/pkgconfig")

#
# Build i2cp library
#

# static
add_library(i2cp_static ${LIBI2CP_SOURCES})
target_link_libraries(i2cp_static ${LIBS})
set_target_properties(i2cp_static PROPERTIES OUTPUT_NAME i2cp)
install (FILES "${CMAKE_CURRENT_BINARY_DIR}/libi2cp.a"
         DESTINATION "${PROJECT_LIBRARY_INSTALL_DIR}")

# shared
add_library(i2cp SHARED ${LIBI2CP_SOURCES})
target_link_libraries(i2cp ${LIBS})
install (FILES "${CMAKE_CURRENT_BINARY_DIR}/libi2cp.so"
         DESTINATION "${PROJECT_LIBRARY_INSTALL_DIR}")

# install the headers
file(GLOB LIBI2CP_HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/includes/i2cp/*.h")
set(LIBI2CP_HEADERS ${LIBI2CP_HEADERS} "${CMAKE_CURRENT_BINARY_DIR}/i2cp/config.h")
install (FILES ${LIBI2CP_HEADERS}
         DESTINATION "${PROJECT_INCLUDE_INSTALL_DIR}/i2cp")

#
# Build tools
#
#add_executable(i2cp-echo tools/echo.c)
#target_link_libraries(i2cp-echo i2cp_static ${LIBS})

#add_executable(i2cp-lookup tools/lookup.c)
#target_link_libraries(i2cp-lookup i2cp_static)

#
# Build unit tests
#
#add_executable(test-stringmap tests/stringmap.c)
#target_link_libraries(test-stringmap i2cp_static)

#add_executable(test-intmap tests/intmap.c)
#target_link_libraries(test-intmap i2cp_static)

#add_executable(test-queue tests/queue.c)
#target_link_libraries(test-queue i2cp_static)

#add_executable(test-tcp tests/tcp.c)
#target_link_libraries(test-tcp i2cp_static)

#add_executable(test-i2cp tests/i2cp.c)
#target_link_libraries(test-i2cp i2cp_static)

#add_executable(test-crypto tests/crypto.c)
#target_link_libraries(test-crypto i2cp_static)

#add_executable(test-destination tests/destination.c)
#target_link_libraries(test-destination i2cp_static)

#add_executable(test-version tests/version.c)
#target_link_libraries(test-version i2cp_static)
