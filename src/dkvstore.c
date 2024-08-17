/**
 * @file dkvstore.c
 * @author Fruerlund
 * @brief 
 * @version 0.1
 * @date 2024-08-03
 * 
 * @copyright Copyright (c) 2024
 * 
 * 
 * 
*/


#include "./include/dkvstore.h"

/* 
[**************************************************************************************************************************************************]
                                                            GLOBAL VARIABLES AND DEFINES
[**************************************************************************************************************************************************]
*/

/* Represents if this server will become a coordinator or a data store. */
uint8_t serverType = SERVER_TYPE_STORE;  

/* Holds requests to be processed. */
queue_requests_t *http_queue = NULL;

/* A store for holding key value pairs */
hashtable_t *store = NULL;

/* A hash ring using consistent hashing */
hashring_t *ring = NULL;

/* Flag for threads to signal program exit*/
bool program_doexit = false;

void help(void) {

    printf("Usage: program_name [-t type] [-s stores] [-h]\n");
    printf("Options:\n");
    printf("  -t, --type   Specify the type of server.\n");
    printf("  -s, --store  Specify the stores to be used. (e.g. 127.0.0.1:5555,127.0.0.1:6666).\n");
    printf("  -p, --port   Local port to run server on.\n");
    printf("  -h, --help   Show this help message.\n");
    exit(EXIT_SUCCESS);

}

/*
[**************************************************************************************************************************************************]
                                                            METHODS / FUNCTIONS
[**************************************************************************************************************************************************]
*/

/**
 * @brief A generic method for sending a HTTP Reply with a specific status code
 * 
 * @param fd 
 * @param code 
 * @return uint32_t 
 */
uint32_t sendHTTPCode(int fd, int code) {

    int MAX_BUFFER_SIZE = 4096;
    char reply[MAX_BUFFER_SIZE];
    memset(reply, '\x00', sizeof(char) * MAX_BUFFER_SIZE);

    switch(code) {

        case 500:
            snprintf(reply, MAX_BUFFER_SIZE,
            "HTTP/1.1 500 Internal Server Error\r\n"
            "Content-Type: text/plain\r\n"
            "Connection: close\r\n"
            "\r\n"
            "HTTP 500 Internal Server Error"
            "\r\n\r\n");
            break;

        case 200:
            snprintf(reply, MAX_BUFFER_SIZE,
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/plain\r\n"
            "Connection: close\r\n"
            "\r\n"
            "HTTP 200 OK"
            "\r\n\r\n");
            break;


        case 400:
            snprintf(reply, MAX_BUFFER_SIZE,
            "HTTP/1.1 400 Bad Request\r\n"
            "Content-Type: text/plain\r\n"
            "Connection: close\r\n"
            "\r\n"
            "HTTP 400 Bad Request"
            "\r\n\r\n");
            break;
    

        case 404:
            snprintf(reply, MAX_BUFFER_SIZE,
            "HTTP/1.1 404 Not Found\r\n"
            "Content-Type: text/plain\r\n"
            "Connection: close\r\n"
            "\r\n"
            "HTTP 404 Not Found"
            "\r\n\r\n");
            break;

        case 501:
            snprintf(reply, MAX_BUFFER_SIZE,
            "HTTP/1.1 501 Not Implemented\r\n"
            "Content-Type: text/plain\r\n"
            "Connection: close\r\n"
            "\r\n"
            "HTTP 501 Not Implemented"
            "\r\n\r\n");
            break;

        default:
            break;

    }


    int len = strlen(reply);
    size_t bytesWritten = write(fd, reply, len);
    return bytesWritten;

}



/*****************************************************************************************************************************************************************************/


/**
 * @brief Parses a buffer containing a HTTP Request into a HTTP Structure by reading headers, HTTP data.
 * 
 * @param h 
 * @param buffer 
 * @param size 
 * @return uint32_t 
 */
