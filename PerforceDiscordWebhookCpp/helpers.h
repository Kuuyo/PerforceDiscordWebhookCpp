#pragma once
// Some helpers

#pragma region HelperStructs
struct FileData
{
	std::string fileName;
	uint32_t revision;
	std::string action; // TODO: Replace by an enum?
	std::string type; // TODO: Replace by an enum?

	bool IsFirstRevision() { return revision == 1; }

	std::string GetCurrentRevString() { return fileName + '#' + std::to_string(revision); }
	std::string GetStringNoPath()
	{
		std::string s = fileName + '.' + std::to_string(revision);
		return s.substr(s.find_last_of('/') + 1);
	}

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
#pragma endregion


#pragma region HelperFunctions
// Gets the Environment Variable by name
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

// Creates a command "git -C $path $args"
std::string GitCommandHelper(std::string path, const std::string &args)
{
	std::string command("git -C ");
	path.pop_back();
	command.append(path);
	command.push_back(' ');
	command.append(args);
	return command;
}

// Read in a file as binary
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

// Writes a file in binary form
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

// Writes a file in binary form
void WriteFile(const std::string &data, const std::string &fileName)
{
	std::ofstream file(fileName, std::ios::binary);

	if (!file.is_open())
	{
		std::cout << "Could not create file: " << fileName << "\n";
		exit(-1);
	}

	std::vector<char> newFile;

	for (const auto &c : data)
		newFile.push_back(c);

	file.write(newFile.data(), newFile.size());

	file.close();
}

// Extracts the second capture group of the regex given
// Adds a '\n' between each entry
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
// Adds a '\n' between each entry
void ExtractSingleLineData(const std::string &data, const std::regex &rgx, std::string &outData)
{
	std::vector<std::string> temp;
	ExtractSingleLineData(data, rgx, temp);

	outData = temp[0];
	outData.pop_back();
}

// The regex given is the "header" you search for, splits data into a vector according to this header
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

// Searches the data given with the given regex and returns a vector with what it found from the second capture group
// Does not add a '\n' between each entry
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

// Searches the data given with the given regex and returns a string with what it found from the second capture group
// Does not add a a '\n' between each entry
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

// Returns the user image for the user
std::string GetUserImage(const std::string &user)
{
	if (user == GetEnv("USER1"))
	{
		return GetEnv("U1ICON");
	}
	else if (user == GetEnv("USER2"))
	{
		return GetEnv("U2ICON");
	}
	else if (user == GetEnv("USER3"))
	{
		return GetEnv("U3ICON");
	}
	else if (user == GetEnv("USER4"))
	{
		return GetEnv("U4ICON");
	}
	else if (user == GetEnv("USER5"))
	{
		return GetEnv("U5ICON");
	}
	else
	{
		std::cout << "User not found!: " << user << std::endl;
		return std::string("");
	}
}

// Returns a color in decimal form according to what actions were carried out on the files
int32_t GetColor(const Changelist &cl)
{
	bool bHasEdit = false;
	bool bHasAdd = false;
	bool bHasDelete = false;

	for (const auto& file : cl.files)
	{
		if (file.action == "edit")
			bHasEdit = true;
		else if (file.action == "add")
			bHasAdd = true;
		else
			bHasDelete = true;
	}

	// Discord embed uses decimal colours, so I thought this was a pretty cool way to solve that
	return ((bHasEdit ? 1 : 0) * 255) + ((bHasAdd ? 1 : 0) * 65280) + ((bHasDelete ? 1 : 0) * 16711680);
}

// Returns a color according to what action this file underwent
int32_t GetColor(const FileData &file)
{
	if (file.action == "edit")
		return 255;
	else if (file.action == "add")
		return 65280;
	else
		return 16711680;
}
#pragma endregion
