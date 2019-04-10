#include <iostream>
#include <sstream>
#include <vector>
#include <regex>
#include <fstream>

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

void Login(ClientUserEx &cu, ClientApi &client, Error &e, StrBuf &msg);

void CheckForNewChangeLists(ClientUserEx &cu, ClientApi &client, uint16_t nrOfChngLsts);

std::vector<char> ReadFile(const std::string &fileName);

void WriteFile(const std::vector<char> &data, const std::string &fileName);

void SendWebhookMessage(ClientUserEx &cu);

void Close(ClientApi &client, Error &e, StrBuf &msg);

int main(int argc, char* argv[])
{
	ClientUserEx cu;
	ClientApi client;
	StrBuf msg;
	Error e;

	Login(cu, client, e, msg);

	CheckForNewChangeLists(cu, client, 5);

	SendWebhookMessage(cu);

	Close(client, e, msg);

	return 0;
}

void ClientUserEx::OutputInfo(char, const char *data)
{
	m_Data.append(data);
	m_Data.push_back('\n');
}

void ClientUserEx::OutputText(const char *data, int)
{
	m_Data.append(data);
	m_Data.push_back('\n');
}

void Login(ClientUserEx &cu, ClientApi &client, Error &e, StrBuf &msg)
{
	client.Init(&e);

#ifndef _WIN32
	char *loginArg[] = { (char*)"-a" };
	client.SetArgv(1, loginArg);
	client.Run("login", &cu);
#endif

	if (e.Test())
	{
		e.Fmt(&msg);
		fprintf(stderr, "%s\n", msg.Text());
		exit(1);
	}
}

void CheckForNewChangeLists(ClientUserEx &cu, ClientApi &client, uint16_t nrOfChngLsts)
{	
	// More info: https://www.perforce.com/manuals/cmdref/Content/CmdRef/p4_changes.html
	char *changelistArg[] = { (char*)"-l", (char*)"-m", (char*)std::to_string(nrOfChngLsts).c_str(), (char*)"-s", (char*)"submitted", (char*)"-t" };
	int changelistC = 6;
	client.SetArgv(changelistC, changelistArg);
	client.Run("changes", &cu);

	std::istringstream dataStream(cu.GetData());
	std::string dataLine = "";
	std::string changeList = "";
	std::vector<std::string> changeLists;
	changeLists.reserve(nrOfChngLsts);
	std::vector<std::string> changeListNrs;
	changeListNrs.reserve(nrOfChngLsts);
	std::regex changeListRgx("(Change )([0-9]+)");
	std::smatch sm;

	while (std::getline(dataStream, dataLine, '\n'))
	{
		if (std::regex_search(dataLine,sm,changeListRgx))
		{
			std::string changeListNr(sm[2]);
			changeListNr.push_back('\n');
			changeListNrs.push_back(changeListNr);

			if (!changeList.empty())
				changeLists.push_back(changeList);

			changeList = "";
			changeList.append(dataLine);
		}
		else
		{
			changeList.append(dataLine);
			changeList.push_back('\n');
		}
	}

	for (auto cl : changeLists)
		std::cout << cl << std::endl;
	for (auto cl : changeListNrs)
		std::cout << cl << std::endl;

	std::string fileName("cl.txt");
	
	std::vector<char> file = ReadFile(fileName);

	std::vector<char> newFile;

	for (auto clnr : changeListNrs)
		for (auto c : clnr)
			newFile.push_back(c);

	WriteFile(newFile, fileName);
}

std::vector<char> ReadFile(const std::string &fileName)
{
	std::ifstream file(fileName, std::ios::binary);

	if (!file.is_open())
	{
		std::cout << "File not found: " << fileName << "\nNew cache file will be made";
		return std::vector<char>();
	}

	std::vector<char> fileVec((std::istreambuf_iterator<char>(file)), (std::istreambuf_iterator<char>()));

	file.close();

	return fileVec;
}

void WriteFile(const std::vector<char> &data, const std::string &fileName)
{
	std::ofstream file(fileName, std::ios::binary);

	if (!file.is_open())
	{
		std::cout << "Could not create file: " << fileName << "\n";
		exit(-1);
	}

	file.write(data.data(), data.size());

	file.close();
}

void SendWebhookMessage(ClientUserEx &cu)
{
	json message{};

#ifdef _WIN32
	message["\"username\""] = "\"Perforce C++ Bot\"";
#else
	message["\"username\""] = "\"Perforce C++ Bot Heroku\"";
#endif	
	message["\"content\""] = "\"Some changelist number\"";

	std::string jsonStr = message.dump();

	std::string webhookCommand("curl -H \"Content-Type:application/json;charset=UTF-8\" -X POST -d ");
	webhookCommand.append(jsonStr);
	webhookCommand += ' ';

#ifdef _WIN32
	char* buff = new char[125];
	size_t nrOfElmnts = 0;
	_dupenv_s(&buff, &nrOfElmnts, "DISCORDWEBHOOK");
#else
	char* buff = getenv("DISCORDWEBHOOK");
#endif

	webhookCommand.append(buff);

	system(webhookCommand.c_str());
}

void Close(ClientApi &client, Error &e, StrBuf &msg)
{
	client.Final(&e);

	if (e.Test())
	{
		e.Fmt(&msg);
		fprintf(stderr, "%s\n", msg.Text());
		exit(1);
	}
}