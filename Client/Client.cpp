#include "Client.h"
#include <assert.h>

Client::Client(int _Port, std::string _IPAdress)
{
	WSADATA wsaData;
	WSAStartup(MAKEWORD(2, 2), &wsaData);

	m_Socket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	m_Port = _Port;
	m_ServerIPAddress = _IPAdress;
}

Client::~Client()
{
	End();
}

bool Client::Connect()
{
	std::cout << "서버에 접속을 시도합니다." << std::endl;

	SOCKADDR_IN ClientArr = {};
	ClientArr.sin_family = AF_INET;
	ClientArr.sin_port = htons(m_Port);
	ClientArr.sin_addr.s_addr = inet_addr(m_ServerIPAddress.c_str());

	if (connect(m_Socket, (SOCKADDR*)&ClientArr, sizeof(ClientArr)) == -1)
	{
		std::cout << "서버 접속 실패" << std::endl;
		return false;
	}

	std::cout << "접속 성공!" << std::endl;

	std::string Name;
	std::cout << "이름을 설정해주세요 : ";
	std::cin >> Name;

	m_MyInfo.Name = Name;

	return true;
}

void Client::Chat()
{
	m_Thread = std::make_shared<std::thread>(std::thread(&Client::ReciveMessage, this));

	while (true)
	{
		std::string Str;
		std::cin >> Str;

		if (Str == "Exit" || Str == "exit")
		{
			SendPacket("", ENUM_PACKET_TYPE::ROOM_EXIT);
			break;
		}

		SendPacket(Str);
	}

	m_Thread->join();
}

void Client::SendPacket(const std::string& _Str, ENUM_PACKET_TYPE _Type)
{
	switch (_Type)
	{
	case ENUM_PACKET_TYPE::CHAT:
	{
		BufferData Buffer;
		Buffer.SetBufferData(_Type);
		Buffer.SetBufferData(m_MyInfo.Index);
		Buffer.SetBufferData(_Str);

		send(m_Socket, Buffer, Buffer.GetSize(), 0);
	}
		break;
	case ENUM_PACKET_TYPE::SEND_USERNAME:
	{
		BufferData Buffer;
		Buffer.SetBufferData(_Type);
		Buffer.SetBufferData(m_MyInfo.Index);
		Buffer.SetBufferData(_Str);

		send(m_Socket, Buffer, Buffer.GetSize(), 0);
	}
		break;
	case ENUM_PACKET_TYPE::ROOM_EXIT:
	{
		BufferData Buffer;
		Buffer.SetBufferData(_Type);

		send(m_Socket, Buffer, Buffer.GetSize(), 0);
	}
		break;
	default:
		break;
	}
}

void Client::ReciveMessage()
{
	while (true)
	{
		BufferData Buffer;
		int Len = recv(m_Socket, Buffer, BUFFER_SIZE, 0);
		if (Len <= 0)
			continue;

		ENUM_PACKET_TYPE Type;
		Buffer.GetBufferData(Type);

		bool EndCheck = false;

		switch (Type)
		{
		case ENUM_PACKET_TYPE::CHAT:
		{
			int ClientIndex;
			Buffer.GetBufferData(ClientIndex);

			char Buf[BUFFER_SIZE];
			Buffer.GetBufferData(Buf);

			std::cout << "[ ";
			PrintName(m_AllUserInfo[ClientIndex]);
			std::cout << "] " << Buf << std::endl;
		}
			break;
		case ENUM_PACKET_TYPE::ROOM_ENTER_MY_INFO:
			Buffer.GetBufferData(m_MyInfo.Index);
			Buffer.GetBufferData(m_MyInfo.IPAdress);
			Buffer.GetBufferData(m_MyInfo.NameColor);

			SendPacket(m_MyInfo.Name, ENUM_PACKET_TYPE::SEND_USERNAME);
			break;
		case ENUM_PACKET_TYPE::ROOM_ENTER_OTHER_INFO:
		{
			int ClientIndex;
			Buffer.GetBufferData(ClientIndex);

			UserInfo Info;
			Buffer.GetBufferData(Info.Name);
			Buffer.GetBufferData(Info.IPAdress);
			Buffer.GetBufferData(Info.NameColor);

			m_AllUserInfo.insert(std::make_pair(ClientIndex, Info));

			PrintName(Info);
			std::cout << "님이 채팅방에 입장하셨습니다." << std::endl;
		}
			break;
		case ENUM_PACKET_TYPE::ROOM_ENTER_ALL_INFO:
		{
			size_t Count;
			Buffer.GetBufferData(Count);

			for (int i = 0; i < Count; i++)
			{
				UserInfo Info;
				Buffer.GetBufferData(Info.Index);
				Buffer.GetBufferData(Info.Name);
				Buffer.GetBufferData(Info.IPAdress);
				Buffer.GetBufferData(Info.NameColor);

				m_AllUserInfo.insert(std::make_pair(Info.Index, Info));
			}
		}
			break;
		case ENUM_PACKET_TYPE::ROOM_EXIT:
		{
			int ClientIndex;
			Buffer.GetBufferData(ClientIndex);

			const UserInfo& Info = m_AllUserInfo[ClientIndex];
			
			PrintName(Info);
			std::cout << "님이 채팅방에서 퇴장하셨습니다." << std::endl;

			m_AllUserInfo.erase(m_AllUserInfo.find(ClientIndex));

			if (ClientIndex == m_MyInfo.Index)
				EndCheck = true;
		}
			break;
		default:
			break;
		}

		if (EndCheck == true)
			break;
	}
}

void Client::PrintName(const UserInfo& _Info)
{
	int Color = 15;
	switch (_Info.NameColor)
	{
	case Client::ENUM_NAME_COLOR::RED:
		Color = 12;
		break;
	case Client::ENUM_NAME_COLOR::BLUE:
		Color = 9;
		break;
	case Client::ENUM_NAME_COLOR::YELLOW:
		Color = 14;
		break;
	default:
		break;
	}

	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), Color);

	std::cout << _Info.Name << " ";

	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 15);
}

void Client::End()
{
	closesocket(m_Socket);

	WSACleanup();
}