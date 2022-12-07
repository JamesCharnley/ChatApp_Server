#pragma once
#include <stdio.h>
#include <winsock.h>
#include <thread>
#include <iostream>
#include <vector>
#include <map>
#include <mutex>
#include <queue>
#include <condition_variable>
#include "Server.h"
#include <string>
#include <iostream>
#include "PacketDecoder.h"

#define BUFFER_SIZE 100


enum class ECOMMAND
{
	None,
	Capitalize,
	Get,
	Put,
	Quit,
	Login,
	Signup
};

class Connection
{

public:

	Connection(class Server* _server, int _clientSocket);
	~Connection();

	void CloseConnection();

	bool IsConnectionClosed();

protected:

	void MessageReceived(int _sender, std::string _message);

	void ExecuteCommand(int _client, ECOMMAND _command, std::string _message);

	void HandleConnection_send(int _socket);
	void HandleConnection_recv(int _socket);

	void PushMessage(std::string _message);

	std::string CapitalizeString(std::string _string);

	std::string ClientInputBuffer = "";

	std::vector<std::string> ClientInputStorage = std::vector<std::string>();

	std::map<int, std::string> CapitalizeContainer = std::map<int, std::string>();

	std::queue<std::string> MessageQueue;

	bool UnlockMutex();

	ECOMMAND CurrentCommand = ECOMMAND::None;

	class Server* ServerClass = nullptr;

	std::mutex Queue_Mutex;
	std::condition_variable Queue_cv;

	std::mutex GetRoom_Mutex;

	int ClientSocket = 0;

	bool ClosingConnection = false;
	bool ConnectionClosed = false;

	bool RecvThread_Active = true;
	bool SendThread_Active = true;

	bool isAuthenticated = false;

	void ExecuteLogin(FCommand_Packet _command_packet);

	void GetRequest(FGet_Packet _packet);

	void GetRequest_Room(FGet_Packet _packet);

};

