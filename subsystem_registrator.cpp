/* SPDX-License-Identifier: MIT */
#include "subsystem_registrator.h"

SuS::logfile::subsystem_registrator::subsystem_registrator(
      const std::string &_name) {
   m_subsystem = SuS::logfile::logger::instance()->register_subsystem(_name);
} // subsystem_registrator constructor

void SuS::logfile::subsystem_registrator::set_min_log_level(
      logger::log_level _level) {
   SuS::logfile::logger::instance()->set_subsystem_min_log_level(
         m_subsystem, _level);
} // subsystem_registrator::set_min_log_level
