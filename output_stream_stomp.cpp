/* SPDX-License-Identifier: MIT */
#include "output_stream_stomp.h"
#include "line_splitter.h"
#include "log_event.h"
#include "parse_url.h"
#include "subsystem_registrator.h"
#include "tcp_client_socket.h"

#include "config.h"

#include <sstream>

#include <array>
#include <cassert>
#include <chrono>
#include <ctype.h>
#include <math.h>
#include <memory>
#ifdef HAVE_PWD_H
#include <pwd.h>
#endif
#include <stdexcept>
#include <string.h>
#include <thread>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

namespace {
SuS::logfile::subsystem_registrator log_id{"stomp"};
}

SuS::logfile::output_stream_stomp::output_stream_stomp(
      const std::string &_app_name, const std::string &_URL)
   : output_stream(), m_app_name(sanitize_string(_app_name)),
     m_stomp_URL{new URL_info}, m_heartbeat_thread(nullptr) {
   // set defaults
   m_stomp_URL->protocol = "stomp";
   m_stomp_URL->port = 61613U;
   m_stomp_URL->path = "LOG";
   parse_URL(_URL, *m_stomp_URL);
   m_socket.reset(new tcp_client_socket{m_stomp_URL->host, m_stomp_URL->port});
   if (m_stomp_URL->protocol == "stomp") {
      // ok
#ifdef OPENSSL_FOUND
   } else if (m_stomp_URL->protocol == "stomp+ssl") {
      m_socket->use_SSL(true);
   } else {
      throw std::invalid_argument{"Only stomp and stomp+ssl protocols are supported."};
   }
#else
   } else {
      throw std::invalid_argument{"Only stomp protocol is supported."};
   }
#endif

#if defined HOST_NAME_MAX
   char hostname[HOST_NAME_MAX + 1];
#elif defined MAXHOSTNAMELEN
   char hostname[MAXHOSTNAMELEN + 1];
#else
   // 256 is a common length limit.
   char hostname[257];
#endif
   if (::gethostname(hostname, sizeof hostname) == 0) {
      m_host = sanitize_string(hostname);
   } else {
      m_host = "/UNKNOWN/";
   } // else

#ifdef HAVE_GETEUID
   const auto uid = ::geteuid();
#ifdef HAVE_SYSCONF
   const auto sc = ::sysconf(_SC_GETPW_R_SIZE_MAX);
   const auto buflen = (sc == -1) ? 256 : sc;
#else
   const auto buflen = 256U;
#endif
   const auto buf = std::unique_ptr<char[]>{new char[buflen]};
   ::passwd pwd;
   ::passwd *result;
   ::getpwuid_r(uid, &pwd, buf.get(), buflen, &result);
   if (result == nullptr)
      m_user = "/UNKNOWN/";
   else
      m_user = sanitize_string(pwd.pw_name);
#else
   m_user = "/UNKNOWN/";
#endif

   // to be compatible, use non-padded strings in uppercase
   for (const auto &i : logger::all_levels()) {
      const auto in = std::string{logger::level_name(i)};
      auto out = std::string{};
      for (const auto p : in) {
         if (::isspace(p))
            continue;
         out += static_cast<char>(::toupper(p));
      }
      m_level_strings.emplace(i, ::strdup(out.c_str()));
   }
   connect();
}

SuS::logfile::output_stream_stomp::~output_stream_stomp() {
   for (const auto &i : m_level_strings) {
      ::free(const_cast<void *>(static_cast<const void *>(i.second)));
   }
}

void SuS::logfile::output_stream_stomp::connect_thread() {
   try {
      m_socket->connect();
   } catch (const std::exception &e) {
      SuS_LOG_STREAM(warning, log_id(), "Connection failed: " << e.what());
      m_connect_thread_done = true;
      return;
   }

   // outgoing: no heartbeat
   // incoming: 5000ms heartbeat
   std::ostringstream s;
   s << "CONNECT\n"
        "accept-version:1.1\n"
        "heart-beat:0,5000\n"
        "host:"
     << m_stomp_URL->host << "\n";
   if (!m_stomp_URL->login.empty()) {
      s << "login:" << m_stomp_URL->login << "\n"
                                             "passcode:"
        << m_stomp_URL->password << "\n";
   }
   s << "\n";
   const auto packet = s.str() += '\0'; // this is a real 0-byte to be sent
   try {
      m_socket->write((const uint8_t *)(packet.c_str()), packet.length());
   } catch (const std::exception &) {
      disconnect();
      m_connect_thread_done = true;
      return;
   }

   m_heartbeat_thread =
         new std::thread{&output_stream_stomp::reader_thread, this};

   std::unique_lock<std::mutex> lk(m_reply_mutex);
   if (!m_reply_queue.size()) {
      // there is a tiny chance that the reply might have been received in the
      // meantime.
      const auto stat = m_reply_cv.wait_for(lk, std::chrono::seconds(5));
      if (stat == std::cv_status::timeout) {
         SuS_LOG(warning, log_id(), "STOMP timeout");
         disconnect();
         m_connect_thread_done = true;
         return;
      }
   }
   const auto reply = m_reply_queue.front();
   m_reply_queue.pop_front();
   if ((reply.command != "CONNECTED") || (!check_server_version(reply)) ||
         (!parse_heartbeat(reply))) {
      disconnect();
      m_connect_thread_done = true;
      return;
   }
   m_connected = true;
   m_connect_thread_done = true;
   return;
}

