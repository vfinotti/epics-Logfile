/* SPDX-License-Identifier: MIT */
#include "logger.h"

#include "config.h"
#include "line_splitter.h"
#include "log_thread.h"
#include "log_event.h"
#include "logger_private.h"
#include "output_stream.h"
#include "subsystem_registrator.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <chrono>
#include <iostream>
#include <signal.h>
#include <sstream>
#include <stdarg.h>
#include <stdlib.h>

#ifdef HAVE_BACKTRACE_SYMBOLS
#include <execinfo.h>
#elif defined HAVE_CAPTURESTACKBACKTRACE
#include <Windows.h>
#include <DbgHelp.h>
#endif

#ifdef __unix__
#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

namespace {
SuS::logfile::subsystem_registrator log_id("logger");
} // namespace

// STATIC MEMBER VARIABLES
SuS::logfile::logger *SuS::logfile::logger::s_instance = nullptr;
std::mutex SuS::logfile::logger::s_instance_mutex;

void (*SuS::logfile::logger::s_old_sighandler)(int) = nullptr;

SuS::logfile::logger::logger() : m_d(new logger_private) {
#ifdef HAVE_SIGACTION
   struct ::sigaction sa, old_sa;
   sa.sa_handler = signal_handler;
   sigemptyset(&sa.sa_mask);
   sa.sa_flags = SA_RESTART;
   ::sigaction(SIGSEGV, &sa, &old_sa);
   s_old_sighandler = old_sa.sa_handler;
#else
   s_old_sighandler = ::signal(SIGSEGV, signal_handler);
#endif
   ::atexit(atexit_handler);

   m_d->m_thread = new log_thread{};
   m_d->m_thread->start();
} // logger constructor

SuS::logfile::logger::~logger() {
   if (m_d->m_thread) {
      m_d->m_thread->terminate();
   } // if
   delete m_d->m_thread;
} // logger destructor

SuS::logfile::logger *SuS::logfile::logger::instance() {
// TODO: this can cause trouble, if s_instance_mutex happens
// to be un-initialized when the first call to instance() comes
// from a static subsystem_registrator.
// it reliably *fails* on windows...
#ifndef _MSC_BUILD
   std::lock_guard<std::mutex> lock(s_instance_mutex);
#endif
   // the same theoretically goes for the !s_instance condition, but we seem
   // to have better.luck, here.
   if (!s_instance)
      s_instance = new logger{};
   return s_instance;
} // logger::instance

void SuS::logfile::logger::log(log_level _level, const subsystem_t _subsystem,
      const std::string &_message, const std::string &_function) {
   const auto i = m_d->m_subsystems.find(_subsystem);
   assert(i != m_d->m_subsystems.end());
   if (_level < i->second.min_level) {
      return;
   }
   const auto now = std::chrono::system_clock::now();
   log_event le = {
         _level, _subsystem, _message, now, _function, i->second.name, ""};
   m_d->m_thread->enqueue(le);
} // logger::log

SuS::logfile::logger::subsystem_t SuS::logfile::logger::register_subsystem(
      const std::string &_name) {
   // allow the same name to be registered several times, e.g. from separate
   // source files.
   // merge all these registrations to one id.
   try {
      const auto id = find_subsystem(_name);
      return id;
   } catch (std::invalid_argument &) {
   }

   // not found => add to list
   const auto new_id = m_d->m_next_free_subsystem_id++;
   m_d->m_subsystems.emplace(
         new_id, subsystem_info{_name, log_level::SuS_LOG_MINLEVEL});
   return new_id;
} // logger::register_subsystem

SuS::logfile::logger::subsystem_t SuS::logfile::logger::find_subsystem(
      const std::string &_name) {
   const auto i =
         std::find_if(m_d->m_subsystems.cbegin(), m_d->m_subsystems.cend(),
               [_name](const logger_private::subsystem_map_t::value_type &_pair)
                     -> bool { return _pair.second.name == _name; });
   if (i != m_d->m_subsystems.cend())
      return i->first;
   throw std::invalid_argument("Unknown subsystem name.");
} // logger::find_subsystem

void SuS::logfile::logger::set_subsystem_min_log_level(
      subsystem_t _subsystem, log_level _level) {
   const auto i = m_d->m_subsystems.find(_subsystem);
   assert(i != m_d->m_subsystems.end());
   i->second.min_level = _level;
} // logger::set_subsystem_min_log_level

void SuS::logfile::logger::add_output_stream(
      output_stream *const _stream, const std::string &_ref) {
   // when no reference is given, refer to it by its name.
   m_d->m_thread->m_streams.emplace(
         _ref.empty() ? _stream->name() : _ref, _stream);
} // logger::add_output_stream

bool SuS::logfile::logger::remove_output_stream(const std::string &_name) {
   const auto i = m_d->m_thread->m_streams.find(_name);
   if (i == m_d->m_thread->m_streams.end())
      return false;

   delete i->second;
   m_d->m_thread->m_streams.erase(i);
   return true;
} // logger::remove_output_stream

void SuS::logfile::logger::dump_configuration(std::ostream &_stream) {
   _stream << "global min. log level (compile-time): "
           << level_name(SuS::logfile::logger::log_level::SuS_LOG_MINLEVEL)
           << std::endl;

   _stream << "active output streams:" << std::endl;
   for (const auto &i : m_d->m_thread->m_streams)
      i.second->dump(_stream);

   _stream << "active logging subsystems" << std::endl;
   for (const auto &i : m_d->m_subsystems)
      _stream << "   - " << i.second.name << std::endl
              << "     min. log level: " << level_name(i.second.min_level)
              << std::endl;
} // logger::dump_configuration

