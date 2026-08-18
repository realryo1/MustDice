#pragma once

#include <string>

void NetClient_Startup(void);
void NetClient_Shutdown(void);
bool NetClient_Connect(const char* host, int port);
void NetClient_Close(void);
bool NetClient_IsConnected(void);
void NetClient_Send(const char* line);
void NetClient_Poll(void);
bool NetClient_PopLine(std::string& outLine);
const char* NetClient_LastError(void);
