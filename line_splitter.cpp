/* SPDX-License-Identifier: MIT */
#include "line_splitter.h"

#include <algorithm>

namespace SuS {
namespace logfile {

line_splitter::line_splitter(std::function<void(const std::string &)> _sink)
   : m_sink(std::move(_sink)) {
}

line_splitter::~line_splitter() {
   if (!m_active_line.empty()) {
      m_sink(m_active_line);
   }
}

void line_splitter::read_and_forward(const char *const _buf, size_t _size) {
   // start-of-line
   const char *sol = _buf;
   const char *const last = _buf + _size;
   while (sol) {
      const char *lf = std::find(sol, last, '\n');
      if (lf != last) {
         m_sink(
               m_active_line + std::string(sol, static_cast<size_t>(lf - sol)));
         m_active_line = "";
         sol = lf + 1;
         if (last == sol) {
            sol = nullptr;
         }
      } else {
         m_active_line += std::string(sol, static_cast<size_t>(last - sol));
         sol = nullptr;
      }
   }
}
}
}
