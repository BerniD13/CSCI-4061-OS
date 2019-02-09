/* CSci4061 Fall 2018 Project 3
* Group ID: 111
* Name: Michael Nguyen, Berni Duong, Catherine Ha
* X500: nguy2571, 			duong142,    haxxx214 */


#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <fcntl.h>

#include <time.h>
#include "util.h"
#include <stdbool.h>
#include <unistd.h>
#include <sys/msg.h>
#include <sys/ipc.h>
#include <sys/queue.h>
#define MAX_THREADS 100
#define INFINITE 99999999
#define MAX_QUEUE_LEN 100
#define MAX_CE 100
#define INVALID -1
#define BUFF_SIZE 1024
#define CACHE_HIT "HIT"
#define CACHE_MISS "MISS"
#define FALSE 0
#define TRUE 1
#define ERROR -1
#define MIN_PORT 1025
#define MAX_PORT 65535
/*
  THE CODE STRUCTURE GIVEN BELOW IS JUST A SUGESSTION. FEEL FREE TO MODIFY AS NEEDED
*/

// structs:
typedef struct request_queue {
   int fd;
   char request[BUFF_SIZE];           // request
} request_t;

typedef struct cache_entry {
    int hit_count;                    // Using for extra credit: LFU
    int len;                          // size of the file in cache
    char request[BUFF_SIZE];          // holding a request for look up later
    char *content;                    // read the content of the file into a pointer
} cache_entry_t;

pthread_cond_t not_empty = PTHREAD_COND_INITIALIZER;;//so that we can dequeue
pthread_cond_t not_full = PTHREAD_COND_INITIALIZER;;//so that we can enqueue

pthread_mutex_t q_lock2 = PTHREAD_MUTEX_INITIALIZER;//for counting number of request in q - EXTRA CREDIT: dynamical worker thread
pthread_mutex_t q_lock = PTHREAD_MUTEX_INITIALIZER;//for calling accept_connection()
pthread_mutex_t read_req_lock = PTHREAD_MUTEX_INITIALIZER;//for calling get_request()
pthread_mutex_t log_lock = PTHREAD_MUTEX_INITIALIZER;//for log file operations
pthread_mutex_t cache_lock = PTHREAD_MUTEX_INITIALIZER;//for cache operations
pthread_mutex_t ret_lock = PTHREAD_MUTEX_INITIALIZER;   //for return value to client side

int cache_size;   // global size of cache - FIX
cache_entry_t *cache;
int sizeof_cache = 0; // global size of cache - DYNAMIC change when encache, counting the element in cache

//==============================================================================QUEUE
int queue_length;   // global size of q- FIX
request_t requestArray[MAX_QUEUE_LEN];    // QUEUE - FIFO

int front = 0;
int rear = -1;
int itemCount = 0;

void init_q() { // initialize for q
  for (int i = 0; i < MAX_QUEUE_LEN; i++)
  {
    request_t temp;
    temp.fd = -1;
    requestArray[i] = temp;
  }
}

request_t peek() {              // get the first element in queue - no delete
   return requestArray[front];
}

bool isEmpty() {              // check q is empty
   return itemCount == 0;
}

bool isFull() {               // check q is full
   return itemCount == queue_length;
}

int size() {            // check current size of q
   return itemCount;
}

void insert(request_t data) {     // insert an element in the q
   if(!isFull()) {
      if(rear == queue_length-1) {
         rear = -1;
      }
      rear++;
      requestArray[rear].fd = data.fd;
      memset(requestArray[rear].request, 0, sizeof(BUFF_SIZE));     // flush the request
      strcpy(requestArray[rear].request, data.request);             // copy the request to the q
      itemCount++;    // Increase size of q
   }
}

