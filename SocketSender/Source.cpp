#define _CRT_SECURE_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#pragma comment(lib, "Ws2_32.lib")
using namespace std;

SOCKADDR_IN initSocketAddress(const char* ip, ADDRESS_FAMILY family, u_short port)
{
	SOCKADDR_IN sin;
	sin.sin_addr.s_addr = inet_addr(ip);
	sin.sin_family = family;
	sin.sin_port = htons(port);
	return sin;
}

void doWsaStartup() {
	WSADATA wsaData;
	int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0)
		throw exception("WSAStartup failed: " + iResult);
}

void initSocket(SOCKET& soc) {
	soc = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (soc == INVALID_SOCKET) {
		int error = WSAGetLastError();
		WSACleanup();
		throw exception("Error at socket(): " + error);
	}
}

void sendMessage(SOCKET& soc, string message) {
	int iResult = send(soc, (char*)message.c_str(), strlen((char*)message.c_str()), 0);
	if (iResult == SOCKET_ERROR) {
		printf("send failed: %d\n", WSAGetLastError());
		iResult = shutdown(soc, SD_SEND);
		if (iResult == SOCKET_ERROR)
			printf("shutdown failed: %d\n", WSAGetLastError());
	}
}

// args : <message> <port> [ip]
// default ip is localhost
int main(int argc, char* argv[]) {
	if (argc < 3) {
		printf("Too few arguments\n");
		return -1;
	} else if (argc > 4) {
		printf("Too many arguments\n");
		return -1;
	}
	 
	const int BUFF_SIZE = 1024;
	const int RECV_TIMEOUT = 2;
	string message = argv[1];
	auto socAddress = initSocketAddress(argc == 4 ? argv[3] : "127.0.0.1", AF_INET, stoi(argv[2]));
	SOCKET senderSocket = INVALID_SOCKET;

	doWsaStartup();
	initSocket(senderSocket);

	// --- Connection
	int iResult = connect(senderSocket, (SOCKADDR*)&socAddress, sizeof(socAddress));
	if (iResult == SOCKET_ERROR) {
		closesocket(senderSocket);
		senderSocket = INVALID_SOCKET;
		printf("Unable to connect\n");
		WSACleanup();
		return -1;
	}
	// ----------------

	printf("Sending message...\n");
	sendMessage(senderSocket, message);

	shutdown(senderSocket, SD_SEND);

	try {
		printf("Receiving...\n");
		char buffer[BUFF_SIZE];
		int iResult = 1;
		while (iResult > 0) {
			fd_set set;
			struct timeval timeout;
			FD_ZERO(&set); /* clear the set */
			FD_SET(senderSocket, &set); /* add our file descriptor to the set */
			timeout.tv_sec = RECV_TIMEOUT;
			timeout.tv_usec = 0;
			int rv = select(senderSocket + 1, &set, NULL, NULL, &timeout);
			if (rv == SOCKET_ERROR || rv == 0) {
				printf("Timeout expired\n");
				throw exception();
			}
			iResult = recv(senderSocket, buffer, BUFF_SIZE, 0);
		}
		if (iResult == 0)
			printf("Remote connection closed\n");
		else
			printf(iResult + " Weird\n");
	} catch (exception) {
	}
	closesocket(senderSocket);
	WSACleanup();
	printf("Connection closed\n");
}