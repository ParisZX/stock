/*
 *      A Stock Market Simulator Skeleton
 *      by Nikos Pitsianis
 *
 *	Based upon the pc.c producer-consumer demo by Andrae Muys
 */

#include <stdio.h>
#include <math.h>
#include <stdlib.h>	
#include <time.h>	
#include <sys/time.h>
#include <string.h>
#include <unistd.h>		
#include <pthread.h>

#define QUEUESIZE 5000

typedef struct {
  long id;        // identification number
  long oldid;     // old identification number for Cancel
  long timestamp; // time of order placement
  int vol;        // number of shares
  int price;      // price limit for Limit orders
  char action;    // 'B' for buy | 'S' for sell
  char type;      // 'M' for market | 'L' for limit | 'C' for cancel
} order;

order makeOrder();
inline long getTimestamp();
void dispOrder(order ord);

int currentPriceX10 = 1000;

struct timeval startwtime, endwtime; 	//Returns 	long int tv_sec
										//			This represents the number of whole seconds of elapsed time. 
										//			long int tv_usec
										// 			This is the rest of the elapsed time (a fraction of a second), represented as the number of microseconds. It is always less than one million.

pthread_mutex_t mut;

typedef struct {
  order item[QUEUESIZE];
  long head, tail;
  int full, empty;
  pthread_mutex_t *mut;
  pthread_cond_t *notFull, *notEmpty;
} queue;

// Added by me
typedef struct {
	queue *bm;
	queue *bl;
	queue *sm;
	queue *sl;
	queue *q;
}argQueues;

void cancelOrder(order ord, argQueues *arg);
void *Prod (argQueues *q);
void *Cons (argQueues *q);
queue *queueInit (void);
void queueDelete (queue *q);
void queueAdd (queue *q, order ord);
void queueDel (queue *q, order *ord);


// ****************************************************************
int main() {

  // number generator seed for different random sequence per run
  // srand(time(NULL) + getpid());
  srand(0); // or to get the same sequence

  // start the time for timestamps
  gettimeofday (&startwtime, NULL);

  pthread_t prod, cons;
  
  queue *q = queueInit();
  queue *bm = queueInit();
  queue *sm = queueInit();
  queue *bl = queueInit();
  queue *sl = queueInit();
  
  argQueues arguments;
  arguments.q = q;
  arguments.bm = bm;
  arguments.sm = sm;
  arguments.bl = bl;
  arguments.sl = sl;
  
  pthread_mutex_init(&mut, NULL);
  
  pthread_create(&prod, NULL, Prod, &arguments);
  pthread_create(&cons, NULL, Cons, &arguments);

  // I actually do not expect them to ever terminate
  pthread_join(prod, NULL);
  pthread_join(cons, NULL);

  pthread_exit(NULL);
}

// ****************************************************************
void *Prod (argQueues *arg){
  queue *q = arg->q;
  int magnitude = 10; 

  while (1) {

    // wait for a random amount of time in useconds
    int waitmsec = ((double)rand() / (double)RAND_MAX * magnitude);
    usleep(waitmsec*1000);

    pthread_mutex_lock (q->mut);
    while (q->full) {
      // This is bad and should not happen!
      printf ("*** Incoming Order Queue is FULL.\n"); fflush(stdout);
      pthread_cond_wait (q->notFull, q->mut);
    }
    queueAdd (q, makeOrder());
    pthread_mutex_unlock (q->mut);
    pthread_cond_signal (q->notEmpty);
  }
}

// ****************************************************************
void *Cons (argQueues *arg){
  queue *q = (queue *) arg->q;
  queue *bm = (queue *) arg->bm;
  queue *sm = (queue *) arg->sm;
  queue *bl = (queue *) arg->bl;
  queue *sl = (queue *) arg->sl;
  
  order ord;
  long i;

  //for (i=0; i<QUEUESIZE; i++){
  while (1) {
    pthread_mutex_lock (q->mut);
    while (q->empty) {
      // printf ("*** Incoming Order Queue is EMPTY.\n"); fflush(stdout);
      pthread_cond_wait (q->notEmpty, q->mut);
    }
    queueDel (q, &ord);
    pthread_mutex_unlock (q->mut);
    pthread_cond_signal (q->notFull);

	    switch(ord.action) {
	    	case 'B' :
	    		if(ord.type=='M') {
	    			pthread_mutex_lock (bm->mut);
	    			queueAdd(bm,ord);
	    			pthread_mutex_unlock (bm->mut);
	    			pthread_cond_signal (bm->notEmpty);
	    		}
	    		else if(ord.type=='L') {
	    			pthread_mutex_lock (bl->mut);
	    			queueAdd(bl,ord);
	    			pthread_mutex_unlock (bl->mut);
	    			pthread_cond_signal (bl->notEmpty);
	    		}
	    		else
	    			cancelOrder(ord,arg);
	    		break;
	    	case  'S' :
	    		if(ord.type=='M') {
	    			pthread_mutex_lock (sm->mut);
	    			queueAdd(sm,ord);
	    			pthread_mutex_unlock (sm->mut);
	    			pthread_cond_signal (sm->notEmpty);
	    		}
	    		else if(ord.type=='L') {
	    			pthread_mutex_lock (sl->mut);
	    			queueAdd(sl,ord);
	    			pthread_mutex_unlock (sl->mut);
	    			pthread_cond_signal (sl->notEmpty);
	    		}
	    		else
	    			cancelOrder(ord,arg);
	    		break;
	    	default : break; //maybe can be done better
	    }
    
    // **** YOUR CODE IS CALLED FROM HERE ****
    // Process that order!

    // 1) Move order from arrival queue to one of your queues
    // 2) Signal appropriate handler to deal with it
    // Display this message when the order is executed
    // Not now!, Not here!
	    printf ("Processing at time %8ld : " ,getTimestamp());
	    dispOrder(ord); fflush(stdout);
	    
	    printf ("Queue q: "); fflush(stdout);
	    for(i=arg->q->head;i<arg->q->tail;i++) {
	    	dispOrder(arg->q->item[i]); fflush(stdout);
	    }

	    printf ("Queue bm: ");
	    for(i=arg->bm->head;i<arg->bm->tail;i++) {
	    	dispOrder(arg->bm->item[i]); fflush(stdout);
	    }

	    printf ("Queue bl: ");
	    for(i=arg->bl->head;i<arg->bl->tail;i++) {
	    	dispOrder(arg->bl->item[i]); fflush(stdout);
	    }

	    printf ("Queue sm: ");
	    for(i=arg->sm->head;i<arg->sm->tail;i++) {
	    	dispOrder(arg->sm->item[i]); fflush(stdout);
	    }

	    printf ("Queue sl: ");
	    for(i=arg->sl->head;i<arg->sl->tail;i++) {
	    	dispOrder(arg->sl->item[i]); fflush(stdout);
	    }
	  
	}
}

