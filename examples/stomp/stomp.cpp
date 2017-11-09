/* SPDX-License-Identifier: MIT */
#include "../../logger.h"
#include "../../output_stream_stomp.h"
#include "../../subsystem_registrator.h"

#include <iostream>
#include <sstream>

namespace {
SuS::logfile::subsystem_registrator log_id("example");
} // namespace

int main() {
   SuS::logfile::logger::instance()->add_output_stream(
         new SuS::logfile::output_stream_stomp(
               "app", "sussrv04.ziti.uni-heidelberg.de"));
   SuS_LOG_STREAM(config, log_id(), "Hallo Welt.");
   SuS_LOG_STREAM(finer, log_id(), "1 + 1 = " << (1 + 1));
   SuS_LOG_PRINTF(fine, log_id(), "3 * 3 = %d", (3 * 3));
   SuS_LOG_STREAM(severe, log_id(), "Oh NO!");

   for (int x = 0; x < 10; ++x) {
      SuS_LOG_STREAM(fine, log_id(), x);
      SuS_LOG_STREAM(info, log_id(), "info: " << x);
   }
   SuS_LOG_STREAM(warning, log_id(), "final");
   std::cout << (*(volatile int *)0); // note: create a segfault by
                                      // dereferencing (void*)0
} // main
