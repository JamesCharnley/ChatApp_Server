#include "Server.h"
#include "Connection.h"


Connection::Connection(Server* _server, int _clientSocket)
{
    ClientSocket = _clientSocket;
    ServerClass = _server;

    // create threads for recv and send
    std::thread recvThread(&Connection::HandleConnection_recv, this, ClientSocket);
    std::thread sendThread(&Connection::HandleConnection_send, this, ClientSocket);

    // detach threads and set their bools
    recvThread.detach();
    RecvThread_Active = true;
    sendThread.detach();
    SendThread_Active = true;
    
    //PushMessage("you are connected to server");
}

Connection::~Connection()
{
    //close socket
    closesocket(ClientSocket);
}

void Connection::CloseConnection()
{
    ClosingConnection = true;
}

bool Connection::IsConnectionClosed()
{
    // check if both recv and send thread have completed
    if (RecvThread_Active == false && SendThread_Active == false)
    {
        return true;
    }
    else
    {
        return false;
    }
}

void Connection::MessageReceived(int _sender, std::string _message)
{
    // c_str() DOES return a cstring with null terminator
    FCommand_Packet command_packet = PacketDecoder::Char_To_Command_Packet(_message.c_str(), _message.length());

    switch (command_packet.Command)
    {
    case ECommand::Login:
        ExecuteLogin(command_packet);
        break;
    case ECommand::Signup:
        break;
    case ECommand::Post:
        break;
    case ECommand::Get:
        break;
    default:
        break;
    }
    
}

void Connection::ExecuteCommand(int _client, ECOMMAND _command, std::string _message)
{
    
    switch (_command)
    {
    case ECOMMAND::Capitalize:
    {
        if (_message == ".")
        {
            std::string msg = "";

            for (auto _char : ClientInputBuffer)
            {
                if (_char != '/')
                {
                    msg += _char;
                }
                else
                {
                    PushMessage(msg);
                    msg = "";
                }
            }
            ClientInputBuffer = "";
            CurrentCommand = ECOMMAND::None;
        }
        else
        {
            _message = CapitalizeString(_message);
            _message += "/";

            ClientInputBuffer += _message;

        }

    }
    break;
    case ECOMMAND::Get:
    {
        if (ClientInputStorage.size() > 0)
        {
            PushMessage(ClientInputStorage[ClientInputStorage.size() - 1]);
        }
        else
        {
            PushMessage("The storage is empty. Use the 'PUT' command to add to storage");
        }
    }
        break;
    case ECOMMAND::Put:
    {
        if (_message == ".")
        {
            CurrentCommand = ECOMMAND::None;
        }
        else
        {
            ClientInputStorage.push_back(_message);
        }
    }
        break;
    case ECOMMAND::Quit:
    {
        PushMessage("200 OK");
        ServerClass->ShutdownServer();
    }
    case ECOMMAND::Login:
    {
        PushMessage("200 OK");
        ServerClass->ShutdownServer();
    }
        break;
    default:
        break;
    }
}

void Connection::HandleConnection_send(int _socket)
{
    
    char buffer[BUFFER_SIZE];
    bool isActive = true;
    while (isActive)
    {
        // lock use of messagequeue while removing the next message
        std::unique_lock<std::mutex>lock(Queue_Mutex);
        Queue_cv.wait(lock, [this] { return UnlockMutex(); });
        if (MessageQueue.size() == 0)
        {
            continue;
        }
        std::string message = MessageQueue.front();
        MessageQueue.pop();
        // unlock messagequeue
        lock.unlock();

        // convert string to char
        int len = message.length();
        for (int i = 0; i < len; i++)
        {
            buffer[i] = message[i];
        }
        buffer[len] = '\0';
         // send message
        int status = send(_socket, buffer, strlen(buffer) + 1, 0);
        std::cout << "Sent Message " << buffer << "END";
        if (status == -1)
        {
            std::cout << "ERROR in send(). Error code: " << WSAGetLastError() << std::endl;
            ClosingConnection = true;
            isActive = false;
            continue;
        }
        
        if (ClosingConnection)
        {
            isActive = false;
        }
    }

    // thread is finished
    SendThread_Active = false;
}

void Connection::HandleConnection_recv(int _socket)
{
    char buffer[BUFFER_SIZE];

    bool isActive = true;
    while (isActive)
    {
        std::cout << "Waiting for message...\n";

        int status = recv(_socket, buffer, BUFFER_SIZE, 0);
        std::cout << "Status = " << status << "\n";

        if (status == -1)
        {
            std::cout << "ERROR in recv(). Error code: " << WSAGetLastError() << std::endl;
            ClosingConnection = true;
            isActive = false;
            continue;
        }
        std::cout << "recv: " << buffer << std::endl;

        buffer[status] = '\0';
        for (int i = 0; i < status + 1; i++)
        {
            if (buffer[i] == '\0')
            {
                std::cout << "null c found" << std::endl;
            }
            std::cout << buffer[i];
        }
        std::cout << std::endl;
        std::string msg(buffer);
        std::cout << "string length " << msg.length() << std::endl;
        MessageReceived(_socket, msg);

        if (ClosingConnection)
        {
            isActive = false;
        }
    }
    RecvThread_Active = false;

    // unlock send thread so the thread can finish
    Queue_cv.notify_one();
    
}

void Connection::PushMessage(std::string _message)
{
    // lock messagequeue and add new message to the queue
    std::lock_guard<std::mutex>lock(Queue_Mutex);
    MessageQueue.push(_message);
    Queue_cv.notify_one();
}

std::string Connection::CapitalizeString(std::string _string)
{
    int len = _string.length();
    for (int i = 0; i < len; i++)
    {
        _string[i] = std::toupper(_string[i]);
    }
    return _string;
}

bool Connection::UnlockMutex()
{
    bool unlock = false;
    if (ClosingConnection == true || !MessageQueue.empty())
    {
        unlock = true;
    }
    return unlock;
}

void Connection::ExecuteLogin(FCommand_Packet _command_packet)
{
    std::cout << "ExecuteLogin: com_packet - " << _command_packet.Content << std::endl;
    FLogin_Packet login_packet = PacketDecoder::Command_Packet_To_Login_Packet(_command_packet);
    if (ServerClass->Login(login_packet))
    {
        isAuthenticated = true;
        int com_int = (int)ECommand::Authorized;
        std::string com_str = std::to_string(com_int) + ";" + login_packet.Username;
        PushMessage(com_str);
    }
}
