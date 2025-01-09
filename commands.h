#ifndef COMMANDS_H
#define COMMANDS_H

#include <libwebsockets.h> 

void handle_command(struct lws *wsi, const char *channel_id, const char *content);
void handle_message(struct lws *wsi, const char *channel_id, const char *content);
void send_message(struct lws *wsi, const char *channel_id, const char *content);

#endif // COMMANDS_H
