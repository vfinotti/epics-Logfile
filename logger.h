/* SPDX-License-Identifier: MIT */
/*! @file */
#pragma once

#include <iosfwd>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "logfile_export.h"

namespace SuS {
namespace logfile {

struct logger_private;
class output_stream;

//! The main class of the logging system.
class LOGFILE_EXPORT logger {
 public:
   //! Enumeration type of all available log levels ordered by severity.
   enum class log_level { finest, finer, fine, config, info, warning, severe };

   //! Type of the subsystem registration handle.
   typedef unsigned subsystem_t;

   //! Get the singleton instance of the logger.
   static logger *instance();

   static const char *level_name(log_level _l);
   static log_level level_by_name(const std::string &_name);
   static std::vector<log_level> all_levels();

   //! Submit a log message.
   /*! This is normally called through one of the SuS_LOG macros.
    *
    *  @param _level The log level to use.
    *  @param _subsystem The id of the subsystem initiating the logging.
    *  @param _message The actual log message.
    *  @param _function The name of the function creating the log message.
    */
   void log(const log_level _level, const subsystem_t _subsystem,
         const std::string &_message,
         const std::string &_function = "/UNKNOWN/");

   //! Register a subsystem with the logging system.
   /*!
    *  Calling this function several times with the same _name will always
    *  return the same id.
    *
    *  @param _name The name of the subsystem to register.
    *  @return The logger id for this subsystem.
    */
   subsystem_t register_subsystem(const std::string &_name);

   subsystem_t find_subsystem(const std::string &_name);

   void set_subsystem_min_log_level(subsystem_t _subsystem, log_level _level);

   void add_output_stream(
         output_stream *const _stream, const std::string &_name = "");

   //! Remove an output stream previously registered with add_output_stream.
   /*!
    * @param _name name of stream to remove
    * @return true, if stream was successfully removed
    */
   bool remove_output_stream(const std::string &_name);

   //! Set the minimum log level for an output stream.
   /*!
    *  @param _stream The name of the stream to modify.
    *  @param _level The new minimum log level for the stream.
    *  @return True on success. False, if the stream could not be found by name.
    */
   bool set_min_log_level(const std::string &_stream, log_level _level);

   //! Dump an overview of the current logger configuration.
   /*!
    *  The dump includes the minimum log level defined at compile time,
    *  a list of all active output streams with the respective active minimum
    *  log level, and a list of all known subsystems.
    *
    *  @param _stream The stream to dump to (e.g. `std::cout`).
    */
   void dump_configuration(std::ostream &_stream);

   static std::string string_format(const char *fmt, ...)
#if defined __GNUC__ || defined __clang__
         __attribute__((__format__(__printf__, 1, 2)))
#endif
         ;

 private:
   logger();

   ~logger();

   std::unique_ptr<logger_private> m_d;

   static logger *s_instance;
   static std::mutex s_instance_mutex;

   static void (*s_old_sighandler)(int);
   static void signal_handler(int _signal);
   static void atexit_handler();
   static bool gdb_backtrace();

   void terminate();
}; // class logger

} // namespace logfile
} // namespace SuS

/*! \def SuS_FUNCNAME
 * \brief Helper macro forwarding to the magic variable that contains the
 * current function name that is provided by the compiler.
 */
#ifdef _MSC_BUILD
#define SuS_FUNCNAME __FUNCSIG__
#else
#define SuS_FUNCNAME __PRETTY_FUNCTION__
#endif

#ifndef SuS_LOG_MINLEVEL
//! Compile-time definition of the minimum log level.
/*!
 *  Override this from the compiler invocation to set a different level, e.g.
 *  for release builds (set to the name of a member of
 *  SuS::logfile::logger::log_level).
 *
 *  Calls to \ref SuS_LOG, \ref SuS_LOG_STREAM, and \ref SuS_LOG_PRINTF
 *  will be completely eliminated at compile time if they are for messages
 *  with a log level smaller than \ref SuS_LOG_MINLEVEL.
 */
#define SuS_LOG_MINLEVEL finest
#endif

//! Log a fixed string.
/*!
 *  @param l The log level to use (member of SuS::logfile::logger::log_level).
 *  @param sys The id of the subsystem initiating the logging.
 *  @param msg The actual log message.
 */
#define SuS_LOG(l, sys, msg)                                                   \
   if (SuS::logfile::logger::log_level::l <                                    \
         SuS::logfile::logger::log_level::SuS_LOG_MINLEVEL)                    \
      ;                                                                        \
   else {                                                                      \
      SuS::logfile::logger::instance()->log(                                   \
            SuS::logfile::logger::log_level::l, sys, msg, SuS_FUNCNAME);       \
   }

//! Log with a printf interface.
/*!
 *  Example:
 *  @code
 *  SuS_LOG_PRINTF(info, log_id(), "3 * 3 = %d", (3 * 3));
 *  @endcode
 *
 *  @param l The log level to use (member of SuS::logfile::logger::log_level).
 *  @param sys The id of the subsystem initiating the logging.
 *  @param format The format string. Use printf syntax.
 *  @param ... The replacements.
 */
#define SuS_LOG_PRINTF(l, sys, format, ...)                                    \
   if (SuS::logfile::logger::log_level::l <                                    \
         SuS::logfile::logger::log_level::SuS_LOG_MINLEVEL)                    \
      ;                                                                        \
   else {                                                                      \
      SuS::logfile::logger::instance()->log(                                   \
            SuS::logfile::logger::log_level::l, sys,                           \
            SuS::logfile::logger::string_format(format, __VA_ARGS__),          \
            SuS_FUNCNAME);                                                     \
   }

//! Log with an ostringstream interface.
/*!
 *  Example:
 *  @code
 *  SuS_LOG_STREAM(info, log_id(), "1 + 1 = " << (1 + 1));
 *  @endcode
 *
 *  @param l The log level to use (member of SuS::logfile::logger::log_level).
 *  @param sys The id of the subsystem initiating the logging.
 *  @param msg The actual log message. This parameter will be <<-ed into an
 *  std::ostringstream, so C++ stream syntax can be used.
 */
#define SuS_LOG_STREAM(l, sys, msg)                                            \
   if (SuS::logfile::logger::log_level::l <                                    \
         SuS::logfile::logger::log_level::SuS_LOG_MINLEVEL)                    \
      ;                                                                        \
   else {                                                                      \
      std::ostringstream very_unlikely_NaMe;                                   \
      very_unlikely_NaMe << msg;                                               \
      SuS::logfile::logger::instance()->log(                                   \
            SuS::logfile::logger::log_level::l, sys, very_unlikely_NaMe.str(), \
            SuS_FUNCNAME);                                                     \
   }
