/* SPDX-License-Identifier: MIT */
#pragma once

// see https://issues.apache.org/jira/browse/AMQ-4710
#define AMQ_4710_workaround

#include "logger.h"
#include "logfile_export.h"
#include "output_stream.h"

#include <atomic>
#include <condition_variable>
#include <deque>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

#ifdef _WINDOWS
#undef ssize_t
typedef int ssize_t;
#endif

namespace SuS {

struct URL_info;

namespace logfile {

class tcp_client_socket;

class LOGFILE_EXPORT output_stream_stomp : public output_stream {
 public:
   output_stream_stomp(const std::string &_app_name, const std::string &_URL);

   virtual ~output_stream_stomp();

   virtual bool do_write(const log_event &_le) override;

   virtual std::string name() override;
   virtual unsigned retry_time() override;

   void reader_thread();

 private:
   // NOP when already connected. Just returns true in this case.
   bool connect();
   // intended to be run in a thread
   void connect_thread();

   void disconnect();

   ssize_t read_with_timeout(
         uint8_t *const _data, size_t _len, unsigned long _timeout_us);

   static std::string replace_all(
         std::string str, const std::string &_pattern, const std::string &_new);
   static std::string sanitize_string(const std::string &_in);

   const std::string m_app_name;
   //! Name of the host running the logger.
   std::string m_host;
   std::unique_ptr<URL_info> m_stomp_URL;
   std::string m_user;
   std::thread *m_heartbeat_thread;
   long m_heartbeat_interval{0};
   std::map<logger::log_level, const char *const> m_level_strings;
   unsigned m_receipt{0U};
#ifdef AMQ_4710_workaround
   bool m_last_was_data = false;
#endif

   std::string m_partial_reply;

   struct stomp_reply {
      std::string command;
      std::map<std::string, std::string> headers;
      std::string body;
   };
   std::deque<stomp_reply> m_reply_queue;
   std::mutex m_reply_mutex;
   std::condition_variable m_reply_cv;

   std::unique_ptr<tcp_client_socket> m_socket;

   bool m_connected{false};
   std::atomic<bool> m_connecting{false};
   std::thread m_connect_thread;
   std::atomic<bool> m_connect_thread_done{false};

   bool handle_reply(const std::string &_frame);

   bool check_server_version(const stomp_reply &_reply);
   bool parse_heartbeat(const stomp_reply &_reply);
}; // class output_stream_stomp
} // namespace logfile
} // namespace SuS
