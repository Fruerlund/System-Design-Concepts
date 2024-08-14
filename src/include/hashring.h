/**
 * @file hashring.h
 * @author Fruerlund
 * @brief 
 * @version 0.1
 * @date 2024-08-03
 * 
 * @copyright Copyright (c) 2024
 * 
 */

#ifndef HASHRING_H
#define HASHRING_H

#include "common-defines.h"


/* 
[**************************************************************************************************************************************************]
                                                            STRUCTUES AND PROTOTYPES
[**************************************************************************************************************************************************]
*/

#define ELEMENT_SERVER  0x1
#define ELEMENT_EMPTY   0x2
#define ELEMENT_KEY     0x3

#define OP_NODEINSERT   0x4
#define OP_NODEDEL      0x5

/**
 * @brief Describes a element in the hash ring that holds a key, value pair.
 */
typedef struct ring_element_key_t {

    char *key;                  
    char *store;
    int port;

} ring_element_key_t;


/**
 * @brief Describes a element in the hash ring that represents a data store.
 */
typedef struct ring_element_server_t {

    char *ip;                           /* IP */
    int port;                           /* PORT */
    size_t size;                        /* Number of key, value pairs in hash store.  */
    size_t maxsize;                     /* Maximum number of kvp in a hash store. */
    uint32_t rangestart;                /* Start of key ranges */
    uint32_t rangeend;                  /* End of key ranges */
    uint32_t numberofvirtualnodes;      /* Number of virtual nodes */
    bool isvirtualnode;                 /* Virtual node indicator */

} ring_element_server_t;

/**
 * @brief Describes a empty element in the hash ring.
 */
typedef struct ring_element_empty_t {

} ring_element_empty_t;



/**
 * @brief Describes a chunk of the hash ring that either represents a empty spot, data store or key, value, pair
 */
typedef struct ring_element_t {

    uint32_t hash;
    struct ring_element_t *left;            /* Pointer to left slice */
    struct ring_element_t *right;           /* Pointer to right slice*/
    struct ring_element_t *parent;
    uint8_t type;                           /* Used to indicate the type(empty, data or server) that his slice holds */

    union element {
        ring_element_server_t *server;
        ring_element_key_t *data;
        ring_element_empty_t *empty;
    } element;

} ring_element_t;


/**
 * @brief Describes a hash ring using consistent hashing.
 */

typedef uint32_t (*hashring_hash_t)(char *key);

typedef struct hashring_t {

    size_t size;                        /*  Size of the hash ring            */
    size_t count;                       /*  Current elemenets in hash ring   */
    struct ring_element_t **elements;
    struct ring_element_t *tree;        /*  Binary search tree representation of hash values of servers */
    uint32_t *servers;                  /*  Pointer to an array of hash values of servers */
    size_t numberofservers;             /*  Number of servers in hash ring */
    hashring_hash_t fn;                 /*  Pointer to the method responsible for hashing   */

} hashring_t;



/* 
[**************************************************************************************************************************************************]
                                                            METHODS / FUNCTIONS
[**************************************************************************************************************************************************]
*/

uint32_t hashring_deleteelement(ring_element_t *e);
ring_element_t * hashring_addserver(hashring_t *r, char *ip, int port, uint32_t virtualNodes);
uint32_t hashring_removeserver(hashring_t *r, char *ip, int port);
void hashring_destroy(hashring_t *r);
hashring_t * hashring_create(size_t size, hashring_hash_t fn);
ring_element_t * hashring_lookupkey(hashring_t *r, char *key );
uint32_t hashring_generatenodesarray(hashring_t *r);
uint32_t hashring_serverfindrange(hashring_t *r, uint32_t start);
uint32_t hashring_addvirtualnodes(hashring_t *r, char *ip, int port, uint32_t i);
uint32_t hashring_updateranges(hashring_t *r);
uint32_t hashring_remapkeys_add(hashring_t *r, ring_element_t *e);
uint32_t hashring_remapkeys_del(hashring_t *r, ring_element_t *e);
ring_element_t *hashring_lookupserver(hashring_t *r, char *ip, int port);
uint32_t hashring_deleteelement(ring_element_t *e);
bool hashring_objectexists(hashring_t *r, uint32_t index, uint8_t type);
uint32_t hashring_addstore_iterative(hashring_t *r, ring_element_t *e);
uint32_t hashring_addstore_bst(hashring_t *r, ring_element_t *e);
ring_element_t * hashring_addkey(hashring_t *r, char *key);
uint32_t hashring_removekey(hashring_t *r, char *key);
void hashring_showranges(hashring_t *r);


