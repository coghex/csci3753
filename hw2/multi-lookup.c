#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <semaphore.h>
#include <unistd.h>

#include "util.h"
#include "queue.h"

#define MINARGS 3
#define USAGE "<inputFilePath> <outputFilePath>"
#define SBUFSIZE 1025
#define INPUTFS "%1024s"

// the queue will hold the request threads' requests
queue q;
// this is the output file
FILE* outputfp = NULL;
// mutexii
// qmutex protects the queue from multiple threads
pthread_mutex_t qmutex;
// outmutex protexts the output file from mutiple
// threads trying to write to it.
pthread_mutex_t outmutex;
// reqmutex protects the request count so that the
// program can figure out when to end.
pthread_mutex_t reqmutex;
// the number of request threads left open in the
// program, when this is zero we can exit.
int reqs = 0;

void* request(void* filename) {
    char* name = filename;
    char* stuff;
    char hostname[1025];
    struct timespec t;
    FILE* f = NULL;

    // open file
    f = fopen(name, "r");
    if(!f) {
        fprintf(stderr, "wow, no file\n");
    }

    while(fscanf(f, INPUTFS, hostname) > 0){
        // get lock for queue
        pthread_mutex_lock(&qmutex);
        // request room to store the hostname so that
        // it can be accesed from the resolver threads
        stuff = malloc(1025*sizeof(char));
        strcpy(stuff, hostname);
        // if queue is full wait, as per the requirements
        while(queue_is_full(&q)){
            pthread_mutex_unlock(&qmutex);
	    t.tv_sec = 0;
            t.tv_nsec = rand()%100;
            nanosleep(&t,NULL);
            pthread_mutex_lock(&qmutex);
        }
        // add to the queue
        if(queue_push(&q, stuff) == QUEUE_FAILURE){
            fprintf(stderr, "such bad queue\n");
        }
        pthread_mutex_unlock(&qmutex);
    }

    // count down the number of requests left
    pthread_mutex_lock(&reqmutex);
    reqs = reqs-1;
    pthread_mutex_unlock(&reqmutex);

    fclose(f);

    return NULL;
}
void* resolve() {
    char hostname[1025];
    char firstipstr[INET6_ADDRSTRLEN];  
    char* thing = NULL;

    while (1) {
        // Check if empty queue and no requests waiting to finish
        pthread_mutex_lock(&qmutex);
        pthread_mutex_lock(&reqmutex);
        if(queue_is_empty(&q)&&(reqs == 0)){
            // if that was the last request, and there are none that
            // havent been queued yet, then we can exit
            pthread_mutex_unlock(&qmutex);
            pthread_mutex_unlock(&reqmutex);
            break;
        }
        pthread_mutex_unlock(&reqmutex);
        // check if queue is empty
        if(queue_is_empty(&q)){
          pthread_mutex_unlock(&qmutex);
        }
	else {
            // if stuff in the queue then we can pop a thing off the
            // queue so we can resolve the DNS.
            pthread_mutex_unlock(&qmutex);
	    if((thing = (char*)queue_pop(&q)) == NULL) {
              fprintf(stderr, "very pop fail\n");
	    }
            strcpy(hostname, thing);
            // DNS lookup function
            if(dnslookup(hostname, firstipstr, sizeof(firstipstr)) == UTIL_FAILURE) {
                fprintf(stderr, "much DNS error\n");
                strncpy(firstipstr, "", sizeof(firstipstr));
            }  

            // write to the file
            pthread_mutex_lock(&outmutex);
            fprintf(outputfp, "%s,%s\n", hostname, firstipstr);
            pthread_mutex_unlock(&outmutex);
            free(thing);
        }
    }

    return NULL;
}

int main(int argc, char* argv[]){

    /* Local Vars */
    int i;

    // Threading variables
    // the number of requester threads is the number of input files.
    int reqthreadnum = argc - 2;
    // reqs is explained in the declaration above
    reqs = argc - 2;
    // the number of resolver threads is any number I want (i like 2)
    int resthreadnum = 2;
    // our threads
    pthread_t reqthreads[reqthreadnum];
    pthread_t resthreads[resthreadnum];    
    int rc;
    long t;

    // as per the requirements, only allow 10 files
    if (reqs>10) {
      fprintf(stderr, "much files, cant handle\n");
      return EXIT_FAILURE;
    }

    // much of the following code comes from the example written by asaylor
    /* Check Arguments */
    if(argc < MINARGS){
	fprintf(stderr, "Not enough arguments: %d\n", (argc - 1));
	fprintf(stderr, "Usage:\n %s %s\n", argv[0], USAGE);
	return EXIT_FAILURE;
    }

    /* Open Output File */
    outputfp = fopen(argv[(argc-1)], "w");
    if(!outputfp){
	perror("Error Opening Output File");
	return EXIT_FAILURE;
    }

    // initialize the queue
    if(queue_init(&q, 25) == QUEUE_FAILURE) {
        fprintf(stderr, "So queue init wrong\n");
    }

    // initialize mutexii
    if(pthread_mutex_init(&qmutex, NULL)){
        fprintf(stderr, "such mutex error\n");
    }
    if(pthread_mutex_init(&outmutex, NULL)){
        fprintf(stderr, "very mutex error\n");
    }
    if(pthread_mutex_init(&reqmutex, NULL)){
        fprintf(stderr, "much mutex error\n");
    }

    // spawn the requester threads
    for(t=0;t<reqthreadnum;t++){
        rc = pthread_create(&(reqthreads[t]), NULL, request, argv[t+1]);
        if (rc) {
            fprintf(stderr, "Much reqpthread fail\n");
            exit(EXIT_FAILURE);
        }
    }

    // spawn the resolver threads
    for (t=0;t<resthreadnum;t++) {
        rc = pthread_create(&(resthreads[t]), NULL, resolve, NULL);
        if (rc) {
            fprintf(stderr, "Much respthread fail\n");
            exit(EXIT_FAILURE);
        }
    }

    // Wait for threads to finish
    for(i=0;i<reqthreadnum;i++){
        pthread_join(reqthreads[i],NULL);
    }
    for(i=0;i<resthreadnum;i++){
        pthread_join(resthreads[i],NULL);
    }

    /* Close Output File */
    fclose(outputfp);

    // cleanup the mutex's and the queue
    queue_cleanup(&q);
    pthread_mutex_destroy(&qmutex);
    pthread_mutex_destroy(&outmutex);
    pthread_mutex_destroy(&reqmutex);

    return EXIT_SUCCESS;
}
