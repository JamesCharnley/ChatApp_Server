// ChatApp.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <stdio.h>
#include "Client.h"
#include "Server.h"

int main()
{
    std::cout << "Enter '1' to create a server or '2' to create client\n";
    int mode = 0;

    std::cin >> mode;

    if (mode == 1)
    {
        USHORT ServerPort = 0;
        std::cout << "Enter port number for server: ";
        std::cin >> ServerPort;
        Server* serv = new Server(ServerPort);
        delete serv;
    }
    else if (mode == 2)
    {
        USHORT LocalPort = 0;
        USHORT ServerPort = 0;
        std::string ServerAddress = "";
        std::cout << "Enter the local port for this client:";
        std::cin >> LocalPort;
        std::cout << "Enter server's address:";
        char add[20];
        std::cin >> add;
        std::string str(add);
        int len = str.length();
        add[len] = '\0';
        std::cout << "Enter server's port:";
        std::cin >> ServerPort;

        
        Client* client = new Client(add, ServerPort, LocalPort);
        delete client;
    }
    
   
    
}

// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file
