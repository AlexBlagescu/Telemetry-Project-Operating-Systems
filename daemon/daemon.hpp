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
#include <time.h>
#include <thread>
#include <functional>
#include <utility>
#include <mutex>
#include <deque>
#include <cstdint>
#include <optional>
#include "packetutils.hpp"

#define TLM_PUBLISHER 0x1
#define TLM_SUBSCRIBER 0x2
#define TLM_BOTH 0x3

#define TLM_MESSAGE_POST 0x1
#define TLM_MESSAGE_READ 0x2
#define TLM_MESSAGE_CALLBACK 0x3

#define TLM_DAEMON_PORT 18273
#define TLM_MTU 2048
#define TLM_CLIENT_BACKLOG 0x10

struct client
{
	int32_t client_sockfd;
	struct sockaddr_in addr;
};

class channel_node
{
    private:
        int8_t node_type;
        std::string name;
        std::vector<channel_node> children;
        std::vector<client> subscribers;
        std::vector<client> publishers;

    public:
        channel_node() : name(), node_type(-1), children(), subscribers(), publishers() {}
        channel_node(const std::string& chname) : name(chname), node_type(-1), children(), subscribers(), publishers() {}
        ~channel_node() {}

        channel_node(const channel_node& other) :
            node_type(other.node_type),
            name(other.name),
            children(other.children),
            subscribers(other.subscribers),
            publishers(other.publishers) {}

        channel_node(channel_node&& other)
            :
            node_type(other.node_type),
            name(std::move(other.name)),
            children(std::move(other.children)),
            subscribers(std::move(other.subscribers)),
            publishers(std::move(other.publishers)) {}

        channel_node& operator=(const channel_node& other)
        {
            this->node_type = other.node_type;
            this->name = other.name;
            this->children = other.children;
            this->subscribers = other.subscribers;
            this->publishers = other.publishers;
            return *this;
        }

        channel_node& operator=(channel_node&& other)
        {
            this->node_type = other.node_type;
            this->name = std::move(other.name);
            this->children = std::move(other.children);
            this->subscribers = std::move(other.subscribers);
            this->publishers = std::move(other.publishers);
            return *this;
        }

        void add_subscriber(const client& subscriber)
        {
            subscribers.push_back(subscriber);
        }

        void add_publisher(const client& publisher)
        {
            publishers.push_back(publisher);
        }

        void add_child(const channel_node& child)
        {
            children.push_back(child);
        }

        channel_node& operator[](size_t idx)
        {
            return children.at(idx);
        }

        std::vector<channel_node>& get_children()
        {
            return children;
        }

        std::vector<client>& get_subscribers()
        {
            return subscribers;
        }

        std::vector<client>& get_publishers()
        {
            return publishers;
        }

        std::string& get_name()
        {
            return name;
        }
};

class Daemon
{
private:
	int32_t socket_fd;
	std::vector<std::thread> client_threads;
	std::vector<client> clients;
	std::thread accept_thread;
    std::vector<channel_node> channels;

public:
    std::vector<std::string> queued_messages;
    std::mutex queue_lock;

	Daemon()
		:
		socket_fd(-1),
		client_threads(),
		clients(),
		accept_thread(),
		channels(),
		queued_messages(),
		queue_lock()
	{
	    channels.emplace_back("");
	}

	~Daemon()
	{
		for(auto& thr : client_threads)
			thr.join();

		accept_thread.join();
	}

	int32_t get_sockfd()
	{
		return socket_fd;
	}

	std::vector<std::thread>& get_threads()
	{
		return client_threads;
	}

	std::vector<client>& get_clients()
	{
		return clients;
	}

    std::vector<channel_node>& get_channel_nodes()
    {
        return channels;
    }

	void initialize(int16_t init_port);
	void join();
	void send(size_t client_idx, const std::string& payload);
	std::string recv(size_t client_idx);

	void send(const client& client_inst, const std::string& payload);
	std::string recv(const client& client_inst);

    std::optional<std::reference_wrapper<channel_node>> find_channel(const std::string& name, channel_node& root_node)
    {
        printf("We are in find_channel of [%s]\n", root_node.get_name().c_str());
        printf("We are looking for [%s]\n", name.c_str());
        for(auto& chan : root_node.get_children())
        {
            if(name == chan.get_name())
            {
                printf("Found match! [%s]\n", name.c_str());
                return chan;
            }

            if(chan.get_children().empty())
                continue;

            auto child_chan = find_channel(name, chan);
            if(child_chan)
                return child_chan;
        }

        return {};
    }

	channel_node construct_channel(const std::string& chan, int8_t client_type, const client& inst, channel_node& root)
    {
        channel_node new_chan(chan);
        if(client_type == TLM_PUBLISHER)
            new_chan.add_publisher(inst);
        else if(client_type == TLM_SUBSCRIBER)
            new_chan.add_subscriber(inst);

        printf("Adding child... [%s]\n", chan.c_str());
        root.add_child(new_chan);
        channels.push_back(new_chan);
        return new_chan;
    }
};

void listening_thread(Daemon& daemon);
void client_interaction_thread(Daemon& daemon, client new_client);
