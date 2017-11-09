/* SPDX-License-Identifier: MIT */
#pragma once

#include "log_event.h"
#include "logger.h"

#include <condition_variable>
#include <map>
#include <thread>

namespace SuS {
namespace logfile {

class output_stream;
class retry_thread;

//! The thread handling the distribution of the log messages.
/*! When a log message is sent, it is put in a FIFO queue of the thread
 *  (\ref m_events). The application then resumes. From within the logging
 *  thread, the message is picked from the queue and processed by passing it
 *  to all configured sinks in turn. When the delivery fails for a sink, a
 *  \ref retry_thread is started for the sink and further messages for the
 *  sink are queue in the retry thread of the sink instead of directly trying
 *  to deliver to the sink. When all messages queued in the retry thread have
 *  been delivered, normal delivery resumes.
 */
class log_thread {
 public:
   log_thread();
   ~log_thread();

   //! Put a new message in the queue and wake up the logging thread.
   void enqueue(const log_event &_event);

   void start();

   void join();

   void terminate();

   typedef std::map<std::string, output_stream *> stream_list_t;
   stream_list_t m_streams;

 private:
   //! List of log events waiting to be processed.
   event_queue_t m_events;

   //! Deliver a log message.
   void log(const log_event &_event);

   //! Helper function to start the logging thread.
   static void run(log_thread *_instance);

   //! Main function of the logging thread.
   void do_run();

   std::thread m_thread;
   //! Mutex protecting access to \ref m_events.
   std::mutex m_mutex;
   //! CV to wake up the thread after queuing a new message or stop request.
   std::condition_variable m_cond;

   bool m_do_terminate;

   size_t m_max_event_queue_size{0};

   typedef std::map<output_stream *, retry_thread *> retry_map_t;
   retry_map_t m_retry_map;
}; // class log_thread

} // namespace logfile
} // namespace SuS
