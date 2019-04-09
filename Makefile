CC=g++
LIBS = -LPerforceDiscordWebhookCpp/lib -lclient -lrpc -lsupp -lssl -lcrypto -lrt

PerforceDiscordWebhookCpp: PerforceDiscordWebhookCpp/PerforceDiscordWebhookCpp.o
	$(CC) -o PerforceDiscordWebhook PerforceDiscordWebhookCpp/PerforceDiscordWebhookCpp.o $(LIBS)