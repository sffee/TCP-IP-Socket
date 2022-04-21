#include <iostream>
#include "Server.h"

#pragma comment(lib, "ws2_32")

#define PORT 4578

int main()
{
	Server S(PORT);
	S.Start();
	S.Listen();

	//WSADATA wsaData;
	//WSAStartup(MAKEWORD(2, 2), &wsaData);

	//SOCKET hListen;
	//hListen = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);

	//SOCKADDR_IN tListenAddr = {};
	//tListenAddr.sin_family = AF_INET;
	//tListenAddr.sin_port = htons(PORT);
	//tListenAddr.sin_addr.s_addr = htonl(INADDR_ANY);

	//bind(hListen, (SOCKADDR*)&tListenAddr, sizeof(tListenAddr));
	//listen(hListen, SOMAXCONN);

	//SOCKADDR_IN tCIntAddr = {};
	//int iCIntSize = sizeof(tCIntAddr);
	//SOCKET hClient = accept(hListen, (SOCKADDR*)&tCIntAddr, &iCIntSize);

	//char cBuffer[1024] = {};
	//recv(hClient, cBuffer, 1024, 0);
	//printf("Recv Msg : %s\n", cBuffer);

	//char cMsg[] = "Server Send";
	//send(hClient, cMsg, strlen(cMsg), 0);

	//closesocket(hClient);
	//closesocket(hListen);

	//WSACleanup();
}