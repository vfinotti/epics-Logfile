/* SPDX-License-Identifier: MIT */
/*! @file */
#pragma once

#include "logfile_export.h"

#include <cstdint>
#include <iosfwd>
#include <string>

namespace SuS {

struct LOGFILE_EXPORT URL_info {
   std::string protocol;
   std::string login;
   std::string password;
   std::string host;
   uint16_t port;
   std::string path;
};

//! Decode a URL containing protocol, login and port information.
/**
 *  This function decodes a URL of the form
 *  protocol://login:password@host:port/path.
 *
 *  The individual fields of _parts are only modified when the corresponding
 *  part is present in _url, so that defaults can be set before calling this
 *  function.
 *
 *  Not supported are IPv6 addresses in the form [::1].
 *
 *  @param _url The URL to parse.
 *  @param _parts The URL_info structure to update.
 */
LOGFILE_EXPORT void parse_URL(const std::string &_url, URL_info &_parts);
}

LOGFILE_EXPORT std::ostream &operator<<(
      std::ostream &_s, const SuS::URL_info &_url);
