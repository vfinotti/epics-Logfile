# - Try to find Logfile
# Once done, this will define
#
#  Logfile_FOUND - system has Logfile
#  Logfile_INCLUDE_DIRS - the Logfile include directories
#  Logfile_LIBRARIES - link these to use Logfile

include(LibFindMacros)

# Use pkg-config to get hints about paths
libfind_pkg_check_modules(Logfile_PKGCONF Logfile)

# Include dir
find_path(Logfile_INCLUDE_DIR
  NAMES logger.h
  PATHS ${Logfile_PKGCONF_INCLUDE_DIRS} /opt/epics/modules/Logfile/include/
)

# Finally the library itself
find_library(Logfile_LIBRARY
  NAMES Logfile
  PATHS ${Logfile_PKGCONF_LIBRARY_DIRS} /opt/epics/modules/Logfile/lib/
)

set(Logfile_PROCESS_INCLUDES Logfile_INCLUDE_DIR)
set(Logfile_PROCESS_LIBS Logfile_LIBRARY)
libfind_process(Logfile)