request_t removeData() {        // deque - get the first element in queue and delete it
   request_t data;
   memset(data.request, 0, sizeof(BUFF_SIZE));  // flush the request
   data.fd = requestArray[front].fd;
   strcpy(data.request, requestArray[front].request);
   front++;                     // q is circular array
   if(front == queue_length) {
      front = 0;                // rotate front
   }

   itemCount--;     // decrease the size
   return data;
}
//=========================================================================== QUEUE


/* ************************************ Cache Code ********************************/

// Function to check whether the given request is present in cache
int getCacheIndex(char *req){
  /// return the index if the request is present in the cache
  for (int i = 0; i < cache_size; i++) {
      if (strcmp(cache[i].request, req)==0) {
        cache[i].hit_count += 1;                // counting hit time - Using for Extra credit: LFU cache
        return i;
      }
  }
  return -1;
}

int encache(char *req, char* content, long size) {          // encache operation, finding the least frequently used element in the cache then replace it. Basically this is addIntoCache
   int index = sizeof_cache; // begin index
   // EXTRA CREDIT: IMPLEMENT LFU - FINDING the cache index has minimun hit_count
	 if(sizeof_cache >= cache_size) {
     int min = INFINITE;
     for (int i = 0; i < cache_size; i++) {     // finding the index element
       if(cache[i].hit_count < min) {
         min = cache[i].hit_count;
         index = i;
       }
     }
	 }
   else {
     sizeof_cache += 1;
   }

  cache[index].len = size;// encache
  cache[index].hit_count = 0;// encache
  memset(cache[index].request, 0, sizeof(BUFF_SIZE));
  strcpy(cache[index].request, req);// encache
  free(cache[index].content); // free before allocated
  cache[index].content = (char* )malloc(size+1);  // adding content to the cache
  memcpy(cache[index].content, content,size);

  return index;
}


// Function to add the request and its file content into the cache
// memory is content, content is content type -> call func.t to get the c content_type
int addIntoCache(char *mybuf, char *memory , int memory_size) {
  // It should add the request at an index according to the cache replacement policy
  // Make sure to allocate/free memeory when adding or replacing cache entries
  return encache(mybuf, memory, memory_size);             // calling encache
}

// clear the memory allocated to the cache
void deleteCache(){
  // De-allocate/free the cache memory
  for (int i = 0; i < cache_size; i++) {
    free(cache[i].content);
  }
  free(cache);
}

// Function to initialize the cache
void initCache(){
  // Allocating memory and initializing the cache array
  cache = (cache_entry_t *) malloc(sizeof(cache_entry_t) * cache_size);
  for (int i = 0; i < cache_size; i++) {
    cache_entry_t temp;
    temp.hit_count = 0;
    temp.len = 0;
    cache[i] = temp;
  }
}

// Function to open and read the file from the disk into the memory
// Add necessary arguments as needed
char* readFromDisk(char *request, long *size /*necessary arguments*/) {
  // Open and read the contents of file given the request
  char buf[BUFF_SIZE];
  memset(buf, 0, sizeof(BUFF_SIZE) );
  sprintf (buf,".%s",request);
  FILE *f = fopen(buf, "rb");         // read binary?

  if (f == NULL) {                      // error
    return NULL;
  }
  else {
    struct stat st;
    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    *size = fsize;            // set up the size of the file, there is multiple ways to do it
    fseek(f, 0, SEEK_SET);  //same as rewind(f);
    unsigned char *file;

    file = (char*) malloc(fsize + 1);
    fread(file, fsize, sizeof(unsigned char), f);   // read the file content

    fclose(f);
    return file;
  }
}

/**********************************************************************************/

/* ************************************ Utilities ********************************/
// Function to get the content type from the request
char* getContentType(char * mybuf) {
  // Should return the content type based on the file type in the request
  // (See Section 5 in Project description for more details)
  // ==============================================================================
  char *get_type = strrchr(mybuf, '.'); // gets characters after '.'
  if (strcmp(get_type, ".html")==0 || strcmp(get_type, ".htm")==0){
    return "text/html";
  }
  else if (strcmp(get_type, ".jpg")==0){
    return "image/jpeg";
  }
  else if (strcmp(get_type, ".gif")==0){
    return "image/gif";
  }
  else{ // default
    return "text/plain";
  }
  // ==============================================================================
}

