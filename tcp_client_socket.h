/* SPDX-License-Identifier: MIT */
/*! @file */
#pragma once

#include "config.h"

#include <cstdint>
#include <memory>
#include <string>

#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#include <sys/types.h>

#ifdef OPENSSL_FOUND
#include <openssl/ssl.h>
#endif

#ifdef _WINDOWS
#include <WinSock2.h>
#include <WS2tcpip.h>
#else
#ifndef INVALID_SOCKET
#define INVALID_SOCKET (-1)
#endif
#endif

namespace SuS {
namespace logfile {

struct tcs_private;

//! A TCP client socket optionally using SOCKS5 and SSL.
/*!
 * The socket is IPv6 capable on systems where getaddrinfo is supported.
 *
 * When using SOCKS, the host name is sent to the SOCKS server, so that it
 * is resolved by the SOCKS server.
 *
 * Only a subset of SOCKS5 is supported at the moment. The implementation
 * is not compliant to RFC1928 until authentication is implemented.
 *
 * When SSL and SOCKS5 are used together, the encryption is between this
 * socket and the actual server, not the SOCKS server.
 */
class tcp_client_socket {
 public:
   //! Initialize the object.
   /*!
    * No connection is established by this constructor.
    * @param _host Host name of the server.
    * @param _port Port number to connect to.
    */
   tcp_client_socket(const std::string &_host, uint16_t _port);

   tcp_client_socket(tcp_client_socket &other) = delete;
   tcp_client_socket &operator=(tcp_client_socket &_other) = delete;

   ~tcp_client_socket();

   //! Try to connect to the configured host/port.
   /*!
    * This function initiates the DNS requests to resolve the host names and
    * tries to connect to all returned IPs until a connection succeeds.
    */
   void connect();
   //! Disconnect from the server.
   void disconnect();

   bool connected();

   //! Read from the socket.
   /*!
    * @param _data Pointer to the buffer to read into.
    * @param _len Maximum number of bytes to read.
    * @return Number of bytes read.  0 means the connection has been closed.
    */
   unsigned read(uint8_t *_data, unsigned _len);

   //! Call select for available data on the socket.
   /*!
    * @param _timeout_us Timeout in microseconds.
    * @return True, if the select terminated with data available. False in
    * case of a timeout.
    */
   bool select_read(uint_fast64_t _timeout_us = 0);

   //! Write to the socket.
   /*!
    * @param _data Pointer to the first byte to send.
    * @param _len Number of bytes to send.
    */
   void write(const uint8_t *_data, unsigned _len);

   //! Enable the SOCKS mode.
   /*!
    * @param _host Host name of the SOCKS server. Empty to disable SOCKS.
    * @param _port Port number of the SOCKS service.
    */
   void use_SOCKS(const std::string &_host, uint16_t _port);

#ifdef OPENSSL_FOUND
   //! Enable SSL encyrption of the connection.
   void use_SSL(bool _self_signed_ok = false);
#endif

 private:
   //! Try to connect to an IP address.
   bool connect_to_ip(int _ai_family, int _socktype, int _protocol,
         ::sockaddr *_ai_addr, ::socklen_t _ai_addrlen);

   void start_SOCKS();

#ifdef OPENSSL_FOUND
   void init_OpenSSL();
   void start_SSL();
#endif

   std::unique_ptr<tcs_private> m_d;
};
}
}
