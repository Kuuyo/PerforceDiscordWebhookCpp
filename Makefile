CC=gcc
LDFLAGS = -LPerforceDiscordWebhookCpp/lib
LIBS = -lclient -lrpc -lsupp

PerforceDiscordWebhookCpp: PerforceDiscordWebhookCpp/PerforceDiscordWebhookCpp.o
	$(CC) -o PerforceDiscordWebhook PerforceDiscordWebhookCpp/PerforceDiscordWebhookCpp.o $(LIBS)