void SuS::logfile::output_stream_stomp::disconnect() {
   m_socket->disconnect();
   m_connected = false;
}

std::string SuS::logfile::output_stream_stomp::name() {
   return std::string{"stomp: "} + m_stomp_URL->host;
} // output_stream_stomp::name

unsigned SuS::logfile::output_stream_stomp::retry_time() {
   // the connection is (hopefully) fast.
   // this leads to a retry period of 2 seconds while we try to connect,
   // and 30 seconds between connection attempts.
   return m_connecting ? 2U : 30U;
}

bool SuS::logfile::output_stream_stomp::connect() {
   if (!m_connected) {
      if (m_connecting.exchange(true)) {
         if (m_connect_thread_done) {
            m_connecting = false;
            m_connect_thread.join();
            if (!m_connected) {
               // connection attempt failed
               return false;
            }
         } else {
            // still trying to connect
            return false;
         }
      } else {
         // m_connecting was false
         SuS_LOG(finest, log_id(), "Starting connect thread.");
         m_connect_thread_done = false;
         m_connect_thread =
               std::thread(&output_stream_stomp::connect_thread, this);
         return false;
      }
   }
   return true;
}

bool SuS::logfile::output_stream_stomp::do_write(const log_event &_le) {
   if (!connect()) {
      return false;
   }

   std::ostringstream body;
   body << "<map>\n"
           "<entry><string>APPLICATION-ID</string><string>"
        << m_app_name << "</string></entry>\n"
                         "<entry><string>CREATETIME</string><string>"
        << _le.time_string << "</string></entry>\n"
                              "<entry><string>HOST</string><string>"
        << m_host << "</string></entry>\n"
                     "<entry><string>NAME</string><string>"
        << sanitize_string(_le.function)
        << "</string></entry>\n"
           "<entry><string>SEVERITY</string><string>"
        << m_level_strings.at(_le.level)
        << "</string></entry>\n"
           "<entry><string>TEXT</string><string>"
        << sanitize_string(_le.message)
        << "</string></entry>\n"
           "<entry><string>TYPE</string><string>log</string></entry>\n"
           "<entry><string>USER</string><string>"
        << m_user << "</string></entry>\n"
                     "<entry><string>CLASS</string><string>"
        << sanitize_string(_le.subsystem_string) << "</string></entry>\n"
                                                    "</map>\n";

   std::ostringstream header;
   // receipt:42
   // => RECEIPT\nreceipt-id:42\n\n\0
   ++m_receipt;
   header
         << "SEND\n"
            "destination:/topic/"
         << m_stomp_URL->path
         << "\n"
            "transformation:jms-map-xml\n"
            // setting content-length as per the STOMP specification gives:
            // org.csstudio.sns.jms2rdb.LogClientThread (onMessage) - Received
            // unhandled message type
            // org.apache.activemq.command.ActiveMQBytesMessage
            // this is probably due to "https://activemq.apache.org/stomp.html":
            // Inclusion of content-length header     Resulting Message
            // yes                                    BytesMessage
            // no                                     TextMessage
            // "content-length:" << body.tellp() << "\n"
            "receipt:"
         << m_receipt << "\n"
                         "\n";
   const auto packet =
         header.str() + body.str() + '\0'; // this is a real 0-byte to be sent
   try {
      m_socket->write((const uint8_t *)(packet.c_str()), packet.length());
   } catch (const std::exception &) {
      disconnect();
      return false;
   }

   std::unique_lock<std::mutex> lk(m_reply_mutex);
   if (!m_reply_queue.size()) {
      // there is a tiny chance that the reply might have been received in the
      // meantime.
      const auto stat = m_reply_cv.wait_for(lk, std::chrono::seconds(6));
      if (stat == std::cv_status::timeout) {
         SuS_LOG(warning, log_id(), "Timeout.");
         disconnect();
         return false;
      }
   }

   const auto reply = m_reply_queue.front();
   m_reply_queue.pop_front();
   lk.unlock();
   if (reply.command != "RECEIPT") {
      // if it's an ERROR, it has already been logged.
      disconnect();
      return false;
   }
   const auto id = reply.headers.find("receipt-id");
   if (id == reply.headers.end()) {
      SuS_LOG(warning, log_id(), "no receipt-id");
      disconnect();
      return false;
   }
   std::ostringstream expected;
   expected << m_receipt;
   return expected.str() == id->second;
} // output_stream_stomp::do_write

