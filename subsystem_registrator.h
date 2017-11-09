/* SPDX-License-Identifier: MIT */
#pragma once

#include "logger.h"

#include "logfile_export.h"

#include <string>

namespace SuS {
namespace logfile {

//! Helper class to automatically register subsystems on application startup.
/*!
 *  Initialize an instance of this class to have your subsystems registered
 *  when the applications starts, so that it is immediately seen by the
 *  logging system.
 *
 *  Typically this is used to initialize a variable in file-global scope,
 *  or a static variable of a class:
 *  @code
 *  namespace {
 *     subsystem_registrator log_id("my_subsystem");
 *  }
 *  @endcode
 *  and then
 *  @code
 *  SuS_LOG(finest, log_id(), "Hello world");
 *  @endcode
 *
 *  Internally, \ref `logger::register_subsystem()` is called, and the returned
 *  id is available through \ref `subsystem_registrator::operator()()`.
 *
 *  When the same subsystem name is registered several times, it is
 *  guaranteed that all registrations are merged and the same id is returned
 *  for all registrations.
 */
class LOGFILE_EXPORT subsystem_registrator {
 public:
   //! Initialize the class, thereby registering the subsystem.
   /*!
    *  @param _name The name of the subsystem to register.
    */
   subsystem_registrator(const std::string &_name);

   //! Return the logger id of the subsystem registered by this class.
   /*!
    *  @return The logger id for the subsystem.
    */
   SuS::logfile::logger::subsystem_t operator()() {
      return m_subsystem;
   } // operator()

   void set_min_log_level(logger::log_level _level);

 private:
   SuS::logfile::logger::subsystem_t m_subsystem;
}; // class subsystem_registrator

} // namespace logfile
} // namespace SuS
