/* SPDX-License-Identifier: MIT */
#include "output_stream_stdout.h"
#include "log_event.h"

#include <iomanip>
#include <iostream>
#include <map>
#include <sstream>
#include <stdlib.h>

#if defined __unix__ | defined __APPLE__
#define SuS_HAS_COLOR
#endif

#ifdef SuS_HAS_COLOR
#define COLOR_ENTIRE_LINE
namespace {
const std::map<SuS::logfile::logger::log_level, const char *> s_colors = {
      {SuS::logfile::logger::log_level::finest, "\033[37m"},
      {SuS::logfile::logger::log_level::finer, "\033[37m"},
      {SuS::logfile::logger::log_level::fine, ""},
      {SuS::logfile::logger::log_level::config, "\033[32m"},
      {SuS::logfile::logger::log_level::info, "\033[33m"},
      {SuS::logfile::logger::log_level::warning, "\033[31m"},
      {SuS::logfile::logger::log_level::severe, "\033[1;31m"}};

const std::map<SuS::logfile::logger::log_level, const char *> s_colors256dark =
      {{SuS::logfile::logger::log_level::finest, "\033[38;5;240m"},
            {SuS::logfile::logger::log_level::finer, "\033[38;5;244m"},
            {SuS::logfile::logger::log_level::fine, "\033[38;5;248m"},
            {SuS::logfile::logger::log_level::config, "\033[32m"},
            {SuS::logfile::logger::log_level::info, "\033[33m"},
            {SuS::logfile::logger::log_level::warning, "\033[31m"},
            {SuS::logfile::logger::log_level::severe, "\033[1;31m"}};

const std::map<SuS::logfile::logger::log_level, const char *> s_colors256light =
      {{SuS::logfile::logger::log_level::finest, "\033[38;5;248m"},
            {SuS::logfile::logger::log_level::finer, "\033[38;5;244m"},
            {SuS::logfile::logger::log_level::fine, "\033[38;5;240m"},
            {SuS::logfile::logger::log_level::config, "\033[32m"},
            {SuS::logfile::logger::log_level::info, "\033[33m"},
            {SuS::logfile::logger::log_level::warning, "\033[31m"},
            {SuS::logfile::logger::log_level::severe, "\033[1;31m"}};
}
#endif

SuS::logfile::output_stream_stdout::output_stream_stdout(
      const std::string &_name, std::ostream &_stream)
   : output_stream(), m_name(_name.empty() ? "stdout" : _name),
     m_stream(_stream) {
   init_colors();
} // output_stream_stdout constructor

void SuS::logfile::output_stream_stdout::init_colors() {
#ifdef SuS_HAS_COLOR
   m_colors = &s_colors;
   // special colors on 256-color terminals
   const char *const term = ::getenv("TERM");
   if (nullptr == term) {
      return;
   }

   const std::string termstr(term);
   if ((9 > termstr.size()) ||
         (0 != termstr.compare(termstr.length() - 9, 9, "-256color"))) {
      return;
   }

   m_colors = &s_colors256dark;
   const char *const colorfgbg = ::getenv("COLORFGBG");
   if (nullptr == colorfgbg) {
      return;
   }

   // COLORFGBG=15;0
   std::string fgbg{colorfgbg};
   try {
      size_t next;
      auto color_fg = std::stoi(fgbg, &next);
      if (next == fgbg.size()) {
         // only one integer.
         return;
      }
      if (fgbg[next] != ';') {
         return;
      }
      if (color_fg < 0) {
         return;
      }
      auto bg = fgbg.substr(next + 1);
      auto color_bg = std::stoi(bg, &next);
      if (next != bg.size()) {
         // more than one integer
         return;
      }
      if (color_bg < 0) {
         return;
      }
      if (color_bg == 7 || color_bg > 9) {
         m_colors = &s_colors256light;
      }
   } catch (std::invalid_argument &) {
      // invalid env. variable => ignore
   }
#endif
}

std::string SuS::logfile::output_stream_stdout::name() {
   return m_name;
} // output_stream_stdout::name

unsigned SuS::logfile::output_stream_stdout::retry_time() {
   return 10;
} // output_stream_stdout::retry_time

bool SuS::logfile::output_stream_stdout::do_write(const log_event &_le) {
   std::string subsystem = _le.subsystem_string;
   subsystem.resize(8, ' ');
   std::stringstream s;
   s
#if defined SuS_HAS_COLOR && defined COLOR_ENTIRE_LINE
         << m_colors->at(_le.level)
#endif
         << _le.time_string << " ["
#if defined SuS_HAS_COLOR && !defined COLOR_ENTIRE_LINE
         << m_colors->at(_le.level)
#endif
         << std::setw(7) << std::left
         << SuS::logfile::logger::level_name(_le.level)
#if defined SuS_HAS_COLOR && !defined COLOR_ENTIRE_LINE
         << "\033[0m"
#endif
         << "] [" << subsystem << "] " << _le.message
#if defined SuS_HAS_COLOR && defined COLOR_ENTIRE_LINE
         << "\033[0m"
#endif
         << std::endl;

   const std::string msg = s.str();

   m_stream << msg;
   return m_stream.good();
} // output_stream_stdout::do_write
