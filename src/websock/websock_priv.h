#include "websock.h"
#include "ws.h"
#include "list_common.h"

struct Websock
{
    bool                configured;
    struct ws_server    ws;
    WebsockConfig_t     config;
    WebsockInterface_t  *itf;
    void                *udata;
    List_t              lConnections;
};


struct WebsockConn
{
    ws_cli_conn_t           conn;
    WebsockConnInterface_t  *itf;
    void                    *udata;
    List_t                  link;
};