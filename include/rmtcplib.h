/*****************************************************************************\
*  Copyright (c) 2023 Ricardo Machado, Sydney, Australia All rights reserved.
*
*  MIT License
*
*  Permission is hereby granted, free of charge, to any person obtaining a copy
*  of this software and associated documentation files (the "Software"), to
*  deal in the Software without restriction, including without limitation the
*  rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
*  sell copies of the Software, and to permit persons to whom the Software is
*  furnished to do so, subject to the following conditions:
*
*  The above copyright notice and this permission notice shall be included in
*  all copies or substantial portions of the Software.
*
*  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
*  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
*  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
*  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
*  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
*  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
*  IN THE SOFTWARE.
*
*  You should have received a copy of the MIT License along with this program.
*  If not, see https://opensource.org/licenses/MIT.
\*****************************************************************************/
#pragma once

#include <string>
#include <array>
#include <vector>

#include <winsock2.h>
#include <ws2tcpip.h>
#define WIN32_SOCKETs

#include "wepoll.h"

// link with Ws2_32.lib
#pragma comment (lib, "Ws2_32.lib")

namespace rmgm {

   namespace startup {


   } // namespace startup

   constexpr const int HN_MAX_HOSTNAME = 512;
   // socket_recv_buffer_size controls the size of a buffer in the stack
   // when socket_base_t::recv(std::string&) is called. Small values will 
   // require more calls to ::recv to receive all data in sockets buffer
   constexpr size_t SOCKET_RECV_SIZE = 4 * 1024;

   // triggers socket_base_t::recv(std::string&) to stop receiving when 
   // the amount received is larger or equal to socket_max_recv_size
   constexpr size_t SOCKET_RECV_MAX_SIZE = 64 * 1024;

   // define default listen() backlog size
   constexpr int SOCKET_LISTEN_BACKLOG = 512;

   // used with select and socket::wait functions
   using wait_timeout_t = int;

   constexpr wait_timeout_t SOCKET_WAIT_FOREVER = -1L;
   constexpr wait_timeout_t SOCKET_WAIT_NEVER = 0L;

   enum class nameres_type_t : int { normal = 0, passive = AI_PASSIVE };
   enum class socket_event_t { recv_ready, send_ready, connect_ready, accept_ready };
   enum class socket_mode_t { blocking, nonblocking };
   enum class socket_close_t : int { send = SD_SEND, recv = SD_RECEIVE, both = SD_BOTH };

   template <typename ErrT, ErrT OK = 0, ErrT NOK = SOCKET_ERROR>
   class status_base_t
   {
      ErrT code_{ OK };

   public:
      using value_type = ErrT;

      status_base_t() = default;
      ~status_base_t() = default;
      status_base_t(const status_base_t&) noexcept = default;
      status_base_t(status_base_t&&) noexcept = default;
      status_base_t& operator=(const status_base_t&) noexcept = default;
      status_base_t& operator=(status_base_t&&) noexcept = default;

      explicit status_base_t(ErrT code) noexcept
         : code_{ (code == NOK) ? WSAGetLastError() : code }
      {}

      friend void swap(status_base_t& lhs, status_base_t& rhs) noexcept
      {
         std::swap(lhs.code_, rhs.code_);
      }

      constexpr bool ok() const noexcept { return code_ == OK; }
      constexpr bool nok() const noexcept { return code_ != OK; }
      constexpr bool would_block() const noexcept { return code_ == WSAEWOULDBLOCK; }
      value_type code() const noexcept { return code_; }
      void code(value_type n) noexcept { code_ = n; };

   }; //class status_base_t

   using status_t = status_base_t<int>;

   class ipaddress_t
   {
      sockaddr addr_{};
      socklen_t len_{ 0 };

   public:
      ipaddress_t() = default;
      ~ipaddress_t() = default;
      ipaddress_t(const ipaddress_t&) = default;
      ipaddress_t(ipaddress_t&&) = default;
      ipaddress_t& operator=(const ipaddress_t&) = default;
      ipaddress_t& operator=(ipaddress_t&&) = default;

      ipaddress_t(const sockaddr* addr, socklen_t len) noexcept
         : addr_{ addr ? *addr : sockaddr{} }
         , len_{ len }
      {}

      ipaddress_t(const sockaddr& addr, socklen_t len) noexcept
         : addr_{ addr }
         , len_{ len }
      {}

      friend void swap(ipaddress_t& lhs, ipaddress_t& rhs) noexcept
      {
         std::swap(lhs.addr_, rhs.addr_);
         std::swap(lhs.len_, rhs.len_);
      }

      const sockaddr* address() const noexcept { return &addr_; }
      socklen_t length() const noexcept { return len_; }

