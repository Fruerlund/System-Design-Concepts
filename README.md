
# System Design Concepts

This repository contains C code written by a hobbyist self-taught programmer key concepts from the books System Design Interview volume one and two.

You '''will''' not find full fledged flawless implementations but instead working proof of concepts utilizing features such as mutexes, multithreading, networking, sockets, custom protocols and APIs.

**Thus you will find, as per disclaimer, among other things lack of return checks, different use of returns (int, bools), large source files, repetive code, lack of project structure**

The code is not flawless and does not account for all edge cases.

Finally this project is not an example of how to structure C projects. In this repository each of the features/concepts have been implemented in a single source file with a single header and are thus built indepedently.

Finally you can find a build script that will utilize docker to build a network where you can chain the different components together finally building a system of your needs.

I hope you find this useful.




## Features

### Load balancer
The load balancer is a multithreaded application that exposes a local server running on a hardcoded port. 

Requests to the server are handled by accept spawning new threads for handling each connection. 

A HTTP request is read from the connection and enqued into a queue for processing by the coordinator.

The coordinator is reponsible for spawning further threads where each thread handles a single request by forwarding and replying back to the original client.

Uses:

* Multithreading
* Mutexes & Locks
* Queue / lists.
* Sockets and networking
* Header Injection
* Session Management

#### Flow Graph
```bash
                                       ┌────────────────┐ ┌─────────────────────────────────────┐            
                                 ┌────►│DEQEUE(REQUESTS)├─► CONSUMERFORWARDSINGLEREQUEST(THREAD)│             
                                 │     └────────────────┘ └─────────────────────────────────────┘            
                                 │                                                                           
┌────────────────────────────────┴─┐   ┌────────────────┐ ┌─────────────────────────────────────┐            
│COORDINATORFORWARDREQUESTS(THREAD)├──►│DEQEUE(REQUESTS)├─► CONSUMERFORWARDSINGLEREQUEST(THREAD)│            
└────────────────────────────────┬─┘   └────────────────┘ └─────────────────────────────────────┘            
                         ▲       │                                                                           
                         │       │     ┌────────────────┐ ┌─────────────────────────────────────┐            
              ┌────┐     │       └────►│DEQEUE(REQUESTS)├─► CONSUMERFORWARDSINGLEREQUEST(THREAD)│            
              │MAIN├─────┘             └────────────────┘ └─────────────────────────────────────┘            
              └─┬──┘                                                                                         
                │                                                                                            
                ▼                                                                                            
       ┌───────────────────────────────┐                                                                     
   ┌───┤BEGINSERVERLISTEN (WHILE LOOP) │                                                                                   
   │   └───────────────────────────────┘                                                                                   
   │                                                                                                         
   │         ┌──────┐                                                                                        
   └────────►│ACCEPT├──────┬──────────────────────┬────────────────────────────────┐                         
             └──────┘      │                      │                                │                         
                           ▼                      ▼                                ▼                         
  ┌─────────────────────────────┐ ┌─────────────────────────────┐  ┌─────────────────────────────┐           
  │ PRODUCERHANDLEACCEPT(THREAD)│ │ PRODUCERHANDLEACCEPT(THREAD)│  │ PRODUCERHANDLEACCEPT(THREAD)│           
  └─────────────────────────────┘ └─────────────────────────────┘  └─────────────────────────────┘           
                ▼                                ▼                                 ▼                         
        ┌────────────────┐              ┌────────────────┐               ┌────────────────┐                  
        │ENQEUE(REQUESTS)│              │ENQEUE(REQUESTS)│               │ENQEUE(REQUESTS)│                  
        └────────────────┘              └────────────────┘               └────────────────┘                  

```



### Distributed key, value pair system.

The distributed key value pair system is a redis style system created in a single application that depedent on user input can transform into a store, also called nodes utilizing a hash table or a coordinator using a hash ring that supports consistent hashing. It uses many of the same features as the load balancer including:

* Multithreading
* Mutexes & Locks
* Queue / lists.
* Sockets and networking

And further:

* Hash tables
* Hash Ring
* Consistent Hashing

Additionally the components utilize a REST Style HTTP Protocol that allows programs and users to interact with the store or coordinator to trigger the API methods that operate on the hash ring or hash table. These protocol commands are:

```bash
SET: Inserts a key, value pair into the store.
GET: Gets a value from a given key from the store.
REM: Removes a value from a given key from the store.
ADD: Adds a new data server into the hash ring.
DEL: Deletes a data server from the hash ring.
SYNC: Sync keys between stores and coordinator.
```

The application uses HTTP POST protocol in the format of:

```bash
POST / HTTP/1.1
Host: server
[headers]
cmd=SET&key=value
```

