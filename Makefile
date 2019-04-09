CC=g++
LIBS = -LPerforceDiscordWebhookCpp/lib -lclient -lrpc -lsupp -lssl -lcrypto

PDWCpp: PerforceDiscordWebhookCpp/PerforceDiscordWebhookCpp.o HerokuSSLFix/libstub.a
	$(CC) -o PDW PerforceDiscordWebhookCpp/PerforceDiscordWebhookCpp.o $(LIBS) -LHerokuSSLFix/ -lstub
	
HerokuSSLFix/libstub.a : HerokuSSLFix/libstub.o
	ar rcs $@ $^
	
HerokuSSLFix/libstub.o: HerokuSSLFix/stub.cpp HerokuSSLFix/openssl/ssl.h
	$(CC) -c -o $@ $<