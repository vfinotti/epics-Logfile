/* SPDX-License-Identifier: MIT */
#include "parse_url.h"

#include <iostream>
#include <sstream>
#include <stdexcept>
#include <stdlib.h>

namespace {
std::string decode_hex_byte(const std::string &_in) {
   auto out = std::string{};
   out.reserve(_in.size());
   for (std::string::size_type i = 0; i < _in.size(); ++i) {
      if (_in[i] == '%') {
         const auto escape = _in.substr(i + 1, 2);
         if (escape.size() != 2) {
            auto errmsg = std::string{"Invalid escape sequence: %"};
            errmsg.append(escape);
            throw std::invalid_argument{errmsg};
         }
         const auto badchar =
               escape.find_first_not_of("0123456789abcdefABCDEF");
         if (badchar != std::string::npos) {
            auto errmsg = std::string{"Invalid escape sequence: %"};
            errmsg.append(escape);
            throw std::invalid_argument{errmsg};
         }

         // cannot overflow, because we have exactly two chars.
         const auto c = char(::strtol(escape.c_str(), nullptr, 16));
         out.push_back(c);
         i += 2; // consumed two additional characters
      } else if (_in[i] == '+') {
         out.push_back(' ');
      } else {
         out.push_back(_in[i]);
      }
   }
   return out;
}
}

void SuS::parse_URL(const std::string &_url, URL_info &_parts) {
   // work on a copy of _parts to return the old state in case of an
   // exception.
   URL_info ret = _parts;

   // protocol://
   auto proto_end = _url.find("://");
   if (proto_end != std::string::npos) {
      ret.protocol = std::string{_url, 0, proto_end};
      proto_end += 3U; // skip the ://
   } else {
      proto_end = 0;
   }

   // login:password@host:port
   // first split into login:password and host:port sections
   const auto at = _url.find('@', proto_end);
   auto host_port = proto_end;
   if (at == std::string::npos) {
      // no '@' => no login required
   } else {
      const auto colon = _url.find(':', proto_end);
      // empty login, or login but no ':' is not valid
      if ((colon == proto_end) || (colon == std::string::npos) ||
            (colon > at)) {
         throw std::invalid_argument{"Cannot parse login information."};
      }
      // the case "login, but no password" should be accepted. There are
      // rumors that such configurations DO exist...
      ret.login = decode_hex_byte(_url.substr(proto_end, colon - proto_end));
      ret.password = decode_hex_byte(_url.substr(colon + 1, at - (colon + 1)));
      host_port = at + 1;
   }

   auto slash = _url.find('/', host_port);
   if (slash != std::string::npos) {
      ret.path = decode_hex_byte(_url.substr(slash + 1));
   } else {
      slash = _url.size();
   }
   const auto colon = _url.find(':', host_port);
   if ((colon == std::string::npos) || (colon > slash)) {
      // no ':' => no port given
      // TODO: support IPv6... [::1]:61613
      ret.host = _url.substr(host_port, slash - host_port);
   } else {
      const auto port = _url.substr(colon + 1, slash - (colon + 1));
      ret.host = _url.substr(host_port, colon - host_port);
      // explicitely give the base to refuse octal, hex
      const auto p = ::atoi(port.c_str());
      // prevent +/-
      const auto badchar = port.find_first_not_of("0123456789");
      // check all error conditions at once
      if ((port.size() > 5) || (badchar != std::string::npos) || (p < 1) ||
            (p > 65535)) {
         auto errmsg = std::string{"Invalid port: "};
         errmsg.append(port);
         throw std::invalid_argument{errmsg};
      } else {
         ret.port = p;
      }
   }
   if (ret.host.empty()) {
      throw std::invalid_argument{"Invalid URL: No hostname."};
   }
   _parts = ret;
}

std::ostream &operator<<(std::ostream &_s, const SuS::URL_info &_url) {
   _s << "protocol: '" << _url.protocol << "'" << std::endl
      << "login:    '" << _url.login << "'" << std::endl
      << "password: '" << _url.password << "'" << std::endl
      << "host:     '" << _url.host << "'" << std::endl
      << "port :    '" << _url.port << "'" << std::endl
      << "path :    '" << _url.path << "'" << std::endl;
   return _s;
}
