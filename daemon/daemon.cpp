#include "daemon.hpp"
#include <errno.h>

void Daemon::initialize(int16_t init_port)
{
	struct sockaddr_in serv_addr;
	this->socket_fd = socket(AF_INET, SOCK_STREAM, 0);
	int32_t opt = 1;

	if(this->socket_fd == 0)
	{
		printf("Could not instantiate socket.\n");
		exit(EXIT_FAILURE);
	}

	if(setsockopt(this->socket_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)))
	{
		printf("Could not set the socket via setsockopt to reuse parameters!\n");
		exit(EXIT_FAILURE);
	}

	memset(&serv_addr, '\0', sizeof(serv_addr));

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr.sin_port = htons(TLM_DAEMON_PORT);

	if(bind(this->socket_fd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) , 0)
	{
		printf("Cannot bind the socket.\n");
		exit(EXIT_FAILURE);
	}

	if(listen(this->socket_fd, TLM_CLIENT_BACKLOG) < 0)
	{
		printf("Cannot listen!\n");
		exit(EXIT_FAILURE);
	}

	this->accept_thread = std::thread(listening_thread, std::ref(*this));
	printf("Daemon is now online on %d\n", TLM_DAEMON_PORT);
}

void listening_thread(Daemon& daemon)
{
	int32_t new_socket;
	struct sockaddr_in client_address;
	socklen_t addrlen = sizeof(client_address);

	while(true)
	{
		new_socket = accept(daemon.get_sockfd(), (struct sockaddr*)&client_address, &addrlen);
		if(new_socket == -1)
		{
			printf("Could not accept socket. errno: %s\n", strerror(errno));
			continue;
		}

		printf("A new client connected! [%s]\n", inet_ntoa(client_address.sin_addr));

		client new_client { .client_sockfd = new_socket, .addr = client_address };
		daemon.get_clients().push_back(new_client);
		daemon.get_threads().emplace_back(client_interaction_thread, std::ref(daemon), new_client);
	}
}

void Daemon::send(size_t client_idx, const std::string& payload)
{
	client target_client = this->clients.at(client_idx);
	if( ::send(target_client.client_sockfd, &payload[0], payload.size(), 0) < 0 )
	{
		printf("Could not send information to client idx %d\n", client_idx);
		return;
	}
}

void Daemon::send(const client& client_inst, const std::string& payload)
{
	if( ::send(client_inst.client_sockfd, &payload[0], payload.size(), 0) < 0 )
	{
		printf("Could not send information to client fd %d\n", client_inst.client_sockfd);
		return;
        }
}

std::string Daemon::recv(size_t client_idx)
{
	client target_client = this->clients.at(client_idx);
	std::string msg(TLM_MTU, '\0');

	int32_t recv_bytes = 0;
	if( (recv_bytes = ::recv(target_client.client_sockfd, &msg[0], msg.size() - 1, 0)) < 0)
	{
		printf("Could not receive from client with idx %d\n", client_idx);
	}

	if(recv_bytes > 0)
	{
		printf("SLICING STRING!\n");
		msg = std::string(&msg[0], recv_bytes);
	}
	return msg;
}

std::string Daemon::recv(const client& client_inst)
{
        std::string msg(TLM_MTU, '\0');
	int32_t recv_bytes = 0;
        if( (recv_bytes = ::recv(client_inst.client_sockfd, &msg[0], msg.size() - 1, 0)) < 0)
        {
                printf("Could not receive from client with fd %d\n", client_inst.client_sockfd);
        }

	if(recv_bytes >= 0)
		msg = std::string(&msg[0], recv_bytes);
        return msg;
}

void Daemon::join()
{
	accept_thread.join();

	for(auto& thr : client_threads)
		thr.join();
}

std::string get_user_type(int8_t packet_type)
{
	return (packet_type == 1) ? "publisher" : ((packet_type == 2) ? "subscriber" : "both");
}

void client_interaction_thread(Daemon& daemon, client new_client)
{
	printf("Client [fd: %d] thread is now online.\n", new_client.client_sockfd);
	daemon.send(new_client, packetutils::assemble_packet(0x01, "ACK"));
	std::string payload = daemon.recv(new_client);
	auto [ packet_type, chan ] = packetutils::parse_packet(payload);
	printf("Client [fd: %d] wishes to be a %s on channel [%s]\n", new_client.client_sockfd,
			get_user_type(packet_type).c_str(), chan.c_str());

	if(chan.find('/') == std::string::npos)
	{
		printf("Client wished to join an invalid channel! Aborting...\n");
		return;
	}

	std::vector<std::string> channels = packetutils::split_string(chan, '/');
	if(channels.empty())
	{
		printf("Invalid channels list\n");
		return;
	}

	std::string prev = channels[0];
	if(!prev.empty())
	{
		printf("Channel does not follow empty root path!\n");
		return;
	}

	std::vector<channel_node>& nodes = daemon.get_channel_nodes();
	channel_node root_channel = nodes.at(0);

	for(size_t i = 1; i < channels.size(); ++i)
	{
		std::string& node = channels.at(i);
		auto parent_node = daemon.find_channel(node, root_channel);

		if(!parent_node)
		{
			printf("Could not find parent node, instantiating node!\n");
			channel_node new_node = daemon.construct_channel(node, packet_type, new_client, root_channel);
			root_channel = new_node;
		}
	}

	while(true)
	{
		std::string response = daemon.recv(new_client);
		if(response.empty())
			continue;

		if(response.size() < 2)
			continue;

		printf("SIZE OF RESP: %d\n", response.size());

		auto [ packet_type, msg ] = packetutils::parse_packet(response);

		printf("---------> RECEIVED MESSAGE [%s]\n", response.c_str());
		if(packet_type == TLM_MESSAGE_POST)
		{
			printf("POST\n");
			daemon.queue_lock.lock();
			daemon.queued_messages.push_back(msg);
			daemon.queue_lock.unlock();
		}
		else if(packet_type == TLM_MESSAGE_READ)
		{
			printf("READ\n");
			daemon.queue_lock.lock();
			if(!daemon.queued_messages.empty())
			{
				std::string msg = daemon.queued_messages.back();
				daemon.send(new_client, msg);
				daemon.queued_messages.pop_back();
			}
			daemon.queue_lock.unlock();
		}
	}
}
