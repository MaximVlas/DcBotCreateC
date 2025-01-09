#include "discord_bot.h"
#include <libwebsockets.h>
#include <signal.h>

static volatile int force_exit = 0;

static void sigint_handler(int sig) {
    force_exit = 1;
}

int main() {
    signal(SIGINT, sigint_handler);
    
    DiscordBot *bot = discord_bot_create("YOUR DISCORD BOT TOKEN");
    if (!bot) {
        fprintf(stderr, "Failed to create bot.\n");
        return 1;
    }

    if (discord_bot_connect(bot) != 0) {
        fprintf(stderr, "Failed to connect bot.\n");
        discord_bot_destroy(bot);
        return 1;
    }

    while (!force_exit) {
        lws_service(bot->context, 50);
        
        usleep(10000); 
    }

    printf("Shutting down bot...\n");
    if (bot->context) {
        lws_context_destroy(bot->context);
    }
    discord_bot_destroy(bot);
    return 0;
}
