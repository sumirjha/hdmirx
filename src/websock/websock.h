#ifndef _WEBSOCK_H_
#define _WEBSOCK_H_

#include <stdint.h>

struct Websock;
struct WebsockConfig;
struct WebsockInterface;
struct WebsockConn;
struct WebsockConnInterface;


typedef struct Websock                  Websock_t;
typedef struct WebsockConfig            WebsockConfig_t;
typedef struct WebsockInterface         WebsockInterface_t;
typedef struct WebsockConn              WebsockConn_t;
typedef struct WebsockConnInterface     WebsockConnInterface_t;


struct WebsockInterface
{
    void (*NewConn)(WebsockConn_t *conn, void *udata);
};

struct WebsockConnInterface
{
    void (*OnData)(WebsockConn_t *conn, uint8_t *data, int size, void *udata);
    void (*Close)(WebsockConn_t *conn, void *udata);
};

struct WebsockConfig
{
    uint16_t    port;
};




Websock_t * websockCreate(WebsockConfig_t *config, WebsockInterface_t *itf, void *udata);

void websockDestroy(Websock_t *sock);

int websockConnSend(WebsockConn_t *conn, uint8_t *data, int len);

void websockConnSetInterface(WebsockConn_t *conn, WebsockConnInterface_t *itf, void *udata);

#endif