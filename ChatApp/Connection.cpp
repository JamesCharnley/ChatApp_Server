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
    if (isAuthenticated) { ServerClass->terminate_user(username); }
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
    std::cout << "MESSAGE RECIEVED" << std::endl;
    // c_str() DOES return a cstring with null terminator
    FCommand_Packet command_packet = PacketDecoder::Char_To_Command_Packet(_message.c_str(), _message.length());

    std::cout << "AFTER" << std::endl;

    switch (command_packet.Command)
    {
    case ECommand::Login:
        ExecuteLogin(command_packet);
        break;
    case ECommand::Signup:
        ExecuteSignup(command_packet);
        break;
    case ECommand::Post:
        PostRequest(PacketDecoder::Command_Packet_To_Get_Post_Packet(command_packet));
        break;
    case ECommand::Get:
        GetRequest(PacketDecoder::Command_Packet_To_Get_Post_Packet(command_packet));
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
            if (!ClosingConnection)
            {
                CloseConnection();
            }
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
            if (!ClosingConnection)
            {
                CloseConnection();
            }
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

bool Connection::UnlockMutex()
{
    if (ClosingConnection == true || !MessageQueue.empty())
    {
        return true;
    }
    return false;
}

void Connection::ExecuteLogin(FCommand_Packet _command_packet)
{
    std::cout << "ExecuteLogin: com_packet - " << _command_packet.Content << std::endl;
    FLogin_Packet login_packet = PacketDecoder::Command_Packet_To_Login_Packet(_command_packet);
    if (ServerClass->Login(login_packet, this))
    {
        username = login_packet.Username;
        isAuthenticated = true;

        int com_int = (int)ECommand::Authorized;
        std::string com_str = std::to_string(com_int) + ";" + login_packet.Username;
        PushMessage(com_str);
    }
}

void Connection::ExecuteSignup(FCommand_Packet _command_packet)
{
    std::cout << "ExecuteSignup: com_packet - " << _command_packet.Content << std::endl;
    FLogin_Packet login_packet = PacketDecoder::Command_Packet_To_Login_Packet(_command_packet);
    if (ServerClass->Signup(login_packet, this))
    {
        isAuthenticated = true;
        int com_int = (int)ECommand::Authorized;
        std::string com_str = std::to_string(com_int) + ";" + login_packet.Username;
        PushMessage(com_str);
    }
}

void Connection::GetRequest(FGet_Post_Packet _packet)
{
    std::cout << "GET REQUEST" << std::endl;
    if (_packet.Sub_Command == ESub_Command::InValid)
    {
        return;
    }

    switch (_packet.Sub_Command)
    {
    case ESub_Command::InValid:
        break;
    case ESub_Command::Room:
        GetRequest_Room(_packet);
        break;
    case ESub_Command::RoomList:
        break;
    case ESub_Command::Contacts:
        break;
    default:
        break;
    }
    
}

void Connection::GetRequest_Room(FGet_Post_Packet _packet)
{
    std::lock_guard<std::mutex>lock(GetRoom_Mutex);
    std::string line;
    std::string file_path = _packet.Content + ".txt";
    std::ifstream myfile(file_path);
    if (myfile.is_open())
    {
        while (getline(myfile, line))
        {
            std::cout << line << '\n';

            int com = (int)ECommand::Post;
            std::string comstring = std::to_string(com);
            std::string packet = comstring + ";" + line;
            PushMessage(packet);
        }
        myfile.close();
    }
    else
    {
        std::cout << "Execute Command: " << "Get;Room;" << _packet.Content << " ERROR " << "file: " << _packet.Content << ".txt " << "failed to open." << std::endl;
    }
}

void Connection::PostRequest(FGet_Post_Packet _packet)
{
    std::cout << "POST REQUEST" << std::endl;
    if (_packet.Sub_Command == ESub_Command::InValid)
    {
        std::cout << "sub com invalid" << std::endl;
        return;
    }

    switch (_packet.Sub_Command)
    {
    case ESub_Command::InValid:
        break;
    case ESub_Command::Room:
        break;
    case ESub_Command::RoomList:
        break;
    case ESub_Command::Contacts:
        break;
    case ESub_Command::Message:
        PostRequest_Message(PacketDecoder::Get_Post_Packet_To_Post_Message_Packet(_packet));
        break;
    default:
        break;
    }
}


void Connection::PostRequest_Message(FPost_Message_Packet _packet)
{
    std::cout << "PostRequest_Message" << std::endl;
    if (isAuthenticated)
    {
        std::cout << "User is auth" << std::endl;
        ServerClass->PostToRoom(_packet);
    }
    
}
