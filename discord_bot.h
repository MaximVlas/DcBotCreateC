// discord_bot.h

#pragma once
#include <libwebsockets.h>
#include <json-c/json.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>

#define DISCORD_GATEWAY_URL "gateway.discord.gg"
#define DISCORD_GATEWAY_PATH "/?v=10&encoding=json"
#define LCCSCF_USE_SSL (1 << 0)
#define LCCSCF_SKIP_SERVER_CERT_HOSTNAME_CHECK (1 << 1)

typedef struct {
    char *token;
    struct lws_context *context;
    struct lws *websocket;
    void (*on_message)(const char *content);
    int is_connected;
} DiscordBot;

struct PerSessionData {
    struct lws *wsi;
    DiscordBot *bot;
    int first_connect;
};



// Function declarations
DiscordBot* discord_bot_create(const char* token);
int discord_bot_connect(DiscordBot* bot);
void discord_bot_destroy(DiscordBot* bot);
void send_heartbeat(struct lws* wsi);
// In discord_bot.h
extern void handle_message(struct lws *wsi, const char *channel_id, const char *content);
extern void handle_command(struct lws *wsi, const char *channel_id, const char *content);


void send_message(struct lws *wsi, const char *channel_id, const char *content);
