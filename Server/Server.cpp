#include "Server.h"
#include <assert.h>
#include <algorithm>

Server::Server(int _Port)
	: m_ClientCount(0)
{
	WSADATA wsaData;
	WSAStartup(MAKEWORD(2, 2), &wsaData);

	m_ListenerSocket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	m_Sockets.push_back(m_ListenerSocket);
	m_Port = _Port;
}

Server::~Server()
{
	End();
}

void Server::Start()
{
	SOCKADDR_IN ListenArr = {};
	ListenArr.sin_family = AF_INET;
	ListenArr.sin_port = htons(m_Port);
	ListenArr.sin_addr.s_addr = htonl(INADDR_ANY);

	bind(m_ListenerSocket, (SOCKADDR*)&ListenArr, sizeof(ListenArr));
	listen(m_ListenerSocket, SOMAXCONN);

	m_Events.emplace_back(WSACreateEvent());
	WSAEventSelect(m_ListenerSocket, m_Events[0], FD_ACCEPT | FD_READ | FD_CLOSE);

	std::cout << ">> 서버 기동 성공" << std::endl;
}

void Server::Listen()
{
	while (true)
	{
		DWORD Result = WSAWaitForMultipleEvents((DWORD)m_Events.size(), &m_Events[0], FALSE, WSA_INFINITE, FALSE);
		if (Result == WSA_WAIT_TIMEOUT)
			continue;

		int EventIdx = Result - WSA_WAIT_EVENT_0;

		WSANETWORKEVENTS NetworkEvents;
		if (WSAEnumNetworkEvents(m_Sockets[EventIdx], m_Events[EventIdx], &NetworkEvents) == SOCKET_ERROR)
			assert(false);

		if (NetworkEvents.lNetworkEvents & FD_ACCEPT)
			AcceptPacket(EventIdx);

		if (NetworkEvents.lNetworkEvents & FD_READ)
			ReadPacket(EventIdx);

		if (NetworkEvents.lNetworkEvents & FD_CLOSE)
			ClosePacket(EventIdx);
	}
}

void Server::SendPacket(const SOCKET& _Socket, const ENUM_PACKET_TYPE _Type, int _ClientIndex, const std::string& _Message)
{
	BufferData Buffer;
	Buffer.SetBufferData(_Type);

	std::cout << _ClientIndex << "번 클라이언트에게 " << PacketName(_Type) << " 패킷 송신" << std::endl;

	switch (_Type)
	{
	case ENUM_PACKET_TYPE::CHAT:
		Buffer.SetBufferData(_ClientIndex);
		Buffer.SetBufferData(_Message);
		break;
	case ENUM_PACKET_TYPE::ROOM_ENTER_MY_INFO:
	{
		const UserInfo& Info = m_AllUserInfo[_ClientIndex];

		Buffer.SetBufferData(Info.Index);
		Buffer.SetBufferData(Info.IPAdress);
		Buffer.SetBufferData(Info.NameColor);
	}
	break;
	case ENUM_PACKET_TYPE::ROOM_ENTER_OTHER_INFO:
	{
		const UserInfo& Info = m_AllUserInfo[_ClientIndex];

		Buffer.SetBufferData(Info.Index);
		Buffer.SetBufferData(Info.Name);
		Buffer.SetBufferData(Info.IPAdress);
		Buffer.SetBufferData(Info.NameColor);
	}
	break;
	case ENUM_PACKET_TYPE::ROOM_ENTER_ALL_INFO:
	{
		Buffer.SetBufferData(m_AllUserInfo.size());

		for (int i = 0; i < m_AllUserInfo.size(); i++)
		{
			const UserInfo& Info = m_AllUserInfo[i];

			Buffer.SetBufferData(i);
			Buffer.SetBufferData(Info.Name);
			Buffer.SetBufferData(Info.IPAdress);
			Buffer.SetBufferData(Info.NameColor);
		}
	}
	break;
	case ENUM_PACKET_TYPE::ROOM_EXIT:
		Buffer.SetBufferData(_ClientIndex);
		break;
	}

	send(_Socket, Buffer, Buffer.GetSize(), 0);
	Sleep(1);
}

