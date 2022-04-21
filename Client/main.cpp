#include <iostream>
#include "Client.h"

#define PORT 4578
#define SERVER_IP "192.168.0.2"

int main()
{
	Client C(PORT, SERVER_IP);
	if (C.Connect())
		C.Chat();

	system("pause");
}