uint32_t requestParseHeaders(http_packet_t *h, char *buffer, size_t size) {

    /* Initialize packet*/
    h->headers = (http_header_t **)malloc( MAX_HEADERS * sizeof(http_header_t *));
    
    memset(h->headers, '\x00', sizeof(http_header_t *) * MAX_HEADERS);
    /* Create Header Table */
    h->headers_table = hashtable_create(MAX_HEADERS, hashtable_hash);

    /* Parse HTTP Headers */
    char *header = strtok(buffer, "\r\n");
    http_header_t *head = NULL;
    while(header != NULL) {

        head = (http_header_t *)malloc(sizeof(struct http_header_t));
        size_t len = strlen(header) + 2; // 2 to adjust for \r\n
        char temp[len];

        memset(temp, '\x00', sizeof(char) * len);
        strncpy(temp, header, len - 2);
        strncat(&temp[strlen(header)], "\r\n", 2);

        head->header = strdup(temp);
        head->header_size = len;
        h->headers[h->numberofheaders] = head;
        h->numberofheaders++;

        if(h->numberofheaders == MAX_HEADERS) {
            break;
        }

        h->headersize += head->header_size; 

        /* Check to see if we found end of headers */
        char pattern[4] = {0x00, 0x0A, 0x0D, 0x0A};
        if(memcmp( &header[len - 2], pattern, 4) == 0) {
            h->headersize = h->headersize + 2;
            break;
        }

        header = strtok(NULL, "\r\n");
    }

    return h->numberofheaders;

}


/*****************************************************************************************************************************************************************************/
/**
 * @brief Parses the header structures and inserts them into a hash table for easy manipulation.
 * 
 * @param h 
 * @return uint32_t 
 */
uint32_t requestHeadersParseTable(http_packet_t *h) {

    for(uint32_t i = 1; i < h->numberofheaders; i++) {

        char *header= strdup(h->headers[i]->header);

        char *key = strtok(header, ":");
        char *value = strtok(NULL, ":");

        if(key != NULL && value != NULL) {
            hashtable_insert(h->headers_table, key, value);
        }
    }

    return EXIT_SUCCESS;
}


/*****************************************************************************************************************************************************************************/
/**
 * @brief Generic method responsible for parsing.
 * 
 * @param h 
 * @param buffer 
 * @param size 
 * @return uint32_t 
 */
uint32_t requestParse(http_packet_t *h, char *buffer, size_t size) {
    
    /* Parse Headers */
    requestParseHeaders(h, buffer, size);

    /* Split headers into table */
    requestHeadersParseTable(h);

    /* Parse request type */
    http_header_t *head = h->headers[0];
    char *method = strtok(head->header, " ");

   /* If request is POST, parse the POST data */
    if(strcmp(method, "POST") == 0) {

        h->datasize = size - h->headersize;

        h->httpData = (char *)malloc(h->datasize);

        memcpy(h->httpData, &buffer[h->headersize], h->datasize);

        h->type = HTTP_POST;

    }

    else if(strcmp(method, "GET") == 0) {
        h->type = HTTP_GET;
    }

    else {
        h->type == HTTP_UNKNOWN;    
    }

    return EXIT_SUCCESS;

}


uint32_t requestToBuffer(http_packet_t *h, char *buffer, size_t maxsize) {

    memset(buffer, '\x00', maxsize);
    int offset = 0;
    for(uint32_t i = 0; i < h->numberofheaders; i++) {
        memcpy(buffer+offset, h->headers[i]->header, h->headers[i]->header_size);
        offset += h->headers[i]->header_size;
    }

    char delim[2] = {0x0D, 0x0A};
    memcpy(buffer + offset, delim, 2);
    offset += 2;

    memcpy(buffer + offset, h->httpData, h->datasize);

    offset += h->datasize;

    return offset;

}

/*****************************************************************************************************************************************************************************/

/**
 * @brief Responsible for executing instructions as per the HTTP request.
 * 
 * @param h 
 * @return uint32_t 
 */
