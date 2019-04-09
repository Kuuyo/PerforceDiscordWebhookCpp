CC=gcc
LIBS = -LPerforceDiscordWebhookCpp/lib -lclient -lrpc -lsupp

PerforceDiscordWebhookCpp: PerforceDiscordWebhookCpp/PerforceDiscordWebhookCpp.o
	$(CC) -o PerforceDiscordWebhook PerforceDiscordWebhookCpp/PerforceDiscordWebhookCpp.o $(LIBS)