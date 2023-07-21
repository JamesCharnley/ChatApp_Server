#include "User.h"
#include "Server.h"

User::User()
{
}

User::User(std::string _username, Connection* _connection, Server* _server)
{
    
    rooms = std::vector<Room*>();

	server = _server;
	connection = _connection;
    connection->set_username(_username);
	username = _username;

    std::string user_profile_path = "userprofiles/" + username + "/rooms.txt";
    if (std::filesystem::exists(user_profile_path))
    {
        std::string line;
        std::ifstream rooms_file(user_profile_path);
        if (rooms_file.is_open())
        {
            while (getline(rooms_file, line))
            {
                if (line.length() > 0)
                {
                    int room_id = std::stoi(line);
                    if (!server->is_room_active(room_id))
                    {
                        server->Activate_Room_By_id(room_id);
                    }
                    Room* room_ptr = server->Get_Room_By_ID(room_id);
                    if (room_ptr != nullptr)
                    {
                        room_ptr->add_active_user(*this);
                        rooms.push_back(room_ptr);
                    }
                    else
                    {
                        std::cout << "room ptr is nullptr" << std::endl;
                    }

                }
            }
            rooms_file.close();
        }
    }

    update_client();
}

void User::terminate_user()
{
    for (std::vector<Room*>::iterator it = rooms.begin(); it < rooms.end(); it++)
    {
        Room* room = *it;
        room->remove_active_user(username);
    }
}

void User::update_client()
{
    std::cout << "update_client" << std::endl;
    std::cout << "room count = " << rooms.size() << std::endl;
    for (std::vector<Room*>::iterator it = rooms.begin(); it != rooms.end(); it++)
    {
        std::cout << "room iteration" << std::endl;
        Room* room = *it;

        std::string name = room->get_room_name();

        FPost_Room_Packet post_room_packet = { ECommand::Post, ESub_Command::Room, room->get_room_id(), room->get_room_name() };

        std::string post_room_packet_string = PacketDecoder::Post_Room_Packet_To_String(post_room_packet);

        connection->PushMessage(post_room_packet_string);
    }
}
