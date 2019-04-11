#include <iostream>
#include <sstream>
#include <vector>
#include <regex>
#include <fstream>

#include "include/p4/clientapi.h"
#include "include/json.hpp"

using json = nlohmann::json;

#define TESTING

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

// Just putting it here for now so I don't have to edit the Makefile..
// TODO: move all these functions and stuff somewhere else so it's not all in one file
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

void CheckForUnsyncedChangeLists(ClientUserEx &cu, ClientApi &client, uint16_t nrOfChngLsts);

void GetLatestChangeListsFromServer(ClientUserEx &cu, ClientApi &client, uint16_t nrOfChngLsts);

// Extracts the second capture group of the regex given
void ExtractSingleLineData(const std::string &data, const std::regex &rgx, std::vector<std::string> &outData);

// Extracts the second capture group of the regex given
void ExtractSingleLineData(const std::string &data, const std::regex &rgx, std::string &outData);

void ExtractChangelistNrs(ClientUserEx &cu, uint16_t nrOfChngLsts, std::vector<std::string> &changeListNrs);

void FetchUnsyncedNrs(const std::string &cacheFileName, const std::vector<std::string> &changeListNrs, std::vector<std::string> &unsyncedNrs);

std::vector<char> ReadFile(const std::string &fileName);

void GetDescriptionsOfChangelists(ClientUserEx &cu, ClientApi &client, const std::vector<std::string> &unsyncedNrs);

void ParseChangelists(ClientUserEx &cu, ClientApi &client);

void ExtractMultiLineData(const std::string &data, const std::regex &rgx, std::vector<std::string> &outData);

void ExtractMultiLineDataFullString(const std::string &data, const std::regex &rgx, std::vector<std::string> &outData);

void ExtractMultiLineDataFullString(const std::string &data, const std::regex &rgx, std::string &outData);

void ExtractChangelists(ClientUserEx &cu, std::vector<std::string> &changelists);

struct FileData;
struct Changelist;
void StoreChangelistsDataInStruct(ClientUserEx &cu, ClientApi &client, const std::vector<std::string> &changelists, std::vector<Changelist> &changelistStructs);

void ParseFiles(ClientUserEx &cu, ClientApi &client, const std::string &cl, std::vector<FileData> &outFiles);

void ParseDiffs(ClientUserEx &cu, ClientApi &client, FileData &fileData);

void GetDiff(ClientUserEx &cu, ClientApi &client, FileData &fileData);

void WriteFile(const std::vector<std::string> &data, const std::string &fileName);

void SendWebhookMessage(ClientUserEx &cu);

void Close(ClientApi &client, Error &e, StrBuf &msg);

///////////////////////////////////////////////////////////////////////
int main(int argc, char* argv[])
{
	ClientUserEx cu;
	ClientApi client;
	StrBuf msg;
	Error e;

	Login(cu, client, e, msg);

	CheckForUnsyncedChangeLists(cu, client, 5);

#ifndef TESTING
	SendWebhookMessage(cu);
#endif // DEBUG

	Close(client, e, msg);

	return 0;
}
///////////////////////////////////////////////////////////////////////

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

