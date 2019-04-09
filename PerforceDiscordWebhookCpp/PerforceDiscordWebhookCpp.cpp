#include <iostream>

#include "include/p4/clientapi.h"
#include "include/json.hpp"

using json = nlohmann::json;

#ifdef _WIN32
#pragma comment(lib, "libclient.lib")
#pragma comment(lib, "librpc.lib")
#pragma comment(lib, "libsupp.lib")
#pragma comment(lib, "SSLStub.lib")
#else
#pragma comment(lib, "libclient.a")
#pragma comment(lib, "librpc.a")
#pragma comment(lib, "libsupp.a")
#endif

class ClientUserEx : public ClientUser
{
public:
	ClientUserEx() = default;
	virtual ~ClientUserEx() {}

	void OutputInfo(char level, const char *data);
	void OutputText(const char *data, int length);
	void ClearBuffer() { m_Data = ""; }

	std::string GetData() const { return m_Data; }

private:
	std::string m_Data;
};

void ClientUserEx::OutputInfo(char, const char *data)
{
	m_Data += data;
}

void ClientUserEx::OutputText(const char *data, int)
{
	m_Data += data;
}

int main(int argc)
{
	ClientUserEx ui;
	ClientApi client;
	StrBuf msg;
	Error e;

	// Any special protocol mods

	//client.SetProtocol( "tag", "");

	// Connect to server

	client.Init(&e);

#ifndef _WIN32
	char *loginArg[] = { (char*)"-a" };
	client.SetArgv(1, loginArg);
	client.Run("login", &ui);
#endif

	if (e.Test())
	{
		e.Fmt(&msg);
		fprintf(stderr, "%s\n", msg.Text());
		return 1;
	}

	char *changelistArg[] = {  (char*)"-l", (char*)"-m", (char*)"5", (char*)"-s", (char*)"submitted", (char*)"-t" };
	int changelistC = 6;
	client.SetArgv(changelistC, changelistArg);
	client.Run("changes", &ui);

	std::string data = ui.GetData();
	std::cout << data.c_str() << std::endl;


	json message{};
	message["\"username\""] = "\"Perforce C++ Bot\"";
	message["\"content\""] = "\"Some changelist number\"";

	std::string jsonStr = message.dump();

	std::string webhookCommand("curl -H \"Content-Type:application/json;charset=UTF-8\" -X POST -d ");
	webhookCommand.append(jsonStr);
	webhookCommand += ' ';
	char* buff = new char[125];
	size_t nrOfElmnts = 0;
	_dupenv_s(&buff, &nrOfElmnts, "DISCORDWEBHOOK");
	webhookCommand.append(buff);
	system(webhookCommand.c_str());

	// Close connection

	client.Final(&e);

	if (e.Test())
	{
		e.Fmt(&msg);
		fprintf(stderr, "%s\n", msg.Text());
		return 1;
	}

	return 0;
}