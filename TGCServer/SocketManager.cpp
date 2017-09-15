#include "stdafx.h"
#include "SocketManager.h"

using namespace rapidjson;

SocketManager::SocketManager()
{
	ListenSocket = INVALID_SOCKET;
	ClientSocket = INVALID_SOCKET;
	recvbuflen = DEFAULT_BUFLEN;

	iResult = Init_Sock();

	if (iResult < 0) {
		printf("Socket Initialization fails.\n");
	}
	else
	{
		printf("Socket Successfully initialized.\n");
	}
}


SocketManager::~SocketManager()
{
}

int SocketManager::Init_Sock()
{
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0) {
		printf("WSAStartup failed with error: %d\n", iResult);
		return -1;
	}

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;

	// Resolve the server address and port
	iResult = getaddrinfo(NULL, DEFAULT_PORT, &hints, &result);
	if (iResult != 0) {
		printf("getaddrinfo failed with error: %d\n", iResult);
		WSACleanup();
		return -1;
	}

	// Create a SOCKET for connecting to server
	ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
	if (ListenSocket == INVALID_SOCKET) {
		printf("socket failed with error: %ld\n", WSAGetLastError());
		freeaddrinfo(result);
		WSACleanup();
		return -1;
	}

	// Setup the TCP listening socket
	iResult = bind(ListenSocket, result->ai_addr, (int)result->ai_addrlen);
	if (iResult == SOCKET_ERROR) {
		printf("bind failed with error: %d\n", WSAGetLastError());
		freeaddrinfo(result);
		closesocket(ListenSocket);
		WSACleanup();
		return -1;
	}
	freeaddrinfo(result);

	iResult = listen(ListenSocket, SOMAXCONN);
	if (iResult == SOCKET_ERROR) {
		printf("listen failed with error: %d\n", WSAGetLastError());
		closesocket(ListenSocket);
		WSACleanup();
		return -1;
	}

	return 0;
}

int SocketManager::Handle_Response()
{
	Document d;
	
	//parse received buffer
	d.Parse(recvbuf);
	
	// Deprecated beginning
	if (d.HasMember("enableRawOutput")) {
	}

	char* tmp_buf = "{\"poorSignalLevel\":0,\"eSense\":{\"attention\":38,\"meditation\":43},\"eegPower\":{\"delta\":1.15e-4,\"theta\":1.41e-6,\"lowAlpha\":1.35e-4,\"highAlpha\":6.69e-5,\"lowBeta\":1.47e-5,\"highBeta\":6.95e-7,\"lowGamma\":5.26e-7,\"highGamma\":1.40e-5}}\
	{\"rawEeg\":238}\
	{\"rawEeg\":282}\
	{\"blinkStrength\":100}\
	{\"mentalEffort\":2.17604843163533}\
	{\"familiarity\":391.789551397294}\
	{\"rawEeg\":239}\
	{\"rawEegMulti\":{\"ch1\":392, \"ch2\" : 352, \"ch3\" : 492, \"ch4\" : 592, \"ch5\" : 692, \"ch6\" : 442, \"ch7\" : 122, \"ch8\" : 552}}";
	
	strcpy_s(sendbuf, tmp_buf);

	iSendResult = send(ClientSocket, sendbuf, sizeof(char)*MAX_BUFLEN, 0);
	if (iSendResult == SOCKET_ERROR) {
		printf("send failed with error: %d\n", WSAGetLastError());
		closesocket(ClientSocket);
		WSACleanup();
		return -1;
	}

	printf("Bytes sent: %d\n", iSendResult);
	return 0;
}

int SocketManager::Handle_Connection()
{
	// Accept a client socket
	ClientSocket = accept(ListenSocket, NULL, NULL);
	if (ClientSocket == INVALID_SOCKET) {
		printf("accept failed with error: %d\n", WSAGetLastError());
		closesocket(ListenSocket);
		WSACleanup();
		return -1;
	}

	// No longer need server socket
	closesocket(ListenSocket);

	// Receive until the peer shuts down the connection
	do {

		iResult = recv(ClientSocket, recvbuf, recvbuflen, 0);
		if (iResult > 0) {
			printf("Bytes received: %d\n", iResult);
			printf("Received texts:\n\%s", recvbuf);

			//handle response
			while (TRUE) {
				// send fails
				if (Handle_Response() < 0) {
					break;
				}
				Sleep(TEST_FREEZE_TIME);
			}
	
		}
		else if (iResult == 0)
			printf("Connection closing...\n");
		else {
			printf("recv failed with error: %d\n", WSAGetLastError());
			closesocket(ClientSocket);
			WSACleanup();
			return -1;
		}

	} while (iResult > 0);

	// cleanup
	closesocket(ClientSocket);
	WSACleanup();

	return 0;
}