void cancelOrder(order ord, argQueues *arg) {
	long i;
	switch(ord.action) {
		case 'B' :
			if(ord.type=='M') {
				for(i=arg->bm->head;i<arg->bm->tail;i++) {
					if(ord.oldid==arg->bm->item[i].id) {					
						arg->bm->item[i].type='C';
						break;
					}
				}
			}
			else {
				for(i=arg->bl->head;i<arg->bl->tail;i++) {
					if(ord.oldid==arg->bl->item[i].id) {					
						arg->bl->item[i].type='C';
						break;
					}
				}
			}
			break;
		case 'S' :
			if(ord.type=='M') {
				for(i=arg->sm->head;i<arg->sm->tail;i++) {
					if(ord.oldid==arg->sm->item[i].id) {					
						arg->bm->item[i].type='C';
						break;
					}
				}
			}
			else {
				for(i=arg->sl->head;i<arg->sl->tail;i++) {
					if(ord.oldid==arg->sl->item[i].id) {					
						arg->sl->item[i].type='C';
						break;
					}
				}
			}
			break;
		default: break;
	}
}

// ****************************************************************
order makeOrder(){

  static count = 0;

  order ord;

  ord.id = count++;
  ord.timestamp = getTimestamp();

  // Buy or Sell
  ord.action = ((double)rand()/(double)RAND_MAX <= 0.5) ? 'B' : 'S';

  // Order type
  double u2 = ((double)rand()/(double)RAND_MAX);
  if (u2 < 0.4){ 
    ord.type = 'M';                 // Market order
    ord.vol = (1 + rand()%50)*100;

  }else if (0.4 <= u2 && u2 < 0.9){
    ord.type = 'L';                 // Limit order
    ord.vol = (1 + rand()%50)*100;
    ord.price = currentPriceX10 + 10*(0.5 -((double)rand()/(double)RAND_MAX));
    
  }else if (0.9 <= u2){
    ord.type = 'C';                 // Cancel order
    ord.oldid = ((double)rand()/(double)RAND_MAX)*count;
  }

  // dispOrder(ord);

  return (ord); 
}

// ****************************************************************
inline long getTimestamp(){

  gettimeofday(&endwtime, NULL);

  return((double)((endwtime.tv_usec - startwtime.tv_usec)/1.0e6
		  + endwtime.tv_sec - startwtime.tv_sec)*1000);
}

// ****************************************************************
void dispOrder(order ord){

  printf("%08ld ", ord.id);
  printf("%08ld ", ord.timestamp);  
  switch( ord.type ) {

  case 'M':
    printf("%c ", ord.action);
    printf("Market (%4d)        ", ord.vol); break;

  case 'L':
    printf("%c ", ord.action);
    printf("Limit  (%4d,%5.1f) ", ord.vol, (float) ord.price/10.0); break;

  case 'C':
    printf("* Cancel  %6ld        ", ord.oldid); break;
  default : break;
  }
  printf("\n");
}

// ****************************************************************
// ****************************************************************
queue *queueInit (void)
{
  queue *q;

  q = (queue *)malloc (sizeof (queue));
  if (q == NULL) return (NULL);

  q->empty = 1;
  q->full = 0;
  q->head = 0;
  q->tail = 0;
  q->mut = (pthread_mutex_t *) malloc (sizeof (pthread_mutex_t));
  pthread_mutex_init (q->mut, NULL);
  q->notFull = (pthread_cond_t *) malloc (sizeof (pthread_cond_t));
  pthread_cond_init (q->notFull, NULL);
  q->notEmpty = (pthread_cond_t *) malloc (sizeof (pthread_cond_t));
  pthread_cond_init (q->notEmpty, NULL);
	
  return (q);
}

// ****************************************************************
void queueDelete (queue *q)
{
  pthread_mutex_destroy (q->mut);
  free (q->mut);	
  pthread_cond_destroy (q->notFull);
  free (q->notFull);
  pthread_cond_destroy (q->notEmpty);
  free (q->notEmpty);
  free (q);
}

// ****************************************************************
void queueAdd (queue *q, order in)
{
  q->item[q->tail] = in;
  q->tail++;
  if (q->tail == QUEUESIZE)
    q->tail = 0;
  if (q->tail == q->head)
    q->full = 1;
  q->empty = 0;

  return;
}

// ****************************************************************
void queueDel (queue *q, order *out)
{
  *out = q->item[q->head];

  q->head++;
  if (q->head == QUEUESIZE)
    q->head = 0;
  if (q->head == q->tail)
    q->empty = 1;
  q->full = 0;

  return;
}
