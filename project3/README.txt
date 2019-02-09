
/* CSci4061 Fall 2018 Project 3
* Group ID: 111
* Name: Michael Nguyen, Berni Duong, Catherine Ha
* X500: nguy2571, 			duong142,    haxxx214 */


====================================================================================

How to compile and run the program.

Step [1] : (in the terminal) make clean
Step [2] : (in the terminal) make
Step [3] : (in the terminal) ./web_server 9000 <path_to_testing>/testing <num_dispatch> <num_worker> 0 <queue_len> <cache_entries>
Step [4] : (in the 2nd terminal) wget -i <path-to-urls>/urls -O myres
Step [5] : (in the 2nd terminal) cat <path _to_urls_file> | xargs -n 1 -P 1 wget
**Step 5 if for testing the bigurls file

The urls file included some simple request at client side, so we will be able to test it.
====================================================================================

How the program works.

In this program, the server or more specifically, the web server, receives client requests to retrieve webpages or files.
These requests are then added to a queue by the dispatcher one at a time per connection.
The worker threads' job is to retrieve the requests in the queue and serve the request's result back to the client.

Each time it retrieves a request from the queue, it searches for whether or not the same request type exists in cache.
If contents are found in the cache, it is represented with a cache HIT. If it is not found in the cache, it will come up as a cache MISS and it will read the file's contents from the disk by calling readFromDisk(char* request, long* size).
After it is read from the disk, it is then stored in the cache for later requests.
Incoming client requests will keep adding them into the cache after reading it from the disk.
Eventually, the cache will be full and it will remove the least frequently used cache block in which we implemented, also known as the LFU cache replacement policy.
The worker sends back the result of the request by using the file discriptor, or the socket in which is how the client is connected to the server.

====================================================================================

Explanation of caching mechanism used.

We implemented the LFU cache replacement policy. This policy replaces the least frequently used used cache block whenever the cache overflows.
It essentially replaces this cache block with the new content from the request read from the disk.

====================================================================================

Explanation of your policy to dynamically change the worker thread pool size.

We dynamically change the worker pool size by decreasing the number of threads and the request in queue.
When the number of items in the queue is double (x2) the number of worker threads. We double the number of working threads.
When the number of working threads is double the number of items in queue. We destroy half of working threads.

====================================================================================

Contributions of each team member towards the project development.

Michael:
	Michael wrote the addIntoCache and deleteCache functions and part of the dispatch function. In addition, he wrote the main arguments and any error checking.


Berni:
	Berni wrote the creating and destroying of threads and helped with the dispatch function, caching, queue. He implemented the majority of the extra credit functions.


Catherine:
	Catherine wrote the README file, the performance analysis report, and the following functions: getContentType, getCacheIndex, and initCache.

====================================================================================
