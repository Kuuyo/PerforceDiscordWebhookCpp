CC=g++
LIBS = -LPerforceDiscordWebhookCpp/lib -lclient -lrpc -lsupp -lssl -lcrypto

PDWCpp: PerforceDiscordWebhookCpp/PerforceDiscordWebhookCpp.o StaticLib1/libstub.a
	$(CC) -o PDW PerforceDiscordWebhookCpp/PerforceDiscordWebhookCpp.o $(LIBS) -LStaticLib1/ -lstub
	
StaticLib1/libstub.a : StaticLib1/libstub.o
	ar rcs $@ $^
	
StaticLib1/libstub.o: StaticLib1/stub.cc StaticLib1/openssl/ssl.h
	$(CC) -c -o $@ $<