#### Store

The application can transform into a store which serves a single purpose of storing data recieved from the coordinator using the above provided API. It uses a hash table with basic methods such as insert, delete and lookup.

#### Coordinator

The application can also transform into a coordinator that is responsible for mapping stores/nodes onto a hash ring which also contains keys. Insertion and deletion of keys will map the key to a given store. Insertion and deletion of stores will trigger remapping of keys & existing store ranges.

#### Flow Graph
```bash
                              ┌─────┐      ┌───────────────┐   ┌─────────┐                     
                   ┌─────────►│STORE├─────►│ HASHTABLE_API ┼──►│HASHTABLE│◄──────────────┐     
                   │          └───┬─┘      └───────────────┘   └─────────┘               │     
                   │              │                                                      │     
┌───────────┐   ┌──┼───┐      ┌───▼────────┐  ┌───────────┐    ┌───────────────┐         │     
│ARGV(INPUT)├──►│ MAIN │      │SERVERLISTEN┼─►│ ACCEPTLOOP┼───►│ENQUE(REQUESTS)│         │     
└───────────┘   └──┬───┘      └─────▲──────┘  └───────────┘    └───────────────┘         │     
                   │                │                                                    │     
                   │          ┌─────┴─────┐   ┌─────────────┐  ┌─────────┐               │     
                   ├─────────►│COORDINATOR┼──►│HASHRING_API ┼─►│ HASHRING│◄──────────────┤     
                   │          └───────────┘   └─────────────┘  └─────────┘               │     
                   │                                                                     │     
                   │                                                                     │     
                   │          ┌──────────────┐ ┌─────────────┐ ┌───────────────┐ ┌───────┼────┐
                   └─────────►│REQUESTWORKERS┼►│DEQUE(REQUEST┼►│HANDLEREQUEST()├─►API HANDLING│
                              └──────────────┘ └─────────────┘ └───────────────┘ └────────────┘
```

### Hash Table

The hash table implements a simplistic key, value pair implementing simple methods (lookup, insert and delete)

It using function prototypes to allow for a custom hashing method to be used.

``` bash
```

### Hash Ring
```bash
```

### Rate Limiter
```bash
```

## Installation  and usage

The individual features can be built manually using a compiler of your choice(GCC) or using the simple provided makefile

For example:

```bash
make loadbalancer
```
    
## Feedback

If you have any feedback, please reach out to me.


