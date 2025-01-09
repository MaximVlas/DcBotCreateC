#include "discord_bot.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <json-c/json.h>
#include <time.h>
#include <unistd.h>
#include "commands.h"

static int heartbeat_interval = 0; 
static int last_sequence_number = -1; 

void send_heartbeat(struct lws *wsi) {
    char heartbeat[128];
    snprintf(heartbeat, sizeof(heartbeat), 
             "{\"op\":1,\"d\":%d}", 
             last_sequence_number);
    
    unsigned char buf[LWS_PRE + 128];
    unsigned char *payload = &buf[LWS_PRE];
    size_t len = strlen(heartbeat);

    memcpy(payload, heartbeat, len);
    lws_write(wsi, payload, len, LWS_WRITE_TEXT);
    printf("Sent heartbeat\n");
}

void send_message(struct lws *wsi, const char *channel_id, const char *content) {
    struct PerSessionData *psd = (struct PerSessionData *)lws_wsi_user(wsi);
    if (!psd || !psd->bot || !psd->bot->token) {
        fprintf(stderr, "Error: Invalid bot data in send_message\n");
        return;
    }

    char url[256];
    snprintf(url, sizeof(url), 
        "https://discord.com/api/v10/channels/%s/messages", channel_id);
    
    char message[1024];
    snprintf(message, sizeof(message),
        "{\"content\":\"%s\"}", content);
    
    struct curl_slist *headers = NULL;
    char auth_header[1024];
    snprintf(auth_header, sizeof(auth_header), 
        "Authorization: Bot %s", psd->bot->token);
    
    headers = curl_slist_append(headers, "Content-Type: application/json");
    headers = curl_slist_append(headers, auth_header);
    
    CURL *curl = curl_easy_init();
    if(curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, message);
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_perform(curl);
        curl_easy_cleanup(curl);
    }
    
    curl_slist_free_all(headers);
}




static time_t last_heartbeat_time = 0;
static int heartbeat_sent = 0;

static int callback_discord(struct lws *wsi, enum lws_callback_reasons reason,
                            void *user, void *in, size_t len) {
    struct PerSessionData *psd = (struct PerSessionData *)user;
    unsigned char buf[LWS_PRE + 2048];
    unsigned char *payload = NULL;
    time_t current_time;

    switch (reason) {
        case LWS_CALLBACK_PROTOCOL_INIT:
            fprintf(stderr, "Protocol initialized.\n");
            break;

        case LWS_CALLBACK_CLIENT_ESTABLISHED:
            printf("Connected to Discord Gateway!\n");
            psd->first_connect = 1;
            lws_callback_on_writable(wsi);
            break;

        case LWS_CALLBACK_CLIENT_RECEIVE:
            if (in && len > 0) {
                struct json_object *response = json_tokener_parse((const char *)in);
                if (!response) {
                    fprintf(stderr, "Error parsing JSON response.\n");
                    break;
                }

                struct json_object *op_value;
                if (json_object_object_get_ex(response, "op", &op_value)) {
                    int op_code = json_object_get_int(op_value);

                    switch (op_code) {
                        case 10: { // HELLO
                            struct json_object *data;
                            if (json_object_object_get_ex(response, "d", &data)) {
                                struct json_object *interval;
                                if (json_object_object_get_ex(data, "heartbeat_interval", &interval)) {
                                    heartbeat_interval = json_object_get_int(interval);
                                    printf("Heartbeat interval set to %d ms\n", heartbeat_interval);

                                    // Send IDENTIFY payload immediately after HELLO
                                    char identify[2048];
                                    int intents = (1 << 0) | (1 << 1) | (1 << 9) | (1 << 12) | (1 << 15); // Add necessary intents

                                    int identify_len = snprintf(
                                        identify, sizeof(identify),
                                        "{\"op\":2,\"d\":{"
                                        "\"token\":\"%s\","
                                        "\"intents\":%d,"
                                        "\"properties\":{"
                                        "\"os\":\"linux\","
                                        "\"browser\":\"discord_bot\","
                                        "\"device\":\"discord_bot\""
                                        "},"
                                        "\"presence\":{"
                                        "\"status\":\"online\","
                                        "\"activities\":[{\"name\":\"Discord\",\"type\":0}],"
                                        "\"since\":null,"
                                        "\"afk\":false"
                                        "}"
                                        "}}",
                                        psd->bot->token,
                                        intents);

                                    payload = &buf[LWS_PRE];
                                    memcpy(payload, identify, identify_len);
                                    lws_write(wsi, payload, identify_len, LWS_WRITE_TEXT);

                                    // Schedule first heartbeat
                                    send_heartbeat(wsi);
                                    time(&last_heartbeat_time);
                                }
                            }
                            break;
                        }
                        case 11: // HEARTBEAT ACK
                            printf("Heartbeat acknowledged\n");
                            heartbeat_sent = 0;
                            break;

                        case 0: { // DISPATCH
                            struct json_object *seq_number, *event_name;
                            if (json_object_object_get_ex(response, "s", &seq_number)) {
                                last_sequence_number = json_object_get_int(seq_number);
                            }
                            if (json_object_object_get_ex(response, "t", &event_name)) {
                                const char *event = json_object_get_string(event_name);
                                if (event && strcmp(event, "READY") == 0) {
                                    printf("Bot is now online!\n");
                                    psd->bot->is_connected = 1;
                                }
                                if (strcmp(event, "MESSAGE_CREATE") == 0) {
                                    struct json_object *data;
                                    if (json_object_object_get_ex(response, "d", &data)) {
                                        struct json_object *content, *channel_id;
                                        if (json_object_object_get_ex(data, "content", &content) &&
                                            json_object_object_get_ex(data, "channel_id", &channel_id)) {
                                            const char *message_content = json_object_get_string(content);
                                            const char *message_channel_id = json_object_get_string(channel_id);

                                            if (message_content && message_channel_id) {
                                                printf("Dispatching message: %s\n", message_content);
                                                handle_message(wsi, message_channel_id, message_content);
                                            }
                                        }
                                    }
                                }

                            }
                            break;
                        }
                    }
                }
                json_object_put(response); // Free parsed JSON object
            }
            break;

        case LWS_CALLBACK_CLIENT_WRITEABLE:
            time(&current_time);
            if (heartbeat_interval > 0 &&
                (current_time - last_heartbeat_time) * 1000 >= heartbeat_interval) {
                send_heartbeat(wsi);
                time(&last_heartbeat_time);
            }
            lws_callback_on_writable(wsi); // Schedule next writable callback
            break;

        case LWS_CALLBACK_WSI_DESTROY:
            if (psd->bot) {
                psd->bot->is_connected = 0; // Mark bot as disconnected
            }
            break;

        default:
            break;
    }

    return 0;
}


