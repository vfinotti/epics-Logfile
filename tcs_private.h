/* SPDX-License-Identifier: MIT */
#pragma once

#ifndef _WINDOWS
typedef int SOCKET;
#endif

namespace SuS {
namespace logfile {

struct tcs_private {
   std::string m_host; ///< The server's host name.
   uint16_t m_port;    ///< The port number to connect to.

   bool m_use_socks{false};  ///< Use a SOCKS5 proxy.
   std::string m_socks_host; ///< Name of the SOCKS server.
   uint16_t m_socks_port;    ///< Port number of the SOCKS service.

   SOCKET m_socket{INVALID_SOCKET}; ///< The socket handle.

#ifdef OPENSSL_FOUND
   bool m_use_ssl{false};            ///< Use SSL.
   bool m_ssl_active{false};         ///< An SSL session has been initialized.
   bool m_ssl_self_signed_ok{false}; ///< Accept self-signed certificates.
   SSL *m_ssl;
   SSL_CTX *m_ssl_ctx;
#endif
};

} // namespace logfile
} // namespace SuS
