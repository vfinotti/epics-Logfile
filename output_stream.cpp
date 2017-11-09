/* SPDX-License-Identifier: MIT */
#include "output_stream.h"

#include "log_event.h"
#include "output_stream_private.h"

#include <iostream>

SuS::logfile::output_stream::output_stream() : m_d{new output_stream_private} {
   m_d->m_minLogLevel = SuS::logfile::logger::log_level::SuS_LOG_MINLEVEL;
} // output_stream::constructor

SuS::logfile::output_stream::~output_stream() {
} // output_stream::destructor

void SuS::logfile::output_stream::dump(std::ostream &_stream) {
   _stream << "   - " << name() << std::endl
           << "     min. log level: "
           << SuS::logfile::logger::level_name(m_d->m_minLogLevel) << std::endl;
}

unsigned SuS::logfile::output_stream::retry_time() {
   return 30;
} // output_stream::retry_time

void SuS::logfile::output_stream::set_min_log_level(logger::log_level _level) {
   m_d->m_minLogLevel = _level;
}

bool SuS::logfile::output_stream::write(const log_event &_le) {
   if (_le.level < m_d->m_minLogLevel)
      return true /* no error */;
   return do_write(_le);
}
