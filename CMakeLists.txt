CMAKE_MINIMUM_REQUIRED(VERSION 3.1)

PROJECT (Logfile)

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
   # using Clang
   ADD_DEFINITIONS(
      -Wall
   )
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
   # using GCC
   ADD_DEFINITIONS(
      -Wall
   )
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Intel")
   # using Intel C++
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
   # using Visual Studio C++
   ADD_DEFINITIONS(
      /W3
      /wd4251
      /wd4068
   )
endif()

INCLUDE (CheckCXXCompilerFlag)
CHECK_CXX_COMPILER_FLAG("-flto" HAS_LTO)
IF (HAS_LTO)
   SET (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -flto")
ENDIF (HAS_LTO)

# 200809L for nanoseconds in stat
SET (FEATURE_TESTS "-D_POSIX_SOURCE -D_POSIX_C_SOURCE=200809L")
SET (CMAKE_REQUIRED_DEFINITIONS "${FEATURE_TESTS}")
ADD_DEFINITIONS ("${FEATURE_TESTS}")

SET (CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}")

FIND_PACKAGE (EPICS)
FIND_PACKAGE (OpenSSL)

INCLUDE (CheckSymbolExists)

IF (${CMAKE_SYSTEM_NAME} STREQUAL "SunOS")
   SET (CMAKE_REQUIRED_LIBRARIES "socket;nsl")
ENDIF (${CMAKE_SYSTEM_NAME} STREQUAL "SunOS")

CHECK_SYMBOL_EXISTS (backtrace_symbols "execinfo.h" HAVE_BACKTRACE_SYMBOLS)
CHECK_SYMBOL_EXISTS (CaptureStackBackTrace "Windows.h" HAVE_CAPTURESTACKBACKTRACE)
CHECK_SYMBOL_EXISTS (getaddrinfo "netdb.h" HAVE_GETADDRINFO)
CHECK_SYMBOL_EXISTS (inet_ntop "arpa/inet.h" HAVE_INET_NTOP)
CHECK_SYMBOL_EXISTS (prctl "sys/prctl.h" HAVE_PRCTL)
CHECK_SYMBOL_EXISTS (sigaction "signal.h" HAVE_SIGACTION)
CHECK_SYMBOL_EXISTS (strerror_r "string.h" HAVE_STRERROR_R)
CHECK_SYMBOL_EXISTS (geteuid "unistd.h;sys/types.h" HAVE_GETEUID)
CHECK_SYMBOL_EXISTS (sysconf "unistd.h" HAVE_SYSCONF)

INCLUDE (CheckCXXSourceCompiles) 
CHECK_CXX_SOURCE_COMPILES( "
#include <cstring>
int main() { char cb[64]; return *(strerror_r(1, cb, sizeof(cb))) == 'x'; }
" STRERROR_R_CHAR_P )

CHECK_CXX_SOURCE_COMPILES("
#include <thread>
static void test() {}
int main() { std::thread x(test); return 0; }
" DONT_NEED_PTHREAD)

INCLUDE (CheckStructHasMember)
CHECK_STRUCT_HAS_MEMBER ("struct stat" st_mtim.tv_nsec "sys/stat.h;sys/types.h" STRUCT_STAT_ST_MTIM_TV_NSEC)
CHECK_STRUCT_HAS_MEMBER ("struct stat" st_mtime "sys/stat.h;sys/types.h" STRUCT_STAT_ST_MTIME)

INCLUDE (CheckIncludeFiles)
CHECK_INCLUDE_FILES ("arpa/inet.h" HAVE_ARPA_INET_H)
CHECK_INCLUDE_FILES ("netdb.h" HAVE_NETDB_H)
CHECK_INCLUDE_FILES ("pwd.h" HAVE_PWD_H)
CHECK_INCLUDE_FILES ("sys/param.h" HAVE_SYS_PARAM_H)
CHECK_INCLUDE_FILES ("sys/select.h" HAVE_SYS_SELECT_H)
CHECK_INCLUDE_FILES ("sys/socket.h" HAVE_SYS_SOCKET_H)
CHECK_INCLUDE_FILES ("unistd.h" HAVE_UNISTD_H)

SET (Logfile_sources
      line_splitter.cpp
      logger.cpp
      logger_private.cpp
      log_event.cpp
      log_thread.cpp
      retry_thread.cpp
      output_stream.cpp
      output_stream_file.cpp
      output_stream_stdout.cpp
      output_stream_stomp.cpp
      parse_url.cpp
      subsystem_registrator.cpp
      tcp_client_socket.cpp
   )

SET (Logfile_headers
      line_splitter.h
      logger.h
      logger_private.h
      log_event.h
      log_thread.h
      retry_thread.h
      output_stream.h
      output_stream_file.h
      output_stream_stdout.h
      output_stream_stomp.h
      parse_url.h
      subsystem_registrator.h
      tcp_client_socket.h
      tcs_private.h
   )

SOURCE_GROUP (Headers FILES ${Logfile_headers})

ADD_LIBRARY (Logfile SHARED
      ${Logfile_sources}
      ${Logfile_headers}
   )

SET_TARGET_PROPERTIES (Logfile PROPERTIES
      VERSION 1.0.0
      SOVERSION 1
   )

SET_PROPERTY (TARGET Logfile PROPERTY CXX_STANDARD 11)
SET_PROPERTY (TARGET Logfile PROPERTY CXX_STANDARD_REQUIRED ON)

INCLUDE (GenerateExportHeader)
SET (CMAKE_CXX_VISIBILITY_PRESET hidden)
SET (CMAKE_VISIBILITY_INLINES_HIDDEN 1)
GENERATE_EXPORT_HEADER (Logfile)

IF (NOT DONT_NEED_PTHREAD)
   IF (${CMAKE_VERSION} VERSION_LESS 2.8.12)
      TARGET_LINK_LIBRARIES (Logfile pthread)
   ELSE (${CMAKE_VERSION} VERSION_LESS 2.8.12)
      TARGET_LINK_LIBRARIES (Logfile PRIVATE pthread)
   ENDIF (${CMAKE_VERSION} VERSION_LESS 2.8.12)
ENDIF (NOT DONT_NEED_PTHREAD)

IF (HAVE_CAPTURESTACKBACKTRACE)
   IF (${CMAKE_VERSION} VERSION_LESS 2.8.12)
      TARGET_LINK_LIBRARIES (Logfile DbgHelp)
   ELSE (${CMAKE_VERSION} VERSION_LESS 2.8.12)
      TARGET_LINK_LIBRARIES (Logfile PRIVATE DbgHelp)
   ENDIF (${CMAKE_VERSION} VERSION_LESS 2.8.12)
ENDIF (HAVE_CAPTURESTACKBACKTRACE)

IF (${CMAKE_SYSTEM_NAME} STREQUAL "SunOS")
   IF (${CMAKE_VERSION} VERSION_LESS 2.8.12)
      TARGET_LINK_LIBRARIES (Logfile socket nsl)
   ELSE (${CMAKE_VERSION} VERSION_LESS 2.8.12)
      TARGET_LINK_LIBRARIES (Logfile PRIVATE socket nsl)
   ENDIF (${CMAKE_VERSION} VERSION_LESS 2.8.12)
ENDIF (${CMAKE_SYSTEM_NAME} STREQUAL "SunOS")

IF (${CMAKE_SYSTEM_NAME} STREQUAL "OpenBSD")
      TARGET_LINK_LIBRARIES (Logfile PRIVATE -pthread)
ENDIF (${CMAKE_SYSTEM_NAME} STREQUAL "OpenBSD")

IF (${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
      TARGET_LINK_LIBRARIES (Logfile PRIVATE Ws2_32)
ENDIF (${CMAKE_SYSTEM_NAME} STREQUAL "Windows")

ADD_SUBDIRECTORY (Qt)
ADD_SUBDIRECTORY (examples)
IF (EPICS_FOUND)
   ADD_SUBDIRECTORY (EPICS)
ENDIF (EPICS_FOUND)

CONFIGURE_FILE (${CMAKE_CURRENT_SOURCE_DIR}/config.h.in ${CMAKE_CURRENT_BINARY_DIR}/config.h)
INCLUDE_DIRECTORIES (${CMAKE_CURRENT_BINARY_DIR})

IF (OPENSSL_FOUND)
   INCLUDE_DIRECTORIES (${OPENSSL_INCLUDE_DIR})
   IF (${CMAKE_VERSION} VERSION_LESS 2.8.12)
      TARGET_LINK_LIBRARIES (Logfile ssl crypto)
   ELSE (${CMAKE_VERSION} VERSION_LESS 2.8.12)
      TARGET_LINK_LIBRARIES (Logfile PRIVATE ssl crypto)
   ENDIF (${CMAKE_VERSION} VERSION_LESS 2.8.12)
ENDIF (OPENSSL_FOUND)

INSTALL (TARGETS Logfile DESTINATION lib)
INSTALL (FILES logger.h DESTINATION include)
INSTALL (FILES ${CMAKE_CURRENT_BINARY_DIR}/logfile_export.h DESTINATION include)
INSTALL (FILES output_stream.h DESTINATION include)
INSTALL (FILES output_stream_file.h DESTINATION include)
INSTALL (FILES output_stream_stomp.h DESTINATION include)
INSTALL (FILES subsystem_registrator.h DESTINATION include)
INSTALL (FILES cmake-scripts/FindLogfile.cmake cmake-scripts/LibFindMacros.cmake DESTINATION cmake)

