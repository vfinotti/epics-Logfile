CMAKE_MINIMUM_REQUIRED (VERSION 3.1)

FIND_PACKAGE (Qt4 4.5.3 COMPONENTS QtCore QtGui)
INCLUDE (${QT_USE_FILE})

IF (QT4_FOUND)
   MESSAGE (STATUS "Package Qt found!")

   ADD_DEFINITIONS (${QT_DEFINITIONS})

   INCLUDE_DIRECTORIES (
         ${CMAKE_CURRENT_SOURCE_DIR}
         ${CMAKE_CURRENT_BINARY_DIR}
         ${CMAKE_CURRENT_SOURCE_DIR}/..
         ${CMAKE_CURRENT_BINARY_DIR}/..
         ${QT_INCLUDE_DIR}
      )

   SET (QLogViewer_sources
         QFilteredLogViewer.cpp
         QLogList.cpp
         QLogViewer.cpp
      )
   SET (QLogViewer_headers
         QFilteredLogViewer.h
         QLogList.h
         QLogViewer.h
      )
   QT4_WRAP_CPP (BC_MOC_SRCS ${QLogViewer_headers})

   ADD_LIBRARY (QLogViewer SHARED
         ${QLogViewer_sources}

         ${BC_MOC_SRCS}
      )

   set_property(TARGET QLogViewer PROPERTY CXX_STANDARD 11)
   set_property(TARGET QLogViewer PROPERTY CXX_STANDARD_REQUIRED ON)

   INSTALL (TARGETS QLogViewer DESTINATION lib)
   INSTALL (FILES QFilteredLogViewer.h DESTINATION include)
ENDIF (QT4_FOUND)

