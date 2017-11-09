/* SPDX-License-Identifier: MIT */
#include "tcp_client_socket.h"
#include "logger.h"
#include "subsystem_registrator.h"
#include "tcs_private.h"

#include "config.h"

#ifdef HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif

#include <cassert>
#include <iterator>
#include <map>

#ifdef HAVE_NETDB_H
#include <netdb.h>
#include <sys/types.h>
#endif

#include <stdexcept>
#include <sstream>
#include <string.h>

#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <vector>

#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif

#ifdef HAVE_SYS_SELECT_H
#include <sys/select.h>
#else
#include <sys/types.h>
#endif

#ifdef HAVE_INET_NTOP
#include <arpa/inet.h>
#endif

#ifdef OPENSSL_FOUND
#include <openssl/err.h>
#include <openssl/ssl.h>
#include <openssl/x509.h>
#include <openssl/x509_vfy.h>

#ifdef __APPLE__
// OpenSSL is deprecated
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#endif
#endif

// write/send should never raise a signal. check different ways to disable
// the signal.
#if defined MSG_NOSIGNAL && !defined OPENSSL_FOUND
// when we use OpenSSL, we have no control over the send() calls, so this
// rules out MSG_NOSIGNAL.
#define USE_MSG_NOSIGNAL
#elif defined SO_NOSIGPIPE
#define USE_SO_NOSIGPIPE
#elif !defined _WINDOWS
#include <signal.h>
#define USE_SIGNAL
#endif

namespace {
std::map<uint8_t, const std::string> s_SOCKS_messages = {{0x00, "succeeded"},
      {0x01, "general SOCKS server failure"},
      {0x02, "connection not allowed by ruleset"},
      {0x03, "Network unreachable"}, {0x04, "Host unreachable"},
      {0x05, "Connection refused"}, {0x06, "TTL expired"},
      {0x07, "Command not supported"}, {0x08, "Address type not supported"}};

SuS::logfile::subsystem_registrator log_id{"TCPSocket"};
}

SuS::logfile::tcp_client_socket::tcp_client_socket(
      const std::string &_host, uint16_t _port)
   : m_d(new tcs_private) {
   m_d->m_host = _host;
   m_d->m_port = _port;

#ifdef _WINDOWS
   // initialize winsock
   WSADATA wsaData;
   auto res = WSAStartup(MAKEWORD(2, 2), &wsaData);
   assert(res == 0 && wsaData.wHighVersion == 2 && wsaData.wVersion == 2);
#endif

#ifdef USE_SIGNAL
   struct ::sigaction sa;
   sa.sa_handler = SIG_IGN;
   sa.sa_flags = 0;
   if (sigaction(SIGPIPE, &sa, 0) == -1) {
      throw std::runtime_error{"Could not set SIGPIPE handler as ignore."};
   }
#ifdef HAVE_SIGACTION
#else
   // TODO: error handling
   ::signal(SIGPIPE, SIG_IGN);
#endif
#endif
}

SuS::logfile::tcp_client_socket::~tcp_client_socket() {
   disconnect();
}

void SuS::logfile::tcp_client_socket::connect() {
   // when using SOCKS, we need to establish the connection to the SOCKS
   // service.
   const auto host = m_d->m_use_socks ? m_d->m_socks_host : m_d->m_host;
   const auto port = m_d->m_use_socks ? m_d->m_socks_port : m_d->m_port;

#if defined HAVE_GETADDRINFO || defined _WINDOWS
   ::addrinfo hints, *res;
   ::memset(&hints, 0, sizeof hints);
   hints.ai_family = AF_UNSPEC;
   hints.ai_socktype = SOCK_STREAM;

   const auto status = ::getaddrinfo(
         host.c_str(), std::to_string(port).c_str(), &hints, &res);
   if (status != 0) {
      auto error = std::string{"getaddrinfo("};
      error += host + ") failed: " + ::gai_strerror(status);
      SuS_LOG(warning, log_id(), error);
      throw std::runtime_error{error};
   }

   // iterate over all returned IPs
   for (const auto *p = res; p; p = p->ai_next) {
      if (connect_to_ip(p->ai_family, p->ai_socktype, p->ai_protocol,
                p->ai_addr, p->ai_addrlen)) {
         // accept the first that results in an established connection.
         break;
      }
   }
   ::freeaddrinfo(res);

   if (m_d->m_socket == INVALID_SOCKET) {
      throw std::runtime_error{"Could not connect."};
   }
#else
   const auto *server = ::gethostbyname(host.c_str());
   if (!server) {
      throw std::runtime_error{"gethostbyname failed."};
   }
   ::sockaddr_in addr;
   ::memset(&addr, 0, sizeof(addr));
   addr.sin_family = AF_INET;
   ::memcpy(&addr.sin_addr.s_addr, server->h_addr, server->h_length);
   addr.sin_port = htons(port);
   if (!connect_to_ip(AF_INET, SOCK_STREAM, 0,
             reinterpret_cast<::sockaddr *>(&addr), sizeof addr)) {
      throw std::runtime_error{"Could not connect."};
   }
#endif

   if (m_d->m_use_socks) {
      start_SOCKS();
   }

#ifdef OPENSSL_FOUND
   if (m_d->m_use_ssl) {
      start_SSL();
   }
#endif
}

