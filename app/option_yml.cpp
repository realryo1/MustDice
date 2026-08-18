#include "option_yml.h"
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <sstream>

static const char* OPTION_PATH = "option.yml";

bool OptionYml_Load(std::string& outIp, int& outPort, std::string& outName)
{
	outIp.clear();
	outPort = OPTION_DEFAULT_PORT;
	outName = "Player";
	std::ifstream in(OPTION_PATH);
	if (!in)
	{
		return false;
	}
	std::string line;
	while (std::getline(in, line))
	{
		const std::string::size_type colon = line.find(':');
		if (colon == std::string::npos)
		{
			continue;
		}
		std::string key = line.substr(0, colon);
		std::string value = line.substr(colon + 1);
		while (!key.empty() && (key.front() == ' ' || key.front() == '\t'))
		{
			key.erase(key.begin());
		}
		while (!key.empty() && (key.back() == ' ' || key.back() == '\t'))
		{
			key.pop_back();
		}
		while (!value.empty() && (value.front() == ' ' || value.front() == '\t'))
		{
			value.erase(value.begin());
		}
		while (!value.empty() && (value.back() == ' ' || value.back() == '\t' || value.back() == '\r'))
		{
			value.pop_back();
		}
		if (key == "server_ip")
		{
			outIp = value;
		}
		else if (key == "server_port")
		{
			outPort = std::atoi(value.c_str());
			if (outPort <= 0)
			{
				outPort = OPTION_DEFAULT_PORT;
			}
		}
		else if (key == "player_name")
		{
			outName = value;
		}
	}
	return !outIp.empty();
}

bool OptionYml_Save(const std::string& ip, int port, const std::string& name)
{
	std::ofstream out(OPTION_PATH, std::ios::trunc);
	if (!out)
	{
		return false;
	}
	out << "server_ip: " << ip << "\n";
	out << "server_port: " << port << "\n";
	out << "player_name: " << name << "\n";
	return true;
}
