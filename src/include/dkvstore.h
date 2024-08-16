/**
 * @file dkvstore.h
 * @author Fruerlund
 * @brief 
 * @version 0.1
 * @date 2024-08-03
 * 
 * @copyright Copyright (c) 2024
 * 
 */



#ifndef DKVSTORE_H
#define DKVSTORE_H

#include "common-defines.h"
#include "./hashtable.h"
#include "./hashring.h"
/* 
[**************************************************************************************************************************************************]
                                                            KEY, VALUE STORE 
[**************************************************************************************************************************************************]
*/

#define PROTO_SET                   0x41
#define PROTO_GET                   0x42
#define PROTO_REM                   0x43
#define PROTO_ADD                   0x44
#define PROTO_DEL                   0x45
#define PROTO_FAIL                  0x46
#define PROTO_OK                    0x47

#define RING_ELEMENT_EMPTY          0x10
#define RING_ELEMENT_SERVER         0x11
#define RING_ELEMENT_DATA           0x12

#define SERVER_TYPE_STORE            0x20
#define SERVER_TYPE_COORDINATOR     0x21

#define MAX_WORKERS 1
#define MAX_INPUT_BUFFER 4096
#define MAX_HEADERS 25


#define HTTP_POST                   0x31
#define HTTP_GET                    0x32
#define HTTP_UNKNOWN                0x33

#define STORE_TABLEMAXSIZE          1024

#define MAX_SERVERS                 100

/* 
[**************************************************************************************************************************************************]
                                                             API PROTOCOL
[**************************************************************************************************************************************************]
*/

/*
A request follows the format of:
POST / HTTP/1.1
Host: server
cmd=SET&key=value
*/


/*
Supported CMDs:
SET: Inserts a key, value pair into the store.
GET: Gets a value from a given key from the store.
REM: Removes a value from a given key from the store.
ADD: Adds a new data server into the hash ring.
DEL: Deletes a data server from the hash ring.
*/


/**
 * @brief Represents a single HTTP Header
 */
typedef struct http_header_t {

    char *header;
    size_t header_size;

} http_header_t;


/**
 * @brief Represents a HTTP Packet
 * 
 */
typedef struct http_packet_t {

    struct http_header_t **headers;         /* Maximum support of MAX_NUMBER_HEADERS headers */
    hashtable_t *headers_table;             /* Hash table containing key -> header mappings */
    uint8_t type;                           /* Method, GET, POST etc. */
    size_t numberofheaders;                 /* Number of parsed headers */
    size_t headersize;                      /* Total size of headers in bytes */
    size_t totalsize;
    size_t datasize;                        /* Total size of request data*/
    char *httpData;                         /* Pointer to request data if any*/
    int clientfd;                           /* File descriptor from which the packet originated.*/
    char *originalRequest;                  /* Unmodified original request */
    size_t originalRequestSize;             /* Size of unmodified original request.*/

} http_packet_t;


/* 
[**************************************************************************************************************************************************]
                                                            QUEUE
[**************************************************************************************************************************************************]
*/


typedef struct queue_entry_t {
    http_packet_t *packet;
    TAILQ_ENTRY(queue_entry_t) entries;
} queue_entry_t;

typedef struct queue_requests_t {

    pthread_mutex_t read_lock;                          /*  Mutex for synchronization    */
    pthread_mutex_t write_lock;                         /*  Mutex for synchronization    */
    TAILQ_HEAD(queue, queue_entry_t) queue;             /*  Queue holding requests       */
    size_t size;

} queue_requests_t;



int32_t coordinator_requestForward(ring_element_t *, http_packet_t *h);

#endif /* DKVSTORE_H */