bool SuS::logfile::tcp_client_socket::connect_to_ip(int _ai_family,
      int _socktype, int _protocol, ::sockaddr *_ai_addr,
      ::socklen_t _ai_addrlen) {
#ifdef HAVE_INET_NTOP
   char ipstr[INET6_ADDRSTRLEN];
   void *src = nullptr;
   if (_ai_family == AF_INET)
      src = &((reinterpret_cast<::sockaddr_in *>(_ai_addr))->sin_addr);
   else if (_ai_family == AF_INET6)
      src = &((reinterpret_cast<::sockaddr_in6 *>(_ai_addr))->sin6_addr);
   if (src && inet_ntop(_ai_family, src, ipstr, sizeof ipstr)) {
      if (m_d->m_use_socks) {
         SuS_LOG_STREAM(info, log_id(), "Connecting to SOCKS server "
                     << m_d->m_socks_host << " (" << ipstr << ":"
                     << m_d->m_socks_port << ").");
      } else {
         SuS_LOG_STREAM(info, log_id(), "Connecting to "
                     << m_d->m_host << " (" << ipstr << ":" << m_d->m_port
                     << ").");
      }
   }
#endif

   m_d->m_socket = ::socket(_ai_family, _socktype, _protocol);
   if (m_d->m_socket < 0) {
      return false;
   }
#ifdef USE_SO_NOSIGPIPE
   const auto val = int{1};
   if (::setsockopt(
             m_d->m_socket, SOL_SOCKET, SO_NOSIGPIPE, &val, sizeof val) != 0) {
      ::close(m_d->m_socket);
      m_d->m_socket = INVALID_SOCKET;
      return false;
   }
#endif

   if (::connect(m_d->m_socket, _ai_addr, _ai_addrlen) != 0) {
#ifdef HAVE_STRERROR_R
      char errstr[256];
#ifdef STRERROR_R_CHAR_P
      SuS_LOG_STREAM(warning, log_id(), "Connection failed: " << ::strerror_r(
                                              errno, errstr, sizeof errstr));
#else
      if (::strerror_r(errno, errstr, sizeof errstr) == 0) {
         SuS_LOG_STREAM(warning, log_id(), "Connection failed: " << errstr);
      }
#endif
#else
      SuS_LOG_STREAM(
            warning, log_id(), "Connection failed: " << ::strerror(errno));
#endif

#ifdef _WINDOWS
      ::closesocket(m_d->m_socket);
#else
      ::close(m_d->m_socket);
#endif
      m_d->m_socket = INVALID_SOCKET;
      return false;
   }
   SuS_LOG(finer, log_id(), "Connected.");
   return true;
}

void SuS::logfile::tcp_client_socket::disconnect() {
   // ignore errors. we want to get rid of the connection in any case.
#ifdef _WINDOWS
   ::closesocket(m_d->m_socket);
#else
   ::close(m_d->m_socket);
#endif
   m_d->m_socket = INVALID_SOCKET;
}

bool SuS::logfile::tcp_client_socket::connected() {
   return m_d->m_socket != INVALID_SOCKET;
}

