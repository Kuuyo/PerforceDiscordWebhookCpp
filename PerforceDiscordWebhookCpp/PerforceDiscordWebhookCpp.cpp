#include <iostream>

#include "include/p4/clientapi.h"

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

int main()
{
	ClientUser ui;
	ClientApi client;
	StrBuf msg;
	Error e;

	// Any special protocol mods

	//client.SetProtocol( "tag", "");

	// Connect to server

	client.Init(&e);
	char    *argv1[] = { (char*)"-a" };
	client.SetArgv(1, argv1);
	client.Run("login", &ui);

	std::cout << client.GetPassword().Text() << std::endl;

	if (e.Test())
	{
		e.Fmt(&msg);
		fprintf(stderr, "%s\n", msg.Text());
		return 1;
	}


	char    *argv[] = {  (char*)"-l", (char*)"-m", (char*)"5", (char*)"-s", (char*)"submitted", (char*)"-t" };
	int     argc = 6;
	client.SetArgv(argc, argv);
	client.Run("changes", &ui);
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