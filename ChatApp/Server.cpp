#include "Server.h"

Server::Server(USHORT _port)
{
    FLogin_Packet p;
    Login(p);
    InitWSA();
    CreateSocket(_port);
    BindSocket();

    std::string packet = "3;1;Public Lounge;hello";

    FCommand_Packet com_pack = PacketDecoder::Char_To_Command_Packet(packet.c_str(), packet.length());
    FGet_Post_Packet get_pack = PacketDecoder::Command_Packet_To_Get_Post_Packet(com_pack);
    FPost_Message_Packet postpack = PacketDecoder::Get_Post_Packet_To_Post_Message_Packet(get_pack);

    std::cout << (int)postpack.Command << " " << (int)postpack.Sub_Command << " " << postpack.Room_Name << " " << postpack.Content << std::endl;

    

    // start a new thread for listening
    std::thread listenerThread(&Server::Listen, this);
    listenerThread.detach();
    // monitor the connections
    MonitorConnections();
}

void Server::ShutdownServer()
{
    // start shutting down server
    ServerShuttingDown = true;
    closesocket(Socket);
    
}

bool Server::Login(FLogin_Packet _login_packet)
{
    std::lock_guard<std::mutex>lock(file_mutex);
    std::string line;
    std::ifstream myfile("users.txt");
    if (myfile.is_open())
    {
        while (getline(myfile, line))
        {
            std::cout << line << '\n';
            
            FCommand_Packet cp = { _login_packet.Command, line.c_str() };
            FLogin_Packet entry = PacketDecoder::Command_Packet_To_Login_Packet(cp);
            if (_login_packet.Username == entry.Username)
            {
                std::cout << "Username match: " << "recieved: " << _login_packet.Username << " saved: " << entry.Username << std::endl;
                if (_login_packet.Password == entry.Password)
                {
                    myfile.close();
                    return true;
                }
                else
                {
                    std::cout << "Failed Login: " << "Password does not match " << "recieved: " << _login_packet.Password << " saved: " << entry.Password << std::endl;
                }
            }
            else
            {
                std::cout << "Failed Login: " << "Username does not match " << "recieved: " << _login_packet.Username << " saved: " << entry.Username << std::endl;
            }
        }
        myfile.close();
    }
    return false;
}

bool Server::Signup(FLogin_Packet _login_packet)
{
    std::lock_guard<std::mutex>lock(file_mutex);
    std::string line;
    std::ifstream myfile("users.txt");
    if (myfile.is_open())
    {
        while (getline(myfile, line))
        {
            std::cout << line << '\n';

            FCommand_Packet cp = { _login_packet.Command, line.c_str() };
            FLogin_Packet entry = PacketDecoder::Command_Packet_To_Login_Packet(cp);
            if (_login_packet.Username == entry.Username)
            {
                std::cout << "Sign up failed: Username already exists" << std::endl;
                myfile.close();
                return false;
            }
            
        }
        
        myfile.close();
    }

    std::ofstream infile;
    infile.open("users.txt", std::ios::app);
    if (infile.is_open())
    {
        std::string newEntry = _login_packet.Username + ";" + _login_packet.Password + "\n";
        infile << newEntry;
        infile.close();
        return true;
    }
    return false;
}

bool Server::PostToRoom(FPost_Message_Packet _packet)
{
    std::cout << "PostToRoom" << std::endl;
    std::lock_guard<std::mutex>lock(room_file_mutex);

    std::ofstream infile;
    std::string room_file = _packet.Room_Name + ".txt";
    std::cout << room_file << std::endl;
    infile.open(room_file, std::ios::app);
    if (infile.is_open())
    {
        std::cout << "File is open" << std::endl;
        std::string newEntry = _packet.Content + "\n";
        infile << newEntry;
        infile.close();

        std::vector<Connection*>::iterator iter;
        for (int i = 0; i < Connections.size(); i++)
        {
            iter = Connections.begin() + i;
            Connection* con = *iter;
            if (con != nullptr)
            {
                int com = (int)ECommand::Post;
                std::string comstring = std::to_string(com);
                std::string packet = comstring + ";" + _packet.Content;
                con->PushMessage(packet);
            }
        }
        return true;
    }

    return false;
}

void Server::Listen()
{

    std::cout << "listening..\n";
    int status = listen(Socket, 5);

    if (status == -1)
    {
        std::cout << "Error in listen(). Error code: " << WSAGetLastError() << std::endl;
        //exit(-1);
        return;
    }

    int listenerSocket;
    sockaddr_in senderAddr;
    bool isActive = true;
    while (isActive)
    {
        int addrSize = sizeof(senderAddr);
        listenerSocket = accept(Socket, (struct sockaddr*)&senderAddr, &addrSize);

        if (listenerSocket == -1)
        {
            if (ServerShuttingDown)
            {
                std::cout << "'QUIT' Command received - Server is shutting down\n";
                isActive = false;
            }
            else
            {
                std::cout << "Error in accept(). Error code: " << WSAGetLastError() << std::endl;
                std::cout << "Accepting next connection...\n\n";
            }
            continue;
        }

        std::cout << "A connection from " << inet_ntoa(senderAddr.sin_addr) << " has been accepted\n";

        // create a connection class and pass it the socket
        Connection* newConnection = new Connection(this, listenerSocket);

        // lock the newconnections vector untill it is empty and add new connection
        std::unique_lock<std::mutex>lock(Queue_Mutex);
        Queue_cv.wait(lock, [this] { return NewConnectionQueue.empty(); });
        NewConnectionQueue.push_back(newConnection);
        lock.unlock();
        
    }
    
    closesocket(listenerSocket);
}


void Server::MonitorConnections()
{
    bool isActive = true;
    while (isActive)
    {
        // lock newconnections while looping through all new connections and adding them to Connections
        std::lock_guard<std::mutex>lock(Queue_Mutex);
        int length = NewConnectionQueue.size();
        for (int i = 0; i < length; i++)
        {
            std::vector<Connection*>::iterator it_new = NewConnectionQueue.begin() + i;
            if (it_new != NewConnectionQueue.end())
            {
                Connection* newCon = *it_new;
                Connections.push_back(newCon);
            }
        }
        NewConnectionQueue.clear();

        // unlock newconnections vector
        Queue_cv.notify_one();

        // loop through all connections and check their status
        length = Connections.size();
        for (int i = 0; i < length; i++)
        {
            std::vector<Connection*>::iterator it = Connections.begin() + i;
            if (it != Connections.end())
            {
                Connection* con = *it;
                // connection has finished closing so delete the pointer and remove from vector
                if (con->IsConnectionClosed())
                {
                    delete *it;
                    Connections.erase(it);
                    break;
                }
                else if (ServerShuttingDown)
                {
                    // start closing connection
                    con->CloseConnection();
                }
                
            }
            
        }
        if (ServerShuttingDown && Connections.size() == 0)
        {
            isActive = false;
        }
        
    }
}