uint32_t requestHandle(http_packet_t *h) {

    /* Perform operation */
    
    switch(h->type) {

        case HTTP_POST:
        
            char *http_data_copy = strdup(h->httpData);

            /* Get command and command data */
            char *op = strtok(http_data_copy, "&");
            char *opdata = strtok(NULL, "&");

            if(op == NULL || opdata == NULL) {
                sendHTTPCode(h->clientfd, 400);
            }

            /* Split op into fields fields*/
            char *op_field = strtok(op, "=");
            char *op_value = strtok(NULL, "=");

            if(op_field == NULL || op_value == NULL) {
                sendHTTPCode(h->clientfd, 400);
            }

            /* Split op data into fields */
            char *op_datafield = strtok(opdata, "=");
            char *op_datavalue = strtok(NULL, "=");

            if(op_datafield == NULL || op_datavalue == NULL) {
                sendHTTPCode(h->clientfd, 400);
            }


            if(strcmp(op_value, "GET") == 0) {
                
                if(serverType == SERVER_TYPE_STORE) {
                    hashtable_bucket_item *r = NULL;
                    if ( ( r = hashtable_lookup(store, op_datavalue)) == NULL) {
                        sendHTTPCode(h->clientfd, 404);
                    }
                    else {
                        char reply[4096];
                        memset(reply, '\x00', sizeof(char) * 4096);
                        snprintf(reply, 4096,
                        "HTTP/1.1 200 Ok\r\n"
                        "Content-Type: text/plain\r\n"
                        "Content-Length: %d\r\n"
                        "Connection: close\r\n"
                        "\r\n"
                        "%s=%s", ( strlen(r->key) + 1 + strlen(r->value)), r->key, r->value);
                        write(h->clientfd, reply, strlen(reply));
                    }
                }

                if(serverType == SERVER_TYPE_COORDINATOR) {
                    ring_element_t *e = hashring_lookupkey(ring, op_datavalue);
                    if(e == NULL) {
                        sendHTTPCode(h->clientfd, 404);
                    }
                    else {
                        coordinator_requestForward(e, h);
                    }    
                }
                break;                
            }

            if(strcmp(op_value, "SET") == 0) {

                if(serverType == SERVER_TYPE_STORE) {
                    if ( hashtable_insert(store, op_datafield, op_datavalue) == true) {
                        sendHTTPCode(h->clientfd, 200);
                    }
                    else {
                        sendHTTPCode(h->clientfd, 400);
                    }
                }
                else {

                    /* Insert key into hashring*/
                    ring_element_t *e = NULL;
                    e = hashring_addkey(ring, op_datafield);
                    if(e == NULL) {
                        sendHTTPCode(h->clientfd, 404);
                        break;
                    }

                    /* Forward key, value to calculated store */
                    coordinator_requestForward(e, h);
                }
                break;
            }

            if(strcmp(op_value, "REM") == 0) {

                if(serverType == SERVER_TYPE_STORE) {
                    bool r = false;
                    if ( ( r = hashtable_remove(store, op_datavalue)) == false) {
                        sendHTTPCode(h->clientfd, 404);
                    }
                    else {
                        sendHTTPCode(h->clientfd, 200);
                    }
                }
                else {
                    sendHTTPCode(h->clientfd, 501);
                }
                break;
            }

            if(strcmp(op_value, "ADD") == 0) {

                if(serverType == SERVER_TYPE_COORDINATOR) {

                    ring_element_t *e = NULL;

                    /* IP in op_datavalue */

                    char *port = strtok( (op_datavalue + (strlen(op_datavalue) + 1)), "&");
                    char *weight = strtok(NULL, "&");

                    char *port_value = strtok(port, "=");
                    port_value = strtok(NULL, "=");

                    char *weight_value = strtok(weight, "=");
                    weight_value = strtok(NULL, "=");

                    if(port_value != NULL || weight_value != NULL || op_datavalue != NULL ) {
                        if ( ( e = hashring_addserver(ring, op_datavalue, atoi(port_value), atoi(weight_value)) ) == NULL) {
                            sendHTTPCode(h->clientfd, 400);
                            break;
                        }
                        else {
                            sendHTTPCode(h->clientfd, 200);
                            break;
                        }
                    }
                    sendHTTPCode(h->clientfd, 501);
                    break;
                }
            }

            if(strcmp(op_value, "DEL") == 0) {

                 if(serverType == SERVER_TYPE_COORDINATOR) {

                    /* IP in op_datavalue */

                    char *port = strtok( (op_datavalue + (strlen(op_datavalue) + 1)), "&");
                    char *port_value = strtok(port, "=");
                    port_value = strtok(NULL, "=");

                    if(port_value != NULL || op_datavalue != NULL ) {
                        
                        if ( ( hashring_removeserver(ring, op_datavalue, atoi(port_value)) ) != EXIT_SUCCESS)  {
                            sendHTTPCode(h->clientfd, 404);
                            break;
                        }
                        else {
                            sendHTTPCode(h->clientfd, 200);
                            break;
                        }
                    }
                    sendHTTPCode(h->clientfd, 501);
                    break;
                }
            }

            if(strcmp(op_value, "SYNC") == 0) {
                /* Return all keys */
            }


            sendHTTPCode(h->clientfd, 400);
            break;

        case HTTP_GET:
            sendHTTPCode(h->clientfd, 200);
            break;

        case HTTP_UNKNOWN:
            sendHTTPCode(h->clientfd, 501);
            break;
    }

    return EXIT_SUCCESS;

}


