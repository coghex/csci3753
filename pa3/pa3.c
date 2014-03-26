#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sched.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <signal.h>

// square root code found at http://en.wikipedia.org/wiki/Fast_inverse_square_root 
float squirt(float number) {
   long i;
   float x2, y;
   const float threehalfs = 1.5F;

   x2 = number * 0.5F;
   y  = number;
   i  = * ( long * ) &y;
   i  = 0x5f3759df - ( i >> 1 );
   y  = * ( float * ) &i;
   y  = y * ( threehalfs - ( x2 * y * y ) );
   y  = y * ( threehalfs - ( x2 * y * y ) );
   y  = y * ( threehalfs - ( x2 * y * y ) );

   return 1/y;
}

int isprime(int a) {
   int c;
 
   for ( c = 2 ; c <= a - 1 ; c++ )
   { 
      if ( a%c == 0 )
	 return 0;
   }
   if ( c == a )
      return 1;
   else return 0;
}

int main(int argc, char* argv[]){

    long util;
    struct sched_param param;
    int policy;
    int type;
    unsigned long long i;
    unsigned long long n;
    unsigned long j,k,l;
    unsigned long long prod;
    unsigned long long ans;
    struct timeval chstop, chstart, chresult;
    struct timeval stop, start, result;
    time_t begin, end;
    pid_t pid;
    int x = 0;
    FILE *devzero;
    FILE *devnull;
    int data;
    int test;

    devzero = fopen("/dev/zero","r");
    devnull = fopen("/dev/urandom","w");
    
    /* Process program arguments to select iterations and policy */
    /* Set default iterations if not supplied */
    if(argc < 2){
	util=5;
    }
    /* Set default policy if not supplied */
    if(argc < 3){
	policy = SCHED_OTHER;
    }
    if(argc < 4){
        type = 0;
    }
    if(argc < 5){
        test = 0;
    }
    /* Set iterations if supplied */
    if(argc > 1){
	util = atol(argv[1]);
	if(util < 1){
	    fprintf(stderr, "Usage:\n    sudo ./pa3 <num of processes> <scheduler> <type> <test>\n");
            fprintf(stderr, "    num of processes: number of processes that will be spawned (integer)\n");
            fprintf(stderr, "    scheduler: SCHED_OTHER, SCHED_FIFO, SCHED_RR\n    type: cpu, io, mixed\n");
            fprintf(stderr,"    test: extime, restime, turntime\n");
	    exit(EXIT_FAILURE);
	}
    }
    /* Set policy if supplied */
    if(argc > 2){
	if(!strcmp(argv[2], "SCHED_OTHER")){
	    policy = SCHED_OTHER;
	}
	else if(!strcmp(argv[2], "SCHED_FIFO")){
	    policy = SCHED_FIFO;
	}
	else if(!strcmp(argv[2], "SCHED_RR")){
	    policy = SCHED_RR;
	}
	else{
	    fprintf(stderr, "Usage:\n    sudo ./pa3 <num of processes> <scheduler> <type> <test>\n");
            fprintf(stderr, "    num of processes: number of processes that will be spawned (integer)\n");
            fprintf(stderr, "    scheduler: SCHED_OTHER, SCHED_FIFO, SCHED_RR\n    type: cpu, io, mixed\n");
            fprintf(stderr,"    test: extime, restime, turntime\n");
	    exit(EXIT_FAILURE);
	}
    }

    if(argc > 3){
	if(!strcmp(argv[3], "cpu")){
	    type = 1;
	}
	else if(!strcmp(argv[3], "io")){
	    type = 2;
	}
	else if(!strcmp(argv[3], "mixed")){
	    type = 3;
	}
	else{
	    fprintf(stderr, "Usage:\n    sudo ./pa3 <num of processes> <scheduler> <type> <test>\n");
            fprintf(stderr, "    num of processes: number of processes that will be spawned (integer)\n");
            fprintf(stderr, "    scheduler: SCHED_OTHER, SCHED_FIFO, SCHED_RR\n    type: cpu, io, mixed\n");
            fprintf(stderr,"    test: extime, restime, turntime\n");
	    exit(EXIT_FAILURE);
	}
    }
    if(argc>4){
	if(!strcmp(argv[4], "extime")){
	    test = 0;
	}
	else if(!strcmp(argv[4], "restime")){
	    test = 1;
	}
	else if(!strcmp(argv[4], "turntime")){
	    test = 2;
	}
        else if(!strcmp(argv[4], "cputime")) {
            test = 3;
        }
	else{
	    fprintf(stderr, "Usage:\n    sudo ./pa3 <num of processes> <scheduler> <type> <test>\n");
            fprintf(stderr, "    num of processes: number of processes that will be spawned (integer)\n");
            fprintf(stderr, "    scheduler: SCHED_OTHER, SCHED_FIFO, SCHED_RR\n    type: cpu, io, mixed\n");
            fprintf(stderr,"    test: extime, restime, turntime\n");
	    exit(EXIT_FAILURE);
	}
}
    
    /* Set process to max prioty for given scheduler */
    param.sched_priority = sched_get_priority_max(policy);
    
    /* Set new scheduler policy */
    //fprintf(stdout, "Current Scheduling Policy: %d\n", sched_getscheduler(0));
    //fprintf(stdout, "Setting Scheduling Policy to: %d\n", policy);
    if(sched_setscheduler(0, policy, &param)){
	perror("Error setting scheduler policy");
	exit(EXIT_FAILURE);
    }
    //fprintf(stdout, "New Scheduling Policy: %d\n", sched_getscheduler(0));

    // The pi code seemed naive to me, so this code will multiply prime numbers
    // together, then re-compute the factors.  This will test not only adds,
    // and multiplies, but also floating point operations, logical compares,
    // and jump operations.  A good all around test of a processor's
    // capabilities.  The factoring code was modified from many others' code
    // online, the functions were made using code found on wikipaedia.
    gettimeofday(&start,NULL);
    while (x < util) {
      pid = fork();
      if (pid==0) {
        gettimeofday(&chstart,NULL);
        timersub(&start, &chstart, &chresult);
        if (test==1) {
        //printf("response in %f seconds\n",(double)(-chresult.tv_sec-
        //     (chresult.tv_usec/1000000.0)));
          printf("%f+",(double)(-chresult.tv_sec-
                (chresult.tv_usec/1000000.0)));
        }
        begin = clock();
        if (type==1) {
          for (j=0;j<1000;j++) {
            for (k=0;k<1000;k++) {
              if (isprime(j) && isprime(k)) {
                prod = j*k;
                //printf("%ld * %ld = %Ld, Refactored: ",j,k,prod);
                n = prod;
                while (n%2 == 0) {
                  //printf("%d ",2);
                  ans=2;
                  n=n/2;
                }
                for (i=3;i<squirt(n);i=i+2) {
                  while(n%i==0) {
                    //printf("%Ld * ",i);
	            ans=i;
                    n=n/i;
                  }
                }
                if (n>2) {
                  ans=n;
                  //printf("%Ld\n",n);
                }
              }
            }
          }
        }
      else if (type==2){
        for(j=0;j<2500000;j++){
          fscanf(devzero, "%d", &data);
          fprintf(devnull, "%d", data);
        }
      }
      else {
          for (j=0;j<700;j++) {
            for (k=0;k<700;k++) {
              if (isprime(j) && isprime(k)) {
                prod = j*k;
                //printf("%ld * %ld = %Ld, Refactored: ",j,k,prod);
                n = prod;
                while (n%2 == 0) {
                  //printf("%d ",2);
                  ans=2;
                  n=n/2;
                }
                for (i=3;i<squirt(n);i=i+2) {
                  while(n%i==0) {
                    //printf("%Ld * ",i);
	            ans=i;
                    n=n/i;
                    for(l=0;l<100;l++){
                      fscanf(devzero, "%d", &data);
                      fprintf(devnull, "%d", data);
                    }
                  }
                }
                if (n>2) {
                  ans=n;
                  //printf("%Ld\n",n);
                }
              }
            }
          }
      }
      end = clock();
      gettimeofday(&chstop,NULL);
      timersub(&chstop, &chstart, &chresult);
      if (test==2){
        printf("%f+",(double)(chresult.tv_sec+
              (chresult.tv_usec/1000000.0)));
      //printf("Process executed in %f seconds using %f seconds of CPU time\n",(double)(chresult.tv_sec+
      //       (chresult.tv_usec/1000000.0)), (double)(end-begin)/CLOCKS_PER_SEC);
      }
      if (test==3){
        printf("%f+",(double)(end-begin)/CLOCKS_PER_SEC);
      }
      exit(EXIT_SUCCESS);
      }
      else if (pid<0) {
        printf("fork fail...\n");
      }
      else {
        x++;
      }
    }
    if (pid != 0) {
      while (waitpid(-1,NULL,0)>0);
    }
    ans = 1;
    gettimeofday(&stop,NULL);
    timersub(&stop,&start,&result);
    //printf("\nTotal done in %f seconds\n",(double)(ans*result.tv_sec+(result.tv_usec/1000000.0)));
    if (test==0){
      printf("%f",(double)(ans*result.tv_sec+(result.tv_usec/1000000.0)));
    }
    if (type==2) {
      fclose(devzero);
      fclose(devnull);
    }
    return 0;
}
