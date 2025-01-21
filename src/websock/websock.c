#include "websock.h"
#include "websock_priv.h"

static Websock_t sock;

static WebsockConn_t *getWebconnFromConnId(ws_cli_conn_t id)
{
	WebsockConn_t *conn = NULL;
	LIST_FOR_EACH(conn, &sock.lConnections, link)
	{
		if(conn->conn == id)
		{
			return conn;
		}
	}

	return NULL;
}

static void onopen(ws_cli_conn_t client)
{
    WebsockConn_t *conn = calloc(1, sizeof(WebsockConn_t));
	listInsert(&sock.lConnections, &conn->link);
	conn->conn = client;
	sock.itf->NewConn(conn, sock.udata);
}


static void onclose(ws_cli_conn_t  client)
{
	WebsockConn_t *conn = getWebconnFromConnId(client);
	if(NULL == conn)
	{
		if(conn->itf != NULL && conn->itf->Close != NULL)
		{
			conn->itf->Close(conn, conn->udata);
		}
		listRemove(&conn->link);
		free(conn);
	}
}


static void onmessage(ws_cli_conn_t client,
	const unsigned char *msg, uint64_t size, int type)
{
	((void)type);
	WebsockConn_t *conn = getWebconnFromConnId(client);
	if(conn->itf != NULL && conn->itf->OnData != NULL)
	{
		conn->itf->OnData(conn, msg, size, conn->udata);
	}
}

Websock_t * websockCreate(WebsockConfig_t *config, WebsockInterface_t *itf, void *udata)
{
	if(!sock.configured)
	{
		memcpy(&sock.config, config, sizeof(WebsockConfig_t));
		sock.itf = itf;
		sock.udata = udata;
		listInit(&sock.lConnections);

		ws_socket(&(struct ws_server){
			.host = "0.0.0.0",
			.port = config->port,
			.thread_loop   = 1,
			.timeout_ms    = 1000,
			.evs.onopen    = &onopen,
			.evs.onclose   = &onclose,
			.evs.onmessage = &onmessage
		});

		sock.configured = true;
	}

    return &sock;
}

void websockDestroy(Websock_t *sock)
{
	WebsockConn_t *conn = NULL, *_conn = NULL;
	LIST_FOR_EACH_SAFE(conn, _conn, &sock->lConnections, link)
	{
		listRemove(conn);
		ws_close_client(conn->conn);
		free(conn);
	}
}

int websockConnSend(WebsockConn_t *conn, uint8_t *data, int len)
{
	return ws_sendframe_bin(conn->conn, data, len);
}

void websockConnSetInterface(WebsockConn_t *conn, WebsockConnInterface_t *itf, void *udata)
{
	conn->itf = itf;
	conn->udata = udata;
}