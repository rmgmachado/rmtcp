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

#include "rmtcplib.h"
#include <iostream>
#include <format>

using namespace rmgm;

class echo_client_blocking_t
{
   socket_t socket_;
   std::string received_;
   size_t index_{ 0 };

public:
   status_t execute(const std::string& message, size_t message_count, const std::string& host = "tcpbin.com", const std::string& port = "4242")
   {
      auto status = connect(host, port);
      if (status.nok()) return status;
      for (auto i = 0; i < message_count; ++i)
      {
         if (status = echo_message(message); status.nok()) return status;
      }
   }

private:
   status_t connect(const std::string& host, const std::string& port) noexcept
   {
      auto [address_list, status] = ipname_resolution(host, port);
      if (status.nok())
      {
         std::cout << std::format("ipname_resolution(): host {} cannot be reached, error {}\n", host, status.code());
         return status;
      }
      for (const auto& addr : address_list)
      {
         socket_t sock;
         if (status = sock.create(); status.ok())
         {
            if (status = sock.connect(addr); status.nok())
            {
               sock.close();
               continue;
            }
            socket_ = sock;
            return status;
         }
         else
         {
            std::cout << std::format("socket_t::create() failed with error {}\n", status.code());
            break;
         }
      }
      std::cout << std::format("connect() failed to connect to host {}\n", status.code());
      return std::make_pair(socket_t(), status);
   }

   status_t send_message(const std::string& message) noexcept
   {

   }

   status_t recv_message(const std::string& message, size_t bytes_expected) noexcept
   {

   }

   status_t echo_message(const std::string& message) noexcept
   {

   }

}; // class echo_client_blocking

int main()
{
   echo_client_blocking_t ec;
   status_t status = ec.execute();
   return 0;
}