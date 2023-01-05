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
#include <string>
#include <iostream>
#include "PacketDecoder.h"

#define BUFFER_SIZE 100

class Connection
{

public:

	Connection(class Server* _server, int _clientSocket);
	~Connection();

	void CloseConnection();

	bool IsConnectionClosed();

	std::string GetUsername() { return username; };
	void set_username(std::string _username) { username = _username; };

	void PushMessage(std::string _message);

protected:

	void MessageReceived(int _sender, std::string _message);

	void HandleConnection_send(int _socket);
	void HandleConnection_recv(int _socket);

	std::queue<std::string> MessageQueue;

	bool UnlockMutex();

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

	std::string username;

	void ExecuteLogin(FCommand_Packet _command_packet);
	void ExecuteSignup(FCommand_Packet _command_packet);

	void GetRequest(FGet_Post_Packet _packet);

	void GetRequest_Room(FGet_Post_Packet _packet);

	void PostRequest(FGet_Post_Packet _packet);

	void PostRequest_Message(FPost_Message_Packet _packet);

};

