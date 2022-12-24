#include "Room.h"

//Room::Room()
//{
//}
//
//Room::Room(std::string _name)
//{
//	name = _name;
//}
//
//void Room::AddActiveUser(Connection& _userConnection)
//{
//	std::vector<Connection&>::iterator it;
//	for (int i = 0; i < activeUsers.size(); i++)
//	{
//		it = activeUsers.begin() + i;
//		Connection& con = *it;
//
//		if (_userConnection.GetUsername() == con.GetUsername())
//		{
//			return;
//		}
//	}
//
//	activeUsers.push_back(_userConnection);
//}
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
//void Room::SendMessageToActiveUsers(std::string _message)
//{
//}
//