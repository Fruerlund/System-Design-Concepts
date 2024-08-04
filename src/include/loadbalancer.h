/**
 * @file loadbalancer.h
 * @author Fruerlund
 * @brief 
 * @version 0.1
 * @date 2024-08-03
 * 
 * @copyright Copyright (c) 2024
 * 
 */


#ifndef LOADBALANCER_H
#define LOADBALANCER_H

#include "common-defines.h"
#include "hashtable.h"
#include <regex.h>

/* 
[**************************************************************************************************************************************************]
                                                            STRUCTUES 
[**************************************************************************************************************************************************]
*/


/**
 * @brief Describes an instance of a server that the load balancer can forward to.
 * 
 */
typedef struct forwarder_t {

    struct sockaddr_in address;         /* IP and PORT */
    socklen_t addrlen;                  
    uint32_t numberofforwards;          /* Counts the number of forwards            */
    pthread_mutex_t lock;               /* Mutex for synchronization                */

} forwarder_t;


/**
 * @brief A global structure that holds instances of servers that can be forwarded to.
 * 
 */
typedef struct servers_t {
    struct forwarder_t **forwarders;    /* An array of possible servers to forward to  */  
    size_t numberofservers;             /* Number of servers in the previous array      */
    hashtable_t *ip_to_fd;              /* Hash table mapping IP to descriptor          */
    hashtable_t *fd_to_ip;              /* Hash table mapping descriptor to IP          */
} servers_t;


/**
 * @brief A structure describing a request.
 * 
 */
typedef struct connection_t {

    struct sockaddr_in address;         /* IP and PORT */
    socklen_t addrlen;
    uint32_t clientfd;                  /* Descriptor to write/read to/from server  */
    uint32_t forwarderindex;            /* Index into forwarder array             */
    uint32_t timestamp;                 /* TImestamp when request was recieved. */
    size_t size;                        /* Size of request */
    TAILQ_ENTRY(connection_t) entries;
    void *request;                      /* Pointer to actual HTTP request data */

} connection_t;


/**
 * @brief A structure holding a queue of all incoming requests.
 * 
 */
typedef struct queue_t {

    pthread_mutex_t read_lock;                          /*  Mutex for synchronization    */
    pthread_mutex_t write_lock;                         /*  Mutex for synchronization    */
    size_t queue_size;                      
    TAILQ_HEAD(requestqueue, connection_t) queue;       /*  Queue holding requests       */

} queue_t;


/* 
[**************************************************************************************************************************************************]
                                                            PROTOTYPES
[**************************************************************************************************************************************************]
*/

int32_t sendHTTPError500(struct connection_t *connection, char *requestBuffer);
void teardown(void);
bool setupForwardServers(struct servers_t *servers, char *serverData);
void beginServerListen(uint32_t port);
void * coordinatorForwardRequests(void *data);
void * producerHandleAccept(void *data);
int32_t httpGetForwardCookie(char *buffer, size_t size);
int32_t sendHTTPError500(struct connection_t *connection, char *requestBuffer);
int32_t pickforwarder(void);
void * consumerForwardSingleRequest(void *data);
size_t buffered_sr(struct connection_t *connection, struct forwarder_t *forwarder, int forwarderfd);

#endif /* LOADBALANCER_H */