static struct lws_protocols protocols[] = {
    {
        .name = "json", 
        .callback = callback_discord,
        .per_session_data_size = sizeof(struct PerSessionData),
        .rx_buffer_size = 65536,
        .tx_packet_size = 65536,
    },
    { NULL, NULL, 0, 0 } 
};



DiscordBot* discord_bot_create(const char* token) {
    if (!token || strlen(token) == 0) {
        fprintf(stderr, "Error: Invalid token\n");
        return NULL;
    }

    DiscordBot* bot = malloc(sizeof(DiscordBot));
    if (!bot) {
        fprintf(stderr, "Error: Memory allocation failed\n");
        return NULL;
    }

    memset(bot, 0, sizeof(DiscordBot));
    bot->token = strdup(token); // Ensure the token is copied
    if (!bot->token) {
        fprintf(stderr, "Error: Token memory allocation failed\n");
        free(bot);
        return NULL;
    }

    bot->on_message = NULL; // Initialize function pointers to NULL
    return bot;
}




int discord_bot_connect(DiscordBot *bot) {
    if (!bot || !bot->token) {
        fprintf(stderr, "Error: Invalid bot or bot token\n");
        return -1;
    }

    struct lws_context_creation_info info = {0};
    info.port = CONTEXT_PORT_NO_LISTEN;
    info.protocols = protocols;
    info.gid = -1;
    info.uid = -1;
    info.options = LWS_SERVER_OPTION_DO_SSL_GLOBAL_INIT | 
                   LWS_SERVER_OPTION_PEER_CERT_NOT_REQUIRED;
    info.ssl_cert_filepath = NULL;
    info.ssl_private_key_filepath = NULL;
    info.ssl_cipher_list = "HIGH:!PSK:!RSP:!eNULL:!aNULL:!RC4:!MD5:!DES:!3DES:!aDH:!kDH:!DSS";
    info.ka_time = 10;
    info.ka_interval = 10;
    info.ka_probes = 3;

    bot->context = lws_create_context(&info);
    if (!bot->context) {
        fprintf(stderr, "Error: Failed to create LWS context\n");
        return -1;
    }

    struct PerSessionData *psd = calloc(1, sizeof(struct PerSessionData));
    if (!psd) {
        fprintf(stderr, "Error: Memory allocation failed\n");
        lws_context_destroy(bot->context);
        return -1;
    }
    psd->bot = bot;

    struct lws_client_connect_info ccinfo = {0};
    ccinfo.context = bot->context;
    ccinfo.address = "gateway.discord.gg";
    ccinfo.port = 443;
    ccinfo.path = "/?v=10&encoding=json";
    ccinfo.host = "gateway.discord.gg";
    ccinfo.origin = "https://gateway.discord.gg";
    ccinfo.protocol = "json";
    ccinfo.userdata = psd;
    ccinfo.ssl_connection = LCCSCF_USE_SSL | 
                           LCCSCF_ALLOW_SELFSIGNED | 
                           LCCSCF_SKIP_SERVER_CERT_HOSTNAME_CHECK;
    ccinfo.ietf_version_or_minus_one = -1;
    ccinfo.local_protocol_name = protocols[0].name;

    psd->wsi = lws_client_connect_via_info(&ccinfo);
    if (!psd->wsi) {
        fprintf(stderr, "Error: Failed to connect to Discord Gateway\n");
        free(psd);
        lws_context_destroy(bot->context);
        return -1;
    }

    return 0;
}


// Discord bot destroy function
void discord_bot_destroy(DiscordBot *bot) {
    if (!bot) return;

    if (bot->token) {
        free(bot->token);
    }

    free(bot);
}

