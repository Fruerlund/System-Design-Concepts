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

/**
 * @brief Describes a element in the hash ring that holds a key, value pair.
 */
typedef struct redis_ring_t_data {

    char *key;
    char *value;
    uint32_t hash;
    
} redis_ring_t_data;


/**
 * @brief Describes a element in the hash ring that represents a data store.
 */
typedef struct redis_ring_t_server {

    struct sockaddr_in address;         /* IP and PORT */
    size_t size;                        /* Number of key, value pairs in hash server */
    
} redis_ring_t_server;

/**
 * @brief Describes a empty element in the hash ring.
 */
typedef struct redis_ring_t_empty {

} redis_ring_t_empty;



/**
 * @brief Describes a chunk of the hash ring that either represents a empty spot, data store or key, value, pair
 */
typedef struct redis_ring_element_t {

    struct redis_ring_t *left;          /* Pointer to left slice */
    struct redis_ring_t *right;         /* Pointer to right slice*/
    uint8_t type;                       /* Used to indicate the type(empty, data or server) that his slice holds */

    union ring_type {
        redis_ring_t_server *server;
        redis_ring_t_data *data;
        redis_ring_t_empty *empty;
    } ring_type;


} redis_ring_element_t;


/**
 * @brief Describes a hash ring using consistent hashing.
 */
typedef struct redis_ring_t {

    size_t size;                        /* Size of the hash ring */
    size_t count;                       /* Current elemenets in hash ring */
    struct redis_ring_element_t **start;

} redis_ring_t;

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

    struct http_header_t **headers;          /* Maximum support of MAX_NUMBER_HEADERS headers */
    hashtable_t *headers_table;
    uint8_t type;                            /* Method, GET, POST etc. */
    size_t numberofheaders;
    size_t headersize;
    size_t totalsize;
    size_t datasize;
    char *httpData;
    int clientfd;                           /* File descriptor from which the packet originated.*/

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



#endif /* DKVSTORE_H */