/**
 * @brief Finds the smallest node in a tree.
 * 
 * @param node 
 * @return node_t* 
 */
ring_element_t * bst_findmin(ring_element_t *e) {

    ring_element_t *current = e;

    while(true) {

        if( current->left != NULL ) {
            current = current->left;
            continue;
        }
        else {
            break;
        }
    }

    return current;
}



/**
 * @brief Logic for inserting a node into a tree
 * 
 * @param tree 
 * @param hash 
 * @param data 
 * @return node_t* 
 */
uint32_t bst_insert(hashring_t *r, ring_element_t *e) {

    /* Inserting into an empty tree */

    if(r->tree == NULL) {
        r->tree = e;
        e->parent = NULL;
        e->right = NULL;
        e->left = NULL;
    }

    else {
            
        /* Traverse tree and insert */
        ring_element_t *current = r->tree;

        while(true) {

            if(e->hash > current->hash) {
                /* Check if we should traverse right */
                if(current->right != NULL) {
                    current = current->right;
                    continue;
                }
                /* Insert right */
                else {
                    current->right = e;
                    e->parent = current;
                    break;
                }
            }

            if(e->hash < current->hash) {
                /* Check if we should traverse left */
                if(current->left != NULL) {
                    current = current->left;
                    continue;
                }
                /* Insert left */
                else {
                    current->left = e;
                    e->parent = current;
                    break;
                }
            }
            
            /* Should never hit */
            break;
        }
    }


    return EXIT_SUCCESS;

}

uint32_t bst_remove(hashring_t *r, ring_element_t *e) {

    ring_element_t *current = e;
    ring_element_t *s = NULL;
    
    /**
     * Case 1: Node has no children.
     */
    if( current->left == NULL && current->right == NULL) {
        if(current->parent != NULL) {
            if( current->parent->left == current) {
                current->parent->left = NULL;
            }
            
            if(current->parent->right == current) {
                current->parent->right = NULL;
            }
        }
        else {
            r->tree = NULL;
        }
    }
    
    /*
     * Case 2: Node has a two children.
     */
    else if(current->left != NULL && current->right != NULL) {

        /* Find minimum node of the right subtree */
        s = bst_findmin(current->right);
        s->parent->left = NULL;
        s->parent = NULL;
        
        /* If our node has a parent, we need to update that one*/
        if(current->parent != NULL) {

            s->parent = current->parent;

            if(current->left != NULL) {
                s->left = current->left;
                current->left->parent = s;
            }
            if(current->right != NULL) {
                s->right = current->right;
                current->right->parent = s;
            }

        }
        else {

            /* If node doesn't have a parent we are deleting the top root*/
            s->right = r->tree->right;
            s->right->parent = s;

            if(r->tree->left != NULL) {
                s->left = r->tree->left;
                r->tree->left->parent = s;
            }

            r->tree = s;

        }
    }

    
    /*
    * Case 3: Node has a single child. 
    */
    else {

        ring_element_t *child;

        if(current->left != NULL) {
            child = current->left;
        }
        else {
            child = current->right;
        }

        /* If current has no parent, we are deleting top root */
        if(current->parent == NULL) {
            child->parent = NULL;
            r->tree = child;
            return EXIT_SUCCESS;
        }
        else {
            if (current->parent->left == current) {
                current->parent->left = child;
            } else {  
                current->parent->right = child;
            }
            child->parent = current->parent;
        }
    }

    return EXIT_SUCCESS;
}



/**
 * @brief Creates a hashring.
 * 
 * @param size 
 * @param fn 
 * @return hashring_t* 
 */
hashring_t * hashring_create(size_t size, hashring_hash_t fn) {

    if(size < 0 || size == 0) {
        perror("size\n");
        return NULL;
    }

    hashring_t *r = (hashring_t *) malloc(sizeof(hashring_t));
    if(r == NULL) {
        perror("malloc\n");
        return NULL;
    }

    r->size = size;
    r->fn = fn;
    r->elements = malloc(size * sizeof(ring_element_t *));
    memset(r->elements, '\x00', size * sizeof(ring_element_t *));

    return r;

}