## Acknowledgements

 - [System Design Book](https://www.google.com/search?hl=da&q=System%20Design%20book)



## Usage/Examples

To interact with the system, which you can choose to adjust accordingly to your needs, you can utilize the two default script provided: create.sh and killcontainers.sh. These two files will setup a simple network consisting of three webservers, a load balancer, a distributed key value store consisting of a coordinator and three stores.

### Create the network
```bash
root@HOST:/home/user/projects/systemdesign/docker# ./create.sh
Starting Docker Service...
Starting Docker: docker
sessions should be nested with care, unset $TMUX to force
Checking for docker image
Building Docker image...
[+] Building 27.4s (12/12) FINISHEDdocker:default
 => [internal] load build definition from Dockerfile0.0s
 => => transferring dockerfile: 352B0.0s
 => [internal] load metadata for docker.io/library/ubuntu:22.041.0s
 => [internal] load .dockerignore0.0s
 => => transferring context: 2B0.0s
 => CACHED [1/7] FROM docker.io/library/ubuntu:22.04@sha256:340d9b015b194dc6e2a13938944e0d016e57b9679963fdeb9ce021daac4302210.0s
 => [internal] load build context0.0s
 => => transferring context: 127B0.0s
 => [2/7] RUN mkdir -p  /app0.3s
 => [3/7] RUN apt-get -y update; apt-get install -y sudo; apt-get install -y libc6 libc6-dev build-essential net-tools iputils-ping iproute2 wget curl24.0s
 => [4/7] WORKDIR /app0.0s
 => [5/7] COPY  ../src/loadbalancer /app/loadbalancer0.0s
 => [6/7] COPY  ../src/dkvstore /app/dkvstore0.0s
 => [7/7] COPY  ../src/server.py /app/server.py0.0s
 => exporting to image1.9s
 => => exporting layers1.9s
 => => writing image sha256:d4649f4a9dc09cd9bf8035b912d3ce2642dd44be6557de7112e232e3273ee2570.0s
 => => naming to docker.io/library/systemdesign0.0s
Setting up firewall rules...
Setting up TMUX...
duplicate session: services
Starting web servers...
Serving HTTP on 0.0.0.0 port 5000 (http://0.0.0.0:5000/) ...
Serving HTTP on 0.0.0.0 port 5002 (http://0.0.0.0:5002/) ...
Serving HTTP on 0.0.0.0 port 5001 (http://0.0.0.0:5001/) ...
Starting load balancer...
To attach to TMUX session, execute the command...
tmux attach-session -t services
```

### Interact
Start sending requests to the loadbalancer exposed on hardcoded port 5555.
```bash
tmux attach-session -t services
/home/user/projects/systemdesign/docker# curl http://127.0.0.1:5555   
```

### Examine
Watch how requests are balanced on the load balanacer
```bash
docker run --rm -it --network="host" --name loadbalancer systemdesign /app/loadbalancer 127.0.0.1:5000 127.0.0.1:5001 127.0.0.1:5002
[+]: Created Main (TID: 1)
[+]: Opened connection to server at: 127.0.0.1:5000
[+]: Opened connection to server at: 127.0.0.1:5001
[+]: Opened connection to server at: 127.0.0.1:5002
[+]: Bind to 5555 OK!
[+]: Created Request Coordinator (TID: 7)
[+]: Listen OK
(FWD) [IP: 127.0.0.1 | Port: 56024 | Byte(s): 78 ]      -->     [ IP: 127.0.0.1 | Port: 5002 ]  127.0.0.1 - - [16/Aug/2024 11:31:15] "GET / HTTP/1.1" 200 -
                                                           (RET) [IP: 127.0.0.1 | Port: 56024 | Byte(s): 336 ]  <--     [ IP: 127.0.0.1 | Port: 5002 ]
(FWD) [IP: 127.0.0.1 | Port: 56032 | Byte(s): 78 ]      -->     [ IP: 127.0.0.1 | Port: 5001 ]
127.0.0.1 - - [16/Aug/2024 11:31:17] "GET / HTTP/1.1" 200 -
                                                           (RET) [IP: 127.0.0.1 | Port: 56032 | Byte(s): 336 ]  <--     [ IP: 127.0.0.1 | Port: 5001 ]
(FWD) [IP: 127.0.0.1 | Port: 56048 | Byte(s): 78 ]      -->     [ IP: 127.0.0.1 | Port: 5000 ]
127.0.0.1 - - [16/Aug/2024 11:31:17] "GET / HTTP/1.1" 200 -
                                                           (RET) [IP: 127.0.0.1 | Port: 56048 | Byte(s): 336 ]  <--     [ IP: 127.0.0.1 | Port: 5000 ]
(FWD) [IP: 127.0.0.1 | Port: 56056 | Byte(s): 78 ]      -->     [ IP: 127.0.0.1 | Port: 5001 ]
127.0.0.1 - - [16/Aug/2024 11:31:18] "GET / HTTP/1.1" 200 -
                                                           (RET) [IP: 127.0.0.1 | Port: 56056 | Byte(s): 336 ]  <--     [ IP: 127.0.0.1 | Port: 5001 ]
(FWD) [IP: 127.0.0.1 | Port: 56070 | Byte(s): 78 ]      -->     [ IP: 127.0.0.1 | Port: 5000 ]
127.0.0.1 - - [16/Aug/2024 11:31:18] "GET / HTTP/1.1" 200 -
                                                           (RET) [IP: 127.0.0.1 | Port: 56070 | Byte(s): 336 ]  <--     [ IP: 127.0.0.1 | Port: 5000 ]
(FWD) [IP: 127.0.0.1 | Port: 56072 | Byte(s): 78 ]      -->     [ IP: 127.0.0.1 | Port: 5001 ]
127.0.0.1 - - [16/Aug/2024 11:31:18] "GET / HTTP/1.1" 200 -
                                                           (RET) [IP: 127.0.0.1 | Port: 56072 | Byte(s): 336 ]  <--     [ IP: 127.0.0.1 | Port: 5001 ]
(FWD) [IP: 127.0.0.1 | Port: 56086 | Byte(s): 78 ]      -->     [ IP: 127.0.0.1 | Port: 5002 ]
127.0.0.1 - - [16/Aug/2024 11:31:19] "GET / HTTP/1.1" 200 -
                                                           (RET) [IP: 127.0.0.1 | Port: 56086 | Byte(s): 336 ]  <--     [ IP: 127.0.0.1 | Port: 5002 ]
```

```bash
root@HOST:/home/user/projects/systemdesign/docker# docker run --rm -it --network="host" --name store1 systemdesign /app/dkvstore -t store -p 6000
[+]: Creating request handlers
[+]: Started KVP Store server: (1)
[+]: HTTP Handler Created (TID: 7)
[+]: Bind to 6000 OK!
[+]: Listen OK
[+]: Server awaiting connections
```

```bash
root@HOST:/home/user/projects/systemdesign/docker# docker run --rm -it --network="host" --name store2 systemdesign /app/dkvstore -t store -p 6001
[+]: Creating request handlers
[+]: Started KVP Store server: (1)
[+]: HTTP Handler Created (TID: 7)
[+]: Bind to 6001 OK!
[+]: Listen OK
[+]: Server awaiting connections
```


```bash
root@HOST:/home/user/projects/systemdesign/docker# docker run --rm -it --network="host" --name store3 systemdesign /app/dkvstore -t store -p 6002
[+]: Creating request handlers
[+]: Started KVP Store server: (1)
[+]: HTTP Handler Created (TID: 7)
[+]: Bind to 6002 OK!
[+]: Listen OK
[+]: Server awaiting connections
```


```bash
root@HOST:/home/user/projects/systemdesign/docker# docker run --rm -it --network="host" --name coordinator systemdesign /app/dkvstore -t coordinator -p 31337 -s "127.0.0.1:6000,127.0.0.1:6001,127.0.0.1:6002"
[+]: Creating request handlers
[+]: Started KVP Coordinator server: (1)
[+]: HTTP Handler Created (TID: 7)
Test: 127.0.0.1:6000,127.0.0.1:6001,127.0.0.1:6002
[*]: Inserted server IP: 127.0.0.1-0    PORT: 6000       INDEX: 2780939
[*]: Inserted server IP: 127.0.0.1-1    PORT: 6000       INDEX: 2232991
[*]: Inserted server IP: 127.0.0.1-2    PORT: 6000       INDEX: 3134266
[*]: Inserted server IP: 127.0.0.1-3    PORT: 6000       INDEX: 808162
[*]: Inserted server IP: 127.0.0.1-4    PORT: 6000       INDEX: 2203716
[*]: Inserted server IP: 127.0.0.1-5    PORT: 6000       INDEX: 2998690
[*]: Inserted server IP: 127.0.0.1-6    PORT: 6000       INDEX: 3520728
[*]: Inserted server IP: 127.0.0.1-7    PORT: 6000       INDEX: 3581073
[*]: Inserted server IP: 127.0.0.1-8    PORT: 6000       INDEX: 3618994
[*]: Inserted server IP: 127.0.0.1-9    PORT: 6000       INDEX: 2139019
[*]: Inserted server IP: 127.0.0.1      PORT: 6000       INDEX: 1265576
[*]: Inserted server IP: 127.0.0.1-0    PORT: 6001       INDEX: 327784
[*]: Inserted server IP: 127.0.0.1-1    PORT: 6001       INDEX: 1986308
[*]: Inserted server IP: 127.0.0.1-2    PORT: 6001       INDEX: 1987507
[*]: Inserted server IP: 127.0.0.1-3    PORT: 6001       INDEX: 840814
[*]: Inserted server IP: 127.0.0.1-4    PORT: 6001       INDEX: 2305900
[*]: Inserted server IP: 127.0.0.1-5    PORT: 6001       INDEX: 2602773
[*]: Inserted server IP: 127.0.0.1-6    PORT: 6001       INDEX: 766492
[*]: Inserted server IP: 127.0.0.1-7    PORT: 6001       INDEX: 107013
[*]: Inserted server IP: 127.0.0.1-8    PORT: 6001       INDEX: 1974546
[*]: Inserted server IP: 127.0.0.1-9    PORT: 6001       INDEX: 3717892
[*]: Inserted server IP: 127.0.0.1      PORT: 6001       INDEX: 2652667
[*]: Inserted server IP: 127.0.0.1-0    PORT: 6002       INDEX: 281139
[*]: Inserted server IP: 127.0.0.1-1    PORT: 6002       INDEX: 2140840
[*]: Inserted server IP: 127.0.0.1-2    PORT: 6002       INDEX: 962123
[*]: Inserted server IP: 127.0.0.1-3    PORT: 6002       INDEX: 1534885
[*]: Inserted server IP: 127.0.0.1-4    PORT: 6002       INDEX: 3710447
[*]: Inserted server IP: 127.0.0.1-5    PORT: 6002       INDEX: 1905291
[*]: Inserted server IP: 127.0.0.1-6    PORT: 6002       INDEX: 3880093
[*]: Inserted server IP: 127.0.0.1-7    PORT: 6002       INDEX: 3807884
[*]: Inserted server IP: 127.0.0.1-8    PORT: 6002       INDEX: 763300
[*]: Inserted server IP: 127.0.0.1-9    PORT: 6002       INDEX: 1901298
[*]: Inserted server IP: 127.0.0.1      PORT: 6002       INDEX: 3312216
[+]: Bind to 31337 OK!
[+]: Listen OK
[+]: Server awaiting connections
```
