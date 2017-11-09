/* SPDX-License-Identifier: MIT */
#include "logger_private.h"

const std::map<SuS::logfile::logger::log_level, const char *const>
      SuS::logfile::logger_private::s_level_names{
            {SuS::logfile::logger::log_level::finest, "finest"},
            {SuS::logfile::logger::log_level::finer, "finer"},
            {SuS::logfile::logger::log_level::fine, "fine"},
            {SuS::logfile::logger::log_level::config, "config"},
            {SuS::logfile::logger::log_level::info, "info"},
            {SuS::logfile::logger::log_level::warning, "warning"},
            {SuS::logfile::logger::log_level::severe, "severe"}};