void SuS::logfile::tcp_client_socket::start_SOCKS() {
   // first handshake: confirm common authentication options.
   // NOTE: we are not compliant to RFC1928 until we support GSSAPI.
   uint8_t hello[]{0x05 /* VER */, 0x01 /* NMETHODS */,
         0x00 /* NO AUTHENTICATION REQUIRED */};
   write(hello, sizeof hello);
   select_read();
   uint8_t version_select[2]{};
   read(version_select, 2);
   if (version_select[0] != 0x05 /* VERSION */
         || version_select[1] != 0x00 /* NO AUTHENTICATION REQUIRED */) {
      throw std::runtime_error{"SOCKS handshake failed."};
   }

   // second contact: request the connection.
   if (m_d->m_host.size() > 255) {
      throw std::invalid_argument{"host name too long for SOCKS request."};
   }
   auto request = std::vector<uint8_t>{0x05 /* VER */, 0x01 /* CONNECT */,
         0x00 /* RSV */, 0x03 /* DOMAIN NAME */,
         uint8_t(m_d->m_host.size()) /* length */};
   std::copy(
         m_d->m_host.cbegin(), m_d->m_host.cend(), std::back_inserter(request));
   request.push_back(m_d->m_port >> 8);
   request.push_back(m_d->m_port & 0xFF);
   write(request.data(), request.size());
   select_read();
   uint8_t connect_reply[10]{}; // TODO: reply is actually variable-length
   read(connect_reply, 10);
   if (connect_reply[0] != 0x05 /* VERSION */
         || connect_reply[2] != 0x00 /* RSV */) {
      throw std::runtime_error{"SOCKS handshake failed."};
   }
   if (connect_reply[1] != 0x00) {
      const auto i = s_SOCKS_messages.find(connect_reply[2]);
      if (i != s_SOCKS_messages.end()) {
         throw std::runtime_error{i->second};
      } else {
         throw std::runtime_error{"Unknown SOCKS error"};
      }
   }
   SuS_LOG(finer, log_id(), "SOCKS connection established.");
}

unsigned SuS::logfile::tcp_client_socket::read(uint8_t *_data, unsigned _len) {
   if (!connected()) {
      throw std::runtime_error{"Not connected."};
   }
#ifdef OPENSSL_FOUND
   if (m_d->m_ssl_active) {
      const auto ret = ::SSL_read(m_d->m_ssl, _data, _len);
      if (ret < 0) {
         // we run with SSL_MODE_AUTO_RETRY, so OpenSSL cannot request
         // action from us to complete the read.
         throw std::runtime_error{"SSL_read failed."};
      }
      return ret;
   }
#endif

#ifdef _WINDOWS
   const auto ret =
         ::recv(m_d->m_socket, reinterpret_cast<char *>(_data), _len, 0);
#else
   const auto ret = ::read(m_d->m_socket, _data, _len);
#endif
   if (ret < 0) {
      throw std::runtime_error{"Read failed."};
   }
   return ret;
}

bool SuS::logfile::tcp_client_socket::select_read(uint_fast64_t _timeout_us) {
   if (!connected()) {
      throw std::runtime_error{"Not connected."};
   }
#ifdef OPENSSL_FOUND
   if (m_d->m_ssl_active && ::SSL_pending(m_d->m_ssl)) {
      // OpenSSL still has data available.
      return true;
   }
#endif
   ::fd_set master_set;
   FD_ZERO(&master_set);
   FD_SET(m_d->m_socket, &master_set);
   ::fd_set except_set;
   ::memcpy(&except_set, &master_set, sizeof master_set);
   ::timeval timeout;
   timeout.tv_sec = _timeout_us / 1000000U;
   timeout.tv_usec = _timeout_us % 1000000U;
   const auto to = (_timeout_us == 0) ? nullptr : &timeout;
#ifdef _WINDOWS
   const auto rc = select(1, &master_set, nullptr, &except_set, to);
#else
   const auto rc =
         select(m_d->m_socket + 1, &master_set, nullptr, &except_set, to);
#endif
   if (rc < 0) {
      throw std::runtime_error{"Select failed."};
   } else if (rc == 0) {
      // timeout
      return false;
   } else if (rc == 1) {
      if (FD_ISSET(m_d->m_socket, &master_set)) {
         return true;
      } else if (FD_ISSET(m_d->m_socket, &except_set)) {
         throw std::runtime_error{"Exception on the socket."};
      } else {
         throw std::runtime_error{"Cannot find what ended select."};
      }
   } else {
      // one descriptor in the set and more than one set in the result!?
      throw std::runtime_error{"Cannot understand the result of select."};
   }
}

