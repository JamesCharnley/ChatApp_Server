#include "Room.h"
#include "User.h"
#include <fstream>
#include <direct.h>
//#include <string.h>
#include <cstdint>
#include <filesystem>
#include "Server.h"
Room::Room()
{
	
}

Room::Room(int _id, Server* _server_class)
{
	std::cout << "Room()" << std::endl;
	id = _id;
	server_class = _server_class;

	std::string room_path = "roomprofiles/" + std::to_string(id);
	
	// get room's name from file
	if (std::filesystem::exists(room_path + "/name.txt"))
	{
		std::ifstream room_name_file(room_path + "/name.txt");
		if (room_name_file.is_open())
		{
			std::string line;
			if (getline(room_name_file, line))
			{
				name = line;
				std::cout << "name: " << GetName() << std::endl;
			}
			room_name_file.close();
			
		}
	}

	// get room's messages from file
	if (std::filesystem::exists(room_path + "/messages.txt"))
	{
		std::ifstream room_messages_file(room_path + "/messages.txt");
		if (room_messages_file.is_open())
		{
			std::string line;
			while (getline(room_messages_file, line))
			{
				messages.push_back(line);
			}
			room_messages_file.close();

		}
	}

	
	isInitialized = true;
}

std::string Room::GetName()
{
	return name;
}

void Room::AddActiveUser(User _user)
{
	std::lock_guard<std::mutex>lock(active_users_mutex);
	std::vector<User>::iterator it;
	for (int i = 0; i < activeUsers.size(); i++)
	{
		it = activeUsers.begin() + i;
		User con = *it;

		if (_user.GetUsername() == con.GetUsername())
		{
			return;
		}
	}

	activeUsers.push_back(_user);

	SendAllMessagesToUser(_user);

}
void Room::remove_active_user(std::string _username)
{
	std::lock_guard<std::mutex>lock(active_users_mutex);
	for (std::vector<User>::iterator it = activeUsers.begin(); it < activeUsers.end(); it++)
	{
		User user = *it;
		if (user.GetUsername() == _username)
		{
			activeUsers.erase(it);
			std::cout << "Terminated active user: " << _username << std::endl;
			break;
		}
	}

	if (activeUsers.size() == 0)
	{
		// de actiavte room
		server_class->deactivate_Room_By_id(id);
	}
}
void Room::PrintTest()
{
	std::cout << "Room test " << name << std::endl;
}
void Room::Add_New_Message(std::string _message)
{
	messages.push_back(_message);

	SendMessageToAllUsers(_message);
}
//
//void Room::RemoveActiveUser(Connection& _userConnection)
//{
//	std::vector<Connection&>::iterator it;
//	for (int i = 0; i < activeUsers.size(); i++)
//	{
//		it = activeUsers.begin() + i;
//		Connection& con = *it;
//
//		if (_userConnection.GetUsername() == con.GetUsername())
//		{
//			activeUsers.erase(it);
//			return;
//		}
//	}
//}
//
//void Room::NewMessage(std::string _message)
//{
//}
//
void Room::SendMessageToAllUsers(std::string _message)
{
	for (std::vector<User>::iterator it = activeUsers.begin(); it < activeUsers.end(); it++)
	{
		User user = *it;

		FPost_Message_Packet packet = { ECommand::Post, ESub_Command::Message, id, _message };

		std::string packet_string = PacketDecoder::Post_Message_Packet_To_String(packet);

		user.GetConnection()->PushMessage(packet_string);
	}
}

void Room::SendMessageToUser(User _user, std::string _message)
{
}

void Room::SendAllMessagesToUser(User _user)
{
	Connection* con = _user.GetConnection();
	if (con)
	{
		for (std::vector<std::string>::iterator it = messages.begin(); it < messages.end(); it++)
		{
			FPost_Message_Packet packet = { ECommand::Post, ESub_Command::Message, id, *it };
			std::string packet_string = PacketDecoder::Post_Message_Packet_To_String(packet);
			con->PushMessage(packet_string);
		}
	}
}
