#pragma once

#include <vector>
#include "Connection.h"
#include <string>

class Room
{

public:

	Room();

	Room(int _id, class Server* _server_class);

	std::string GetName();
	int Get_ID() { return id; };

	void AddActiveUser(class User _user);
	void remove_active_user(std::string _username);
//
//	void RemoveActiveUser(Connection& _userConnection);
//
//	void NewMessage(std::string _message);

	void PrintTest();

	bool isInitialized = false;

	void Add_New_Message(std::string _message);

private:

	std::vector<User> activeUsers = std::vector<User>();

	std::string name = "";

	int id = 0;

	std::vector<std::string> members = std::vector<std::string>();

	std::vector<std::string> messages = std::vector<std::string>();

	void SendMessageToAllUsers(std::string _message);
	void SendMessageToUser(User _user, std::string _message);
	void SendAllMessagesToUser(User _user);

	std::mutex active_users_mutex;

	class Server* server_class = nullptr;
};

