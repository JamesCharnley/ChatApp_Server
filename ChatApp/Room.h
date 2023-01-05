#pragma once

#include <vector>
#include "Connection.h"
#include <string>

class Room
{

public:

	Room();

	Room(int _id, class Server* _server_class);

	std::string get_room_name();
	int get_room_id() { return id; };

	void add_active_user(class User _user);
	void remove_active_user(std::string _username);

	bool is_initialized = false;

	void add_new_message(std::string _message);

private:

	std::vector<User> active_users = std::vector<User>();

	std::string name = "";

	int id = 0;

	std::vector<std::string> members = std::vector<std::string>();

	std::vector<std::string> messages = std::vector<std::string>();

	void send_message_to_all_users(std::string _message);
	void send_message_to_user(User _user, std::string _message);
	void send_all_messages_to_user(User _user);

	std::mutex active_users_mutex;

	class Server* server_class = nullptr;
};

