#pragma once
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <cstdint>
#include <functional>
#include <algorithm>
#include <vector>
#include <string>
#include <utility>
#include <thread>

#include "packetutils.hpp"

#define TLM_PUBLISHER 0x1
#define TLM_SUBSCRIBER 0x2
#define TLM_BOTH 0x3

#define TLM_MESSAGE_POST 0x1
#define TLM_MESSAGE_READ 0x2
#define TLM_MESSAGE_CALLBACK 0x3

#define TLM_DAEMON_PORT 18273
#define TLM_MTU 2048

// open(type, channel)
// callback(type, channel) -> spawnez thread si fac polling
// read(token, msgid)
// post(token, msg)
// close()

class tlm_t
{
private:
	int32_t socket_fd;
	std::function<void(tlm_t&, const std::string&)> callback_pfn;
	int8_t user_type;

public:
	tlm_t()
		:
		socket_fd(-1),
		callback_pfn(),
		user_type(-1)
	{}

	tlm_t(int32_t socket_fd)
		:
		socket_fd(socket_fd),
		callback_pfn(),
		user_type(-1)
	{}


	tlm_t(const tlm_t& other)
		:
		socket_fd(other.socket_fd),
		callback_pfn(other.callback_pfn),
		user_type(other.user_type)
	{}

	tlm_t(tlm_t&& other)
		:
		socket_fd(other.socket_fd),
		callback_pfn(std::move(other.callback_pfn)),
		user_type(other.user_type)
	{}

	tlm_t& operator=(const tlm_t& other)
	{
		this->socket_fd = other.socket_fd;
		this->callback_pfn = other.callback_pfn;
		this->user_type = other.user_type;
		return *this;
	}

	tlm_t& operator=(tlm_t&& other)
	{
		this->socket_fd = other.socket_fd;
		this->callback_pfn = std::move(other.callback_pfn);
		this->user_type = other.user_type;
		return *this;
	}

	int32_t get_sockfd()
	{
		return this->socket_fd;
	}

	int32_t get_type()
	{
		return this->user_type;
	}

	void register_callback(std::function<void(tlm_t&, const std::string&)> pfn)
	{
		this->callback_pfn = pfn;
	}

	~tlm_t() {}
};

tlm_t tlm_open(int32_t type, const std::string& channel_name);
std::string tlm_read(tlm_t& token, uint32_t* message_id);
std::thread tlm_callback(tlm_t& token, std::function<void(tlm_t&, const std::string&)>);
int32_t tlm_post(tlm_t& token, const std::string& message);
void tlm_close(tlm_t& token);
int32_t tlm_type(tlm_t& token);
