#include<iostream>
#include <SFML/Network.hpp>
#include <SFML/Graphics.hpp>
#include <thread>
#include <chrono>

#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <stdio.h>

#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "ws2_32.lib")

#define MALLOC(x) HeapAlloc(GetProcessHeap(), 0, (x))
#define FREE(x) HeapFree(GetProcessHeap(), 0, (x))

std::string get_local_ipv4()
{
    int i;

    PMIB_IPADDRTABLE pIPAddrTable;
    DWORD dwSize = 0;
    DWORD dwRetVal = 0;
    IN_ADDR IPAddr;
    LPVOID lpMsgBuf;

    pIPAddrTable = (MIB_IPADDRTABLE*)MALLOC(sizeof(MIB_IPADDRTABLE));

    if (pIPAddrTable) {
        if (GetIpAddrTable(pIPAddrTable, &dwSize, 0) == ERROR_INSUFFICIENT_BUFFER) {
            FREE(pIPAddrTable);
            pIPAddrTable = (MIB_IPADDRTABLE*)MALLOC(dwSize);
        }
        if (pIPAddrTable == NULL) {
            exit(1);
        }
    }
    if ((dwRetVal = GetIpAddrTable(pIPAddrTable, &dwSize, 0)) != NO_ERROR) 
	{
        exit(1);
    }

    IPAddr.S_un.S_addr = (u_long)pIPAddrTable->table[(int)pIPAddrTable->dwNumEntries - 1].dwAddr;
	std::string ip(inet_ntoa(IPAddr));

    if (pIPAddrTable) {
        FREE(pIPAddrTable);
        pIPAddrTable = NULL;
    }

	return ip;
}

void print_player(const char* msg, sf::TcpSocket* player)
{
	std::cout << msg << (*player).getRemoteAddress().toString() << ":" << (*player).getRemotePort() << std::endl;
}

void remove_from_players(sf::TcpSocket* player, std::vector<sf::TcpSocket*>* players)
{
	for (size_t i = 0; i < players->size(); i++)
	{
		if ((*player).getRemoteAddress() == (*(*players)[i]).getRemoteAddress() && (*player).getRemotePort() == (*(*players)[i]).getRemotePort())
		{
			players->erase(players->begin() + i);
			break;
		}
	}
}

void player_thread(sf::TcpSocket* player, std::vector<sf::TcpSocket*>* players)
{
	print_player("[new player] ", player);
	
	while (true)
	{
		sf::Packet packet;
		if (player->receive(packet) == sf::Socket::Done)
		{
			for (size_t i = 0; i < players->size(); i++)
			{
				if (!((*player).getRemoteAddress() == (*(*players)[i]).getRemoteAddress() && (*player).getRemotePort() == (*(*players)[i]).getRemotePort()))
				{
					if ((*players)[i]->send(packet) != sf::Socket::Done)
					{
						remove_from_players((*players)[i], players);
						i--;
					}
				}
			}
		}
		else
		{
			remove_from_players(player, players);
			break;
		}
	}

	print_player("[player leave] ", player);
	delete player;
}

void players_socket_thread(sf::TcpListener& server)
{
	std::vector<sf::TcpSocket*> players = {};
	while (true)
	{
		sf::TcpSocket* player = new sf::TcpSocket();
		if (server.accept((*player)) == sf::Socket::Done)
		{
			players.push_back(player);
			std::thread([&]() {
				player_thread(players[players.size()-1], &players);
				}).detach();
		}
	}
}

int main(int argc, const char* argv[])
{
	std::string ip = get_local_ipv4();
	sf::TcpListener server;
	if (server.listen(53000) != sf::Socket::Done)
	{
		return 1;
	}
	std::cout << "[READY] " << ip <<":"<< server.getLocalPort() << "" << std::endl;
	
	players_socket_thread(server);

	return 0;
}