/*****************************************************************************************************************************************************************************/
/**
 * @brief Cleans up a HTTP Request structure by freeing allocated memory.
 * 
 * @param h 
 * @return uint32_t 
 */
uint32_t requestDestroy(http_packet_t *h) {

    /* Destroy hash table */
    hashtable_delete(h->headers_table);

    /* Destroy headers */
    for(uint32_t i = 0; i < MAX_HEADERS; i++) {
        if(h->headers[i] != NULL) {
            free(h->headers[i]->header);
            free(h->headers[i]);
        }
    }

    free(h->originalRequest);
    free(h->headers);   
    free(h);

    return EXIT_SUCCESS;

}

int32_t coordinator_requestForward(ring_element_t *e, http_packet_t *h) {

    /* Find store object */
    ring_element_t *server = hashring_lookupserver(ring, e->element.data->store, e->element.data->port);
    if(server == NULL) {
        return -1;
    }

    /* Remove -(virtualnodeindicator) if exists */
    char *ip = strtok(server->element.server->ip, "-");
    int port = server->element.server->port;
    struct sockaddr_in serv_addr;
    socklen_t addr_size;

    if(port < 0 || ip == NULL) {
        return -1;
    }
    

    /* Open a connection to store */
    int socketfd = -1;
    if ((socketfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("[-]: Socket creation error \n");
        return -1;
    }

    /* If connection fails return a HTTP 500 */
    
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);

    // Convert IPv4 and IPv6 addresses from text to binary
    // form
    if (inet_pton(AF_INET, ip, &serv_addr.sin_addr)
        <= 0) {
        printf("\nInvalid address/ Address not supported \n");
        return -1;;
    }
    addr_size = sizeof(serv_addr);
    int result = connect(socketfd, (struct sockaddr*)&serv_addr, addr_size);
    if(result < 0) {
        return -1;
    }

    /* Write */
    int bytesWritten = write(socketfd, h->originalRequest, h->originalRequestSize);

    /* Read */
    char buffer[MAX_INPUT_BUFFER] = { 0 };
    int bytesRead = read(socketfd, buffer, MAX_INPUT_BUFFER);

    /* Return */
    size_t bytesReturned = write(h->clientfd, buffer, bytesRead);

    /* Close */
    close(socketfd);

    /* Return */

}


/*****************************************************************************************************************************************************************************/
/**
 * @brief Request worker that is responsible for extracting a HTTP Request from the queue and handling it.
 * 
 * @param data 
 * @return void* 
 */