      std::pair<std::string, std::string> peer_name() const noexcept
      {
         std::array<char, NI_MAXHOST> host = {};
         std::array<char, NI_MAXSERV> port = {};
         if (getnameinfo(&addr_, len_, host.data(), (socklen_t)host.size(), port.data(), (socklen_t)port.size(), (NI_NUMERICHOST | NI_NUMERICSERV)) == 0)
         {
            return std::make_pair(std::string(host.begin(), host.end()), std::string(port.begin(), port.end()));
         }
         return std::make_pair(std::string(), std::string());
      }
   }; // class ipaddress_t

   inline std::pair<std::string, status_t> local_host_name() noexcept
   {
      std::array<char, HN_MAX_HOSTNAME> name = {};
      status_t status(gethostname(name.data(), (int)name.size()));
      std::string host;
      if (status.ok())
      {
         host = std::string(name.data());
      }
      return std::make_pair(host, status);
   }

   using ipaddress_list_t = std::vector<ipaddress_t>;

   inline std::pair<ipaddress_list_t, status_t> ipname_resolution(const std::string& host, const std::string& port, nameres_type_t type = nameres_type_t::normal) noexcept
   {
      ipaddress_list_t list;

      PADDRINFOA addr{ nullptr };
      PADDRINFOA ptr{ nullptr };
      ADDRINFOA hints;
      ZeroMemory(&hints, sizeof(hints));
      hints.ai_flags = (int)type;
      hints.ai_family = AF_UNSPEC;
      hints.ai_socktype = SOCK_STREAM;
      hints.ai_protocol = IPPROTO_TCP;
      int retval = getaddrinfo(host.c_str(), port.c_str(), &hints, &addr);
      if (retval == 0)
      {
         ptr = addr;
         while (ptr != nullptr)
         {
            if (ptr->ai_addr != nullptr && ptr->ai_addrlen > 0)
            {
               list.emplace_back(ipaddress_t(ptr->ai_addr, (int)ptr->ai_addrlen));
            }
            ptr = ptr->ai_next;
         }
      }
      freeaddrinfo(addr);
      return std::make_pair(list, status_t{ retval });
   }

   class socket_t
   {
      using handle_type_t = SOCKET;
      mutable SOCKET handle_{ INVALID_SOCKET };
      mutable socket_mode_t mode_{ socket_mode_t::blocking };

   public:
      ~socket_t() = default;
      socket_t() = default;
      socket_t(const socket_t&) = default;
      socket_t& operator=(const socket_t&) = default;

      explicit socket_t(SOCKET handle, socket_mode_t mode = socket_mode_t::blocking) noexcept
         : handle_{ handle }
         , mode_{ mode }
      {}

      socket_t(socket_t&& other) noexcept
         : handle_{ other.handle_ }
         , mode_{ other.mode_ }
      {
         other.handle_ = INVALID_SOCKET;
         other.mode_ = socket_mode_t::blocking;
      }

      socket_t& operator=(socket_t&& other) noexcept
      {
         if (this != &other)
         {
            handle_ = other.handle_;
            mode_ = other.mode_;
            other.handle_ = INVALID_SOCKET;
            other.mode_ = socket_mode_t::blocking;
         }
         return *this;
      }

      friend void swap(socket_t& lhs, socket_t& rhs) noexcept
      {
         std::swap(lhs.handle_, rhs.handle_);
         std::swap(lhs.mode_, rhs.mode_);
      }

      SOCKET handle() const noexcept
      {
         return handle_;
      }

      status_t create() noexcept
      {
         handle_ = ::socket(AF_UNSPEC, SOCK_STREAM, IPPROTO_TCP);
         mode_ = socket_mode_t::blocking;
         return status_t((handle_ != INVALID_SOCKET) ? 0 : SOCKET_ERROR);
      }

      status_t connect(const ipaddress_t& addr) const noexcept
      {
         return status_t(::connect(handle_, addr.address(), addr.length()));
      }

      status_t close() const noexcept
      {
         status_t status(::closesocket(handle_));
         if (status.ok())
         {
            handle_ = INVALID_SOCKET;
            mode_ = socket_mode_t::blocking;
         }
         return status;
      }

      status_t bind(const ipaddress_t& addr) const noexcept
      {
         return status_t(::bind(handle_, addr.address(), addr.length()));
      }

      status_t listen(int backlog) const noexcept
      {
         return status_t(::listen(handle_, backlog));
      }

      status_t accept(socket_t& client) const noexcept
      {
         sockaddr addr;
         int addrlen = sizeof(addr);
         if (SOCKET handle = ::accept(handle_, &addr, &addrlen); handle != INVALID_SOCKET)
         {
            client.handle_ = handle;
            client.mode_ = socket_mode_t::blocking;
            return status_t(0);
         }
         return status_t(SOCKET_ERROR);
      }