/**
 * @brief Destroys a hash ring by first deleleting elements and finally deleting the hash ring.
 * @param r 
 */
void hashring_destroy(hashring_t *r) {

    for(uint32_t i = 0; i < r->size; i++) {
        if(r->elements[i] != NULL) {
            hashring_deleteelement(r->elements[i]);
        }
    }

    free(r->elements);
    free(r->servers);
    free(r);

}



/**
 * @brief Destroys a hashring element.
 * 
 * @param e 
 * @return uint32_t 
 */
uint32_t hashring_deleteelement(ring_element_t *e) {

    if(e == NULL) {
        return EXIT_FAILURE;
    }

    switch(e->type) {
        case ELEMENT_EMPTY:
            break;
        case ELEMENT_KEY:
            printf("[*]: Deleting (key: %s) (store: %s)\n", e->element.data->key, e->element.data->store);
            free(e->element.data->key);
            free(e->element.data->store);
            break;
        case ELEMENT_SERVER:
            printf("[*]: Deleting (server: %s)\n", e->element.server->ip);
            free(e->element.server->ip);
            break;
    }

    free(e);
}




/**
 * @brief Looks up a given key in the hash ring and returns it if it exists.
 * 
 * @param r 
 * @param key 
 * @return ring_element_t* 
 */
ring_element_t * hashring_lookupkey(hashring_t *r, char *key ) {

    if(r == NULL || key == NULL) {
        return NULL;
    }

    uint32_t hash = r->fn(key);

    if(r->elements[hash] != NULL) {
        
        if(r->elements[hash]->type == ELEMENT_KEY) {
            return r->elements[hash];
        }
    }

    return NULL;
}



/**
 * @brief Dynamically generates a sorted array of all the nodes/stores in the hash ring.
 * 
 * @param r 
 * @return uint32_t 
 */
uint32_t hashring_generatenodesarray(hashring_t *r) {

    if(r->servers != NULL) {
        free(r->servers);
    }

    r->servers = (uint32_t *)malloc( sizeof(uint32_t) * r->numberofservers);
    uint32_t index = 0;
    for(uint32_t i = 0; i < r->size; i++) {
        if(r->elements[i] != NULL && r->elements[i]->type == ELEMENT_SERVER) {
            r->servers[index] = r->elements[i]->hash;
            index++;
        }
    }

}



/**
 * @brief Responsible for creating a virtual node of a node by prepending a suffix to the server ip.
 * 
 */
uint32_t hashring_addvirtualnodes(hashring_t *r, char *ip, int port, uint32_t i) {

    /* Suffix: 127.0.0.1 -> 127.0.0.1-1 */

    char buffer[4096] = { 0 };
    snprintf(buffer, ((size_t)4096), "%s-%u", ip, i);
    char *ip_modified = (char *)malloc( strlen(buffer) );
    strncpy(ip_modified, buffer, strlen(buffer));
    
    ring_element_t *e = hashring_addserver(r, ip_modified, port, 0);

    if( e != NULL) {
        e->element.server->isvirtualnode = true;
    }

    free(ip_modified);

}



/**
 * @brief Iteratively updating ranges for the nodes to account for inserted and deleted nodes/servers.
 * 
 * @param r 
 * @return uint32_t 
 */
uint32_t hashring_updateranges(hashring_t *r) {

    if(r->numberofservers <= 0) {
        return EXIT_FAILURE;
    }

    for(uint32_t x = 0; x < r->numberofservers; x++) {

        ring_element_t *e = r->elements[ r->servers[x] ];
        uint32_t start = e->hash;
        uint32_t i = start + 1;

        while(true) {

            if(i > r->size) {
                /* Loop around */
                i = 0;
                continue;
            }

            if(i == start) {
                break;
            }

            if( r->elements[i] != NULL) {
                if(r->elements[i]->type == ELEMENT_SERVER) {
                    break;
                }
            }
            i++;
        }

        e->element.server->rangeend = i - 1;

    }


}


/**
 * @brief Iteratively remap stores for keys upon insertion and or deletion of a node.
 * 
 * @param r 
 * @param e 
 * @return uint32_t 
 */