void SuS::logfile::output_stream_stomp::reader_thread() {
   char buf[1537];
   while (true) {
      // heartbeat interval 0 => no timeout in call to select().
      // buf-1 : make sure that we can append a '\0'.
      ssize_t bytesLeft = read_with_timeout(
            reinterpret_cast<uint8_t *>(buf), sizeof buf-1, m_heartbeat_interval *
                  1000 /* ms to us */ * 1.5 /* grace period. */);
      // when using SSL it could happen that select is woken up by a SSL
      // control message and no user data is available.
      // TODO: how to detect this case. how likely is it anyway?
      if (bytesLeft <= 0) {
         SuS_LOG(warning, log_id(), "read_with_timeout failed");
         disconnect();
         // end thread
         break;
      }
      // 1 past the received data.
      const auto end = buf + bytesLeft;
      // make sure the strchr below terminates within buf.
      *end = '\0';
      // start-of-(non-heartbeat)-data
      const char *sod = buf;
      // past last received byte
      while (bytesLeft) {
         // !m_partial_reply.empty() => in the middle of a reply.
         while (m_partial_reply.empty() && bytesLeft && (*sod == '\n')) {
            // first bytes is '\n' => heartbeat received
            // there might still be data to follow, if the heartbeat came right
            // before other data.
            // while: theoretically, we could have several heartbeat bytes.
            ++sod;
            --bytesLeft;
         }
         if (!bytesLeft) {
            // heartbeat, but no data => just move on.
            continue;
         }
         // ptr points to the first non-heartbeat data byte.
         m_last_was_data = true;

         const char *i = ::strchr(sod, '\0');
         // i either points to the end of the string, or to the \0 byte.
         // in each case, this is just what we want to append, i.e. past the
         // last interesting byte.
         m_partial_reply.append(sod, i);
         if (i != end) {
            // '\0' found => reply complete
            const bool ret = handle_reply(m_partial_reply);
            m_partial_reply.clear();
            if (!ret) {
               disconnect();
               // end thread
               break;
            }
            // skip the '\0'
            ++i;
         }
         sod = i;
         bytesLeft = end - sod;
      }
   }
} // output_stream_stomp::reader_thread

bool SuS::logfile::output_stream_stomp::handle_reply(
      const std::string &_frame) {
   // first line is the command that triggered the reply.
   auto i = _frame.find('\n');
   if (i == std::string::npos)
      return false;
   stomp_reply reply;
   reply.command.assign(_frame, 0, i);

   const auto size = _frame.size();
   // parse the headers
   while (true) {
      if (size < i + 1) {
         // overran string
         return false;
      }
      // start with the next character after the '\n' i points to.
      const auto oldi = i + 1;
      i = _frame.find('\n', oldi);
      if (i == std::string::npos) {
         // a STOMP frame always ends with '\n' (the '\0' has already been
         // dropped).
         return false;
      }
      if (i == oldi) { // two \n in a row => end of headers
         break;
      }
      const auto line = std::string{_frame, oldi, i - oldi};
      const auto colon = line.find(':');
      if (colon == std::string::npos) {
         // no ':' in a header line
         return false;
      }
      const auto key = std::string{line, 0, colon};
      // TODO: undo encoding according to STOMP specification
      const auto value = std::string{line, colon + 1, std::string::npos};
      if (reply.headers.find(key) != reply.headers.end()) {
         // repeated keys are ignored
         continue;
      }
      // TODO: handle content-length
      reply.headers.emplace(key, value);
   }

   // remaining string: body
   if (size < i + 1) {
      // overran string
      return false;
   }
   reply.body.assign(_frame, i + 1, std::string::npos);

   if (reply.command == "ERROR") {
      std::string err_text{"ERROR from server"};
      const auto &msg = reply.headers.find("message");
      if (msg != reply.headers.end()) {
         err_text.append(": ");
         err_text.append(msg->second);
      }
      SuS_LOG(warning, log_id(), err_text);
      line_splitter ls{
            [](const std::string &_s) { SuS_LOG(fine, log_id(), _s); }};
      ls.read_and_forward(reply.body.data(), reply.body.size());
   }

   {
      std::lock_guard<std::mutex> lk(m_reply_mutex);
      m_reply_queue.push_back(reply);
   }
   m_reply_cv.notify_one();
   return true;
} // output_stream_stomp::handle_reply