void Server::SendPacketAll(const ENUM_PACKET_TYPE _Type, int _ClientIndex, const std::string& _Message, SOCKET* _ExceptSocket)
{
	BufferData Buffer;
	Buffer.SetBufferData(_Type);

	std::cout << "모든 클라이언트에게 " << PacketName(_Type) << " 패킷 송신" << std::endl;

	switch (_Type)
	{
	case ENUM_PACKET_TYPE::CHAT:
		Buffer.SetBufferData(_ClientIndex);
		Buffer.SetBufferData(_Message);
		break;
	case ENUM_PACKET_TYPE::ROOM_ENTER_MY_INFO:
	{
		const UserInfo& Info = m_AllUserInfo[_ClientIndex];

		Buffer.SetBufferData(Info.Index);
		Buffer.SetBufferData(Info.IPAdress);
		Buffer.SetBufferData(Info.NameColor);
	}
		break;
	case ENUM_PACKET_TYPE::ROOM_ENTER_OTHER_INFO:
	{
		const UserInfo& Info = m_AllUserInfo[_ClientIndex];

		Buffer.SetBufferData(Info.Index);
		Buffer.SetBufferData(Info.Name);
		Buffer.SetBufferData(Info.IPAdress);
		Buffer.SetBufferData(Info.NameColor);
	}
		break;
	case ENUM_PACKET_TYPE::ROOM_ENTER_ALL_INFO:
	{
		Buffer.SetBufferData(m_AllUserInfo.size());

		for (int i = 0; i < m_AllUserInfo.size(); i++)
		{
			const UserInfo& Info = m_AllUserInfo[i];

			Buffer.SetBufferData(i);
			Buffer.SetBufferData(Info.Name);
			Buffer.SetBufferData(Info.IPAdress);
			Buffer.SetBufferData(Info.NameColor);
		}
	}
		break;
	case ENUM_PACKET_TYPE::ROOM_EXIT:
		Buffer.SetBufferData(_ClientIndex);
		break;
	}

	for (int i = 1; i < m_Sockets.size(); i++)
	{
		if (_ExceptSocket != nullptr && &m_Sockets[i] == _ExceptSocket)
				continue;

		send(m_Sockets[i], Buffer, Buffer.GetSize(), 0);
	}

	Sleep(1);
}

void Server::End()
{
	for (const auto& Socket : m_Sockets)
		closesocket(Socket);

	for (const auto& Event : m_Events)
		WSACloseEvent(Event);

	WSACleanup();
}

void Server::AcceptPacket(int _EventIndex)
{
	SOCKADDR_IN ClientArr = {};
	int ClientSize = sizeof(ClientArr);

	SOCKET ClientSocket = accept(m_Sockets[_EventIndex], (SOCKADDR*)&ClientArr, &ClientSize);
	if (ClientSocket == -1)
		assert(false);

	WSAEVENT Event = WSACreateEvent();
	WSAEventSelect(ClientSocket, Event, FD_READ | FD_CLOSE);

	m_Sockets.push_back(ClientSocket);
	m_Events.push_back(Event);

	int ClientIndex = (int)m_Sockets.size() - 2;

	UserInfo Info;
	Info.Index = ClientIndex;
	Info.Name = "";
	Info.IPAdress = inet_ntoa(ClientArr.sin_addr);
	Info.NameColor = static_cast<ENUM_NAME_COLOR>(ClientIndex % (int)ENUM_NAME_COLOR::END);
	m_AllUserInfo.push_back(Info);

	printf("클라이언트가 접속하였습니다. (Index : %d / IP주소 : %s)\n", ClientIndex, Info.IPAdress.c_str());

	SendPacket(ClientSocket, ENUM_PACKET_TYPE::ROOM_ENTER_MY_INFO, ClientIndex);

	m_ClientCount++;
}