uint32_t hashring_remapkeys_add(hashring_t *r, ring_element_t *e) {
   
    if(r->numberofservers <= 0) {
        return EXIT_FAILURE;
    }

    uint32_t i = e->hash + 1;
    while(true) {

        if( i > r->size) {
            i = 0;
            continue;
        }

        if(i == e->element.server->rangeend + 1) {
            break;
        }

        if(r->elements[i] != NULL) {
            
            if(r->elements[i]->type == ELEMENT_SERVER) {
                /* Should not hit */
                break;
            }
        
            free(r->elements[i]->element.data->store);
            r->elements[i]->element.data->store = strdup(e->element.server->ip);
            r->elements[i]->element.data->port = e->element.server->port;

        }
        i++;
    }   

}


uint32_t hashring_remapkeys_del(hashring_t *r, ring_element_t *e) {

    if(r->numberofservers <= 0) {
        return EXIT_FAILURE;
    }

    ring_element_t *new = NULL;
    int32_t i = e->hash - 1;
    while(true) {

        if(i < 0) {
            i = r->size;
            continue;
        }

        if(i == e->hash) {
            printf("Error\n");
            break;
        }

        if( r->elements[i] != NULL) {
            if(r->elements[i]->type == ELEMENT_SERVER) {
                new = r->elements[i];
                break;
            }
        }
        i--;
    }

    i = e->hash + 1;
    while(true) {

        if(i > r->size) {
            i = 0;
            continue;
        }

        if(r->elements[i] != NULL) {

            if(r->elements[i]->type == ELEMENT_SERVER) {
                // Shouldn't happend.
                break;
            }

            char *old = r->elements[i]->element.data->store;
            r->elements[i]->element.data->store = strdup(new->element.server->ip);
            e->element.data->port = new->element.server->port;

            printf("[*]: Update key (%s) from store: %s to store %s\n",
            r->elements[i]->element.data->key,
            old,
            r->elements[i]->element.data->store);
        }
        i++;
    }
}




/**
 * @brief Adds a node/store/server to the hash ring and creates a specified amount of virtualnodes
 * 
 * @param r 
 * @param ip 
 * @param virtualNodes 
 * @return ring_element_t* 
 */
ring_element_t * hashring_addserver(hashring_t *r, char *ip, int port, uint32_t virtualNodes) {

    if(r == NULL || ip == NULL) {
        return NULL;
    }

    char buffer[4096] = { 0 };
    snprintf(buffer, ((size_t)4096), "%s-%u", ip, port);
    uint32_t hash = r->fn(buffer);

    ring_element_t *e = (ring_element_t *)malloc(sizeof(struct ring_element_t));
    if(e == NULL) {
        perror("malloc\n");
        return NULL;
    }

    /* Intiailize ring_element_t */
    e->hash = hash;
    e->left = NULL;
    e->right = NULL;
    e->type = ELEMENT_SERVER;

    /* Intialize ring_element_server_t */
    e->element.server = (ring_element_server_t *)malloc( sizeof(ring_element_server_t));
    if(e->element.server == NULL) {
        perror("malloc\n");
        free(e);
        return NULL;
    }
    e->element.server->ip = strdup(ip);
    e->element.server->port = port;
    e->element.server->rangestart = hash;
    e->element.server->numberofvirtualnodes = virtualNodes;

    /* Add hash to ring */
    if(r->elements[hash] != NULL) {
        perror("collision\n");
        hashring_deleteelement(e);
        return NULL;
    }
    r->elements[hash] = e;
    r->numberofservers++;
    r->count++;

    /* Add virtual nodes */
    for(uint32_t i = 0; i < virtualNodes; i++) {
        hashring_addvirtualnodes(r, ip, port, i);
    }

    /* Update binary search tree */
    bst_insert(r, e);

    /* Generate linear sorted array of nodes*/
    hashring_generatenodesarray(r);

    /* Update server ranges to account for inserted server*/
    hashring_updateranges(r);

    if(r->numberofservers > 1) {

        /* Remap keys */
        hashring_remapkeys_add(r, e);

    }

    printf("[*]: Inserted server IP: %s \tPORT: %u\t INDEX: %d\n", ip, port, hash);

    return e;

}



