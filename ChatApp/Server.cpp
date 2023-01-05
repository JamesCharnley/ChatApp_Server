#include "Server.h"
#include "User.h"


Server::Server(USHORT _port)
{
    // check if requires directories exist... Create them if they don't
    if (!std::filesystem::exists("userprofiles"))
    {
        if (!std::filesystem::create_directory("userprofiles"))
        {
            std::cout << "failed to create folder: userprofiles";
        }
        else
        {
            std::cout << "Folder created: userprofiles" << std::endl;
            std::filesystem::permissions("userprofiles", std::filesystem::perms::others_all, std::filesystem::perm_options::remove);
        }
    }
    if (!std::filesystem::exists("roomprofiles"))
    {
        if (!std::filesystem::create_directory("roomprofiles"))
        {
            std::cout << "failed to create folder: roomprofiles";
        }
        else
        {
            std::cout << "Folder created: roomprofiles" << std::endl;
            std::filesystem::permissions("roomprofiles", std::filesystem::perms::others_all, std::filesystem::perm_options::remove);
        }
    }
    if (!std::filesystem::exists("saveddata"))
    {
        if (!std::filesystem::create_directory("saveddata"))
        {
            std::cout << "failed to create folder: saveddata";
        }
        else
        {
            std::cout << "Folder created: saveddata" << std::endl;
            std::filesystem::permissions("saveddata", std::filesystem::perms::others_all, std::filesystem::perm_options::remove);
        }
    }

    // get current room id counter from file
    std::lock_guard<std::mutex>lock(room_id_mutex);
    std::string line;
    std::ifstream myfile("saveddata/roomidcount.txt");
    if (myfile.is_open())
    {
        getline(myfile, line);
        room_id_count = std::stoi(line);

        std::cout << "Room_ID_Count: " << room_id_count << std::endl;
    }

    // start server
    InitWSA();
    CreateSocket(_port);
    BindSocket();

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

bool Server::Login(FLogin_Packet _login_packet, Connection* _connection)
{
    std::lock_guard<std::mutex>lock(users_file_mutex);
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
                    Initialize_User(_connection, _login_packet);
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

bool Server::Signup(FLogin_Packet _login_packet, Connection* _connection)
{
    if (UsernameExists(_login_packet.Username))
    {
        return false;
    }

    AddNewUser(_login_packet);
    Initialize_User(_connection, _login_packet);
    return true;
}

bool Server::PostToRoom(FPost_Message_Packet _packet)
{
   std::cout << "PostToRoom" << std::endl;
   std::lock_guard<std::mutex>lock(room_file_mutex);
   
   std::string room_path = "roomprofiles/" + std::to_string(_packet.Room_ID);
   if (std::filesystem::exists(room_path))
   {
       std::string room_messages_file_path = room_path + "/messages.txt";

       std::ofstream infile;
       infile.open(room_messages_file_path, std::ios::app);
       if (infile.is_open())
       {
           std::cout << "File is open" << std::endl;
           std::string newEntry = _packet.Content + "\n";
           infile << newEntry;
           infile.close();

           Room* room = Get_Room_By_ID(_packet.Room_ID);
           if (room != nullptr)
           {
               room->add_new_message(_packet.Content);
           }
           return true;
       }
   }
   

    return false;
}

void Server::Activate_Room_By_id(int _room_id)
{
    Room* room = new Room(_room_id, this);
    active_rooms.push_back(room);
}

void Server::deactivate_Room_By_id(int _room_id)
{
    for (std::vector<Room*>::iterator it = active_rooms.begin(); it < active_rooms.end(); it++)
    {
        Room* room = *it;
        if (room->get_room_id() == _room_id)
        {
            delete room;
            active_rooms.erase(it);
            std::cout << "De-activated room: " << _room_id << std::endl;
            break;
        }
    }
}

Room* Server::Get_Room_By_ID(int _id)
{
    std::vector<Room*>::iterator it;
    for (int i = 0; i < active_rooms.size(); i++)
    {
        it = active_rooms.begin() + i;
        Room* room = *it;
        if (room->get_room_id() == _id)
        {
            return room;
        }
    }
    return nullptr;
}

void Server::Listen()
{

    std::cout << "listening..\n";
    int status = listen(Socket, 5);

    if (status == -1)
    {
        std::cout << "Error in listen(). Error code: " << WSAGetLastError() << std::endl;
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
       //for (std::vector<Room>::iterator it = active_rooms.begin(); it != active_rooms.end(); it++)
       //{
       //    Room room = *it;
       //    room.PrintTest();
       //}
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

bool Server::UsernameExists(std::string _username)
{
    std::lock_guard<std::mutex>lock(users_file_mutex);
    std::string line;
    std::ifstream myfile("users.txt");
    if (myfile.is_open())
    {
        while (getline(myfile, line))
        {
            std::cout << line << '\n';

            // create a command packet with the file line as content
            FCommand_Packet cp = { ECommand::Login, line.c_str() };
            // convert the command packet to a login packet so the username from line can be compared
            FLogin_Packet entry = PacketDecoder::Command_Packet_To_Login_Packet(cp);

            if (_username == entry.Username)
            {
                std::cout << "Sign up failed: Username already exists" << std::endl;
                myfile.close();
                return true;
            }

        }

        myfile.close();
    }
    return false;
}

void Server::AddNewUser(FLogin_Packet _packet)
{
    std::ofstream infile;
    infile.open("users.txt", std::ios::app);
    if (infile.is_open())
    {
        std::string newEntry = _packet.Username + ";" + _packet.Password + "\n";
        infile << newEntry;
        infile.close();
    }

    CreateUserProfile(_packet);
}

void Server::CreateUserProfile(FLogin_Packet _packet)
{
    std::string user_profile_path = "userprofiles/" + _packet.Username;
    if (!std::filesystem::exists(user_profile_path))
    {
        std::filesystem::create_directory(user_profile_path);
        std::string room_file_path = user_profile_path + "/rooms.txt";
        std::ofstream rooms_file(room_file_path);
        if (rooms_file.is_open())
        {
            rooms_file << "1\n";
            rooms_file.close();
        }

    }
    if (std::filesystem::exists("roomprofiles/1/members.txt"))
    {
        std::ofstream member_file("roomprofiles/1/members.txt");
        if (member_file.is_open())
        {
            member_file << _packet.Username + "\n";
            member_file.close();
        }
    }
}

void Server::Initialize_User(Connection* _connection, FLogin_Packet _login_Packet)
{
    
    User new_user = User(_login_Packet.Username, _connection, this);
    active_users.push_back(new_user);

    
}

bool Server::is_room_active(int _room_id)
{
    std::vector<Room*>::iterator it;
    for (int i = 0; i < active_rooms.size(); i++)
    {
        it = active_rooms.begin() + i;
        Room* room = *it;
        if (room->get_room_id() == _room_id)
        {
            return true;
        }
    }
    return false;
}

void Server::terminate_user(std::string _user)
{
    std::lock_guard<std::mutex>lock(active_user_mutex);

    for (std::vector<User>::iterator it = active_users.begin(); it < active_users.end(); it++)
    {
        User user = *it;
        if (user.GetUsername() == _user)
        {
            user.terminate_user();
            active_users.erase(it);
            break;
        }
    }
}
