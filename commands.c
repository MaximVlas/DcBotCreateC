#include "discord_bot.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

// Command handler function
// You can add your discord commands here
// I need to fix the !help command
void handle_command(struct lws *wsi, const char *channel_id, const char *content) {
    if (content[0] != '!') return;
    content++;

    char command[32];
    int i = 0;
    while (content[i] && content[i] != ' ' && i < 31) {
        command[i] = content[i];
        i++;
    }
    command[i] = '\0';

    char response[1024];
    if (strcmp(command, "ping") == 0) {
        snprintf(response, sizeof(response), "Pong!");
        send_message(wsi, channel_id, response);
    }
    else if (strcmp(command, "hello") == 0) {
        snprintf(response, sizeof(response), "Hello there!");
        send_message(wsi, channel_id, response);
    }
    else if (strcmp(command, "help") == 0) {
        snprintf(response, sizeof(response),
            "Available commands:\n"
            "!ping - Check if bot is alive\n"
            "!hello - Get a greeting\n"
            "!help - Show this help message\n"
            "!roll - Roll a random number (1-100)\n"
            "!fact - Get a random fact\n"
            "!time - Show the server time\n"
            "!echo <message> - Repeat your message");
        send_message(wsi, channel_id, response);
    }
    else if (strcmp(command, "roll") == 0) {
        srand(time(NULL));
        int roll = (rand() % 100) + 1;
        snprintf(response, sizeof(response), "🎲 You rolled a %d!", roll);
        send_message(wsi, channel_id, response);
    }
    else if (strcmp(command, "fact") == 0) {
        const char *facts[] = {
            "EXAMPLE FACT",
            "EXAMPLE FACT2"

        };
        srand(time(NULL));
        int fact_index = rand() % (sizeof(facts) / sizeof(facts[0]));
        snprintf(response, sizeof(response), "📚 Fact: %s", facts[fact_index]);
        send_message(wsi, channel_id, response);
    }
    else if (strcmp(command, "time") == 0) {
        time_t now = time(NULL);
        struct tm *t = localtime(&now);
        strftime(response, sizeof(response), "⏰ Current server time: %Y-%m-%d %H:%M:%S", t);
        send_message(wsi, channel_id, response);
    }
    else if (strcmp(command, "echo") == 0) {
        const char *message_start = content + i + 1;
        if (strlen(message_start) > 0) {
            snprintf(response, sizeof(response), "You said: %s", message_start);
        } else {
            snprintf(response, sizeof(response), "Please provide a message to echo!");
        }
        send_message(wsi, channel_id, response);
    }
    else {
        snprintf(response, sizeof(response), "Unknown command: %s. Try !help for a list of commands.", command);
        send_message(wsi, channel_id, response);
    }
}
// Messange handler function
void handle_message(struct lws *wsi, const char *channel_id, const char *content) {
    if (!content || !channel_id) return;
    printf("Received message: %s\n", content);
    
    if (content[0] == '!') {
        handle_command(wsi, channel_id, content);
    }
}
