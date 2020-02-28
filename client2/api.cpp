#include "api.hpp"

tlm_t tlm_open(int32_t type, const std::string& channel_name)
{
	struct sockaddr_in serv_addr;
	int32_t clientfd = socket(AF_INET, SOCK_STREAM, 0);
	memset(&serv_addr, 0, sizeof(serv_addr));

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(TLM_DAEMON_PORT);
	serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

	if(connect(clientfd, reinterpret_cast<struct sockaddr*>(&serv_addr), sizeof(serv_addr)) < 0)
	{
		printf("Could not connect to the daemon process!\n");
		return {};
	}

	std::string packet = packetutils::assemble_packet(type & 0xFF, channel_name);
	if(send(clientfd, &packet[0], packet.size(), 0) < 0)
	{
		printf("Could not send information to the daemon socket\n");
		return {};
	}

	std::string buffered_reply(TLM_MTU, '\0');
	int32_t bytes_read = 0;
	if((bytes_read = read(clientfd, &buffered_reply[0], TLM_MTU - 1)) < 0)
	{
		printf("An error occured when receiving data from the userspace daemon.\n");
	}

	printf("Received packet from daemon: %s\n", &buffered_reply[0]);
	tlm_t token(clientfd);
	return token;
}

std::string tlm_read(tlm_t& token, uint32_t* message_id = nullptr)
{
	int32_t socket_fd = token.get_sockfd();
	std::string buffered_reply(TLM_MTU, '\0');
	int32_t bytes_read = 0;

	std::string new_pack = packetutils::assemble_packet(TLM_MESSAGE_READ, "SYN");
	send(socket_fd, &new_pack[0], new_pack.size(), 0);

	if((bytes_read = read(socket_fd, &buffered_reply[0], TLM_MTU - 1)) < 0)
	{
		printf("Could not receive data from the userspace daemon.\n");
		return {};
	}

	if(bytes_read >= 0)
		buffered_reply = std::string(&buffered_reply[0], bytes_read);

	return buffered_reply;
}

int32_t tlm_post(tlm_t& token , const std::string& message)
{
	int32_t socket_fd = token.get_sockfd();
	int32_t bytes_read = 0;

	std::string to_send = packetutils::assemble_packet(TLM_MESSAGE_POST, message);

	if(send(socket_fd, &to_send[0], to_send.size(), 0) < 0)
	{
		printf("Could not send data to the userspace daemon.\n");
		return {-1};
	}

	return 1;
}

std::thread tlm_callback(tlm_t& token, std::function<void(tlm_t&, const std::string&)> pfn_callback)
{
	token.register_callback(pfn_callback);
	std::thread polling_thread(
		[&] (tlm_t& token) -> void
		{
			std::string response, std_answr;
			while(true)
			{
				response = tlm_read(token, nullptr);
				if(response == std_answr)
					continue;

				pfn_callback(token, response);
			}
		}, std::ref(token));

	return std::move(polling_thread);
}

void tlm_close(tlm_t& token)
{
	int32_t socket_fd = token.get_sockfd();
	close(socket_fd);
}

int32_t tlm_type(tlm_t& token)
{
	return token.get_type();
}
