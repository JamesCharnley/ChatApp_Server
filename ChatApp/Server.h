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

	bool Login(FLogin_Packet _login_packet);

	bool Signup(FLogin_Packet _login_packet);

	bool PostToRoom(FPost_Message_Packet _packet);

protected:

	std::vector<UserPass> UserDB = std::vector<UserPass>();

	void Listen();

	void MonitorConnections();

	std::vector<Connection*> Connections = std::vector<Connection*>();
	std::vector<Connection*> NewConnectionQueue = std::vector<Connection*>();

	std::mutex Queue_Mutex;
	std::condition_variable Queue_cv;

	std::mutex file_mutex;
	std::mutex room_file_mutex;

	bool ServerShuttingDown = false;
};

