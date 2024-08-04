/**
 * @file loadbalancer.c
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


#include "./include/loadbalancer.h"

/* 
[**************************************************************************************************************************************************]
                                                            GLOBAL VARIABLES AND DEFINES
[**************************************************************************************************************************************************]
*/

#define MAX_BUFFER_SIZE 4096 * 8


/* Condition variables/flag to terminate coordinator threads */
pthread_mutex_t mutexCoordinatorForwardTerminate = PTHREAD_MUTEX_INITIALIZER;
bool coordinatorForwardTerminate = false;

/* A structure with a nested array of structures representing possible servers to forward to */
struct servers_t servers;       

/* Queue that holds our requests. Is lockable by a read and/or write mutex. */
queue_t requests;


/* 
[**************************************************************************************************************************************************]
                                                            METHODS / FUNCTIONS
[**************************************************************************************************************************************************]
*/

/**
 * @brief Forwards data from a given request to the specified load server. The data is read back in buffers from the load server and sent back to the client. 
 * 
 * @param connection 
 * @param forwarder 
 * @param forwarderfd 
 * @return size_t 
 */
size_t buffered_sr(struct connection_t *connection, struct forwarder_t *forwarder, int forwarderfd) {

    char ip_str[INET6_ADDRSTRLEN];
    char ip_str_forwarder[INET6_ADDRSTRLEN];

    void *ip_addr;
    void *ip_addr_forwarder;

    /* Get local copies of client fd */
    int clientfd = connection->clientfd;

    /* Get string represention of  IP address */
    ip_addr = &(connection->address.sin_addr);
    if (inet_ntop(AF_INET, ip_addr, ip_str, sizeof(ip_str)) == NULL) {
        perror("inet_ntop");
        sendHTTPError500(connection, connection->request);
        pthread_exit(NULL);
    }

    ip_addr_forwarder = &( forwarder->address.sin_addr);
    if (inet_ntop(AF_INET, ip_addr_forwarder, ip_str_forwarder, sizeof(ip_str_forwarder)) == NULL) {
        perror("inet_ntop");
        sendHTTPError500(connection, connection->request);
        pthread_exit(NULL);
    }

    /* Send original request as a single packet request. */
    int bytesSent = send(forwarderfd, connection->request, connection->size, MSG_NOSIGNAL);
    if(bytesSent < 0) {
        perror("sigpipe");
        sendHTTPError500(connection, connection->request);
        pthread_exit(NULL);
    }
    
    printf("(FWD) [IP: %s | Port: %d | Byte(s): %d ]\t-->\t[ IP: %s | Port: %d ]\n", ip_str, ntohs(connection->address.sin_port), bytesSent, ip_str_forwarder, ntohs(forwarder->address.sin_port));

    /* Get buffered response from server and send to client. Repeat until no more data. */
    char serverResponse[MAX_BUFFER_SIZE] = { 0 };
    bytesSent = 0;
    while(true) {

        int bytesRecieved = read(forwarderfd, serverResponse, MAX_BUFFER_SIZE);
        if(bytesRecieved <= 0) {
            break;
        }

        int sent = send(clientfd, serverResponse, bytesRecieved, MSG_NOSIGNAL);
        if(sent <= -1) {
            break;
        }
        bytesSent += sent;
    }

    printf("(RET) [IP: %s | Port: %d | Byte(s): %d ]\t<--\t[ IP: %s | Port: %d ]\n", ip_str, ntohs(connection->address.sin_port), bytesSent, ip_str_forwarder, ntohs(forwarder->address.sin_port));

    /* Close used sockets */
    close(forwarderfd);
    close(clientfd);

    return bytesSent;
}


/**
 * @brief Thread responsible for handling a single request.
 * 
 * @param data 
 * @return void* 
 */