// This function returns the current time in microseconds
long getCurrentTimeInMicro() {
  struct timeval curr_time;
  gettimeofday(&curr_time, NULL);
  return curr_time.tv_sec * 1000000 + curr_time.tv_usec;
}

/**********************************************************************************/

// Function to receive the request from the client and add to the queue
void * dispatch(void *arg) {
  int tid = *( (int *) arg);
  int fd;
  while (1) {
     // Accept client connection
     int fd =  accept_connection();
     if (fd >= 0)  // success
     {
       // Get request from the client
       char filename [BUFF_SIZE];
       memset(filename, 0, sizeof(BUFF_SIZE) );

       if (get_request(fd, filename) != 0) {      // read request to filename
         //perror("Error: read request\n");
         // currently do not thing
       }
       else // success
       {
         request_t current_request;
         memset(current_request.request, 0, sizeof(BUFF_SIZE) );
         current_request.fd = fd;
         strcpy(current_request.request, filename);         // set up a temporary file for request

         pthread_mutex_lock(&q_lock);           // LOCK   before add to the queue request
         while (isFull()) {
           pthread_cond_wait(&not_full, &q_lock);   // wait if the queue is full
         }

         insert(current_request);   // Add the request into the queue
         pthread_cond_broadcast(&not_empty); // signal to all thread
         pthread_mutex_unlock(&q_lock); // unlock
       }
     }
     else {
       printf("Failed connection\n");
     }
  }
  return NULL;
}

/**********************************************************************************/
void request_logging(int tid, int reqNum, int fd, char* request, int bytes, int times, char *status) {            // this func is using for log the information file
  FILE *f = fopen("web_server_log", "a"); // we know it is there, do not need to check error
  char buf[BUFF_SIZE];
  memset(buf, 0, sizeof(BUFF_SIZE) );
  if (f != NULL) {
    if (bytes == -1) {
      sprintf (buf,"[%d][%d][%d][%s][%s][%d us][%s]\n", tid, reqNum, fd, request, "Requested file not found.", times, status);
      fputs(buf, stdout); // same fprintf
      fprintf(f, "[%d][%d][%d][%s][%s][%d us][%s]\n", tid, reqNum, fd, request, "Requested file not found.", times, status);
    } else {
      sprintf (buf,"[%d][%d][%d][%s][%d][%d us][%s]\n", tid, reqNum, fd, request, bytes, times, status);
      fputs(buf, stdout); // same fprintf
      fprintf(f, "[%d][%d][%d][%s][%d][%d us][%s]\n", tid, reqNum, fd, request, bytes, times, status);
    }
    fclose(f);
  }
}

