#pragma once

#include "Net_Socket.h"
#include <iostream>

class Client : Net_Socket
{

public:

	Client(const char* _addressServer, USHORT _portServer, USHORT _portClient);

protected:

	void Connect();

	void HandleConnection_Recv(int _socket, sockaddr_in _address);

	void HandleConnection_Send(int _socket, sockaddr_in _address);

	USHORT ServerPort;
	const char* ServerAddress;
};