uint32_t hashring_removevirtualnodes(hashring_t *r, ring_element_t *e) {

    /* Suffix: 127.0.0.1 -> 127.0.0.1-1 */

    char buffer[4096] = { 0 };
    ring_element_t *v = NULL;

    for(uint32_t i = 0; i < e->element.server->numberofvirtualnodes; i++) {

        snprintf(buffer, ((size_t)4096), "%s-%u", e->element.server->ip, i);
        char *ip_modified = (char *)malloc( strlen(buffer) );
        strncpy(ip_modified, buffer, strlen(buffer));

        uint32_t hash = r->fn(ip_modified);
        v = r->elements[hash];

        if(v != NULL) {
            hashring_removeserver(r, ip_modified, e->element.server->port);
        }

        v = NULL;

        free(ip_modified);

    }



}

/**
 * @brief Removes a server from the hash ring and deletes the appropiate virtual nodes.
 * 
 * @param r 
 * @param ip 
 * @return uint32_t 
 */
uint32_t hashring_removeserver(hashring_t *r, char *ip, int port) {


    if(r == NULL || ip == NULL) {
        return EXIT_FAILURE;
    }

    ring_element_t *e = hashring_lookupserver(r, ip, port);
    
    if(e == NULL) {
        return EXIT_FAILURE;
    }
    
    r->elements[e->hash] = NULL;
    r->count--;
    r->numberofservers--;
    printf("[*]: Removing server: %s\n", e->element.server->ip);

    /* Remove virtual nodes */
    if(e->element.server->isvirtualnode == false) {
        hashring_removevirtualnodes(r, e);
    }

    /* Generate linear sorted array of nodes*/
    hashring_generatenodesarray(r);

    // Remove from tree
    bst_remove(r, e);
    
    /* Update server ranges to account for inserted server*/
    hashring_updateranges(r);

    /* Remap keys */
    hashring_remapkeys_del(r, e);

    hashring_deleteelement(e);

    return EXIT_SUCCESS;
}



/**
 * @brief Returns the server structure element for a given ip
 * 
 * @param r 
 * @param ip 
 * @return ring_element_server_t* 
 */
ring_element_t *hashring_lookupserver(hashring_t *r, char *ip, int port) {

    if(ip == NULL) {
        return NULL;
    }

    char buffer[4096] = { 0 };
    snprintf(buffer, ((size_t)4096), "%s-%u", ip, port);

    uint32_t hash = r->fn(buffer);

    if( r->elements[hash] == NULL) {
        return NULL;
    }

    if( r->elements[hash]->type != ELEMENT_SERVER ) {
        return NULL;
    }


    return r->elements[hash];

}



/**
 * @brief Checks if a hashring object exists at a given index.
 * 
 * @param r 
 * @param index 
 * @param type 
 * @return true 
 * @return false 
 */
bool hashring_objectexists(hashring_t *r, uint32_t index, uint8_t type) {

    if( r->elements[index] != NULL ) {
        if( r->elements[index]->type == type) {
            return true;
        }
    }

    return false;
}



/**
 * @brief Finds the store responsible for holding the key by interatively walking the hash ring.
 * 
 * @param r 
 * @param e 
 * @return uint32_t 
 */
uint32_t hashring_addstore_iterative(hashring_t *r, ring_element_t *e) {

    ring_element_t *current = e;
    ring_element_t *s = NULL;

    int32_t i = e->hash;

    /* Find the store responsible for holding the key by walking the hash ring in counter clockwise direction */
    while(true) {

        if(i < 0) {
            /* Loop around */
            i = r->size;
            continue;
        }

        if(r->elements[i] != NULL) {
            if(r->elements[i]->type == ELEMENT_SERVER) {
                s = r->elements[i];
                break;
            }
        }
        i--;
        
    }
    
    e->element.data->store = strdup(s->element.server->ip);
    e->element.data->port = s->element.server->port;
    
    return EXIT_SUCCESS;

}



/**
 * @brief Finds the store responsible for holding the key by walking the binary search tree
 * 
 * @param r 
 * @param e 
 * @return uint32_t 
 */