// Function to retrieve the request from the queue, process it and then return a result to the client
void * worker(void *arg) {
    pthread_setcancelstate(PTHREAD_CANCEL_DISABLE,NULL);          // setup for dynamic thread
   int tid = *( (int *) arg);
   int total_request = 0;

   while (1) {
    char content_type[BUFF_SIZE];                   // content type
    memset(content_type, 0, sizeof(BUFF_SIZE) );
    int disk_index = ERROR;       // some local variable
    int error = FALSE;
    long size = ERROR;
    int cache_index = ERROR;
    char status[BUFF_SIZE];
    memset(status, 0, sizeof(BUFF_SIZE) );
    strcpy(status, CACHE_MISS);

    // Get the request from the queue
    pthread_mutex_lock(&read_req_lock);
    while (isEmpty()) {
      pthread_cond_wait(&not_empty, &read_req_lock);
    }

    request_t request = removeData();                       // get request from queue
    pthread_cond_signal(&not_full);
    pthread_mutex_unlock(&read_req_lock);

    // Start recording time
    int cur_time = getCurrentTimeInMicro();

    pthread_mutex_lock(&cache_lock);
    cache_index =  getCacheIndex(request.request);          // check index in cache
    pthread_mutex_unlock(&cache_lock);

    if (cache_index == ERROR) {
      //cache miss
      char * disk_content = readFromDisk(request.request, &size);
      if (size == ERROR) {
        error = TRUE;
      }
      else {
        // read from disk ss
        strcpy(content_type, getContentType(request.request));
        pthread_mutex_lock(&cache_lock);
        // Get the data from the disk or the cache
        cache_index = addIntoCache(request.request, disk_content, size);
        pthread_mutex_unlock(&cache_lock);
      }
      free(disk_content);   // free pointer after add in cache
    } else {
      //cache hit
      memset(status, 0, sizeof(BUFF_SIZE) );
      strcpy(status,CACHE_HIT);
    }
    // Stop recording the time
    int end_time = getCurrentTimeInMicro();
    int times = end_time - cur_time;

    total_request++;    // the number request each thread has handled
    // return the result
    if (error == TRUE) {
      // Log the request into the file and terminal
      pthread_mutex_lock(&log_lock);
      request_logging(tid, total_request, request.fd, request.request, ERROR, times, status);
      pthread_mutex_unlock(&log_lock);

      int ret = return_error(request.fd, "ERROR");

    } else {
      // Log the request into the file and terminal
      pthread_mutex_lock(&log_lock);
      request_logging(tid, total_request, request.fd, request.request, cache[cache_index].len, times, status);       // request logging file, lock when we adding it
      pthread_mutex_unlock(&log_lock);

      strcpy(content_type, getContentType(request.request));            // local variable

      pthread_mutex_lock(&ret_lock);
      int ret = return_result(request.fd, content_type , cache[cache_index].content, cache[cache_index].len);       // return information to client
      pthread_mutex_unlock(&ret_lock);

    }

    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE,NULL);         // set a breakpoint for dynamic thread
    pthread_testcancel();
    pthread_setcancelstate(PTHREAD_CANCEL_DISABLE,NULL);
  }
  return NULL;
}

/**********************************************************************************/


/* ************************ Dynamic Pool Code ***********************************/
// Extra Credit: This function implements the policy to change the worker thread pool dynamically
// depending on the number of requests
void * dynamic_pool_size_update(void *arg) {
  pthread_t t_worker[MAX_THREADS];
  int wids[MAX_THREADS];
  int num_working_thread = 1;

  pthread_create(&t_worker[0],   NULL, worker, (void *) &(wids[0]));

  while(1) {
    // Run at regular intervals
    // Increase / decrease dynamically based on your policy
    pthread_mutex_lock(&q_lock2);
    int size_q = size();
    pthread_mutex_unlock(&q_lock2);
    int num_create = 0;
    int num_delete = 0;
    if (size_q > num_working_thread*2 ) {             // checking request number
      if (num_working_thread*2 > MAX_THREADS) {
        num_create = MAX_THREADS - num_working_thread;      // calculate the number of thread will be create
      } else {
        num_create = num_working_thread*2 - num_working_thread;     // calculate the number of thread will be create
      }
      if (num_create != 0)
        printf("Created <%d> worker threads because request is <%d> vs thread is <%d>.\n", num_create,  size_q, num_working_thread);

      for(int i=num_working_thread; (i< num_working_thread + num_create); i+=1) {       // create some threads respect to num_create
        wids[i] = i;
        pthread_create(&t_worker[i],   NULL, worker, (void *) &(wids[i]));
      }

      num_working_thread+= num_create;      // LOCAL

    } else {

      if (size_q*2 < num_working_thread && size_q != 0) {
        num_delete = num_working_thread/2;                    // calculate the number of thread will be DELETE, MAKE SURE 1 THREAD RUNNING ALL TIME
      }

      if (num_delete != 0)
        printf("Removed <%d> worker threads because request is <%d> vs thread is <%d>.\n", num_delete,  size_q, num_working_thread);
      for(int i=num_working_thread-1; i>num_working_thread - num_delete-1;i--) {
         pthread_cancel((t_worker[i]));     // DELETE THREAD
      }
      num_working_thread-= num_delete;
    }
  }
}
/**********************************************************************************/






