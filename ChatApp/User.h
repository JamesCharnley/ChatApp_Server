#pragma once
#include <string>
#include "Connection.h"
#include "Room.h"
#include <vector>

class Server;

class User
{

public:

	User();

	User(std::string _username, Connection* _connection, Server* _server);

	std::string GetUsername() { return username; };

	Connection* GetConnection() { return connection; };

	void terminate_user();

protected:

	std::string username;

	Server* server;

	Connection* connection;

	std::vector<Room*> rooms;

	void update_client();
};