inline char* GetEnv(const char* varName)
{
	// Visual Studio complains when I use getenv
#ifdef _WIN32
	char* buff = new char[125];
	size_t nrOfElmnts = 0;
	_dupenv_s(&buff, &nrOfElmnts, varName);
#else
	char* buff = getenv(varName);
#endif

	return buff;
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

void CheckForUnsyncedChangeLists(ClientUserEx &cu, ClientApi &client, uint16_t nrOfChngLsts)
{	
	GetLatestChangeListsFromServer(cu, client, nrOfChngLsts);

	std::vector<std::string> changeListNrs;
	ExtractChangelistNrs(cu, nrOfChngLsts, changeListNrs);

	std::string cacheFileName("cl.txt");
	
	std::vector<std::string> unsyncedNrs;
	FetchUnsyncedNrs(cacheFileName, changeListNrs, unsyncedNrs);

	if (unsyncedNrs.size() > 0)
	{		
#ifndef TESTING
		WriteFile(unsyncedNrs, cacheFileName);
#endif
		GetDescriptionsOfChangelists(cu, client, unsyncedNrs);

		ParseChangelists(cu, client);
	}
}

void GetLatestChangeListsFromServer(ClientUserEx &cu, ClientApi &client, uint16_t nrOfChngLsts)
{
	char* filterPath = GetEnv("P4FILTERPATH");

	// More info: https://www.perforce.com/manuals/cmdref/Content/CmdRef/p4_changes.html
	char *changelistArg[] = { (char*)"-l", (char*)"-m", (char*)std::to_string(nrOfChngLsts).c_str(), (char*)"-s", (char*)"submitted", (char*)"-t", filterPath };
	int changelistC = 7;
	client.SetArgv(changelistC, changelistArg);
	client.Run("changes", &cu);
	// Don't need to return anything as it gets stored in m_Data in ClientUserEx
}

// Extracts the second capture group of the regex given
void ExtractSingleLineData(const std::string &data, const std::regex &rgx, std::vector<std::string> &outData)
{
	std::istringstream dataStream(data);

	std::string dataLine = "";
	std::smatch sm;

	while (std::getline(dataStream, dataLine, '\n'))
	{
		if (std::regex_search(dataLine, sm, rgx))
		{
			std::string dataString(sm[2]);
			dataString.push_back('\n');
			outData.push_back(dataString);
		}
	}
}

// Extracts the second capture group of the regex given
void ExtractSingleLineData(const std::string &data, const std::regex &rgx, std::string &outData)
{
	std::vector<std::string> temp;
	ExtractSingleLineData(data,rgx,temp);

	outData = temp[0];
	outData.pop_back();
}

void ExtractChangelistNrs(ClientUserEx &cu, uint16_t nrOfChngLsts, std::vector<std::string> &changeListNrs)
{
	changeListNrs.reserve(nrOfChngLsts);
	std::regex changeListRgx("(^Change )([0-9]+)");
	ExtractSingleLineData(cu.GetData(), changeListRgx, changeListNrs);

	cu.ClearBuffer();
}

void FetchUnsyncedNrs(const std::string &cacheFileName, const std::vector<std::string> &changeListNrs, std::vector<std::string> &unsyncedNrs)
{
	std::vector<char> file = ReadFile(cacheFileName);

	if (file.size() != 0)
	{
		std::string cachedNr;
		std::vector<std::string> cachedNrs;
		for (const auto &c : file)
		{
			if (c != '\n')
				cachedNr.push_back(c);
			else
			{
				cachedNr.push_back(c);
				cachedNrs.push_back(cachedNr);
				cachedNr = "";
			}
		}

		size_t equalIndex = changeListNrs.size();

		for (size_t i = 0; i < changeListNrs.size(); ++i)
		{
			for (size_t j = 0; j < cachedNrs.size(); ++j)
			{
				int clNr = std::stoi(changeListNrs[j]);
				int caNr = std::stoi(cachedNrs[i]);

				if (clNr > caNr)
				{
					continue;
				}
				else if (clNr == caNr)
				{
					equalIndex = j;
					break;
				}
				else
				{
					std::cout << "WARNING: Changes possibly missed > Consider increasing number of changelists or check interval\n\n";
				}
			}
			if (equalIndex < changeListNrs.size())
			{
				break;
			}
		}

		for (size_t i = 0; i < equalIndex; ++i)
		{
			unsyncedNrs.push_back(changeListNrs[i]);
		}
	}
	else
	{
		unsyncedNrs = changeListNrs;
	}
}

std::vector<char> ReadFile(const std::string &fileName)
{
	std::ifstream file(fileName, std::ios::binary);

	if (!file.is_open())
	{
		std::cout << "File not found: " << fileName << "\n> New cache file will be made\n\n";
		return std::vector<char>();
	}

	std::vector<char> fileVec((std::istreambuf_iterator<char>(file)), (std::istreambuf_iterator<char>()));

	file.close();

	return fileVec;
}

void GetDescriptionsOfChangelists(ClientUserEx &cu, ClientApi &client, const std::vector<std::string> &unsyncedNrs)
{
	for (auto clId : unsyncedNrs)
	{
		clId.pop_back(); // Giving newline to the command makes it fail

		// clId = "69183"; // TODO: REMOVE THIS

		// More info: https://www.perforce.com/manuals/cmdref/Content/CmdRef/p4_describe.html
		// Added -s here, as diffs will be handled later instead of them being dragged all the way from here
		char *clArg[] = { (char*)"-s", (char*)clId.c_str() };
		int clC = 2;
		client.SetArgv(clC, clArg);
		client.Run("describe", &cu);
	}
	// Don't need to return anything as it gets stored in m_Data in ClientUserEx
}

struct FileData
{
	std::string fileName;
	uint32_t revision;
	std::string action; // TODO: Replace by an enum?
	std::string type; // TODO: Replace by an enum?

	bool IsFirstRevision() { return revision == 1; }

	std::string GetCurrentRevString() { return fileName + '#' + std::to_string(revision); }

	// These two were written much shorter, but gave unexpected results
	char* GetCurrentRev()
	{
		std::string s(fileName);
		s.push_back('#');
		s.append(std::to_string(revision));
#ifdef _WIN32
		return _strdup(s.c_str());
#else
		return strdup(s.c_str());
#endif		
	}

	char* GetPreviousRev()
	{
		std::string s(fileName);
		s.push_back('#');
		uint32_t r = revision - 1;
		s.append(std::to_string(r));
#ifdef _WIN32
		return _strdup(s.c_str());
#else
		return strdup(s.c_str());
#endif	
	}
};

struct Changelist
{
	uint32_t id;
	std::string author;
	std::string workspace;
	std::string timestamp;
	std::string description;
	std::vector<FileData> files;
};

void ParseChangelists(ClientUserEx &cu, ClientApi &client)
{
	std::vector<std::string> changelists;
	ExtractChangelists(cu, changelists);
	
	std::vector<Changelist> changelistStructs;
	StoreChangelistsDataInStruct(cu, client, changelists, changelistStructs);


}

void ExtractMultiLineData(const std::string &data, const std::regex &rgx, std::vector<std::string> &outData)
{
	// Slight repeat of code from ExtractSingleLineData,
	// but then storing the multi lines of data from a single line
	// I'd rather keep them two seperate functions to not add more conditionals internally

	std::istringstream dataStream(data);

	std::string dataLine = "";
	std::string dataCat = "";
	std::smatch sm;

	while (std::getline(dataStream, dataLine, '\n'))
	{
		if (std::regex_search(dataLine, sm, rgx))
		{
			if (!dataCat.empty())
				outData.push_back(dataCat);

			dataCat = "";
			dataCat.append(dataLine);
		}
		else
		{
			dataCat.append(dataLine);
			dataCat.push_back('\n');
		}
	}

	if (!dataCat.empty())
		outData.push_back(dataCat);
}

void ExtractMultiLineDataFullString(const std::string &data, const std::regex &rgx, std::vector<std::string> &outData)
{
	std::istringstream dataStream(data);

	std::string dataLine = "";
	std::smatch sm;

	while (std::getline(dataStream, dataLine, '\n'))
	{
		if (std::regex_search(dataLine, sm, rgx))
		{
			std::string dataString(sm[2]);
			outData.push_back(dataString);
		}
	}
}

void ExtractMultiLineDataFullString(const std::string &data, const std::regex &rgx, std::string &outData)
{
	std::vector<std::string> temp;
	ExtractMultiLineDataFullString(data, rgx, temp);

	outData = "";
	for (const auto &str : temp)
	{
		outData.append(str);
		outData.push_back('\n');
	}
}

void ExtractChangelists(ClientUserEx &cu, std::vector<std::string> &changelists)
{
	std::regex changeListRgx("(^Change )([0-9]+)");
	ExtractMultiLineData(cu.GetData(), changeListRgx, changelists);

	cu.ClearBuffer();

#ifdef TESTING
	for (const auto &cl : changelists)
	{
		std::cout << "CHANGELIST" << std::endl;
		std::cout << cl;
	}
#endif
}

void StoreChangelistsDataInStruct(ClientUserEx &cu, ClientApi &client, const std::vector<std::string> &changelists, std::vector<Changelist> &changelistStructs)
{
	for (const std::string &cl : changelists)
	{
		Changelist clStrct;

		std::string data;

		std::regex rgx("(^Change )([0-9]+)");
		ExtractSingleLineData(cl, rgx, data);
		clStrct.id = std::stoi(data);

		rgx = ("(by )(.+)(@)");
		ExtractSingleLineData(cl, rgx, data);
		clStrct.author = data;

		rgx = ("(@)(.+)( on )");
		ExtractSingleLineData(cl, rgx, data);
		clStrct.workspace = data;

		rgx = ("( on )(.+)($)");
		ExtractSingleLineData(cl, rgx, data);
		clStrct.timestamp = data;

		rgx = ("(^\\t)(.+)($)");
		ExtractMultiLineDataFullString(cl, rgx, data);
		clStrct.description = data;

		std::vector<FileData> files;
		ParseFiles(cu, client, cl, files);
		clStrct.files = files;

		changelistStructs.push_back(clStrct);
	}
}

void ParseFiles(ClientUserEx &cu, ClientApi &client, const std::string &cl, std::vector<FileData> &outFiles)
{
	std::string filterPath(GetEnv("P4FILTERPATH"));
	filterPath = filterPath.substr(0, filterPath.size() - 3); // As a filterpath ends in 3 dots

	std::stringstream fileRegx;
	fileRegx << "(^)" << "(" << filterPath << ".*)" << "($)";

	std::vector<std::string> files;
	std::regex rgx(fileRegx.str());
	ExtractMultiLineDataFullString(cl, rgx, files);

	for (const std::string &fileStr : files)
	{
		FileData fileData;

		std::regex fileStringRgx("(^.+)(?:#)([0-9]+)(?: )(.+$)");
		std::smatch sm;

		if (std::regex_match(fileStr, sm, fileStringRgx))
		{
			fileData.fileName = sm[1];
			fileData.revision = std::stoi(sm[2]);
			fileData.action = sm[3];
		}
		else
		{
			std::cout << "WARNING: Could not parse file: " << fileStr << std::endl;
			continue;
		}

		ParseDiffs(cu, client, fileData);

		outFiles.push_back(fileData);
	}
}

void ParseDiffs(ClientUserEx &cu, ClientApi &client, FileData &fileData)
{
	GetDiff(cu, client, fileData);

	std::string diff(cu.GetData());
	cu.ClearBuffer();

	std::regex rgx("(?:^==== .+?-.+?\\()(.+?)(?:\\) ==== .+?\\n)([\\s\\S]*)");
	std::smatch sm;

	if (std::regex_search(diff, sm, rgx))
	{
		fileData.type = sm[1];
		if (fileData.action != "edit")
			return;
		diff = sm[2];
	}
	else
	{
		std::cout << "WARNING: Error parsing diff:\n" << diff << std::endl;
	}

	std::cout << diff << std::endl;
}

void CreateDiffHTML()
{

}

void GetDiff(ClientUserEx &cu, ClientApi &client, FileData &fileData)
{
	int diffC = 2;
	// I need to do get the diff even if it's the first rev, to know the file's type
	if (fileData.IsFirstRevision())
	{
		char *diffArg[] = { fileData.GetCurrentRev(), fileData.GetCurrentRev() };
		client.SetArgv(diffC, diffArg);
	}
	else
	{
		char *diffArg[] = { fileData.GetPreviousRev(), fileData.GetCurrentRev() };
		client.SetArgv(diffC, diffArg);
	}	

	// More info: https://www.perforce.com/manuals/cmdref/Content/CmdRef/p4_diff2.html
	client.Run("diff2", &cu);
}

void WriteFile(const std::vector<std::string> &data, const std::string &fileName)
{
	std::ofstream file(fileName, std::ios::binary);

	if (!file.is_open())
	{
		std::cout << "Could not create file: " << fileName << "\n";
		exit(-1);
	}

	std::vector<char> newFile;

	for (const auto &clnr : data)
		for (const auto &c : clnr)
			newFile.push_back(c);

	file.write(newFile.data(), newFile.size());

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

	char* discordWebHook = GetEnv("DISCORDWEBHOOK");
	webhookCommand.append(discordWebHook);

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