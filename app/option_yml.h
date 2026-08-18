#pragma once

#include <string>

static const int OPTION_DEFAULT_PORT = 7777;

bool OptionYml_Load(std::string& outIp, int& outPort, std::string& outName);
bool OptionYml_Save(const std::string& ip, int port, const std::string& name);
