/* SPDX-License-Identifier: MIT */
#include "output_stream_file.h"

#include "config.h"
#include "log_event.h"
#include "logger.h"

#include <iomanip>
#include <sstream>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>

SuS::logfile::output_stream_file::output_stream_file(
      const std::string &_filename, std::streampos _maxsize)
   : output_stream(), m_filename(_filename), m_maxsize(_maxsize) {
   open();
} // output_stream_file constructor

SuS::logfile::output_stream_file::~output_stream_file() {
   close();
}

std::string SuS::logfile::output_stream_file::name() {
   auto ret = std::string{"file: '"};
   ret.append(m_filename);
   ret.append("'");
   return ret;
}

bool SuS::logfile::output_stream_file::do_write(const log_event &_le) {
   if (!m_isopen) {
      if (!open()) {
         return false;
      }
   }

   std::ostringstream ss;
   ss << "<message level=\"" << SuS::logfile::logger::level_name(_le.level)
      << "\"><time>" << _le.time_string << "</time><subsystem>"
      << _le.subsystem_string << "</subsystem><function>" << _le.function
      << "</function><text>" << formatCData(_le.message) << "</text></message>"
      << std::endl;
   const auto line = ss.str();
   // TODO: too expensive to call tellp every time?
   // => how accurate is manual bookkeeping (line ending)?
   auto new_size = m_stream.tellp();
   new_size += line.size() + 12 /* "</logfile>\CR\LF" */;
   if (new_size > m_maxsize) {
      rotate();
   } // if

   m_stream << line;
   m_stream.flush();
   return m_stream.good();
} // output_stream_file::do_write

bool SuS::logfile::output_stream_file::open() {
   struct ::stat s;
   if (::stat(m_filename.c_str(), &s) == 0) {
#ifdef STRUCT_STAT_ST_MTIM_TV_NSEC
      auto tp = std::chrono::system_clock::from_time_t(s.st_mtim.tv_sec);
      tp += std::chrono::milliseconds(s.st_mtim.tv_nsec / (1000 * 1000));
      if (!archive(tp))
#elif defined STRUCT_STAT_ST_MTIME
      if (!archive(std::chrono::system_clock::from_time_t(s.st_mtime)))
#endif
         return false;
   }
   m_stream.open(m_filename);
   m_stream << "<logfile>" << std::endl;
   m_stream.flush();
   m_isopen = m_stream.good();
   return m_isopen;
}

bool SuS::logfile::output_stream_file::close() {
   m_stream << "</logfile>" << std::endl;
   m_stream.flush();
   m_stream.close();
   return archive();
}

bool SuS::logfile::output_stream_file::archive(
      const std::chrono::system_clock::time_point &t) {
   std::ostringstream as;
   as << m_filename << "-" << SuS::logfile::format_timestamp(t);
   if (::rename(m_filename.c_str(), as.str().c_str()) != 0)
      return false;
   return true;
}

bool SuS::logfile::output_stream_file::rotate() {
   close();
   return open();
}

std::string SuS::logfile::output_stream_file::formatCData(
      const std::string &_data) {
   std::ostringstream ss;
   ss << "<![CDATA[";
   std::string::size_type pos = 0U;
   for (auto i = _data.find("]]>", pos); i != std::string::npos;
         i = _data.find("]]>", pos)) {
      ss << _data.substr(pos, i - pos + 2);
      ss << "]]><![CDATA[";
      pos = i + 2;
   }
   ss << _data.substr(pos);
   ss << "]]>";
   return ss.str();
} // output_stream_file::formatCData
