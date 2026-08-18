#include "net_client.h"
#include <cstdio>
#include <vector>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#include <vector>

#pragma comment(lib, "ws2_32.lib")

static SOCKET g_sock = INVALID_SOCKET;
static bool g_wsa = false;
static std::string g_recvBuf;
static std::vector<std::string> g_lines;
static std::string g_lastError = "";

void NetClient_Startup(void)
{
	if (!g_wsa)
	{
		WSADATA data = {};
		if (WSAStartup(MAKEWORD(2, 2), &data) == 0)
		{
			g_wsa = true;
		}
	}
}

void NetClient_Shutdown(void)
{
	NetClient_Close();
	if (g_wsa)
	{
		WSACleanup();
		g_wsa = false;
	}
}

void NetClient_Close(void)
{
	if (g_sock != INVALID_SOCKET)
	{
		closesocket(g_sock);
		g_sock = INVALID_SOCKET;
	}
	g_recvBuf.clear();
}

bool NetClient_IsConnected(void)
{
	return g_sock != INVALID_SOCKET;
}

const char* NetClient_LastError(void)
{
	return g_lastError.c_str();
}

bool NetClient_Connect(const char* host, int port)
{
	NetClient_Startup();
	NetClient_Close();
	g_lastError.clear();
	g_lines.clear();

	addrinfo hints = {};
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	char portText[16] = {};
	std::snprintf(portText, sizeof(portText), "%d", port);
	addrinfo* result = nullptr;
	if (getaddrinfo(host, portText, &hints, &result) != 0)
	{
		g_lastError = "getaddrinfo failed";
		return false;
	}

	SOCKET sock = INVALID_SOCKET;
	for (addrinfo* p = result; p != nullptr; p = p->ai_next)
	{
		sock = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
		if (sock == INVALID_SOCKET)
		{
			continue;
		}
		u_long nonBlock = 1;
		ioctlsocket(sock, FIONBIO, &nonBlock);
		const int cr = connect(sock, p->ai_addr, static_cast<int>(p->ai_addrlen));
		if (cr == 0)
		{
			break;
		}
		if (WSAGetLastError() == WSAEWOULDBLOCK)
		{
			fd_set writeSet;
			FD_ZERO(&writeSet);
			FD_SET(sock, &writeSet);
			timeval tv;
			tv.tv_sec = 3;
			tv.tv_usec = 0;
			const int sel = select(0, nullptr, &writeSet, nullptr, &tv);
			if (sel > 0)
			{
				int soErr = 0;
				int soLen = sizeof(soErr);
				getsockopt(sock, SOL_SOCKET, SO_ERROR, reinterpret_cast<char*>(&soErr), &soLen);
				if (soErr == 0)
				{
					break;
				}
			}
		}
		closesocket(sock);
		sock = INVALID_SOCKET;
	}
	freeaddrinfo(result);
	if (sock == INVALID_SOCKET)
	{
		g_lastError = "connect failed";
		return false;
	}
	g_sock = sock;
	return true;
}

void NetClient_Send(const char* line)
{
	if (g_sock == INVALID_SOCKET || !line)
	{
		return;
	}
	std::string packed = line;
	packed += "\n";
	send(g_sock, packed.c_str(), static_cast<int>(packed.size()), 0);
}

void NetClient_Poll(void)
{
	if (g_sock == INVALID_SOCKET)
	{
		return;
	}
	char chunk[512];
	while (true)
	{
		const int n = recv(g_sock, chunk, sizeof(chunk), 0);
		if (n > 0)
		{
			g_recvBuf.append(chunk, chunk + n);
			continue;
		}
		if (n == 0)
		{
			NetClient_Close();
			g_lastError = "disconnected";
			break;
		}
		const int err = WSAGetLastError();
		if (err == WSAEWOULDBLOCK)
		{
			break;
		}
		NetClient_Close();
		g_lastError = "recv failed";
		break;
	}

	std::string::size_type pos = 0;
	while (true)
	{
		const std::string::size_type nl = g_recvBuf.find('\n', pos);
		if (nl == std::string::npos)
		{
			if (pos > 0)
			{
				g_recvBuf.erase(0, pos);
			}
			break;
		}
		std::string line = g_recvBuf.substr(pos, nl - pos);
		if (!line.empty() && line.back() == '\r')
		{
			line.pop_back();
		}
		g_lines.push_back(line);
		pos = nl + 1;
	}
}

bool NetClient_PopLine(std::string& outLine)
{
	if (g_lines.empty())
	{
		return false;
	}
	outLine = g_lines.front();
	g_lines.erase(g_lines.begin());
	return true;
}
