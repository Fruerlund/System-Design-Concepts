/**
 * @file hashtable.h
 * @author Fruerlund
 * @brief 
 * @version 0.1
 * @date 2024-08-03
 * 
 * @copyright Copyright (c) 2024
 * 
 */

#ifndef HASTABLE_H
#define HASHTABLE_H

#include "common-defines.h"



/* 
[**************************************************************************************************************************************************]
                                                            STRUCTUES AND PROTOTYPES
[**************************************************************************************************************************************************]
*/

/**
 * @brief Function protoype for hashing methods
 * 
 */
typedef uint32_t (*hashtable_hashmethod)(char *key);



/**
 * @brief Describes the linked list entry.
 * 
 */
typedef struct hashtable_bucket_item {

    char *key;
    char *value;
    LIST_ENTRY(hashtable_bucket_item) entries;

} hashtable_bucket_item;



/**
 * @brief Describes the bucket in a hash table. Each bucket holds a linked list.
 * 
 */
typedef struct hashtable_bucket_t {

    LIST_HEAD(hashtable_list, hashtable_bucket_item) list;

} hashtable_bucket_t;




/**
 * @brief Describes the hash table. The hash table holds buckets with linked lists.
 * 
 * 
 * Our table has the following layout utilizing linked lists for handling collisions
 * 
 * TABLE:
 *      BUCKET -> LINKED LIST -> ENTRY 1, ENTRY 2
 *      BUCKET -> LINKED LIST -> ENTRY 1, ENTRY 2
 *      BUCKET -> LINKED LIST -> ENTRY 1, ENTRY 2
 *      BUCKET -> LINKED LIST -> ENTRY 1, ENTRY 2
 */

typedef struct hashtable_t {

    uint32_t size;                          /*      Holds the size of the hash table                */
    uint32_t count;                         /*      Holds the number of current elements            */
    hashtable_bucket_t **buckets;           /*      Array of hashtable buckets                      */
    hashtable_hashmethod hashmethod;        /*      Pointer to the method responsible for hashing   */

} hashtable_t;



/* 
[**************************************************************************************************************************************************]
                                                            METHODS / FUNCTIONS
[**************************************************************************************************************************************************]
*/



/**
 * @brief Creates a hash bucket initializing the linked list in the bucket.
 * 
 * @return hashtable_bucket_t* 
 */
hashtable_bucket_t *hashbucket_create() {

    hashtable_bucket_t *bucket = (hashtable_bucket_t *) malloc (sizeof(hashtable_bucket_t ));
    
    if(bucket == NULL) {
        // printf("[ERROR]: Failed to create bucket in hash table\n");
    }

    LIST_INIT(&bucket->list);
    return bucket;

}



/**
 * @brief Deletes the contents of the linked list in the hash bucket.
 * 
 * @param bucket 
 */
void hashbucket_delete(hashtable_bucket_t *bucket) {

    hashtable_bucket_item *h1, *h2;
    h1 = LIST_FIRST(&bucket->list);
    while( h1 != NULL ) {
        h2 = LIST_NEXT(h1, entries);
        free(h1);
        h1 = h2;
    }
}



/**
 * @brief Creates a hash table by allocating memory for a specified number of buckets.
 * 
 * @param size 
 * @param hashmethod 
 * @return hashtable_t* 
 */
hashtable_t * hashtable_create(uint32_t size, hashtable_hashmethod hashmethod) {

    hashtable_t *table = NULL;

    table = (hashtable_t *) malloc (sizeof(hashtable_t));
    
    /* Failed to allocate new hash table */
    if(table == NULL) {
        exit(EXIT_FAILURE);
    }

    table->size = size;
    table->count = 0;

    table->buckets = malloc(table->size * sizeof(hashtable_bucket_t *));

    for(uint32_t i = 0; i < size; i++) {
        table->buckets[i] = hashbucket_create();
    }

    table->hashmethod = hashmethod;

    return table;

}



/**
 * @brief Deletes a hash table by freeing allocated buckets and finally the table itself.
 * 
 * @param table 
 */
void hashtable_delete(hashtable_t *table) {

    /* Iterate over all buckets, then over each entry in the nested list */
    for(uint32_t i = 0; i < table->size; i++) {
        if(table->buckets[i] != NULL) {
            hashbucket_delete(table->buckets[i]);
            free(table->buckets[i]);
        }   
    }
    free(table->buckets);
    free(table);
}



