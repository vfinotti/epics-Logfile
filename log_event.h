/* SPDX-License-Identifier: MIT */
#pragma once

#include "logger.h"

#include <chrono>
#include <list>
#include <string>

namespace SuS {
namespace logfile {

struct log_event {
 public:
   logger::log_level level;
   logger::subsystem_t subsystem;
   std::string message;
   std::chrono::system_clock::time_point time;
   std::string function;
   mutable std::string subsystem_string;
   mutable std::string time_string;
}; // struct log_event

//! Format time as string in ISO format.
std::string format_time(const std::chrono::system_clock::time_point &_t);
//! Format time as string in seconds-since-epoch format.
std::string format_timestamp(const std::chrono::system_clock::time_point &_t);

using event_queue_t = std::list<log_event>;

} // namespace logfile
} // namespace SuS