void *requestWorker(void *data) {

    printf("[+]: HTTP Handler Created (TID: %d)\n", gettid());
    
    while(true) {

        if(program_doexit == true) {
            break;
        }

        else {
            
            struct queue_entry_t *h = NULL;
            pthread_mutex_lock(&http_queue->read_lock);

            /* Extract a request to handle from the queue */
            if (!TAILQ_EMPTY(&http_queue->queue)) {
                h = TAILQ_LAST(&http_queue->queue, queue);
                if( h != NULL) {
                    http_queue->size--;
                    TAILQ_REMOVE(&http_queue->queue, h, entries);
                }
            }
            pthread_mutex_unlock(&http_queue->read_lock);

            if(h != NULL) {
                /* Handle request */
                http_packet_t *p = h->packet;
                free(h);
                requestHandle(p);
                close(p->clientfd);
                requestDestroy(p);
            }

        }
    }
    printf("[+]: HTTP Handler Finished (TID: %d)\n", gettid());
   
}


/*****************************************************************************************************************************************************************************/
/**
 * @brief Reads a HTTP Request into a buffer, transforms it and finally enqueues it.
 * 
 * @param socketfd 
 * @return uint32_t 
 */
uint32_t serverHandleRequest(int socketfd) {

    char *buffer = (char *)malloc(sizeof(char) * MAX_INPUT_BUFFER);
    memset(buffer, '\x00', sizeof(char) * MAX_INPUT_BUFFER);

    size_t bytesRead = 0;
    size_t totalRead = 0;
    uint32_t i = 2;

    /* Read */
    while(true) {

        /* To do: Handle large reads */
        bytesRead = read(socketfd, buffer + bytesRead, MAX_INPUT_BUFFER);

        if(bytesRead < 0) {
            perror("Error in reading!\n");
            free(buffer);
            return 0;
        }

        if(bytesRead == 0) {
            break;
        }

        else {
            totalRead += bytesRead;
            if(totalRead == MAX_INPUT_BUFFER) {
                i = i + 2;
                char *newBuffer = realloc(buffer, MAX_INPUT_BUFFER * i);
                if(newBuffer != NULL) {
                    buffer = newBuffer;
                    continue;
                }
                else {
                    perror("Failed to allocate larger buffer\n");
                    break;
                }
            }
            else {
                break;
            }
        }
    
    }

    /* Allocate */
    http_packet_t *packet = (http_packet_t *) malloc(sizeof(struct http_packet_t));
    if(packet == NULL) {
        return 0;
    }
    memset(packet, '\x00', sizeof(struct http_packet_t));
    packet->clientfd = socketfd;

    /* Save the original request */
    char *requestcopy = (char *)malloc(totalRead);
    memcpy(requestcopy, buffer, totalRead);
    packet->originalRequest = requestcopy;
    packet->originalRequestSize = totalRead;

    /* Transform to HTTP Protocol */
    requestParse(packet, buffer, bytesRead);

    /* Enqueue request for handling */
    queue_entry_t *entry = (queue_entry_t *)malloc(sizeof(queue_entry_t));
    if(entry == NULL) {
        perror("malloc\n");
        exit(EXIT_FAILURE);
    }

    entry->packet = packet;
    pthread_mutex_lock(&http_queue->write_lock);
    TAILQ_INSERT_HEAD(&http_queue->queue, entry, entries);
    http_queue->size++;
    pthread_mutex_unlock(&http_queue->write_lock);

    /* Cleanup */
    free(buffer);

    return totalRead;
    
}


/*****************************************************************************************************************************************************************************/
/**
 * @brief Accept handler (thread)
 * 
 * @param data 
 * @return void* 
 */
void *serverHandleAccept(void *data) {

    printf("[+]: Accept Handler Created (TID: %d)\n", gettid());

    int clientfd = * (( int* )data);
    serverHandleRequest(clientfd);

    printf("[+]: Accept Handler  Finished (TID: %d)\n", gettid());
}


/*****************************************************************************************************************************************************************************/
/**
 * @brief Accept loop where each connection is passed onto a new thread.
 * 
 * @param socketfd 
 */
