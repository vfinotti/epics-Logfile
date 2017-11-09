/* SPDX-License-Identifier: MIT */
#ifdef _MSC_BUILD
// to be able to use std::max => max is a #define otherwise.
#define NOMINMAX
#endif

#include "log_thread.h"

#include "config.h"
#include "log_event.h"
#include "output_stream_stdout.h"
#include "retry_thread.h"

#include <algorithm>
#include <cassert>
#include <sstream>
#include <string.h>
#ifdef HAVE_PRCTL
#include <sys/prctl.h>
#endif

SuS::logfile::log_thread::log_thread() : m_thread(), m_do_terminate(false) {
   m_streams.emplace(std::string("stdout"), new output_stream_stdout);
} // log_thread constructor

SuS::logfile::log_thread::~log_thread() {
#if 0
   std::cout << "max event queue size: " << m_max_event_queue_size
      << std::endl << std::endl;
#endif

   // doesn't work with QLogList: Gets freed by Qt
   // but live with that crash on exit for now instead of penalizing all
   // other streams.
   for (const auto &i : m_streams) {
      delete i.second;
   } // for i
} // log_thread destructor

void SuS::logfile::log_thread::enqueue(const log_event &_event) {
   m_mutex.lock();
   m_events.emplace_back(_event);
   m_max_event_queue_size = std::max(m_max_event_queue_size, m_events.size());
   m_mutex.unlock();
   m_cond.notify_one();
} // log_thread::enqueue

void SuS::logfile::log_thread::start() {
   std::thread thread(run, this);
   m_thread.swap(thread);
} // log_thread::start

void SuS::logfile::log_thread::run(log_thread *_instance) {
// additionally check for PR_GET_NAME since the prctl function is older than
// PR_GET_NAME (and PR_GET_NAME is newer than PR_SET_NAME).
#if defined HAVE_PRCTL && defined PR_GET_NAME
   char threadname[17];
   ::prctl(PR_GET_NAME, threadname, 0, 0, 0);
   // "The buffer should allow space for up to 16 bytes; the returned string
   // will be null-terminated *if it is shorter than that*."
   threadname[16] = '\0';
   ::strncat(threadname, " (log)", 16 - ::strlen(threadname));
   ::prctl(PR_SET_NAME, threadname, 0, 0, 0);
#endif
   _instance->do_run();
} // log_thread::run

void SuS::logfile::log_thread::join() {
   m_thread.join();
} // log_thread::join

void SuS::logfile::log_thread::terminate() {
   m_mutex.lock();
   m_do_terminate = true;
   m_mutex.unlock();
   m_cond.notify_one();
   join();
} // log_thread::terminate

void SuS::logfile::log_thread::log(const log_event &_event) {
   _event.time_string = format_time(_event.time);

   for (const auto &j : m_streams) {
      auto t = m_retry_map.find(j.second);
      if (t != m_retry_map.end()) {
         if (!t->second) {
            m_retry_map.erase(t);
         } else if (!t->second->active()) {
            // the retry thread's queue is empty => back to normal operation
            t->second->m_thread.join();
            delete t->second;
            m_retry_map.erase(t);
         } else {
            t->second->enqueue(_event);
            continue;
         } // else
      }    // if

      if (!j.second->write(_event)) {
         retry_thread *thread = nullptr;
         try {
            thread = new retry_thread(j.second);
         } catch (const std::system_error &e) {
            std::cerr << "Initializing retry_thread: Caught system_error "
                         "with code " << e.code() << ", meaning " << e.what()
                      << std::endl;
         }
         if (thread) {
            thread->enqueue(_event);
            m_retry_map.emplace(j.second, thread);
         }
      }
   } // for j
} // log_thread::log

void SuS::logfile::log_thread::do_run() {
   while (true) {
      // take the accumulated events and give the other thread a new queue
      // to fill.
      event_queue_t local;
      m_mutex.lock();
      m_events.swap(local); // swap is O(1) => mutex is not locked for long
      m_mutex.unlock();

      // now take our time to process the events
      for (const auto &i : local) {
         log(i);
      } // while
      local.clear();

      // wait until there is work to do or termination is requested
      std::unique_lock<std::mutex> lock(m_mutex);
      if (m_do_terminate && m_events.empty()) {
         lock.unlock();
         auto still_active = false;
         for (auto &i : m_retry_map) {
            if (i.second->active()) {
               still_active = true;
            } else {
               i.second->m_thread.join();
               delete i.second;
               i.second = nullptr;
            }
         }
         if (still_active) {
            // avoid a tight loop
            std::this_thread::sleep_for(std::chrono::seconds(1U));
            // and do not wait for a signal
            continue;
         } else {
            lock.lock();
            if (m_events.empty()) {
               // no messages in our queue
               // + no retry threads still active
               // => thread done
               return;
            } else {
               // more messages arrived while we checked the retry threads.
               lock.unlock();
               continue;
            }
         }
      } // if
      // the mutex is still locked, so we know that m_do_terminate is false
      // if !m_events.size().
      if (m_events.empty()) {
         // m_events.size > 0: events have been put in the queue in the
         // meantime. their cond_signal events have been missed!
         m_cond.wait(lock);
      } // if
   }    // while
} // log_thread::do_run
