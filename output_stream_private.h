/* SPDX-License-Identifier: MIT */
#pragma once

#include "logger.h"

namespace SuS {
namespace logfile {

struct output_stream_private {
   logger::log_level m_minLogLevel;
}; // struct output_stream_private

} // namespace logfile
} // namespace SuS