void serverAcceptLoop(int socketfd) {

    printf("[+]: Server awaiting connections\n");


    /* Accept incoming connections. For each connection we spawn a new thread(producer) that is responsible for reading the request and enqeueing it for further handling. */
    struct sockaddr_in address_client;
    socklen_t addrlen = sizeof(address_client);

    while(true) {

        int clientfd  = accept(socketfd, (struct sockaddr *)&address_client, &addrlen);
        pthread_t thread;

        /* Spawn new thread for handling the new connection */
        pthread_create(&thread, NULL, serverHandleAccept, &clientfd);
        pthread_detach(thread);

    }
}


/*****************************************************************************************************************************************************************************/
/**
 * @brief Starts the listener.
 * 
 * @param port 
 * @return uint32_t 
 */
uint32_t serverListen(int port) {

    /* Create socket*/
    int socketfd = -1;
    if ((socketfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("[-]: Socket creation error \n");
        exit(EXIT_FAILURE);
    }

    /* Address structure */
    struct sockaddr_in address;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_family = AF_INET;
    address.sin_port = htons(port);

    /* Socket options */
    int optval = 1;
    if (setsockopt(socketfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) == -1) {
        perror("[-]: setsockopt");
        close(socketfd);
    }

    /* Bind & Listen*/
    if ( (bind(socketfd, (struct sockaddr*)&address, sizeof(address))) < 0) {
        printf("[-]: Failed to bind to port: %d\n", port);
        exit(EXIT_FAILURE);
    }

    printf("[+]: Bind to %d OK!\n", port);

    if ((listen(socketfd, 100) == 0)) {
        printf("[+]: Listen OK\n");
    }
    else {
        printf("[-]: Failed to listen!\n");
        exit(EXIT_FAILURE);

    }

    /* Enter accept loop */
    serverAcceptLoop(socketfd);

}


/*****************************************************************************************************************************************************************************/
/**
 * @brief Transforms the server into a store responsible for insertion, extraction and deletion of data.
 * 
 * @param argc 
 * @param argv 
 * @return uint32_t 
 */
uint32_t serverBecomeStore(int port) {

    if( serverType != SERVER_TYPE_STORE || port < 0 || port < 1000) {
        printf("[!]: Invalid or missing options\n");
        exit(EXIT_FAILURE);
    }

    printf("[+]: Started KVP Store server: (%d)\n", gettid());

    /* Create and initialize hash table */
    store = hashtable_create(STORE_TABLEMAXSIZE, hashtable_hash);

    while(true) {

        serverListen(port);
        
    }

    return EXIT_SUCCESS;
}


/*****************************************************************************************************************************************************************************/
/**
 * @brief Transforms the server into a coordinator responsible for forwarding requests onto the store servers.
 * 
 * @param argc 
 * @param argv 
 * @return uint32_t 
 */
uint32_t serverBecomeCoordinator(int port, char *stores) {

    if( serverType != SERVER_TYPE_COORDINATOR || port == 0 || port < 1000 || stores == NULL) {
        printf("[!]: Invalid or missing options\n");
        exit(EXIT_FAILURE);
    }

    printf("[+]: Started KVP Coordinator server: (%d)\n", gettid());

    /* Create and intialize hash ring */
    ring = hashring_create(4000000, hashring_hash_jenkins);

    /* Add store servers */
    char *storeline= NULL;
    char *storeip;
    char *storeport;
    char delim[] = ",";

    printf("Test: %s\n", stores);
    storeline = strtok(stores, delim);
    char *servers[MAX_SERVERS] = { 0 };
    size_t size = 0;

    while(storeline != NULL) {
        if(size > MAX_SERVERS) {
            break;
        }
        servers[size] = strdup(storeline);
        size++;
        storeline = strtok(NULL, delim);
    }


    for(uint32_t i = 0; i < size; i++) {
        
        storeip = strtok(servers[i], ":");
        storeport = strtok(NULL, ":");
    
        if (storeip != NULL && storeport != NULL) {
            hashring_addserver(ring, storeip, atoi(storeport), 10);
        } else {
            fprintf(stderr, "Invalid format in list\n");
        }
        
        free(servers[i]);
    }

    while(true) {

        serverListen(port);

    }

    return EXIT_SUCCESS;
}


/*****************************************************************************************************************************************************************************/
/**
 * @brief Cleanup
 * 
 */
void exit_cleanup(void) {

    /*
    Empty queue
    */
    free(http_queue);

    /*
    Destroy key, value store.
    */
    if(store != NULL) {
        hashtable_delete(store);
    }

    if(ring != NULL) {
        hashring_destroy(ring);
    }

    /* Destory locks */
    pthread_mutex_destroy(&http_queue->write_lock);
    pthread_mutex_destroy(&http_queue->read_lock);


}


/**
 * @brief Initializer for setting up the request queue.
 * 
 * @return uint8_t 
 */
uint8_t init_httprequestqueue(void) {
    
    /* Setup request queue */
    http_queue = (queue_requests_t *)malloc(sizeof(queue_requests_t));
    if(http_queue == NULL) {
        perror("Malloc request\n");
        return EXIT_FAILURE;
    }

    /* Setup locks */
    pthread_mutex_init(&http_queue->write_lock, NULL);
    pthread_mutex_init(&http_queue->read_lock, NULL);

    return EXIT_SUCCESS;

}


/* 
[**************************************************************************************************************************************************]
                                                            MAIN
[**************************************************************************************************************************************************]
*/

/**
 * @brief Main
 * 
 * @param argc 
 * @param argv 
 * @return int 
 */
int main(int argc, char **argv) {

    int typeflag = 0;
    char *stores = NULL;
    char *type = NULL;
    int port = 0;
    int opt;

    struct option long_options[] = {
        {"help",  no_argument,       NULL, 'h'},
        {"store", required_argument, NULL, 's'},
        {"type",  required_argument, NULL, 't'},
        {"port",  required_argument, NULL, 'p'},
        {NULL,    0,                 NULL,  0 }
    };

    while( (opt = getopt_long(argc, argv, "t:s:p:h?", long_options, NULL)) != -1) {

        switch(opt) {
            case 't':
                type = strdup(optarg);
                break;
            case 's':
                stores = strdup(optarg);
                break;
            case 'p':
                port = atoi(strdup(optarg));
                break;
            case '?':
                printf("Bad option: %c\n", optopt); 
                help();
                break;
            default:
                help();
        }
    }        

    
    if(type == NULL) {
        help();
        exit(EXIT_FAILURE);
    }

    if(strcmp(type, "COORDINATOR") == 0 || strcmp(type, "coordinator") == 0) {
        serverType = SERVER_TYPE_COORDINATOR;
    }
    else if( strcmp(type, "STORE") == 0 || strcmp(type, "store") == 0 ) {
        serverType = SERVER_TYPE_STORE;
    }
    else {
        help();
    }
    
    /* Setup request queue */
    if ( init_httprequestqueue() == EXIT_FAILURE ) {
        printf("[!]: Failed to allocate memory for requests queue\n");
        exit(EXIT_FAILURE);
    }

    /* Setup request handling workers */
    printf("[+]: Creating request handlers\n");
    pthread_t workers[MAX_WORKERS] = { 0 };
    for(uint32_t i = 0; i < MAX_WORKERS; i++) {
        pthread_create(&workers[i], NULL, requestWorker, NULL);
    }

    switch(serverType) {

        case SERVER_TYPE_COORDINATOR:
            serverBecomeCoordinator(port, stores);
            break;

        case SERVER_TYPE_STORE:
            serverBecomeStore(port);
            break;

        default:
            printf("[-]: Invalid server option selected\n");
            exit(EXIT_FAILURE);

    }

     /* Wait for workers to finish */
    for(uint32_t i = 0; i < MAX_WORKERS; i++) {
        void *ret;
        pthread_join(workers[i], &ret);
    }
     /* Clean up */
     exit_cleanup();
}

