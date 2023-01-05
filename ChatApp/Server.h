#pragma once
#include "Net_Socket.h"
#include <iostream>
#include <thread>
#include <map>
#include <vector>
#include <mutex>
#include <condition_variable>
#include "Connection.h"
#include "PacketDecoder.h"
#include <iostream>
#include <fstream>
#include <direct.h>
#include <string.h>
#include <cstdint>
#include <filesystem>
#include "Room.h"
#include "User.h"

struct UserPass
{
public:
	std::string Username;
	std::string Password;
};

class Server : Net_Socket
{

public:

	Server(USHORT _port);

	void ShutdownServer();

	bool Login(FLogin_Packet _login_packet, Connection* _connection);

	bool Signup(FLogin_Packet _login_packet, Connection* _connection);

	bool PostToRoom(FPost_Message_Packet _packet);

	void Activate_Room_By_id(int _room_id);
	void deactivate_Room_By_id(int _room_id);

	Room* Get_Room_By_ID(int _id);

	std::vector<Room*> active_rooms = std::vector<Room*>();
	bool is_room_active(int _room_id);

	void terminate_user(std::string _user);

protected:

	void Listen();

	void MonitorConnections();

	bool UsernameExists(std::string _username);
	void AddNewUser(FLogin_Packet _packet);
	void CreateUserProfile(FLogin_Packet _packet);

	void Initialize_User(Connection* _connection, FLogin_Packet _login_Packet);

	std::vector<User> active_users = std::vector<User>();

	std::vector<Connection*> Connections = std::vector<Connection*>();
	std::vector<Connection*> NewConnectionQueue = std::vector<Connection*>();

	std::mutex Queue_Mutex;
	std::condition_variable Queue_cv;

	std::mutex users_file_mutex;
	std::mutex room_file_mutex;

	bool ServerShuttingDown = false;

	int room_id_count = 0;
	std::mutex room_id_mutex;

	std::mutex active_user_mutex;
};

