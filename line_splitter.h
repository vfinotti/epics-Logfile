/* SPDX-License-Identifier: MIT */
#pragma once

#include <functional>
#include <string>

namespace SuS {
namespace logfile {

class line_splitter {
 public:
   line_splitter(std::function<void(const std::string &)> _sink);
   ~line_splitter();

   void read_and_forward(const char *const _buf, size_t _size);

 private:
   std::string m_active_line = "";
   const std::function<void(const std::string &)> m_sink;
};
}
}
