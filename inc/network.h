#ifndef __NETWORK_H__
#define __NETWORK_H__


#include <stdint.h>
#include <pthread.h>
#include <list_common.h>
#include "common.h"
#include "buffer.h"

struct Net;
struct NetCon;
struct NetConfig;
struct NetConConfig;
struct NetInterface;
struct NetConInterface;

typedef struct Net                  Net_t;
typedef struct NetConfig            NetConfig_t;
typedef struct NetConConfig         NetConConfig_t;
typedef struct NetCon               NetCon_t;
typedef struct NetInterface         NetInterface_t;
typedef struct NetConInterface      NetConInterface_t;

struct NetInterface
{
    void (*NewClient)(NetCon_t *con, void *udata);
};


struct NetConInterface
{
    void (*Close)(NetCon_t *con, void *udata);
};

struct NetCon
{
    char                name[8];
    int                 fd;
    NetConInterface_t   *itf;
    void                *udata;
    uint8_t             *buffer;
    List_t              lSend;
    List_t              lFree;
    pthread_t           senderThread;
    pthread_mutex_t     lock;
    bool                running;
    bool                destroyed;
    List_t              link;
};

struct NetConfig
{
    uint16_t port;
};

struct Net
{
    int                 fd;
    NetConfig_t         config;
    NetInterface_t      *itf;
    void                *udata;
};


struct NetConConfig
{
    int bufferCount;
};

/****************************************************************************** */
/**************************** Network Server APIs ***************************** */
/****************************************************************************** */

/**
 * Create Network
 */
Net_t *netCreate(NetConfig_t *config, NetInterface_t *itf, void *udata);


/**
 * Destroy Network
 */
void netDestroy(Net_t *n);


/**
 * Get Network Fd to add into poll
 */
int netGetFd(Net_t *n);


/**
 * Dispatch for incomming connection
 */
void netDispatch(Net_t *n);


/****************************************************************************** */
/**************************** Network Connection APIs ************************* */
/****************************************************************************** */

void netConnInit(NetCon_t *con, NetConInterface_t *itf, void *udata);

/**
 * Close a network connection
 */
void netConClose(NetCon_t *con);


/**
 * Send data over a network connection
 */
int netConSend(NetCon_t *n, NetBuffer_t *buf);


/**
 * Get Network Fd to add into poll
 */
int netConRecv(Net_t *n, uint8_t *buffer, int capacity);


#endif