void Server::ReadPacket(int _EventIndex)
{
	BufferData Buffer;
	int Len = recv(m_Sockets[_EventIndex], Buffer, BUFFER_SIZE, 0);
	if (0 < Len)
	{
		ENUM_PACKET_TYPE Type;
		Buffer.GetBufferData(Type);

		std::cout << _EventIndex - 1 << "번 클라이언트로부터 " << PacketName(Type) << " 패킷 수신" << std::endl;

		switch (Type)
		{
		case Server::ENUM_PACKET_TYPE::CHAT:
		{
			int ClientIndex;
			Buffer.GetBufferData(ClientIndex);

			std::string Message;
			Buffer.GetBufferData(Message);

			SendPacketAll(ENUM_PACKET_TYPE::CHAT, ClientIndex, Message);

			std::cout << "채팅 내용 : " << Message << std::endl;
		}
			break;
		case Server::ENUM_PACKET_TYPE::SEND_USERNAME:
		{
			int ClientIndex;
			Buffer.GetBufferData(ClientIndex);
			Buffer.GetBufferData(m_AllUserInfo[ClientIndex].Name);

			SendPacket(m_Sockets[_EventIndex], ENUM_PACKET_TYPE::ROOM_ENTER_ALL_INFO, ClientIndex);
			SendPacketAll(ENUM_PACKET_TYPE::ROOM_ENTER_OTHER_INFO, ClientIndex);
		}
			break;
		case Server::ENUM_PACKET_TYPE::ROOM_ENTER_MY_INFO:
			break;
		case Server::ENUM_PACKET_TYPE::ROOM_ENTER_OTHER_INFO:
			break;
		case Server::ENUM_PACKET_TYPE::ROOM_EXIT:
			ClosePacket(_EventIndex);
			break;
		default:
			break;
		}
	}
}

void Server::ClosePacket(int _EventIndex)
{
	int ClientIndex = _EventIndex - 1;

	const UserInfo& Info = m_AllUserInfo[ClientIndex];
	printf("클라이언트와 연결이 끊어졌습니다. (Index : %d / IP주소 : %s)\n", Info.Index, Info.IPAdress.c_str());

	SendPacketAll(ENUM_PACKET_TYPE::ROOM_EXIT, ClientIndex);

	m_Sockets.erase(std::find(m_Sockets.begin(), m_Sockets.end(), m_Sockets[_EventIndex]));
	m_Events.erase(std::find(m_Events.begin(), m_Events.end(), m_Events[_EventIndex]));
	
	m_AllUserInfo.erase(std::find(m_AllUserInfo.begin(), m_AllUserInfo.end(), m_AllUserInfo[ClientIndex]));
}

std::string Server::GetColorStr(ENUM_NAME_COLOR _Color)
{
	std::string Str;

	switch (_Color)
	{
	case Server::ENUM_NAME_COLOR::RED:
		Str = "RED";
		break;
	case Server::ENUM_NAME_COLOR::BLUE:
		Str = "BLUE";
		break;
	case Server::ENUM_NAME_COLOR::YELLOW:
		Str = "YELLOW";
		break;
	default:
		break;
	}

	return Str;
}

std::string Server::PacketName(const ENUM_PACKET_TYPE _Type)
{
	std::string Name;

	switch (_Type)
	{
	case ENUM_PACKET_TYPE::CHAT:
		Name = "CHAT";
		break;
	case ENUM_PACKET_TYPE::SEND_USERNAME:
		Name = "SEND_USERNAME";
		break;
	case ENUM_PACKET_TYPE::ROOM_ENTER_MY_INFO:
		Name = "ROOM_ENTER_MY_INFO";
		break;
	case ENUM_PACKET_TYPE::ROOM_ENTER_OTHER_INFO:
		Name = "ROOM_ENTER_OTHER_INFO";
		break;
	case ENUM_PACKET_TYPE::ROOM_ENTER_ALL_INFO:
		Name = "ROOM_ENTER_ALL_INFO";
		break;
	case ENUM_PACKET_TYPE::ROOM_EXIT:
		Name = "ROOM_EXIT";
		break;
	default:
		break;
	}

	return Name;
}