void * consumerForwardSingleRequest(void *data) {

    struct connection_t *connection = (connection_t *)data;
    struct forwarder_t *forwarder = servers.forwarders[connection->forwarderindex];

    /* TO DO: Add X-FORWARDED-FOR Header */

    /* TO DO: Set clientID cookie for subsequent reuse */

    /* Open a connection to backend server */
    int socketfd = -1;
    if ((socketfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("[-]: Socket creation error \n");
    }

    /* If connection fails return a HTTP 500 */
    int result = connect(socketfd, (struct sockaddr*)&forwarder->address, sizeof(forwarder->address));
    if(result < 0) {
        sendHTTPError500(connection, connection->request);
        pthread_exit(NULL);
    }

    /* Send and recieve. */
    size_t dataTransfered = buffered_sr(connection, forwarder, socketfd);
  
    /* Free allocated structures */

    free(connection->request);
    free(connection);
}


/**
 * @brief A method for picking a server to forward too. This implementation is simplistic and uses rand() to pick the server allowing for a uneven distribution. Alternative solution could be round-robin 
 * 
 * @return forwarder_t* 
 */
int32_t pickforwarder(void) {

    uint32_t index = rand() % servers.numberofservers;

    struct forwarder_t *forwarder = servers.forwarders[index];
    /* Shouldn't happen. */
    if(forwarder == NULL) {
        perror("[-]: Picked a NULL Server\n");
        return -1;
    }
    
    /* Update statistics by incrementing the number of usage. */
    pthread_mutex_lock(&forwarder->lock);
    forwarder->numberofforwards++;
    pthread_mutex_unlock(&forwarder->lock);

    return index;
    
}


/**
 * @brief Returns a HTTP Reply with status code 500. Used to indicate internal server error in the case of application error.
 * 
 * @param connection 
 * @param requestBuffer 
 * @return int32_t 
 */
int32_t sendHTTPError500(struct connection_t *connection, char *requestBuffer) {

    char reply[MAX_BUFFER_SIZE];
    memset(reply, '\x00', sizeof(char) * MAX_BUFFER_SIZE);

    snprintf(reply, MAX_BUFFER_SIZE,
            "HTTP/1.1 500 Internal Server Error\r\n"
            "Content-Type: text/plain\r\n"
            "\r\n"
            "500 Internal Server Error");
    
    int len = strlen(reply);

    write(connection->clientfd, reply, len);
    close(connection->clientfd);

    free(requestBuffer);
    free(connection);

}


/**
 * @brief Subsequent requests to the load server will use session management so that identical clients based on their IP are given the same backend server. This method
 * uses regex to extract the cookie from the HTTP request extracting the identifier for the backend server to use.
 * 
 * @param buffer 
 * @param size 
 * @return int32_t 
 */
int32_t httpGetForwardCookie(char *buffer, size_t size) {

    regex_t regex;
    regcomp(&regex, "forwarderid=([^;]+)", REG_EXTENDED);
    regmatch_t matches[2];
    
    /* Regex Logic to extract the cookie forwaderid from the HTTP Request if one exists */

    if(regexec(&regex, buffer, 2, matches, 0) == 0) {
        
        int start = matches[1].rm_so;
        int end = matches[1].rm_eo;
        int len = end - start;
        char indexStr[len + 1];
        memset(indexStr, '\x00', len);
        memcpy(indexStr, &buffer[start], len);
        int index = atoi(indexStr);
        return index;

    }

    return -1;

}


/**
 * @brief Thread for handling a connection given by accept(). Uses buffered reading to read the HTTP Request from the TCP connection, then picks a server to forward to and finally enqeues the new request.
 * 
 * @param data 
 * @return void* 
 */
void * producerHandleAccept(void *data) {

    struct connection_t *connection = (struct connection_t *)data;

    /* Read HTTP Request. */
    char *requestBuffer = (char *)malloc(sizeof(char) * MAX_BUFFER_SIZE);
    if(requestBuffer == NULL) {
        perror("[-]: Failed to allocate buffer for HTTP Request\n");
        pthread_exit(NULL);
    }
    memset( requestBuffer, '\x00', sizeof(char) * MAX_BUFFER_SIZE);

    /* Read HTTP Data. Only supports 4096 bytes*/
    ssize_t bytes_recieved = read(connection->clientfd, requestBuffer, 4096);
    connection->size = bytes_recieved;
    connection->request = requestBuffer;
    
    /* If there was a fail in reading data we abort and clean up*/
    if(bytes_recieved < 0) {
        perror("[-]: Failed to read data from socket\n");
        sendHTTPError500(connection, requestBuffer);
        pthread_exit(NULL);
    }

    /* Only accept HTTP Get request as of current implementation */
    regex_t regex;
    regcomp(&regex, "^GET /([^ ]*) HTTP/1", REG_EXTENDED);
    regmatch_t matches[2];

    if(regexec(&regex, requestBuffer, 2, matches, 0) != 0) {
        /* HTTP Request is NOT GET*/   
        perror("[-]: Balancer only supports HTTP GET\n");     
        sendHTTPError500(connection, requestBuffer);
        pthread_exit(NULL);
    }

    /* Check for header/cookie to specify the server to forward to */
    int cookieId = httpGetForwardCookie(requestBuffer, bytes_recieved);
    if(cookieId >= 0 && cookieId < servers.numberofservers) {
        connection->forwarderindex = cookieId;
    }
    else {

        /* If the cookie doesn't exists pick a new forwarder. */
        int32_t forwarderindex = pickforwarder();
        if(forwarderindex < 0) {
            sendHTTPError500(connection, requestBuffer);
            pthread_exit(NULL);
        }
        connection->forwarderindex = forwarderindex;
    }

    /* If request is a HTTP GET Request, obtain lock for request queue and enqueue the request for forwarding */
    pthread_mutex_lock(&requests.write_lock);
    TAILQ_INSERT_HEAD(&requests.queue, connection, entries);
    requests.queue_size++;
    pthread_mutex_unlock(&requests.write_lock);

}



/**
 * @brief A coordinator thread that is responsible for reading requests from the queue and passing these two child threads that handle the specific request.
 * 
 * @param data 
 * @return void* 
 */
void * coordinatorForwardRequests(void *data) {

    printf("[+]: Created Request Coordinator (TID: %d)\n", gettid());

    struct queue_t *q = (queue_t *)data;
    struct connection_t *connection = NULL;
    pthread_t consumer;

    while (true) {

        /* Check if the condition for exiting has been set */
        pthread_mutex_lock(&mutexCoordinatorForwardTerminate);
        if (coordinatorForwardTerminate) {
            pthread_mutex_unlock(&mutexCoordinatorForwardTerminate);
            break;
        }
        pthread_mutex_unlock(&mutexCoordinatorForwardTerminate);

        /* Obtain read lock on the queue. Then deqeue a connection request and finally spawn a new thread for handling the request */
        pthread_mutex_lock(&q->read_lock);
        if (!TAILQ_EMPTY(&q->queue)) {
            connection = TAILQ_LAST(&q->queue, requestqueue);
            if (connection != NULL) {
                TAILQ_REMOVE(&q->queue, connection, entries);
                requests.queue_size--;
            }
        }
        pthread_mutex_unlock(&q->read_lock);

        /* Create new thread(consumer) for forwarding a specific request. */
        if (connection != NULL) {
            pthread_create(&consumer, NULL, consumerForwardSingleRequest, connection);
            pthread_detach(consumer);
        }
        
        connection = NULL;  /* Reset variable */

    }

    /* Tear down upon condition trigger */
    printf("[+]: Killed Coordinator (TID: %d)\n", gettid());

}


/**
 * @brief Implements a server while loop that allows for handling multiple requests/connections using accept. 
 * 
 * @param port 
 */
void beginServerListen(uint32_t port) { 

    /* Setup server socket */
    int serverfd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in address;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_family = AF_INET;
    address.sin_port = htons(port);

    int optval = 1;
    if (setsockopt(serverfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) == -1) {
        perror("[-]: setsockopt");
        close(serverfd);
    }

    /* Bind & Listen*/
    if ( (bind(serverfd, (struct sockaddr*)&address, sizeof(address))) < 0) {
        printf("[-]: Failed to bind to port: %d\n", port);
        exit(EXIT_FAILURE);
    }

    printf("[+]: Bind to %d OK!\n", port);

    if ((listen(serverfd, 100) == 0)) {
        printf("[+]: Listen OK\n");
    }
    else {
        printf("[-]: Failed to listen!\n");
        exit(EXIT_FAILURE);

    }

    /* Accept incoming connections. For each connection we spawn a new thread(producer) that is responsible for reading the request and enqeueing it for further handling. */
    struct sockaddr_in address_client;
    socklen_t addrlen = sizeof(address_client);
    

    while(true) {
        
        int clientfd  = accept(serverfd, (struct sockaddr *)&address_client, &addrlen);
        pthread_t thread;

        /* Data & argument for new thread */
        connection_t *connectionData =(connection_t *) malloc(sizeof(connection_t));
        connectionData->addrlen = addrlen;
        connectionData->clientfd = clientfd;
        connectionData->forwarderindex = 0;
        connectionData->timestamp = 0;
        connectionData->request = NULL;
        memcpy(&connectionData->address, &address_client, sizeof(struct sockaddr_in));
        
        /* Spawn new thread for handling the new connection */
        pthread_create(&thread, NULL, producerHandleAccept, connectionData);
        pthread_detach(thread);
    }

} 


/**
 * @brief Reads and imports possible servers that the load balancer can forward too. Accepts input data of the foormat <ip:port> split by space.
 * 
 * @param servers 
 * @param serverData 
 * @return true 
 * @return false 
 */
bool setupForwardServers(struct servers_t *servers, char *serverData) {

    /* Parse serverData <ip:port> */
    char input[256];
    strncpy(input, serverData, sizeof(input));
   
    char *ip = strtok(input, ":");
    char *port = strtok(NULL, ":");
    int socketfd = 0;

    if (ip == NULL || port == NULL) {
        printf("Invalid input format. Expected <ip:port>.\n");
        return false;
    }

    /* Format address input into appropiate network structure */
    struct sockaddr_in address;
    socklen_t addrlen = sizeof(address);
    address.sin_family = AF_INET;
    address.sin_port = htons(atoi(port));

    if (inet_pton(AF_INET, ip, &address.sin_addr) <= 0) {
        printf("[-]: Invalid address/ Address not supported \n");
        return false;
    }

    /* Create a socket descriptor */
    if ((socketfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("[-]: Socket creation error \n");
    }

    /* Test connections to backend servers */
    int result = connect(socketfd, (struct sockaddr*)&address, sizeof(address));

    if(result >= 0) {
        printf("[+]: Opened connection to server at: %s:%s\n", ip, port);
    }
    else {
        printf("[-]: Failed to open connection to server at: %s:%s\n", ip, port);
        return false;
    }

    close(socketfd);


    /* If succesfull connection create a structure for the connection. Initialize contents of structure including mutex lock*/
    forwarder_t *forwarder = (forwarder_t *)malloc(sizeof(forwarder_t));
    if(forwarder == NULL) {
        return false;
    }

    forwarder->numberofforwards = 0;

    if (pthread_mutex_init(&forwarder->lock, NULL) != 0) {
        perror("Failed to initialize mutex");
        close(socketfd);
        free(forwarder);
        return false;
    }

    /* Copy the filled client address structure into the forwarder structure */
    memcpy(&forwarder->address, &address, sizeof(struct sockaddr_in));
    
    servers->forwarders[servers->numberofservers] = forwarder;
    servers->numberofservers++;

}



void teardown(void) {

    /* Join all threads */

    /* Free all allocated data */
    for(uint32_t i = 0; i < servers.numberofservers; i++) {
        pthread_mutex_destroy(&servers.forwarders[i]->lock);
        free(servers.forwarders[i]);
    }

    free(servers.forwarders);

    /* Destroy mutexes */
    pthread_mutex_destroy(&requests.write_lock);
    pthread_mutex_destroy(&requests.write_lock);

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

    printf("[+]: Created Main (TID: %d)\n", gettid());

    uint32_t serverport = 5555;

    srand(time(NULL));

    if (pthread_mutex_init(&requests.write_lock, NULL) != 0) {
        perror("Failed to initialize mutex");
        exit(EXIT_FAILURE);
    }

    if (pthread_mutex_init(&requests.read_lock, NULL) != 0) {
        perror("Failed to initialize mutex");
        exit(EXIT_FAILURE);
    }

    TAILQ_INIT(&requests.queue);

    /* Input handling is intentional left simplistic. */
    if(argc < 2) {
        printf("./program 127.0.0.1:1234 127.0.0.2:1234 ...\n");
        exit(EXIT_FAILURE);
    }

    /* Setup data structures for servers and open connections to the servers */
    servers.forwarders = calloc( (size_t) (argc - 2), sizeof(struct forwarder_t *));
    servers.numberofservers = 0;

    struct forwarder_t **forwarders = calloc( (size_t) (argc - 2), sizeof(struct forwarder_t *));
    for(uint32_t i = 1; i < argc; i++) {
        setupForwardServers(&servers, argv[i]);
    }

    if(servers.numberofservers == 0) {
        exit(EXIT_FAILURE);
    }

    /* Create a coordinator thread that reads requests from queue, spawns a thread from, that is responsible for forwarding requests */
    pthread_t coordinatorForward;
    pthread_create(&coordinatorForward, NULL, coordinatorForwardRequests, &requests);

    /* Start load balanacer server listener and enqeue incoming connection / requests. */
    beginServerListen(serverport);


    /* Only hit when beginServerListen breaks the while-accept loop */
    teardown();


}

