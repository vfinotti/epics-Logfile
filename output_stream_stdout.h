/* SPDX-License-Identifier: MIT */
#pragma once

#include "output_stream.h"
#include "logfile_export.h"

#include <iostream>
#include <map>
#include <string>

namespace SuS {
namespace logfile {

class LOGFILE_EXPORT output_stream_stdout : public output_stream {
 public:
   output_stream_stdout(
         const std::string &_name = "", std::ostream &_stream = std::cout);
   virtual std::string name() override;
   virtual unsigned retry_time() override;

   virtual bool do_write(const log_event &_le) override;

 private:
   void init_colors();
   const std::map<SuS::logfile::logger::log_level, const char *> *m_colors;
   const std::string m_name;
   std::ostream &m_stream;
}; // class output_stream_stdout

} // namespace logfile
} // namespace SuS