int main(int argc, char **argv) {

  // Error check on number of arguments
  if(argc != 8){
    printf("usage: %s port path num_dispatcher num_workers dynamic_flag queue_length cache_size\n", argv[0]);
    return -1;
  }
  // Get the input args
  int port = atoi(argv[1]);		// read the 2nd arg, number
  char *path = argv[2];		// read the 3rd arg, char
  int num_dispatcher = atoi(argv[3]);		// read the 4th arg
  int num_workers = atoi(argv[4]);		// read the 5th arg
  int dynamic_flag = atoi(argv[5]);		// read the 6th arg
  queue_length = atoi(argv[6]);		// read the 7th arg
  cache_size = atoi(argv[7]); // read the 8th arg for cache entries

  initCache();
  //init_q();

  // Perform error checks on the input arguments
	// Checks to see if the port number is within the range by default
  if (port < MIN_PORT || port > MAX_PORT) {
    perror("Error: The port number may only be in range 1025-65535");
  }

  // DEBUG: (TESTING) (Optional?)
  // Checks to see if the path matches the wrb root location (for this case the location of the testing folder)

  // Chck to make sure that num_dispatcher and num_workers is non-negative
  if (num_dispatcher < 1) {
    perror("Error: The number of dispatchers must be at least 1");
  }
  if (num_workers < 1) {
    perror("Error: The number of workers must be at least 1");
  }
  // Checks to make sure that the dynamic flag is default or on
  if (dynamic_flag != 0) {
    if (dynamic_flag != 1) {
      perror("Error: The dynamic flag is 0 by default on with 1");
    }
  }
  // Checks to make sure the queue length is valid
  if (queue_length > MAX_QUEUE_LEN) {
    perror("Error: The queue length must be at max 100");
  }
  // Checks to make sure the number of cache entries is valid
  if (cache_size > MAX_CE) {
    perror("Error: The number of cache entries must be at max 100");
  }

  // Change the current working directory to server root directory
  // In this case the server directory on my pc is ~/4061/4061_Project/testing

  int check = chdir(path);

  // Start the server and initialize cache
	init(port); //Call this only once in the main thread, if any errors, it will call exit

  // Create dispatcher and worker threads

  pthread_t t_dispatcher[num_dispatcher];
  pthread_t t_worker[num_workers];
  pthread_t t_main_worker;
  int dids[num_dispatcher];
  int wids[num_workers];
  int main_worker;

  for(int i=0; i<num_dispatcher; i+=1) {      // create dispatchers
    dids[i] = i;
    pthread_create(&t_dispatcher[i],   NULL, dispatch, (void *) &(dids[i]));
  }

  if (dynamic_flag != 0) {                              //   == 1
      pthread_create(&t_main_worker,   NULL, dynamic_pool_size_update, (void *) &main_worker);
  } else {
    for(int j=0; j<num_workers;j+=1){     // create workers
      wids[j] = j;
      pthread_create(&t_worker[j],   NULL, worker, (void *) &(wids[j]));
    }
    for(int i=0; i<num_workers; i++) {                 // Wait for all worker threads to complete
      pthread_join(t_worker[i], NULL);
    }
  }

  for(int i=0; i<num_dispatcher; i++) {                 // Wait for all dispatcher threads to complete
    pthread_join(t_dispatcher[i], NULL);
  }


  // Clean up
  deleteCache();

  return 0;
}