void SuS::logfile::tcp_client_socket::use_SOCKS(
      const std::string &_host, uint16_t _port) {
   if (_host.empty()) {
      m_d->m_use_socks = false;
      return;
   }
   // else
   m_d->m_use_socks = true;
   m_d->m_socks_host = _host;
   m_d->m_socks_port = _port;
}

void SuS::logfile::tcp_client_socket::write(
      const uint8_t *_data, unsigned _len) {
   if (!connected()) {
      throw std::runtime_error{"Not connected"};
   }

#ifdef OPENSSL_FOUND
   if (m_d->m_ssl_active) {
      if (::SSL_write(m_d->m_ssl, _data, _len) != int(_len)) {
         throw std::runtime_error{"Write failed."};
      }
      return;
   }
#endif

#ifdef USE_MSG_NOSIGNAL
   if (::send(m_d->m_socket, _data, _len, MSG_NOSIGNAL) != _len) {
#elif defined _WINDOWS
   if (::send(m_d->m_socket, reinterpret_cast<const char *>(_data), _len, 0) != _len) {
#else
   if (::write(m_d->m_socket, _data, _len) != _len) {
#endif
      throw std::runtime_error{"Write failed."};
   }
}

#ifdef OPENSSL_FOUND
void SuS::logfile::tcp_client_socket::use_SSL(bool _self_signed_ok) {
   m_d->m_use_ssl = true;
   m_d->m_ssl_self_signed_ok = _self_signed_ok;
}

void SuS::logfile::tcp_client_socket::init_OpenSSL() {
   ::OpenSSL_add_all_algorithms();
   ::ERR_load_crypto_strings();
   ::SSL_load_error_strings();
   if (::SSL_library_init() < 0)
      throw std::runtime_error{"Failed to initialize OpenSSL."};
}

void SuS::logfile::tcp_client_socket::start_SSL() {
   init_OpenSSL();

   if ((m_d->m_ssl_ctx = ::SSL_CTX_new(::SSLv23_client_method())) == nullptr) {
      throw std::runtime_error{"Could not create SSL context."};
   }

   // tell OpenSSL to take care of handshakes without telling us.
   ::SSL_CTX_set_mode(m_d->m_ssl_ctx, SSL_MODE_AUTO_RETRY);

   // disable insecure SSLv2.
   ::SSL_CTX_set_options(m_d->m_ssl_ctx, SSL_OP_NO_SSLv2);

   const auto store = ::SSL_CTX_get_cert_store(m_d->m_ssl_ctx);
   auto lookup = ::X509_STORE_add_lookup(store, ::X509_LOOKUP_file());
   // nullptr + X509_FILETYPE_DEFAULT => default location
   ::X509_LOOKUP_load_file(lookup, nullptr, X509_FILETYPE_DEFAULT);
   lookup = ::X509_STORE_add_lookup(store, ::X509_LOOKUP_hash_dir());
   // nullptr + X509_FILETYPE_DEFAULT => default location
   ::X509_LOOKUP_add_dir(lookup, nullptr, X509_FILETYPE_DEFAULT);

   m_d->m_ssl = ::SSL_new(m_d->m_ssl_ctx);
   ::SSL_set_fd(m_d->m_ssl, m_d->m_socket);

   if (::SSL_connect(m_d->m_ssl) != 1) {
      throw std::runtime_error{"Could not establish SSL session."};
   }
   const auto cert = ::SSL_get_peer_certificate(m_d->m_ssl);
   SuS_LOG_STREAM(config, log_id(), "name: " << cert->name);

   const auto verify_result = ::SSL_get_verify_result(m_d->m_ssl);

   if (verify_result != X509_V_OK) {
      if (m_d->m_ssl_self_signed_ok &&
            ((verify_result == X509_V_ERR_DEPTH_ZERO_SELF_SIGNED_CERT) ||
                  (verify_result == X509_V_ERR_SELF_SIGNED_CERT_IN_CHAIN))) {
         // self-signed
         SuS_LOG(finest, log_id(), "Accepting self-signed certificate.");
      } else {
         const auto verify_error =
               ::X509_verify_cert_error_string(verify_result);
         throw std::runtime_error{verify_error};
      }
   }
   m_d->m_ssl_active = true;
   SuS_LOG(finer, log_id(), "SSL connection established.");
}
#endif
