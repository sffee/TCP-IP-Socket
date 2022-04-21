#define _WINSOCK_DEPRECATED_NO_WARNINGS

#pragma once

#include <iostream>
#include <WinSock2.h>

#include <vector>
#include <map>

#include <thread>
#include <mutex>
#include <string>

#include <memory>

#pragma comment(lib, "ws2_32")

#define BUFFER_SIZE 1024

class Client
{
private:
	class BufferData
	{
	public:
		BufferData()
			: m_Buffer(new char[BUFFER_SIZE])
			, m_BufferSize(0)
		{
		}

		BufferData(const BufferData& _Other)
		{
			memcpy(m_Buffer, _Other.m_Buffer, BUFFER_SIZE);
			m_BufferSize = _Other.m_BufferSize;
		}

		~BufferData()
		{
			delete[] m_Buffer;
		}

	private:
		char* m_Buffer;
		int m_BufferSize;

	public:
		template<typename T>
		void SetBufferData(const T& _Data)
		{
			int DataSize = sizeof(_Data);
			memcpy(&m_Buffer[m_BufferSize], &DataSize, sizeof(int));
			m_BufferSize += sizeof(int);

			memcpy(&m_Buffer[m_BufferSize], &_Data, sizeof(_Data));
			m_BufferSize += sizeof(_Data);
		}

		template<>
		void SetBufferData(const std::string& _Data)
		{
			int DataSize = (int)_Data.size() + 1;
			memcpy(&m_Buffer[m_BufferSize], &DataSize, sizeof(int));
			m_BufferSize += sizeof(int);

			memcpy(&m_Buffer[m_BufferSize], _Data.c_str(), DataSize);
			m_BufferSize += DataSize;
		}

		template <typename T>
		void GetBufferData(T& _Data)
		{
			int DataSize;
			memcpy(&DataSize, &m_Buffer[m_BufferSize], sizeof(int));
			m_BufferSize += sizeof(int);

			memcpy(&_Data, &m_Buffer[m_BufferSize], DataSize);
			m_BufferSize += DataSize;
		}

		template <>
		void GetBufferData(std::string& _Data)
		{
			int DataSize;
			memcpy(&DataSize, &m_Buffer[m_BufferSize], sizeof(int));
			m_BufferSize += sizeof(int);

			char Buf[BUFFER_SIZE];
			memcpy(&Buf, &m_Buffer[m_BufferSize], DataSize);
			m_BufferSize += DataSize;

			_Data = Buf;
		}

	public:
		int GetSize()
		{
			return m_BufferSize;
		}

	public:
		operator char* ()
		{
			return m_Buffer;
		}

		operator const char* ()
		{
			return m_Buffer;
		}
	};

private:
	enum class ENUM_NAME_COLOR
	{
		RED,
		BLUE,
		YELLOW,
		END
	};

	enum class ENUM_PACKET_TYPE
	{
		CHAT,
		SEND_USERNAME,
		ROOM_ENTER_MY_INFO,
		ROOM_ENTER_OTHER_INFO,
		ROOM_ENTER_ALL_INFO,
		ROOM_EXIT
	};

private:
	struct UserInfo
	{
		int Index;
		std::string Name;
		std::string IPAdress;
		ENUM_NAME_COLOR NameColor;

		bool operator==(const UserInfo& _Info)
		{
			return Name == _Info.Name && IPAdress == _Info.IPAdress && NameColor == _Info.NameColor;
		}
	};

	struct PacketData
	{
		ENUM_PACKET_TYPE Type;
		UserInfo Info;
		char* Message;
	};

public:
	Client(int _Port, std::string _IPAdress);
	~Client();

private:
	SOCKET m_Socket;
	std::string m_ServerIPAddress;
	int m_Port;

private:
	UserInfo m_MyInfo;

	std::map<int, UserInfo> m_AllUserInfo;

private:
	std::shared_ptr<std::thread> m_Thread;
	std::mutex m_Mutex;

public:
	bool Connect();
	void Chat();

private:
	void SendPacket(const std::string& _Str, ENUM_PACKET_TYPE _Type = ENUM_PACKET_TYPE::CHAT);
	void ReciveMessage();

private:
	void PrintName(const UserInfo& _Info);

private:
	void End();
};