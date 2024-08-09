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

uint8_t serverType = SERVER_TYPE_STORE;   /* Represents if this server will become a coordinator or a data store. */

queue_requests_t *http_queue = NULL;
hashtable_t *store = NULL;
bool program_doexit = false;
/*
[**************************************************************************************************************************************************]
                                                            METHODS / FUNCTIONS
[**************************************************************************************************************************************************]
*/


/* 
[**************************************************************************************************************************************************]
                                                            MAIN
[**************************************************************************************************************************************************]
*/

/*****************************************************************************************************************************************************************************/


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
            "\r\n"
            "HTTP 400 Bad Request"
            "\r\n\r\n");
            break;
    

        case 404:
            snprintf(reply, MAX_BUFFER_SIZE,
            "HTTP/1.1 404 Not Found\r\n"
            "Content-Type: text/plain\r\n"
            "\r\n"
            "HTTP 404 Not Found"
            "\r\n\r\n");
            break;

        case 501:
            snprintf(reply, MAX_BUFFER_SIZE,
            "HTTP/1.1 501 Not Implemented\r\n"
            "Content-Type: text/plain\r\n"
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

            /* Get command and command data */
            char *op = strtok(h->httpData, "&");
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
                    "Content-Length: 8\r\n"
                    "\r\n");

                    write(h->clientfd, reply, strlen(reply));
                }
                break;
            }

            if(strcmp(op_value, "SET") == 0) {
                if ( hashtable_insert(store, op_datafield, op_datavalue) == true) {
                    sendHTTPCode(h->clientfd, 200);
                }
                break;
            }

            if(strcmp(op_value, "REM") == 0) {

            }

            if(strcmp(op_value, "ADD") == 0) {

            }

            if(strcmp(op_value, "DEL") == 0) {

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
    for(uint32_t i = 0; i< MAX_HEADERS; i++) {
        if(h->headers[i] != NULL) {
            free(h->headers[i]->header);
            free(h->headers[i]);
        }
    }

    free(h->headers);

    return EXIT_SUCCESS;

}

http_packet_t * coordinator_requestForward(int socketfd) {

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

    /* If server type is a coordinator, do not handle request but forward it and return reply */
    if ( serverType == SERVER_TYPE_COORDINATOR) {
        coordinator_requestForward(clientfd);
    }

    /* If server type is a store, handle request */
    else {
        serverHandleRequest(clientfd);
    }

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
uint32_t serverBecomeStore(int argc, char **argv) {

    if( serverType != SERVER_TYPE_STORE) {
        exit(EXIT_FAILURE);
    }

    printf("[+]: Started KVP Store server: (%d)\n", gettid());

    int port = atoi(argv[1]);   /* Port */

    /* Create and initialize hash table */
    store = hashtable_create(256, hashtable_hash);

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
uint32_t serverBecomeCoordinator(int argc, char **argv) {

    if( serverType != SERVER_TYPE_COORDINATOR) {
        exit(EXIT_FAILURE);
    }

    printf("[+]: Started KVP Coordinator server: (%d)\n", gettid());

    /* Create and intialize hash ring */

    int port = atoi(argv[1]);   /* Port */

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
        free(store);
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



/**
 * @brief Main
 * 
 * @param argc 
 * @param argv 
 * @return int 
 */
int main(int argc, char **argv) {


    /* 
    TO DO: 
    Input handling, for now we manually control the input 
    */

    /* Setup request queue */
    if ( init_httprequestqueue() == EXIT_FAILURE ) {
        printf("[!]: Failed to allocate memory for requests queue\n");
        exit(EXIT_FAILURE);
    }

    /* Setup request handling workers */
    printf("[+]: Creating requets handlers\n");
    pthread_t workers[MAX_WORKERS] = { 0 };
    for(uint32_t i = 0; i < MAX_WORKERS; i++) {
        pthread_create(&workers[i], NULL, requestWorker, NULL);
    }


    switch(serverType) {

        case SERVER_TYPE_COORDINATOR:
            serverBecomeCoordinator(argc, argv);
            break;

        case SERVER_TYPE_STORE:
            serverBecomeStore(argc, argv);
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

