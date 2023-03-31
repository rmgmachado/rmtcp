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
#include "rmunit.h"
#include "rmsockets.h"

using namespace rmsockets;

const std::string echo_server_url{ "tcpbin.com" };
const std::string echo_server_numeric{ "45.79.112.203" };
const std::string echo_server_port{ "4242" };

const std::string bogus_url("total_bogus_host_url");
const std::string bogus_port("10101");

TEST_CASE("Test status_t class", "[status_t]")
{
	SECTION("Test status_t default constructor")
	{
		status_t status;
		REQUIRE(status.ok());
		REQUIRE(!status.nok());
		REQUIRE(!status.would_block());
		REQUIRE(status.code() == 0);
	}
	SECTION("Test status_t explicit constructor")
	{
		status_t status(WSAEWOULDBLOCK);
		REQUIRE(!status.ok());
		REQUIRE(status.nok());
		REQUIRE(status.would_block());
		REQUIRE(status.code() == WSAEWOULDBLOCK);
	}
}

TEST_CASE("Test functions", "[functions]")
{
	SECTION("Test local_host_name() function")
	{
		auto [name, status] = local_host_name();
		REQUIRE(status.ok());
		REQUIRE(!status.nok());
		REQUIRE(!status.would_block());
		REQUIRE(name.length() > 0);
	}
	SECTION("Test ipname_resolution() url")
	{
		auto [address_list, status] = ipname_resolution(echo_server_url, echo_server_port);
		REQUIRE(status.ok());
		REQUIRE(!status.nok());
		REQUIRE(!status.would_block());
		REQUIRE(address_list.size() > 0);
		auto [host, port] = address_list[0].peer_name();
		REQUIRE(host == echo_server_numeric);
		REQUIRE(port == echo_server_port);
	}
	SECTION("Test ipname_resolution() numeric")
	{
		auto [address_list, status] = ipname_resolution(echo_server_numeric, echo_server_port);
		REQUIRE(status.ok());
		REQUIRE(!status.nok());
		REQUIRE(!status.would_block());
		REQUIRE(address_list.size() > 0);
		auto [host, port] = address_list[0].peer_name();
		REQUIRE(host == echo_server_numeric);
		REQUIRE(port == echo_server_port);
	}
	SECTION("Test ipname_resolution() failure")
	{
		auto [address_list, status] = ipname_resolution(bogus_url, bogus_port);
		REQUIRE(!status.ok());
		REQUIRE(status.nok());
		REQUIRE(!status.would_block());
		REQUIRE(address_list.empty());
	}
}

TEST_CASE("Test socket_t class", "[socket_t]")
{
	SECTION("Test socket_t default constructor")
	{
		socket_t socket;
		REQUIRE(socket.get_mode() == socket_mode_t::blocking);
		REQUIRE(socket.handle() == INVALID_SOCKET);
		REQUIRE(!socket.created());
	}
	SECTION("Test socket_t::connect() function blocking")
	{
		socket_t socket;
		auto [address_list, status] = ipname_resolution(echo_server_numeric, echo_server_port);
		REQUIRE(status.ok());
		REQUIRE(address_list.size() > 0);
		status = socket.connect(address_list[0]);
		REQUIRE(status.ok());
		status = socket.close();
		REQUIRE(status.ok());
	}
	SECTION("Test socket_t::connect() failed blocking")
	{
		socket_t socket;
		status_t status;
		status = socket.connect(ipaddress_t());
		REQUIRE(status.nok());
		status = socket.close();
		REQUIRE(status.nok());
	}
}

status_t send_message(const socket_t& socket, const std::string& message) noexcept
{
	size_t index = 0;
	size_t sent = 0;
	status_t status;
	while (status.ok() && index < message.length())
	{
		status = socket.send(&message[index], (message.length() - index), sent);
		index += sent;
	}
	return status;
}

status_t recv_message(const socket_t& socket, std::string& message) noexcept
{
	status_t status;
	message.clear();
	while (status.ok())
	{
		size_t received = 0;
		std::array<char, 1024> buffer;
		status = socket.recv(buffer.data(), buffer.size(), received);
		if (status.ok())
		{
			if (received == 0) break;
			message.append(buffer.begin(), buffer.begin() + received);
			if (message.find('\n') != std::string::npos) break;
		}
	}
	return status;
}

status_t echo_message(const socket_t& socket, const std::string& message_sent, std::string& message_recv) noexcept
{
	status_t status = send_message(socket, message_sent);
	if (status.ok())
	{
		status = recv_message(socket, message_recv);
	}
	return status;
}

TEST_CASE("Test tcp_client() blocking", "[tcp_client]")
{
	SECTION("Test tcp_client() function")
	{
		auto [socket, status] = tcp_client(echo_server_url, echo_server_port);
		REQUIRE(status.ok());
		REQUIRE(socket.created());
		status = socket.close();
		REQUIRE(status.ok());
	}
	SECTION("Test tcp_client() echofunction")
	{
		std::string message_send{ "Mary had a little lamb, little lamb\n" };
		std::string message_recv;
		auto [socket, status] = tcp_client(echo_server_url, echo_server_port);
		REQUIRE(status.ok());
		REQUIRE(socket.created());
		REQUIRE(echo_message(socket, message_send, message_recv).ok());
		REQUIRE(message_send == message_recv);
		status = socket.close();
		REQUIRE(status.ok());
	}
}
