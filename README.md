
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

### Rate Limiter
```bash
```

### Distributed key, value store
```bash
```

### Hash Table

The hash table implements a simplistic key, value pair implementing simple methods (lookup, insert and delete)

It using function prototypes to allow for a custom hashing method to be used.

``` bash

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

Running the loadbalancer

```bash
./loadbalancer 127.0.0.1:1234 127.0.0.1:1235
```

Start sending requests to the loadbalancer exposed on hardcoded port 5555.

```bash
(FWD) [IP: 127.0.0.1 | Port: 43186 | Byte(s): 78 ]      -->     [ IP: 127.0.0.1 | Port: 1234 ]
(RET) [IP: 127.0.0.1 | Port: 43182 | Byte(s): 6343 ]    <--     [ IP: 127.0.0.1 | Port: 1234 ]
(RET) [IP: 127.0.0.1 | Port: 43186 | Byte(s): 6343 ]    <--     [ IP: 127.0.0.1 | Port: 1234 ]
(FWD) [IP: 127.0.0.1 | Port: 43188 | Byte(s): 78 ]      -->     [ IP: 127.0.0.1 | Port: 1234 ]
(FWD) [IP: 127.0.0.1 | Port: 43198 | Byte(s): 78 ]      -->     [ IP: 127.0.0.1 | Port: 1234 ]
(RET) [IP: 127.0.0.1 | Port: 43188 | Byte(s): 6343 ]    <--     [ IP: 127.0.0.1 | Port: 1234 ]
(RET) [IP: 127.0.0.1 | Port: 43198 | Byte(s): 6343 ]    <--     [ IP: 127.0.0.1 | Port: 1234 ]
(FWD) [IP: 127.0.0.1 | Port: 43212 | Byte(s): 78 ]      -->     [ IP: 127.0.0.1 | Port: 1234 ]
(FWD) [IP: 127.0.0.1 | Port: 43216 | Byte(s): 78 ]      -->     [ IP: 127.0.0.1 | Port: 1234 ]
(RET) [IP: 127.0.0.1 | Port: 43212 | Byte(s): 6343 ]    <--     [ IP: 127.0.0.1 | Port: 1234 ]
(RET) [IP: 127.0.0.1 | Port: 43216 | Byte(s): 6343 ]    <--     [ IP: 127.0.0.1 | Port: 1234 ]
(FWD) [IP: 127.0.0.1 | Port: 43230 | Byte(s): 78 ]      -->     [ IP: 127.0.0.1 | Port: 1234 ]
(FWD) [IP: 127.0.0.1 | Port: 43240 | Byte(s): 78 ]      -->     [ IP: 127.0.0.1 | Port: 1234 ]
(RET) [IP: 127.0.0.1 | Port: 43230 | Byte(s): 6343 ]    <--     [ IP: 127.0.0.1 | Port: 1234 ]
(RET) [IP: 127.0.0.1 | Port: 43240 | Byte(s): 6343 ]    <--     [ IP: 127.0.0.1 | Port: 1234 ]
(FWD) [IP: 127.0.0.1 | Port: 43256 | Byte(s): 78 ]      -->     [ IP: 127.0.0.1 | Port: 1234 ]
(FWD) [IP: 127.0.0.1 | Port: 43262 | Byte(s): 78 ]      -->     [ IP: 127.0.0.1 | Port: 1234 ]
(RET) [IP: 127.0.0.1 | Port: 43256 | Byte(s): 6343 ]    <--     [ IP: 127.0.0.1 | Port: 1234 ]
(RET) [IP: 127.0.0.1 | Port: 43262 | Byte(s): 6343 ]    <--     [ IP: 127.0.0.1 | Port: 1234 ]
(FWD) [IP: 127.0.0.1 | Port: 43278 | Byte(s): 78 ]      -->     [ IP: 127.0.0.1 | Port: 1234 ]
(FWD) [IP: 127.0.0.1 | Port: 43282 | Byte(s): 78 ]      -->     [ IP: 127.0.0.1 | Port: 1234 ]
(RET) [IP: 127.0.0.1 | Port: 43278 | Byte(s): 6343 ]    <--     [ IP: 127.0.0.1 | Port: 1234 ]
(RET) [IP: 127.0.0.1 | Port: 43282 | Byte(s): 6343 ]    <--     [ IP: 127.0.0.1 | Port: 1234 ]
(FWD) [IP: 127.0.0.1 | Port: 43298 | Byte(s): 78 ]      -->     [ IP: 127.0.0.1 | Port: 1234 ]
(FWD) [IP: 127.0.0.1 | Port: 43314 | Byte(s): 78 ]      -->     [ IP: 127.0.0.1 | Port: 1234 ]
(RET) [IP: 127.0.0.1 | Port: 43314 | Byte(s): 156 ]     <--     [ IP: 127.0.0.1 | Port: 1234 ]
(RET) [IP: 127.0.0.1 | Port: 43298 | Byte(s): 6343 ]    <--     [ IP: 127.0.0.1 | Port: 1234 ]
(FWD) [IP: 127.0.0.1 | Port: 43328 | Byte(s): 78 ]      -->     [ IP: 127.0.0.1 | Port: 1234 ]
(RET) [IP: 127.0.0.1 | Port: 43328 | Byte(s): 6343 ]    <--     [ IP: 127.0.0.1 | Port: 1234 ]
(FWD) [IP: 127.0.0.1 | Port: 43344 | Byte(s): 78 ]      -->     [ IP: 127.0.0.1 | Port: 1234 ]
(RET) [IP: 127.0.0.1 | Port: 43344 | Byte(s): 6343 ]    <--     [ IP: 127.0.0.1 | Port: 1234 ]
(FWD) [IP: 127.0.0.1 | Port: 43354 | Byte(s): 78 ]      -->     [ IP: 127.0.0.1 | Port: 1234 ]
(RET) [IP: 127.0.0.1 | Port: 43354 | Byte(s): 6343 ]    <--     [ IP: 127.0.0.1 | Port: 1234 ]
(FWD) [IP: 127.0.0.1 | Port: 43368 | Byte(s): 78 ]      -->     [ IP: 127.0.0.1 | Port: 1234 ]
(RET) [IP: 127.0.0.1 | Port: 43368 | Byte(s): 6343 ]    <--     [ IP: 127.0.0.1 | Port: 1234 ]
(FWD) [IP: 127.0.0.1 | Port: 43370 | Byte(s): 78 ]      -->     [ IP: 127.0.0.1 | Port: 1234 ]
(RET) [IP: 127.0.0.1 | Port: 43370 | Byte(s): 156 ]     <--     [ IP: 127.0.0.1 | Port: 1234 ]
```