bool SuS::logfile::output_stream_stomp::check_server_version(
      const stomp_reply &_reply) {
   // from the STOMP spec:
   // STOMP 1.1 servers MUST set the following headers:
   // version : The version of the STOMP protocol the session will be using.
   // See Protocol Negotiation for more details.
   const auto &ver = _reply.headers.find("version");
   if (ver == _reply.headers.end()) {
      SuS_LOG(warning, log_id(), "No version header in CONNECTED frame.");
      return false;
   }
   if (ver->second != "1.1") {
      SuS_LOG_STREAM(warning, log_id(), "STOMP server requests version "
                  << ver->second << ", that we don't speak.");
      return false;
   }
   return true;
} // output_stream_stomp::check_server_version

bool SuS::logfile::output_stream_stomp::parse_heartbeat(
      const stomp_reply &_reply) {
   // from the STOMP spec:
   // CONNECTED
   // heart-beat:<sx>,<sy>

   const auto &hb = _reply.headers.find("heart-beat");
   if (hb == _reply.headers.end()) {
      SuS_LOG(info, log_id(), "No heart-beat header in CONNECTED frame.");
      m_heartbeat_interval = 0;
      return true;
   }

   const auto comma = hb->second.find(',');
   if (std::string::npos == comma) {
      SuS_LOG_STREAM(warning, log_id(), "Cannot parse heart-beat header '"
                  << hb->second << "': no comma.");
      return false;
   }

   const auto sy = hb->second.substr(comma + 1);
   if (sy != "0") {
      // we cannot send heart-beats (yet).
      SuS_LOG_STREAM(warning, log_id(), "Cannot accept heart-beat header '"
                  << hb->second << "': sy != 0.");
      return false;
   }

   const auto sx = hb->second.substr(0, comma);
   char *endptr;
   m_heartbeat_interval = ::strtol(sx.c_str(), &endptr, 10);
   if (*endptr != '\0') {
      SuS_LOG_STREAM(warning, log_id(), "Cannot parse heart-beat header '"
                  << hb->second << "': sx no integer.");
      return false;
   }
   if (m_heartbeat_interval) {
      SuS_LOG_STREAM(finer, log_id(),
            "Heartbeat interval = " << m_heartbeat_interval << "ms.");
   } else {
      SuS_LOG_STREAM(config, log_id(), "No heartbeat.");
   }
   return true;
}

ssize_t SuS::logfile::output_stream_stomp::read_with_timeout(
      uint8_t *const _data, size_t _len, unsigned long _timeout_us) {
   try {
      if (!m_socket->select_read(_timeout_us)) {
#ifdef AMQ_4710_workaround
         // bug AMQ-4710: after a data packet, the next heartbeat period might
         // be twice what was requested.
         // TODO: in principle, the connection string from the server could be
         // checked against a list of servers requiring this workaround.
         // "server:ActiveMQ/5.6.0"
         // https://issues.apache.org/jira/browse/AMQ-4710 : fix foreseen for
         // 5.12.0.
         if (m_last_was_data) {
            m_last_was_data = false;
            return read_with_timeout(_data, _len, _timeout_us);
         }
#endif
         return -1;
      } else {
         return m_socket->read(_data, _len);
      }
   } catch (const std::runtime_error &) {
      m_socket->disconnect();
      return -1;
   }
} // output_stream_stomp::read_with_timeout

std::string SuS::logfile::output_stream_stomp::replace_all(
      std::string str, const std::string &_pattern, const std::string &_new) {
   auto pos = std::string::size_type{0};
   while ((pos = str.find(_pattern, pos)) != std::string::npos) {
      str.replace(pos, _pattern.length(), _new);
      pos += _new.length();
   }
   return str;
}

std::string SuS::logfile::output_stream_stomp::sanitize_string(
      const std::string &_in) {
   // "&" must go first! std::map re-orders, so place this here.
   auto cur = replace_all(_in, "&", "&amp;");

   static const std::map<const char *, const char *> replacements = {
         {"\"", "&quot;"}, {"'", "&apos;"}, {"<", "&lt;"}, {">", "&gt;"}};
   for (const auto &i : replacements) {
      cur = replace_all(cur, i.first, i.second);
   }
   // we use const char* in the map, so \0 would be empty => special case.
   cur = replace_all(cur, std::string("\0", 1), "\\0");
   return cur;
} // output_stream_stomp::sanitize_string
