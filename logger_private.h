/* SPDX-License-Identifier: MIT */
#pragma once

#include "logger.h"

#include <map>

namespace SuS {
namespace logfile {

class log_thread;

struct subsystem_info {
   std::string name;
   logger::log_level min_level;
};

struct logger_private {
   // Hide from library users.
   // Using this directly would break Windows DLLs.
   static const std::map<logger::log_level, const char *const> s_level_names;

   using subsystem_map_t = std::map<logger::subsystem_t, subsystem_info>;
   subsystem_map_t m_subsystems;

   logger::subsystem_t m_next_free_subsystem_id{0};

   log_thread *m_thread;
};

} // namespace logfile
} // namespace SuS
