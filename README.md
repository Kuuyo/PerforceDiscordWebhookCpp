# PerforceDiscordWebhookCpp
C++ console application to send the latest Perforce submits to Discord through a webhook, intended to run on Heroku

## Some information about how this works
The bot
- is written using the [C/C++ Perforce API](https://www.perforce.com/downloads/helix-core-c/c-api) | [Docs](https://www.perforce.com/manuals/p4api/Content/P4API/Home-p4api.html)
- is intended to run on [Heroku](https://www.heroku.com/)
- does also work on windows as long as the Environment Variables listed below are set up on the system | [How do I do that?](http://www.dowdandassociates.com/blog/content/howto-set-an-environment-variable-in-windows/)
- fetches the latest 5 changes every 3 minutes (I wanted it to be 5 minutes, but then the bot perma-sleeps on Heroku)
- writes a simple "cache file" called `cl.txt` with the latest update(s) it sent
- compares what it found to this cache file to know if there are any unsynced changes
- will process and parse the information of the changes to send to Discord through a webhook
- **will generate a .html if the change is an edit, with the information of the standard diff2 of Perforce**
- **uses a Github repo that has Github Pages enabled to upload these htmls to**
  - **_So you will need a GitHub repo that the bot can manage this on_**

## Github repo with Github Pages
[How do I do this?](https://pages.github.com/)

I recommend you add a stylesheet called 'styles.css', [this is the one I wrote to use in mine](PerforceDiscordWebhookCpp/Diffs/styles.css)

## How to set up Heroku for this bot
1. Add this buildpack -> https://github.com/heroku/heroku-buildpack-c
2. Create the required Config Vars
    - DISCORDWEBHOOK : The discord webhook you want to send the messages to
    - EMBEDTHUMB : A url to an image to use in the embed as thumbnail
    - EMBEDURL : A url which the title of the embed will link to
    - GITHUBPAGESFULLPATH : The full path of the Github Pages repo you upload the generated HTMLs to
      - e.g. `http://username.github.io/repository/`
    - GITHUBREPO : The url to the repo itself
      - e.g. `https://github.com/username/repository.git`
    - GITHUBTOKEN : Your github password or a [token](https://github.com/settings/tokens)
      - I use a private repo for my generated html repo, so I needed this
    - GITHUBUSER : Your github username
    - P4AVATAR : The avatar your bot will use
    - P4CLIENT : The Perforce client workspace for this bot (I would advice you to make a workspace specifically for this bot)
    - P4FILTERPATH : A path the bot filters to when it looks for changelists
      - e.g. `//cool_project/...` -> Will only get changelists that affected cool_project
    - P4HOST : The "host" that will be using the workspace (Make sure this is the same name as who you set up the workspace for)
    - P4PORT : The port of the Perforce server
    - P4SECRET : Your Perforce password
    - P4USER : Your Perforce username
    - USEREMAIL : Your git email
      - This and the next are used so git knows who you are
      - It is basically your git config user.name and user.email
    - USERFULLNAME : Your full name
	- USERCOUNT : Number of users that have an icon (remember to fill in 0 if you don't want to use this feature)

3. If you want to give your team members an icon (THESE ARE THE ONLY ONES NOT REQUIRED)
    - USER1 : Perforce username of one of the changelist authors
	- USER2 : Perforce username of one of the changelist authors
    - U1ICON : An image for user 1
	- U2ICON : An image for user 2

Credits:
- Some of the code is recycled from my previous two attempts at accomplishing this in [Ruby](https://github.com/Kuuyo/PerforceDiscordWebhook) and [.NET](https://github.com/Kuuyo/PerforceDiscordWebhookNET)
### Include folder contains [headers supplied by Perforce](PerforceDiscordWebhookCpp/include/p4) from [C/C++ API](https://www.perforce.com/downloads/helix-core-c/c-api)
### Include folder also contains [json.hpp](PerforceDiscordWebhookCpp/include/json.hpp) by [nlohmann](https://github.com/nlohmann/)
### Lib folder contains [libs supplied by Perforce](PerforceDiscordWebhookCpp/lib) from [C/C++ API](https://www.perforce.com/downloads/helix-core-c/c-api)
