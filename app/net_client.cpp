#include "net_client.h"
#include <cstdio>
#include <atomic>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>

#pragma comment(lib, "ws2_32.lib")

static SOCKET g_sock = INVALID_SOCKET;
static bool g_wsa = false;
static std::string g_recvBuf;
static std::vector<std::string> g_lines;
static std::string g_lastError = "";

static std::mutex g_connectMutex;
static std::thread g_connectThread;
static std::atomic<bool> g_connecting{ false };
static std::atomic<bool> g_connectFinished{ false };
static std::atomic<bool> g_connectCancel{ false };
static bool g_connectOk = false;
static SOCKET g_connectSock = INVALID_SOCKET;
static std::string g_connectError;

static void NetClient_JoinConnectThread(void)
{
	if (g_connectThread.joinable())
	{
		g_connectThread.join();
	}
}

static void NetClient_ConnectWorker(std::string host, int port)
{
	char portText[16] = {};
	std::snprintf(portText, sizeof(portText), "%d", port);

	addrinfo hints = {};
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	addrinfo* result = nullptr;
	const int gai = getaddrinfo(host.c_str(), portText, &hints, &result);
	if (gai != 0 || result == nullptr)
	{
		std::lock_guard<std::mutex> lock(g_connectMutex);
		g_connectOk = false;
		g_connectSock = INVALID_SOCKET;
		g_connectError = "getaddrinfo failed";
		g_connectFinished = true;
		g_connecting = false;
		return;
	}

	SOCKET sock = INVALID_SOCKET;
	for (addrinfo* p = result; p != nullptr; p = p->ai_next)
	{
		if (g_connectCancel.load())
		{
			break;
		}
		sock = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
		if (sock == INVALID_SOCKET)
		{
			continue;
		}
		const int cr = connect(sock, p->ai_addr, static_cast<int>(p->ai_addrlen));
		if (cr == 0)
		{
			break;
		}
		closesocket(sock);
		sock = INVALID_SOCKET;
	}
	freeaddrinfo(result);

	if (g_connectCancel.load())
	{
		if (sock != INVALID_SOCKET)
		{
			closesocket(sock);
			sock = INVALID_SOCKET;
		}
		std::lock_guard<std::mutex> lock(g_connectMutex);
		g_connectOk = false;
		g_connectSock = INVALID_SOCKET;
		g_connectError = "cancelled";
		g_connectFinished = true;
		g_connecting = false;
		return;
	}

	if (sock != INVALID_SOCKET)
	{
		u_long nonBlock = 1;
		ioctlsocket(sock, FIONBIO, &nonBlock);
	}

	std::lock_guard<std::mutex> lock(g_connectMutex);
	g_connectOk = (sock != INVALID_SOCKET);
	g_connectSock = sock;
	g_connectError = g_connectOk ? "" : "connect failed";
	g_connectFinished = true;
	g_connecting = false;
}

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
	g_connectCancel = true;
	NetClient_JoinConnectThread();
	NetClient_Close();
	if (g_wsa)
	{
		WSACleanup();
		g_wsa = false;
	}
}

void NetClient_CancelConnect(void)
{
	g_connectCancel = true;
}

void NetClient_Close(void)
{
	if (g_connecting.load())
	{
		g_connectCancel = true;
		return;
	}
	NetClient_JoinConnectThread();
	if (g_connectSock != INVALID_SOCKET && g_connectSock != g_sock)
	{
		closesocket(g_connectSock);
		g_connectSock = INVALID_SOCKET;
	}
	if (g_sock != INVALID_SOCKET)
	{
		closesocket(g_sock);
		g_sock = INVALID_SOCKET;
	}
	g_recvBuf.clear();
	g_lines.clear();
	g_connectFinished = false;
	g_connectCancel = false;
}

bool NetClient_IsConnected(void)
{
	return g_sock != INVALID_SOCKET;
}

bool NetClient_IsConnecting(void)
{
	return g_connecting.load();
}

const char* NetClient_LastError(void)
{
	return g_lastError.c_str();
}

void NetClient_BeginConnect(const char* host, int port)
{
	NetClient_Startup();
	if (g_connecting.load())
	{
		return;
	}
	NetClient_JoinConnectThread();
	if (g_sock != INVALID_SOCKET)
	{
		closesocket(g_sock);
		g_sock = INVALID_SOCKET;
	}
	g_lastError.clear();
	g_lines.clear();
	g_recvBuf.clear();
	g_connectCancel = false;
	g_connectFinished = false;
	g_connectOk = false;
	g_connectSock = INVALID_SOCKET;
	g_connecting = true;
	const std::string hostCopy = host ? host : "";
	g_connectThread = std::thread(NetClient_ConnectWorker, hostCopy, port);
}

bool NetClient_TakeConnectFinished(bool& outOk)
{
	if (!g_connectFinished.load())
	{
		return false;
	}
	NetClient_JoinConnectThread();
	std::lock_guard<std::mutex> lock(g_connectMutex);
	outOk = g_connectOk;
	g_lastError = g_connectError;
	if (outOk)
	{
		g_sock = g_connectSock;
		g_connectSock = INVALID_SOCKET;
	}
	else if (g_connectSock != INVALID_SOCKET)
	{
		closesocket(g_connectSock);
		g_connectSock = INVALID_SOCKET;
	}
	g_connectFinished = false;
	g_connecting = false;
	return true;
}

bool NetClient_Connect(const char* host, int port)
{
	NetClient_BeginConnect(host, port);
	while (g_connecting.load() || g_connectFinished.load())
	{
		bool ok = false;
		if (NetClient_TakeConnectFinished(ok))
		{
			return ok;
		}
		Sleep(10);
	}
	return false;
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
