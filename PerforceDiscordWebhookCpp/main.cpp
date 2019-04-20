#include <iostream>
#include <sstream>
#include <vector>
#include <regex>
#include <fstream>
#include <chrono>
#include <thread>

#include "include/p4/clientapi.h"
#include "include/json.hpp"

using json = nlohmann::json;

#include "helpers.h"

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

struct FileData;
struct Changelist;

#pragma region ForwardDeclMain
void Login(ClientUserEx &cu, ClientApi &client, Error &e, StrBuf &msg, int argc);

void CheckAndGetGithubRepo(std::string &path);

void CheckForUnsyncedChangeLists(ClientUserEx &cu, ClientApi &client, uint16_t nrOfChngLsts, const std::string &path);

void SendWebhookMessage(ClientUserEx &cu, std::vector<Changelist> &changelistStructs);

void Close(ClientApi &client, Error &e, StrBuf &msg);
#pragma endregion

///////////////////////////////////////////////////////////////////////
WebhookWarning* m_Warnings = new WebhookWarning();

int main(int argc, char* argv[])
{
	ClientUserEx cu;
	ClientApi client;
	StrBuf msg;
	Error e;

	Login(cu, client, e, msg, argc);

	std::string path;
	CheckAndGetGithubRepo(path);

	while (true)
	{
		CheckForUnsyncedChangeLists(cu, client, 5, path);

		m_Warnings->SendWarnings();
		m_Warnings->Clear();

		std::this_thread::sleep_for(std::chrono::milliseconds(180000));
	}

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

#pragma region ForwardDecl
void GetLatestChangeListsFromServer(ClientUserEx &cu, ClientApi &client, uint16_t nrOfChngLsts);

void ExtractChangelistNrs(ClientUserEx &cu, uint16_t nrOfChngLsts, std::vector<std::string> &changeListNrs);

void FetchUnsyncedNrs(const std::string &cacheFileName, const std::vector<std::string> &changeListNrs, std::vector<std::string> &unsyncedNrs);

void GetDescriptionsOfChangelists(ClientUserEx &cu, ClientApi &client, const std::vector<std::string> &unsyncedNrs);

void ParseChangelists(ClientUserEx &cu, ClientApi &client, const std::string &path, std::vector<Changelist> &changelistStructs);

void ExtractChangelists(ClientUserEx &cu, std::vector<std::string> &changelists);

void StoreChangelistsDataInStruct(ClientUserEx &cu, ClientApi &client, const std::string &path, const std::vector<std::string> &changelists, std::vector<Changelist> &changelistStructs);

void ParseFiles(ClientUserEx &cu, ClientApi &client, const std::string &path, const Changelist &clStrct, const std::string &cl, std::vector<FileData> &outFiles);

void ParseDiffs(ClientUserEx &cu, ClientApi &client, const std::string &path, const Changelist &clStrct, FileData &fileData);

void CreateDiffHTML(std::vector<std::string> &diffVec, const std::string &path, const Changelist &clStrct, FileData &fileData);

void DiffInfoToHTML(std::vector<std::string> &diffVec, std::string &out);

void GetDiff(ClientUserEx &cu, ClientApi &client, FileData &fileData);
#pragma endregion


void Login(ClientUserEx &cu, ClientApi &client, Error &e, StrBuf &msg, int argc)
{
	client.Init(&e);

#ifndef _WIN32
	char *loginArg[] = { (char*)"-a" };
	client.SetArgv(1, loginArg);
	client.Run("login", &cu);
	std::cout << std::endl;
#endif

	if (e.Test())
	{
		e.Fmt(&msg);
		fprintf(stderr, "%s\n", msg.Text());
		exit(1);
	}

	std::cout << "Login passed." << std::endl;
}

void CheckAndGetGithubRepo(std::string &path)
{
#ifndef _WIN32
	std::string userEmailCommand("git config --global user.email ");
	userEmailCommand.append(GetEnv("USEREMAIL"));
	system(userEmailCommand.c_str());

	std::string userNameCommand("git config --global user.name ");
	userNameCommand.append(GetEnv("USERFULLNAME"));
	system(userNameCommand.c_str());
#endif

	// Could probably check if it exists first
	std::string pullCommand("git clone ");

	std::string repo(GetEnv("GITHUBREPO"));
	size_t slashPos = repo.find('/');
	slashPos += 2;

	std::string login(GetEnv("GITHUBUSER"));
	login.push_back(':');
	login.append(GetEnv("GITHUBTOKEN"));
	login.push_back('@');

	repo.insert(slashPos, login);

	pullCommand.append(repo);

	system(pullCommand.c_str());

	slashPos = repo.find_last_of('/');
	++slashPos;
	repo = repo.substr(slashPos, repo.size() - slashPos - 4);

	path = repo + "/";

	system(GitCommandHelper(path, " pull").c_str());

	std::cout << "End of Checking Github Pages repo." << std::endl;
}

void CheckForUnsyncedChangeLists(ClientUserEx &cu, ClientApi &client, uint16_t nrOfChngLsts, const std::string &path)
{	
	std::cout << "Checking for recent changes." << std::endl;

	GetLatestChangeListsFromServer(cu, client, nrOfChngLsts);

	std::vector<std::string> changeListNrs;
	ExtractChangelistNrs(cu, nrOfChngLsts, changeListNrs);

	std::string cacheFileName("cl.txt");
	std::string cacheFilePath(path);
	cacheFilePath.append(cacheFileName);
	
	std::vector<std::string> unsyncedNrs;
	FetchUnsyncedNrs(cacheFilePath, changeListNrs, unsyncedNrs);

	if (unsyncedNrs.size() > 0)
	{
		std::cout << "Changes found." << std::endl;
		// Only the very latest number
		WriteFile(unsyncedNrs[0], cacheFilePath);

		// Add the cache file to the repo
		// TODO: Only do this if it's needed
		std::string command(" add ");
		command.append(cacheFileName);

		system(GitCommandHelper(path, command).c_str());
		//

		std::cout << "Getting descriptions." << std::endl;
		GetDescriptionsOfChangelists(cu, client, unsyncedNrs);

		std::cout << "Parsing changelists." << std::endl;
		std::vector<Changelist> changelistStructs;
		ParseChangelists(cu, client, path, changelistStructs);

		SendWebhookMessage(cu, changelistStructs);

		std::cout << "Sync complete." << std::endl;
	}
	else
	{
		std::cout << "No changes." << std::endl;
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

		for (const auto &c : file)
		{
			cachedNr.push_back(c);
		}

		size_t equalIndex = changeListNrs.size();
		bool bEqualFound = false;

		for (size_t i = 0; i < changeListNrs.size(); ++i)
		{
			int clNr = std::stoi(changeListNrs[i]);
			int caNr = std::stoi(cachedNr);

			if (clNr > caNr)
			{
				continue;
			}
			else if (clNr == caNr)
			{
				bEqualFound = true;
				equalIndex = i;
				break;
			}			
		}

		if (!bEqualFound)
		{
			m_Warnings->StoreWarning("\nWARNING: Changes possibly missed > Consider increasing number of changelists or check interval\n\n");
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

void GetDescriptionsOfChangelists(ClientUserEx &cu, ClientApi &client, const std::vector<std::string> &unsyncedNrs)
{
	for (auto clId : unsyncedNrs)
	{
		clId.pop_back(); // Giving newline to the command makes it fail

		// More info: https://www.perforce.com/manuals/cmdref/Content/CmdRef/p4_describe.html
		// Added -s here, as diffs will be handled later instead of them being dragged all the way from here
		char *clArg[] = { (char*)"-s", (char*)clId.c_str() };
		int clC = 2;
		client.SetArgv(clC, clArg);
		client.Run("describe", &cu);
	}
	// Don't need to return anything as it gets stored in m_Data in ClientUserEx
}

void ParseChangelists(ClientUserEx &cu, ClientApi &client, const std::string &path, std::vector<Changelist> &changelistStructs)
{
	std::vector<std::string> changelists;
	ExtractChangelists(cu, changelists);
	std::cout << "Extracted changelists." << std::endl;
	
	StoreChangelistsDataInStruct(cu, client, path, changelists, changelistStructs);
}

void ExtractChangelists(ClientUserEx &cu, std::vector<std::string> &changelists)
{
	std::regex changeListRgx("(^Change )([0-9]+)");
	ExtractMultiLineData(cu.GetData(), changeListRgx, changelists);

	cu.ClearBuffer();
}

void StoreChangelistsDataInStruct(ClientUserEx &cu, ClientApi &client, const std::string &path, const std::vector<std::string> &changelists, std::vector<Changelist> &changelistStructs)
{
	std::cout << "Storing changelist data." << std::endl;

	for (const std::string &cl : changelists)
	{
		Changelist clStrct;

		std::string data;

		std::regex rgx("(^Change )([0-9]+)");
		ExtractSingleLineData(cl, rgx, data);
		clStrct.id = std::stoi(data);
		std::cout << "> ID stored." << std::endl;

		rgx = ("(by )(.+)(@)");
		ExtractSingleLineData(cl, rgx, data);
		clStrct.author = data;
		std::cout << "> Author stored." << std::endl;

		rgx = ("(@)(.+)( on )");
		ExtractSingleLineData(cl, rgx, data);
		clStrct.workspace = data;
		std::cout << "> Worspace stored." << std::endl;

		rgx = ("( on )(.+)($)");
		ExtractSingleLineData(cl, rgx, data);
		clStrct.timestamp = std::regex_replace(data, std::regex("/"), "-");
		std::cout << "> Timestamp stored." << std::endl;

		rgx = ("(^\\t)(.+)($)");
		ExtractMultiLineDataFullString(cl, rgx, data);
		clStrct.description = data;
		std::cout << "> Description stored." << std::endl;

		std::vector<FileData> files;
		ParseFiles(cu, client, path, clStrct, cl, files);
		clStrct.files = files;
		std::cout << "> File data stored." << std::endl;

		changelistStructs.push_back(clStrct);
	}

	std::cout << "Changelist structs parsed, commiting and pushing changes." << std::endl;

	system(GitCommandHelper(path, " commit -m heroku").c_str());

	system(GitCommandHelper(path, " push").c_str());
}

void ParseFiles(ClientUserEx &cu, ClientApi &client, const std::string &path, const Changelist &clStrct, const std::string &cl, std::vector<FileData> &outFiles)
{
	std::cout << ">> Parsing files." << std::endl;
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
			std::string wrng("WARNING: Could not parse file: ");
			wrng.append(fileStr);
			m_Warnings->StoreWarning(wrng);
			continue;
		}

		std::cout << ">> Parsing diff for " << fileData.fileName << std::endl;

		ParseDiffs(cu, client, path, clStrct, fileData);

		outFiles.push_back(fileData);
	}
}

void ParseDiffs(ClientUserEx &cu, ClientApi &client, const std::string &path, const Changelist &clStrct, FileData &fileData)
{
	GetDiff(cu, client, fileData);

	std::string diff(cu.GetData());
	cu.ClearBuffer();

	std::regex rgx("(?:^==== .+?-.+?\\()(.+?)(?:\\) ==== .+?\\n)([\\s\\S]*)");
	std::smatch sm;

	if (std::regex_search(diff, sm, rgx))
	{
		fileData.type = sm[1];
		if (fileData.action != "edit" || fileData.type == "binary+F")
			return;
		diff = sm[2];
	}
	else
	{
		std::string wrng("WARNING: Error parsing diff:");
		wrng.append(diff);
		m_Warnings->StoreWarning(wrng);
	}

	std::string line;
	std::istringstream stream(diff);
	std::vector<std::string> diffVec;

	while (std::getline(stream, line))
	{
		diffVec.push_back(line);
	}

	CreateDiffHTML(diffVec, path, clStrct, fileData);
}

void CreateDiffHTML(std::vector<std::string> &diffVec, const std::string &path, const Changelist &clStrct, FileData &fileData)
{
	std::string diffInfo;
	DiffInfoToHTML(diffVec, diffInfo);
	
	std::vector<std::string> html;
	html.push_back("<!DOCTYPE html>\n");
	html.push_back("<html>\n");
	html.push_back("<head>\n");
	html.push_back("<link rel=\"stylesheet\" type=\"text/css\" href=\"styles.css\">\n");
	html.push_back("<script src=\"https://cdn.jsdelivr.net/gh/google/code-prettify@master/loader/run_prettify.js\"></script>\n");
	html.push_back("</head>\n");
	html.push_back("<body>\n");
	html.push_back("<h1>\n");
	html.push_back("Changelist: <b>" + std::to_string(clStrct.id) + "</b>\n");
	html.push_back("</h1>\n");
	html.push_back("<h2>\n");
	html.push_back("Author: <b>" + clStrct.author + "</b>\n");
	html.push_back("</h2>\n");
	html.push_back("<h3>\n");
	html.push_back("File: <b>" + fileData.GetCurrentRevString() + "</b>\n");
	html.push_back("</h3>\n");
	html.push_back(diffInfo);
	html.push_back("</body>\n");
	html.push_back("</html\n>");

	std::string file(fileData.GetStringNoPath());
	file.append(".html");

	std::string filePath;
	filePath.append(path);
	filePath.append(file);

	WriteFile(html, filePath);

	std::string command(" add ");
	command.append(file);

	system(GitCommandHelper(path, command).c_str());
}

void DiffInfoToHTML(std::vector<std::string> &diffVec, std::string &out)
{
	// Shamelessly copied from myself!
	// Basically just a port of what I had in the .NET version
	size_t lastHeadIndex = 0;
	std::string changeSpecifiers("acd");
	std::regex headRgx("^\\d.+");
	std::regex subRgx("^---");
	std::string lineNumber2 = "";
	for (size_t i = 0; i < diffVec.size(); ++i)
	{
		if (std::regex_match(diffVec[i], headRgx))
		{
			lastHeadIndex = i;

			if (i > 0)
				diffVec[i - 1].append("\n</pre>\n</div>\n");

			size_t commaIndex1 = diffVec[i].find(',', 0);
			std::string lineNumber1 = "";

			size_t changeSpecifierIndex = diffVec[i].find_first_of(changeSpecifiers);

			if (commaIndex1 < changeSpecifierIndex && commaIndex1 > 0)
				lineNumber1 = diffVec[i].substr(0, commaIndex1);
			else
				lineNumber1 = diffVec[i].substr(0, changeSpecifierIndex);

			size_t commaIndex2 = diffVec[i].find(',', changeSpecifierIndex);

			if (commaIndex2 > 0)
				lineNumber2 = diffVec[i].substr(changeSpecifierIndex + 1, commaIndex2 - changeSpecifierIndex - 1);
			else
				lineNumber2 = diffVec[i].substr(changeSpecifierIndex + 1, diffVec[i].size() - changeSpecifierIndex - 1);

			std::string preSpecifier = "\n<div class=\"block\">\n<h4>\n";
			std::string postSpecifier = "\n</h4>\n<pre class=\"prettyprint lang-csharp linenums:" + lineNumber1;

			char changeSpecifier = diffVec[i].at(changeSpecifierIndex);
			switch (changeSpecifier)
			{
			case 'a':
				diffVec[i] = preSpecifier + "Add:" + postSpecifier + " add\">";
				break;
			case 'c':
				diffVec[i] = preSpecifier + "Change:" + postSpecifier + " delete\">";
				break;
			case 'd':
				diffVec[i] = preSpecifier + "Delete:" + postSpecifier + " delete\">";
				break;
			default:
				std::cout << "Unknown Change Specifier: " << changeSpecifier << std::endl;
				break;
			}
		}
		else if (std::regex_match(diffVec[i], subRgx))
		{
			lastHeadIndex = i;
			diffVec[i] = "\n</pre>\n<pre class=\"prettyprint lang-csharp linenums:" + lineNumber2 + " add\">";
		}
		else
		{
			if (diffVec[i].size() > 0)
			{
				diffVec[lastHeadIndex] += "\n" + diffVec[i].substr(1);
				diffVec[i] = "";
			}
		}
	}

	diffVec.push_back("</pre>\n</div>");

	diffVec.erase(std::remove(diffVec.begin(), diffVec.end(), ""), diffVec.end());

	const char* const delim = "\n";
	std::ostringstream cat;
	std::copy(diffVec.begin(), diffVec.end(), std::ostream_iterator<std::string>(cat, delim));

	out = cat.str();
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

void SendWebhookMessage(ClientUserEx &cu, std::vector<Changelist> &changelistStructs)
{
	std::cout << "Sending webhooks." << std::endl;

	std::reverse(changelistStructs.begin(), changelistStructs.end());

	for (const auto &cl : changelistStructs)
	{
		json message{};

#ifdef _WIN32
		message["username"] = "Perforce C++ Bot";
#else
		message["username"] = "Perforce C++ Bot Heroku";
#endif		
		std::string content;
		content.append("Perforce change ");
		content.append(std::to_string(cl.id));
		message["content"] = content;
		
		message["avatar_url"] = GetEnv("P4AVATAR");

		message["embeds"] =
		{
			{
				{"author",
					{
						{"name", cl.author},
						{"icon_url", GetUserImage(cl.author)}
					}
				},
				{"color", GetColor(cl)},
				{"description", cl.description},
				{"url", GetEnv("EMBEDURL")},
				{"title", cl.workspace},
				{"thumbnail",
					{
						{"url",GetEnv("EMBEDTHUMB")}
					}
				},
				{"footer",
					{
						{"text", "Helix Core"},
						{"icon_url", GetEnv("P4AVATAR")}
					}
				},
				{"timestamp", cl.timestamp}
			}
		};

		for (auto file : cl.files)
		{
			message["embeds"].push_back(
				{
					{"title", file.action + " " + file.type},
					{"description", file.GetCurrentRevString()},
					{"color", GetColor(file)},
					{"url", ((file.action != "edit" || file.type == "binary+F") ? "" : GetEnv("GITHUBPAGESFULLPATH") + file.GetStringNoPath())}
				}
			);
		}

		std::string jsonStr = message.dump();

		WriteFile(jsonStr, "message.json");

		std::string webhookCommand("curl -s -S -H \"Content-Type:application/json;charset=UTF-8\" -X POST -d @");
		webhookCommand.append("message.json");
		webhookCommand += ' ';

		char* discordWebHook = GetEnv("DISCORDWEBHOOK");
		webhookCommand.append(discordWebHook);

		// Just to counter rate limit a little ..
		// TODO: Do this properly
		std::this_thread::sleep_for(std::chrono::milliseconds(3000));
		system(webhookCommand.c_str());
	}
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
