INCLUDE_DIRECTORIES (${EPICS_INCLUDES})
INCLUDE_DIRECTORIES (${CMAKE_CURRENT_SOURCE_DIR}/..)
INCLUDE_DIRECTORIES (${CMAKE_CURRENT_BINARY_DIR}/..)

ADD_LIBRARY (EPICSLog SHARED
      EPICSLog.cpp
      iocCommands.c
   )

TARGET_LINK_LIBRARIES (EPICSLog
      ${EPICS_LIBRARIES}
      Logfile
   )

INSTALL (TARGETS EPICSLog DESTINATION lib/${EPICS_HOST_ARCH})
INSTALL (FILES Logfile.dbd DESTINATION dbd)

# make the library compatible with the EPICS build system.
INSTALL (CODE "
   FILE(GLOB libs ${CMAKE_INSTALL_PREFIX}/lib/libLogfile.so*)
   FOREACH (l IN ITEMS \${libs})
      GET_FILENAME_COMPONENT (n \${l} NAME)
      EXECUTE_PROCESS (COMMAND ${CMAKE_COMMAND} -E create_symlink ../\${n} ${CMAKE_INSTALL_PREFIX}/lib/${EPICS_HOST_ARCH}/\${n})
   ENDFOREACH (l)
   ")
