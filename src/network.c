#include "network.h"
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <netinet/in.h>
#include <netdb.h>
#include <signal.h>
#include <time.h>


/****************************************************************************** */
/**************************** Network Server APIs ************************* */
/****************************************************************************** */


Net_t *netCreate(NetConfig_t *config, NetInterface_t *itf, void *udata)
{
    Net_t *n = calloc(1, sizeof(NetCon_t));
    memcpy(&n->config, config, sizeof(NetConfig_t));
    n->itf = itf;
    n->udata = udata;

    int ret;
    struct sockaddr_in servaddr;

    do
    {
        //Intialise TCP Server
        n->fd = socket(AF_INET, SOCK_STREAM, 0);
        OKAY_STOP(n->fd <= 0, "failed to create socket : errorno (%d)\n", errno);

        setsockopt(n->fd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int));

        bzero(&servaddr, sizeof(servaddr));
        // assign IP, PORT 
        servaddr.sin_family = AF_INET;
        servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
        servaddr.sin_port = htons(config->port);

        ret = fcntl(n->fd, F_SETFL, O_NONBLOCK);
        OKAY_STOP(ret != 0, "failed to set N_BLOCK with errno %d\n", errno);

        ret = bind(n->fd, (struct sockaddr *)&servaddr, sizeof(servaddr));
        OKAY_STOP(ret != 0, "failed to find tcp at :%d, with errno %d\n", n->config.port, errno);
            
        ret = listen(n->fd, 5);
        OKAY_STOP(ret != 0, "failed to listen tcp at :%d, with errno %d\n", n->config.port, errno);

        return n;

    } while (false);

    if(n->fd > 0)
    { close(n->fd); }

    if(NULL == n)
    { free(n); }

    return NULL;
}


void netDestroy(Net_t *n)
{
    //Close fd
    if(n->fd > 0)
    { close(n->fd); }

    free(n);
}


int netGetFd(Net_t *n)
{
    return n->fd;
}

void netDispatch(Net_t *n)
{
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    int fd = accept(n->fd, (struct sockaddr *)&address, (socklen_t*)&addrlen);
    if(fd < 0)
    { return; }

    NetCon_t * con = calloc(1, sizeof(NetCon_t));
    con->fd = fd;
    n->itf->NewClient(con, n->udata);
}

/****************************************************************************** */
/**************************** Network Connection APIs ************************* */
/****************************************************************************** */

static void *senderThread(void *args)
{
    int bytesSend = 0;
    struct timespec last;
    signal(SIGPIPE, SIG_IGN);
    clock_gettime(CLOCK_REALTIME, &last);
    NetCon_t *con = args;
    while (con->running)
    {
        NetBuffer_t *buf = NULL;
        pthread_mutex_lock(&con->lock);
        LIST_PEEK_FRONT(buf, &con->lSend, link);
        pthread_mutex_unlock(&con->lock);
        
        if(buf == NULL)
        { 
            usleep(1000);
            continue;
        }

        //Send the packet and put it back into lFree
        int ret = write(con->fd, buf->buffer, buf->size);
        if(ret < 0)
        { 
            if(errno == EWOULDBLOCK)
            { continue; }
            else
            {
                con->destroyed = true;
                con->running = false; 
                break;
            }
        }
        else
        {
            buf->offset += ret;
            bytesSend += ret;
            if(buf->offset >= buf->size)
            {
                //Put the list back into free list
                pthread_mutex_lock(&con->lock);
                listRemove(&buf->link);
                listInsert(&con->lFree, &buf->link);
                pthread_mutex_unlock(&con->lock);
            }
        }

        struct timespec now;
        double start_sec, end_sec, elapsed_sec;
        clock_gettime(CLOCK_REALTIME, &now);

        end_sec = now.tv_sec + now.tv_nsec / NANO_PER_SEC;
        start_sec = last.tv_sec + last.tv_nsec / NANO_PER_SEC;
        elapsed_sec = end_sec - start_sec;

        if(elapsed_sec >= 1)
        {
            last = now;
            printf(" [%s] Bytes/Sec : %d\n", con->name, bytesSend);
            bytesSend = 0;
        }
    }
    
}

char *tab_alphabet="abcdefghijklmnopqrstuvwxyz";

int nombreAlea(int min, int max)
{
    return (rand()%(max-min+1) + min);
}
void genererName(int length, char n[])
{
    int i ;
    for (i=0;i<length;i++){
        int k = nombreAlea(1,26);// from the table of the alphabet
        n[i] = tab_alphabet[k-1];
    }
    n[i] = '\0';
}

void netConnInit(NetCon_t *con, NetConInterface_t *itf, void *udata)
{
    listInit(&con->lSend);
    listInit(&con->lFree);
    
    con->itf = itf;
    con->udata = udata;
    con->running = true;
    con->destroyed = false;

    memset(con->name, 0, sizeof(con->name));
    genererName(sizeof(con->name) - 1, con->name); 
    con->buffer = calloc(sizeof(NetBuffer_t), TS_TOTAL_PACKET);
	OKAY_RETURN(con->buffer == NULL, , "failed to allocate network buffer\n");
	for(int i =0; i < TS_TOTAL_PACKET; i++)
	{
		NetBuffer_t *buf = (NetBuffer_t *)((uint8_t *)con->buffer + (i * sizeof(NetBuffer_t)));
		buf->size = 0;
		buf->capacity = TS_PACKET_SIZE;
		listInsert(&con->lFree, &buf->link);
	}

    pthread_mutex_init(&con->lock, NULL);
    pthread_create(&con->senderThread, NULL, senderThread, con);
}


void netConClose(NetCon_t *con)
{
    con->running = true;
    if(con->fd > 0)
    { close(con->fd); }
    pthread_join(con->senderThread, NULL);
    pthread_mutex_lock(&con->lock);
    NetBuffer_t *buf = NULL, *_buf = NULL;
    LIST_FOR_EACH_SAFE(buf, _buf, &con->lSend, link)
    {
        listRemove(&buf->link);
    }

    buf = NULL, _buf = NULL;
    LIST_FOR_EACH_SAFE(buf, _buf, &con->lFree, link)
    {
        listRemove(&buf->link);
    }
    pthread_mutex_unlock(&con->lock);
    free(con->buffer);
    free(con);

}


int netConSend(NetCon_t *n, NetBuffer_t *buf)
{
    if(n->destroyed)
    {
        n->itf->Close(n, n->udata);
        netConClose(n);
        return -1;
    }

    pthread_mutex_lock(&n->lock);
    NetBuffer_t *b = NULL;
    LIST_POP_BACK(b, &n->lFree, link);
    if(b == NULL)
    {
        printf("failed to enqueue ts packet\n");
        pthread_mutex_unlock(&n->lock);
        return -2;
    }
    listInsertBack(&n->lSend, &b->link);
    pthread_mutex_unlock(&n->lock);

    memcpy(b->buffer, buf->buffer, buf->size);
    b->offset = 0;
    b->size = buf->size;
    return 0;
}


int netConRecv(Net_t *n, uint8_t *buffer, int capacity)
{
    (void)n;
    (void)buffer;
    (void)capacity;
    return 0;
}