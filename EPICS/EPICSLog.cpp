/* SPDX-License-Identifier: MIT */
#include "EPICSLog.h"

#include "logger.h"
#include "output_stream_file.h"
#include "output_stream_stomp.h"

#include <iostream>

void LogfileAddFileStream(const char *_filename) {
   SuS::logfile::logger::instance()->add_output_stream(
         new SuS::logfile::output_stream_file{_filename});
}

void LogfileAddStompStream(const char *_appname, const char *_server) {
   SuS::logfile::logger::instance()->add_output_stream(
         new SuS::logfile::output_stream_stomp{_appname, _server});
}

void LogfileDump() {
   SuS::logfile::logger::instance()->dump_configuration(std::cout);
}

void LogfileSetMinLevel(const char *_streamname, const char *_level) {
   using namespace SuS::logfile;
   try {
      const auto level = logger::level_by_name(_level);
      if (!logger::instance()->set_min_log_level(_streamname, level)) {
         std::cerr << "Unknown output stream '" << _streamname << "'"
                   << std::endl;
      }
   } catch (std::invalid_argument &) {
      std::cerr << "Unknown log level '" << _level << "'" << std::endl
                << "Known levels are:";
      for (const auto &i : logger::all_levels()) {
         std::cerr << "   - " << logger::level_name(i) << std::endl;
      }
      return;
   }
}

void LogfileSetSubsystemMinLevel(const char *_subsystem, const char *_level) {
   using namespace SuS::logfile;
   try {
      const auto level = logger::level_by_name(_level);
      const auto sub_id = logger::instance()->find_subsystem(_subsystem);
      logger::instance()->set_subsystem_min_log_level(sub_id, level);
   } catch (std::invalid_argument &_ex) {
      _ex.what();
      return;
   }
}
