#undef UNICODE

#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"

#pragma once
// Need to link with Ws2_32.lib
#pragma comment (lib, "Ws2_32.lib")
// #pragma comment (lib, "Mswsock.lib")

#define DEFAULT_BUFLEN 1024
#define MAX_BUFLEN 4096
#define DEFAULT_PORT "13854" // TGC Server designated port number
#define TEST_FREEZE_TIME 1000 // 1 second

class SocketManager
{
public:
	WSADATA wsaData;
	int iResult;

	SOCKET ListenSocket;
	SOCKET ClientSocket;

	struct addrinfo *result = NULL;
	struct addrinfo hints;

	int iSendResult;
	char recvbuf[DEFAULT_BUFLEN];
	char sendbuf[MAX_BUFLEN];
	int recvbuflen;

	SocketManager();
	~SocketManager();
	int Handle_Connection();

private:
	int Init_Sock();
	int Handle_Response();
};