bool SuS::logfile::logger::set_min_log_level(
      const std::string &_stream, log_level _level) {
   const auto &i = m_d->m_thread->m_streams.find(_stream);
   if (i == m_d->m_thread->m_streams.end())
      return false;
   i->second->set_min_log_level(_level);
   return true;
} // logger::set_min_log_level

const char *SuS::logfile::logger::level_name(log_level _l) {
   return logger_private::s_level_names.at(_l);
} // logger::name_for_level

SuS::logfile::logger::log_level SuS::logfile::logger::level_by_name(
      const std::string &_name) {
   for (const auto &i : logger_private::s_level_names) {
      if (i.second == _name) {
         return i.first;
      }
   }
   throw std::invalid_argument{"Unknown level name."};
}

std::vector<SuS::logfile::logger::log_level>
SuS::logfile::logger::all_levels() {
   std::vector<log_level> ret{logger_private::s_level_names.size()};
   std::transform(logger_private::s_level_names.begin(),
         logger_private::s_level_names.end(), ret.begin(),
         [](const std::pair<log_level, const char *const> &_i) {
            return _i.first;
         });
   return ret;
}

#ifdef __unix__
bool SuS::logfile::logger::gdb_backtrace() {
   // try running gdb
   char pid[16];
   ::snprintf(pid, 16u, "%d", getpid());

   // for communication with gdb
   int fds[2];
   if (::pipe(fds) != 0) {
      return false;
   }

   auto child = fork();
   if (child == 0) {
      ::dup2(fds[1], STDOUT_FILENO);
      ::close(fds[0]);
      ::close(fds[1]);
      ::execl("/usr/bin/gdb", "gdb", "-q", "-nw", "-batch", "-p", pid, "-ex",
            "thread apply all bt full", nullptr);
      // if we get here, the exec failed.
      return false;
   }

   ::close(fds[1]);

   char backtracebuf[4096];
   line_splitter ls{
         [](const std::string &_s) { SuS_LOG(severe, log_id(), _s); }};
   // read gdb output
   ssize_t size = ::read(fds[0], backtracebuf, sizeof backtracebuf);
   while (size > 0) {
      ls.read_and_forward(backtracebuf, size);
      size = ::read(fds[0], backtracebuf, sizeof backtracebuf);
   }

   ::close(fds[0]);
   int status;
   ::waitpid(child, &status, 0);
   return (status == 0);
}
#else
bool SuS::logfile::logger::gdb_backtrace() {
   return false;
}
#endif

void SuS::logfile::logger::signal_handler(int _sig) {
   std::cout << "SIGNAL " << _sig << std::endl;

   SuS_LOG_STREAM(severe, log_id(), "SIGNAL " << _sig << " received.");

   if (!gdb_backtrace()) {
#ifdef HAVE_BACKTRACE_SYMBOLS
      void *bt[30];
      const auto bt_size = ::backtrace(bt, 30);
      const auto bt_syms = ::backtrace_symbols(bt, bt_size);
      for (int i = 0; i < bt_size; ++i) {
         std::cerr << bt_syms[i] << std::endl;
         SuS_LOG(severe, log_id(), bt_syms[i]);
      }
#elif defined HAVE_CAPTURESTACKBACKTRACE
      auto process = ::GetCurrentProcess();
      ::SymInitialize(process, nullptr, TRUE);

      void *stack[100];
      const auto frames = ::CaptureStackBackTrace(0, 100, stack, nullptr);
      auto symbol = (SYMBOL_INFO *)::calloc(
            sizeof(SYMBOL_INFO) + 256 * sizeof(char), 1);
      symbol->MaxNameLen = 255;
      symbol->SizeOfStruct = sizeof(SYMBOL_INFO);

      for (unsigned i = 0; i < frames; i++) {
         ::SymFromAddr(process, (DWORD64)(stack[i]), 0, symbol);
         SuS_LOG_STREAM(severe, log_id(), (frames - i - 1)
                     << ": " << symbol->Name << " - 0x" << std::hex
                     << symbol->Address);
      }

      ::free(symbol);
#else
      SuS_LOG(severe, log_id(), "No backtrace available.");
#endif
   }

   instance()->terminate();
   if (s_old_sighandler) {
      s_old_sighandler(_sig);
   } // if
   else {
      exit(1);
   } // else
} // logger::signal_handler

void SuS::logfile::logger::atexit_handler() {
   std::cout << "exit" << std::endl;
   if (s_instance) {
      delete s_instance;
   } // if
} // logger::atexit_handler

void SuS::logfile::logger::terminate() {
   // wait until all log events have been processed.
   // otherwise chances are high that events just prior to the crash that
   // may help with debugging the crash are lost.
   if (instance()->m_d->m_thread) {
      instance()->m_d->m_thread->terminate();
      delete instance()->m_d->m_thread;
      instance()->m_d->m_thread = nullptr;
   } // if
} // logger::terminate

std::string SuS::logfile::logger::string_format(const char *fmt, ...) {
   int size = 100;
   std::string str;
   va_list ap;
   while (1) {
      str.resize(size);
      va_start(ap, fmt);
      int n = vsnprintf((char *)str.c_str(), size, fmt, ap);
      va_end(ap);
      if (n > -1 && n < size) {
         str.resize(n);
         return str;
      }
      if (n > -1)
         size = n + 1;
      else
         size *= 2;
   }
   return str;
}
