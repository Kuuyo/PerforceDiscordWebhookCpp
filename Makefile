CC=g++
LIBS = -LPerforceDiscordWebhookCpp/lib -lclient -lrpc -lsupp -lssl -lcrypto
DEPS = PerforceDiscordWebhookCpp/helpers.h

PDWCpp: PerforceDiscordWebhookCpp/main.o HerokuSSLFix/libstub.a $(DEPS)
	$(CC) -o PDW PerforceDiscordWebhookCpp/main.o $(LIBS) -LHerokuSSLFix/ -lstub
	
HerokuSSLFix/libstub.a : HerokuSSLFix/libstub.o
	ar rcs $@ $^
	
HerokuSSLFix/libstub.o: HerokuSSLFix/stub.cpp HerokuSSLFix/openssl/ssl.h
	$(CC) -c -o $@ $<