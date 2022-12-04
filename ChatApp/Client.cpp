#include "Client.h"

Client::Client(const char* _addressServer, USHORT _portServer, USHORT _portClient)
{

    LocalPort = _portClient;
    ServerPort = _portServer;
    ServerAddress = _addressServer;
    InitWSA();
    CreateSocket(_portClient);
    BindSocket();
    Connect();
}

void Client::Connect()
{
    // create address for server connection
    sockaddr_in receiverAddr;
    receiverAddr.sin_family = AF_INET;
    receiverAddr.sin_port = htons(ServerPort);
    receiverAddr.sin_addr.s_addr = inet_addr(ServerAddress);

    // try connect to server
    int status = connect(Socket, (struct sockaddr*)&receiverAddr, sizeof(struct sockaddr));

    if (status == -1)
    {
        std::cout << "Error in connect(). Error code: " << WSAGetLastError() << "\n";
        return;
    }

    // pass connection to new thread
    std::thread sendThread(&Client::HandleConnection_Send, this, Socket, receiverAddr);
    std::thread recvThread(&Client::HandleConnection_Recv, this, Socket, receiverAddr);
    sendThread.join();
    recvThread.join();
    closesocket(Socket);
}

void Client::HandleConnection_Recv(int _socket, sockaddr_in _address)
{

    char buffer[BUFFER_SIZE];

    bool isActive = true;

    while (isActive)
    {

        int status = recv(_socket, buffer, BUFFER_SIZE, 0);

        if (status == -1)
        {
            std::cout << "ERROR in recv(). Error code: " << WSAGetLastError() << std::endl;
            isActive = false;
            continue;
        }
        buffer[status] = '\0';

        std::string msg(buffer);

        std::cout << "The message received is: " << msg << std::endl;

        if (ClientShuttingDown)
        {
            isActive = false;
        }

    }
}

void Client::HandleConnection_Send(int _socket, sockaddr_in _address)
{

    char buffer[BUFFER_SIZE];

    bool isActive = true;

    while (isActive)
    {
        std::cout << "Enter a message to be sent: ";
        gets_s(buffer);
        std::string str(buffer);
        if (str.length() < 1)
        {
            continue;
        }

        int status = send(_socket, buffer, strlen(buffer), 0);

        if (status == -1)
        {
            std::cout << "Error in send().Error code: " << WSAGetLastError();
            isActive = false;
            continue;
        }

        std::string message(buffer);
        if (message == "QUIT")
        {
            ClientShuttingDown = true;
            closesocket(Socket);
        }

        if (ClientShuttingDown)
        {
            isActive = false;
        }
    }
}
