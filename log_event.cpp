/* SPDX-License-Identifier: MIT */
#include "log_event.h"

#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

std::string SuS::logfile::format_time(
      const std::chrono::system_clock::time_point &_t) {
   const auto ttime_t = std::chrono::system_clock::to_time_t(_t);
   const auto tp_sec = std::chrono::system_clock::from_time_t(ttime_t);
   const auto ms =
         std::chrono::duration_cast<std::chrono::milliseconds>(_t - tp_sec);
#ifdef SuS_LOG_GMT
   const auto ttm = std::gmtime(&ttime_t);
#else
   const auto ttm = std::localtime(&ttime_t);
#endif
   char out[22];
   std::strftime(out, sizeof out, "%Y-%m-%d %H:%M:%S", ttm);
   std::ostringstream ss;
   ss << out << "." << std::setw(3) << std::setfill('0') << ms.count();
#ifdef SuS_LOG_GMT
   ss << " GMT";
#endif
   return ss.str();
} // format_time

std::string SuS::logfile::format_timestamp(
      const std::chrono::system_clock::time_point &_t) {
   const auto ttime_t = std::chrono::system_clock::to_time_t(_t);
   const auto tp_sec = std::chrono::system_clock::from_time_t(ttime_t);
   const auto ms =
         std::chrono::duration_cast<std::chrono::milliseconds>(_t - tp_sec);
   std::ostringstream ss;
   ss << ttime_t << "." << std::setw(3) << std::setfill('0') << ms.count();
   return ss.str();
} // format_timestamp