uint32_t hashring_addstore_bst(hashring_t *r, ring_element_t *e) {
    
    if(r->numberofservers == 0 || r->servers == NULL) {
        return EXIT_FAILURE;
    }

    /* Transform tree to array */
    uint32_t servers[r->numberofservers];

    /* Walk the array to find store for hash value, using two indices approach */
    uint32_t hashofstore = 0;

    uint32_t x = 0;

    while(true) {

        uint32_t y = x + 1;
        if(y > r->numberofservers) {
            hashofstore = servers[0];
            break;
        }

        if(e->hash > servers[y]) {
            x++;
            continue;
        }

        if(e->hash < servers[y] && e->hash > servers[x]) {
            hashofstore = servers[y];
            break;
        }

        x++;   
    }

    e->element.data->store = strdup(r->elements[hashofstore]->element.server->ip);

    return EXIT_SUCCESS;

}



/**
 * @brief Adds a key to the hash ring
 * 
 * @param r 
 * @param key 
 * @return uint32_t 
 */
ring_element_t * hashring_addkey(hashring_t *r, char *key) {

    if( r == NULL || key == NULL) {
        return NULL;
    }

    uint32_t hash = r->fn(key);

    /* Collisionen, an entry already exists on calculated element. */

    /* TODO: Handle when a key matches the hash value of a node(server) */
    if(hashring_objectexists(r, hash, ELEMENT_KEY) != false) {
        return NULL;
    }
    
    ring_element_t *e = (ring_element_t *)malloc(sizeof(struct ring_element_t));
    if(e == NULL) {
        perror("malloc\n");
        return NULL;
    }

    /* Intiailize ring_element_t */
    e->hash = hash;
    e->left = NULL;
    e->right = NULL;
    e->type = ELEMENT_KEY;

    /* Intialize ring_element_key_t */
    e->element.data = (ring_element_key_t *)malloc( sizeof(ring_element_key_t));
    if(e->element.data == NULL) {
        perror("malloc\n");
        free(e);
        return NULL;
    }
    e->element.data->key = strdup(key);
    e->element.data->store = NULL;

    /* Add to hash ring */
    r->elements[hash] = e;

    /* Update store linkage */
    hashring_addstore_iterative(r, e);

    printf("[*]: Inserted Key %s at index %d in server %s\n", e->element.data->key, e->hash, e->element.data->store);

    r->count++;

    return e;

}



/**
 * @brief Removes a key from the hash ring.
 * 
 * @param r 
 * @param key 
 * @return uint32_t 
 */
uint32_t hashring_removekey(hashring_t *r, char *key) {


      if( r == NULL || key == NULL) {
        return EXIT_FAILURE;
    }

    uint32_t hash = r->fn(key);

    ring_element_t *e = hashring_lookupkey(r, key);

    if( e == NULL) {
        return EXIT_FAILURE;
    }

    if( e->type != ELEMENT_KEY) {
        return EXIT_FAILURE;
    }

    r->elements[hash] = NULL;

    printf("[*]: Removing key %s from server %s\n", key, e->element.data->store);

    hashring_deleteelement(e);

    r->count--;

    return EXIT_SUCCESS;
}



/**
 * @brief DJB2 Hashing method.
 * 
 * @param key 
 * @return uint32_t 
 */
uint32_t hashring_hash_djb2(char *key) {

    uint32_t hash = 5381;
    int c;

    while ((c = *key++)) {
        hash = ((hash << 5) + hash) + c;
    }

    uint32_t val = hash % 4000000; 

    return val;
}



/**
 * @brief Jenkisn Hashing Method
 * 
 * @param key 
 * @return uint32_t 
 */
uint32_t hashring_hash_jenkins(char *key) {

    size_t len = strlen(key);

    uint32_t hash = 1;
    for (uint32_t i = 0; i < len; ++i) {
        hash += key[i];
        hash += (hash << 10);
        hash ^= (hash >> 6);
    }

    hash += (hash << 3);
    hash ^= (hash >> 11);
    hash += (hash << 15);

    uint32_t val = (hash % 4000000) + 1;
    

    //printf("%s->%d\n", key, val);

    return val;

}



/**
 * @brief Shows the reponsible key range for the nodes in the hash ring.
 * 
 * @param r 
 */
void hashring_showranges(hashring_t *r) {
            
    for(uint32_t i = 0; i < r->numberofservers; i++) {
        ring_element_t *e = r->elements[r->servers[i]];
        printf("%s (%d) | %d -> %d\n", r->elements[ r->servers[i] ]->element.server->ip, e->hash, e->element.server->rangestart, e->element.server->rangeend);
    }

}

#endif