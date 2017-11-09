/* SPDX-License-Identifier: MIT */
#pragma once

#include "output_stream.h"
#include "logfile_export.h"

#include <chrono>
#include <fstream>

namespace SuS {
namespace logfile {

class LOGFILE_EXPORT output_stream_file : public output_stream {
 public:
   output_stream_file(const std::string &_filename,
         std::streampos _maxsize = 10 * 1024 * 1024);
   virtual ~output_stream_file();

   virtual bool do_write(const log_event &_le) override;

   virtual std::string name() override;

 private:
   const std::string m_filename;
   std::ofstream m_stream;
   std::streampos m_maxsize;
   bool m_isopen;

   bool close();
   bool open();
   bool rotate();
   bool archive(const std::chrono::system_clock::time_point &t =
                      std::chrono::system_clock::now());

   static std::string formatCData(const std::string &_data);
}; // class output_stream_file

} // namespace logfile
} // namespace SuS
