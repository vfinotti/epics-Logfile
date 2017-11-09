/* SPDX-License-Identifier: MIT */
#ifdef _MSC_BUILD
#define NOMINMAX
#endif

#include <chrono>
#include <ctime>
#include <deque>
#include <iostream>
#include <limits>
#include <map>

#include "logger.h"
#include "retry_thread.h"
#include "output_stream.h"
#include "log_event.h"

namespace {

void thread_main(SuS::logfile::retry_thread *const _this) {
   _this->run();
} // thread_main

// expiry times for the different log levels in seconds
const std::map<SuS::logfile::logger::log_level, const std::time_t> expiry_time =
      {
            {SuS::logfile::logger::log_level::finest, 900},
            {SuS::logfile::logger::log_level::finer, 900},
            {SuS::logfile::logger::log_level::fine, 1800},
            {SuS::logfile::logger::log_level::config, 1800},
            {SuS::logfile::logger::log_level::info, 3600},
            {SuS::logfile::logger::log_level::warning, 10 * 3600},
            {SuS::logfile::logger::log_level::severe,
                  std::numeric_limits<std::time_t>::max()},
};
} // namespace

SuS::logfile::retry_thread::retry_thread(output_stream *const _stream)
   : m_stream(_stream), m_thread(thread_main, this) {
   std::cout << "starting retry thread for " << m_stream->name() << std::endl;
} // retry_thread constructor

bool SuS::logfile::retry_thread::active() {
   std::lock_guard<std::mutex> lock(m_mutex);
   return !m_event_queue.empty();
}

void SuS::logfile::retry_thread::enqueue(const log_event &_le) {
   std::lock_guard<std::mutex> lock(m_mutex);
   m_event_queue.push_back(_le);
} // retry_thread::enqueue

void SuS::logfile::retry_thread::run() {
   while (true) {
      std::this_thread::sleep_for(std::chrono::seconds(m_stream->retry_time()));

      // try to write
      for (;;) {
         m_mutex.lock();
         if (m_event_queue.empty()) {
            m_mutex.unlock();
            break;
         } // if

         const auto &first_event = m_event_queue.front();
         m_mutex.unlock();
         if (!m_stream->write(first_event)) {
            break;
         } // if
         // written successfully => erase from queue
         m_mutex.lock();
         m_event_queue.pop_front();
         m_mutex.unlock();
      } // forever

      // expire what has not been written
      const auto tpnow = std::chrono::system_clock::now();
      m_mutex.lock();
      auto i = m_event_queue.begin();
      auto expired = 0U;
      while (i != m_event_queue.end()) {
         if (std::chrono::duration_cast<std::chrono::seconds>(tpnow - i->time)
                     .count() > expiry_time.at(i->level)) {
            i = m_event_queue.erase(i);
            ++expired;
         } else
            ++i;
      } // for i
      m_mutex.unlock();

      if (expired > 0)
         std::cerr << "expired " << expired << " entries for logger \""
                   << m_stream->name() << "\"" << std::endl;

      // terminate, if the queue is empty.
      if (!active())
         break;
   } // while
   std::cout << "stopping retry thread for " << m_stream->name() << std::endl;
} // retry_thread::run
