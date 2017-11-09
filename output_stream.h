/* SPDX-License-Identifier: MIT */
#pragma once

#include "logger.h"

#include <iosfwd>
#include <memory>
#include <string>

namespace SuS {
namespace logfile {

struct log_event;
struct output_stream_private;

class LOGFILE_EXPORT output_stream {
 public:
   output_stream();
   virtual ~output_stream();

   virtual void dump(std::ostream &_stream);

   virtual std::string name() = 0;

   virtual unsigned retry_time();

   virtual void set_min_log_level(logger::log_level _level);

   bool write(const log_event &_le);

 private:
   virtual bool do_write(const log_event &_le) = 0;

   std::unique_ptr<output_stream_private> m_d;
}; // class output_stream

} // namespace logfile
} // namespace SuS