/**
 * @brief Implements a simple DJB2_HASH algorithm for use in this hash table. See resource at: https://stackoverflow.com/questions/7666509/hash-function-for-string
 * 
 * @param key 
 * @return uint32_t 
 */
uint32_t hashtable_hash(char *key) {    

    uint32_t hash = 5381;
    int c;
    while (c = *key++)
        hash = ((hash << 5) + hash) + c; 
    return hash;


}



/**
 * @brief Performs a lookup in the hash table
 * 
 * @param table 
 * @param key 
 * @return hashtable_bucket_item* 
 */
hashtable_bucket_item *hashtable_lookup(hashtable_t *table, char *key) {

    hashtable_bucket_item *result = NULL;

    /* Calculate hash/index  */
    uint32_t hash = table->hashmethod(key);

    /* Get bucket at index */
    uint32_t index = hash % table->size;
    hashtable_bucket_t *bucket = table->buckets[index];

    /* Check if list in bucket is empty or not */
    if(LIST_EMPTY(&bucket->list)) {
        return result;
    }

    /* If list in bucket is not empty, it may have multiple entries due to collisions, thus we iterate over each list entry to find our entry */
    hashtable_bucket_item *n1 = LIST_FIRST(&bucket->list);
    while (n1 != NULL) {
        if( strcmp(key, n1->key) == 0) {
            result = n1;
            return result;
        }
        n1 = LIST_NEXT(n1, entries);
    }
    

    return result;
}



/**
 * @brief Inserts a key, value pair into a bucket placed the hash table. In case of collision the new kvp is stored as a linked list entry.
 * 
 * @param table 
 * @param key 
 * @param value 
 * @return true 
 * @return false 
 */
bool hashtable_insert(hashtable_t *table, char *key, char *value) {

    /* Check if key already exists */
    if(hashtable_lookup(table, key) != NULL) {
        // perror("[ERROR]: Key already exists in hash table\n");
        return false;
    }

    /* TODO: Check if our hash table will overspill, if so perform resizing*/

    /* Calculate hash/index  */
    uint32_t hash = table->hashmethod(key);

    /* Get bucket at index */
    uint32_t index = hash % table->size;
    hashtable_bucket_t *bucket = table->buckets[index];

    /* Insert new item into list in bucket */
    hashtable_bucket_item *n1 = malloc(sizeof(struct hashtable_bucket_item));
    if(n1 == NULL) {
        return false;
    }

    /* DEBUG: Check if for a collision in bucket */
    if(LIST_EMPTY(&bucket->list)) {
        // perror("[DEBUG]: Collision\n");
    }

    n1->key = strdup(key);
    n1->value = strdup(value);
    LIST_INSERT_HEAD(&bucket->list, n1, entries);

    //printf("[*]: Inserted key %s at index: %d\n", key, index);

    table->count++;
    
    return true;

}


/**
 * @brief Removes a key, value pair from the hash table.
 * 
 * @param table 
 * @param key 
 * @return true 
 * @return false 
 */
bool hashtable_remove(hashtable_t *table, char *key) {

     /* Check if key  exists */
    if(hashtable_lookup(table, key) == NULL) {
        // perror("[ERROR]: Key doesn't exists in hash table\n");
        return false;
    }

    /* Calculate hash/index  */
    uint32_t hash = table->hashmethod(key);

    /* Get bucket at index */
    uint32_t index = hash % table->size;
    hashtable_bucket_t *bucket = table->buckets[index];

    /* If list in bucket is not empty we iterate over each list entry */
    hashtable_bucket_item *n1 = LIST_FIRST(&bucket->list);
    while (n1 != NULL) {
        if( strcmp(key, n1->key) == 0) {
            LIST_REMOVE(n1, entries);
            free(n1);
            break;
        }
        n1 = LIST_NEXT(n1, entries);
    }

    table->count--;
    return true;
    
}



void hashtable_display(hashtable_t *table) {
    

    for(uint32_t i = 0; i < table->size + 1; i++) {

        /* Get bucket */
        hashtable_bucket_t *bucket = table->buckets[i];
        if(bucket != NULL) {
            /* Check if list in bucket is empty or not */
            if(LIST_EMPTY(&bucket->list)) {
                continue;
            }
            else {
                hashtable_bucket_item *n1 = LIST_FIRST(&bucket->list);
                while (n1 != NULL) {
                    printf("%s -> %s\n", n1->key, n1->value);
                    n1 = LIST_NEXT(n1, entries);
                }
            }
        }

    }
}

   

#endif