      status_t accept(socket_t& client, wait_timeout_t timeout_ms) const noexcept
      {
         status_t status = wait(socket_event_t::accept_ready, timeout_ms);
         if (status.ok())
         {
            status = accept(client);
         }
         return status;
      }

      status_t shutdown(socket_close_t how = socket_close_t::send) const noexcept
      {
         return status_t(::shutdown(handle_, (int)how));
      }

      status_t ioctlsocket(long cmd, u_long* argp) const noexcept
      {
         return status_t(::ioctlsocket(handle_, cmd, argp));
      }

      status_t setsockopt(int level, int optname, const char* optval, int optlen) const noexcept
      {
         return status_t(::setsockopt(handle_, level, optname, optval, optlen));
      }

      status_t getsockopt(int level, int optname, char* optval, int* optlen) const noexcept
      {
         return status_t(::getsockopt(handle_, level, optname, optval, optlen));
      }

      socket_mode_t mode() const noexcept
      {
         return mode_;
      }

      status_t mode(socket_mode_t sm) const noexcept
      {
         u_long um = (sm == socket_mode_t::nonblocking) ? 1 : 0;
         status_t status(::ioctlsocket(handle_, FIONBIO, &um));
         mode_ = status.ok() ? sm : mode_;
         return status;
      }

      status_t send(const char* buffer, size_t len, size_t& bytes_sent) const noexcept
      {
         int count = ::send(handle_, buffer, static_cast<int>(len), 0);
         bytes_sent = (count != SOCKET_ERROR) ? count : 0;
         return status_t((count != SOCKET_ERROR) ? 0 : SOCKET_ERROR);
      }

      status_t send(const char* buffer, size_t len, size_t& bytes_sent, wait_timeout_t timeout_ms) const noexcept
      {
         status_t status = wait(socket_event_t::send_ready, timeout_ms);
         if (status.ok())
         {
            status = send(buffer, len, bytes_sent);
         }
         return status;
      }

      // if status_t::ok() == true and bytes_received == 0, peer terminating connection
      status_t recv(char* buffer, size_t len, size_t& bytes_received) const noexcept
      {
         int count = ::recv(handle_, buffer, static_cast<int>(len), 0);
         bytes_received = (count != SOCKET_ERROR) ? count : 0;
         return status_t(count != SOCKET_ERROR ? 0 : SOCKET_ERROR);
      }

      status_t recv(char* buffer, size_t len, size_t& bytes_received, wait_timeout_t timeout_ms) const noexcept
      {
         status_t status = wait(socket_event_t::recv_ready, timeout_ms);
         if (status.ok())
         {
            status = recv(buffer, len, bytes_received);
         }
         return status;
      }

      // timeout_us specifies how long wait will wait until an event occurs
      // SOCKET_WAIT_FOREVER to block until an event occurs
      // SOCKET_WAIT_NEVER to return immediately after checking
      // greater than zero value sets milli-seconds to wait until event occurs
      // wait succeeds:
      //    status_t::ok() == true, status_t::nok() == false, status_t::would_block() == false
      //
      // wait timeout
      //    status_t::ok() == false, status_t::nok() == true, status_t::would_block() == true
      // 
      // wait fails
      //    status_t::ok() == false, status_t::nok() == true, status_t::would_block() == false
      status_t wait(socket_event_t event, wait_timeout_t timeout_ms = SOCKET_WAIT_NEVER) const noexcept
      {
         WSAPOLLFD fdset;
         fdset.fd = handle_;
         fdset.revents = 0;
         fdset.events = (event == socket_event_t::recv_ready || event == socket_event_t::accept_ready) ? POLLRDNORM : POLLWRNORM;
         int count = WSAPoll(&fdset, 1, timeout_ms);
         if (count == 0) return status_t(WSAEWOULDBLOCK);
         if (count == SOCKET_ERROR) return status_t(SOCKET_ERROR);
         if (event == socket_event_t::connect_ready && fdset.revents & (POLLHUP | POLLERR | POLLWRNORM)) return status_t(WSAECONNREFUSED);
         return (fdset.events & (POLLHUP | POLLRDNORM | POLLWRNORM)) ? status_t(0) : status_t(WSAEWOULDBLOCK);
      }

   }; // class socket_t

   namespace startup {

      class socket_init_t
      {
         mutable WSADATA wd_;

      public:
         socket_init_t() noexcept
         {
            status_t s(::WSAStartup(MAKEWORD(2, 2), &wd_));
         }

         ~socket_init_t() noexcept
         {
            ::WSACleanup();
         }

         socket_init_t(const socket_init_t&) = delete;
         socket_init_t(socket_init_t&&) = delete;
         socket_init_t& operator=(const socket_init_t&) = delete;
         socket_init_t& operator=(socket_init_t&&) = delete;
      }; // class socket_init_t

      const static socket_init_t socket_startup_;

   } // namespace startup

} // namespace rmgm