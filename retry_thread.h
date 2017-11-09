/* SPDX-License-Identifier: MIT */
#pragma once

#include "log_event.h"

#include <mutex>
#include <thread>

namespace SuS {
namespace logfile {

class log_thread;
class output_stream;

class retry_thread {
 public:
   retry_thread(output_stream *const _stream);

   bool active();
   void enqueue(const log_event &_le);
   void run();

 protected:
   friend class SuS::logfile::log_thread;

   output_stream *const m_stream;
   std::thread m_thread;
   event_queue_t m_event_queue;
   std::mutex m_mutex;
}; // class retry_thread

} // namespace logfile
} // namespace SuS
