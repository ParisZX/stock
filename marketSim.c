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
void addAndSortLimitQueue(order ord, queue *l, int flag);

void *Prod (argQueues *q);
void *Cons (argQueues *q);
void *ConsBuyMarket(argQueues *q);
// void *ConsBuyLimit(argQueues *q);
// void *ConsSellMarket(argQueues *q);
// void *ConsSellLimit(argQueues *q);
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

  pthread_t prod, cons, consBuyMarket, consBuyLimit, consSellMarket, consSellLimit;
  
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

  //Execute Orders
  pthread_create(&consBuyMarket, NULL, ConsBuyMarket, &arguments);
  //pthread_create(&consBuyLimit, NULL, ConsBuyLimit, &arguments);
  //pthread_create(&consSellMarket, NULL, ConsSellMarket, &arguments);
  //pthread_create(&consSellLimit, NULL, ConsSellLimit, &arguments);
  
  // I actually do not expect them to ever terminate
  pthread_join(prod, NULL);
  pthread_join(cons, NULL);
  pthread_join(consBuyMarket, NULL);
  // pthread_join(consBuyLimit, NULL);
  // pthread_join(consSellMarket, NULL);
  // pthread_join(consSellLimit, NULL);
  pthread_exit(NULL);
}

// ****************************************************************
void *Prod (argQueues *arg){
  // queue *q = arg->q;
  int magnitude = 10; 

  while (1) {

    // wait for a random amount of time in useconds
    int waitmsec = ((double)rand() / (double)RAND_MAX * magnitude);
    usleep(waitmsec*1000); //changed, original 1000

    pthread_mutex_lock (arg->q->mut);
    while (arg->q->full) {
      // This is bad and should not happen!
      printf ("*** Incoming Order Queue is FULL.\n"); fflush(stdout);
      pthread_cond_wait (arg->q->notFull, arg->q->mut);
    }
    queueAdd (arg->q, makeOrder());
    pthread_mutex_unlock (arg->q->mut);
    pthread_cond_signal (arg->q->notEmpty);
  }
}

// ****************************************************************
void *Cons (argQueues *arg){
  
  order ord;
  long i;
  int flag;

  //for (i=0; i<QUEUESIZE; i++){
  while (1) {
    pthread_mutex_lock (arg->q->mut);
    while (arg->q->empty) {
      // printf ("*** Incoming Order Queue is EMPTY.\n"); fflush(stdout);
      pthread_cond_wait (arg->q->notEmpty, arg->q->mut);
    }
    queueDel (arg->q, &ord);
    pthread_mutex_unlock (arg->q->mut);
    pthread_cond_signal (arg->q->notFull);

	    switch(ord.action) {
	    	case 'B' :
	    		if(ord.type=='M') {
	    			pthread_mutex_lock (arg->bm->mut);
	    			queueAdd(arg->bm,ord);
	    			pthread_mutex_unlock (arg->bm->mut);
	    			pthread_cond_signal (arg->bm->notEmpty);
	    		}
	    		else if(ord.type=='L') {
	    			flag=0;
	    			addAndSortLimitQueue(ord,arg->bl,flag);
	    		}
	    		else
	    			cancelOrder(ord,arg);
	    		break;
	    	case 'S' :
	    		if(ord.type=='M') {
	    			pthread_mutex_lock (arg->sm->mut);
	    			queueAdd(arg->sm,ord);
	    			pthread_mutex_unlock (arg->sm->mut);
	    			pthread_cond_signal (arg->sm->notEmpty);
	    		}
	    		else if(ord.type=='L') {
	    			flag=1;
	    			addAndSortLimitQueue(ord,arg->sl,flag);
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
	    
	    // printf ("Processing at time %8ld : " ,getTimestamp());
	    // dispOrder(ord); fflush(stdout);

	    printf ("Queue q: \n"); fflush(stdout);
	    for(i=arg->q->head;i<arg->q->tail;i++) {
	    	dispOrder(arg->q->item[i]); fflush(stdout);
	    }

	    printf ("Queue bm: \n");
	    for(i=arg->bm->head;i<arg->bm->tail;i++) {
	    	dispOrder(arg->bm->item[i]); fflush(stdout);
	    }

	    printf ("Queue bl: \n");
	    for(i=arg->bl->head;i<arg->bl->tail;i++) {
	    	dispOrder(arg->bl->item[i]); fflush(stdout);
	    }

	    printf ("Queue sm: \n");
	    for(i=arg->sm->head;i<arg->sm->tail;i++) {
	    	dispOrder(arg->sm->item[i]); fflush(stdout);
	    }

	    printf ("Queue sl: \n");
	    for(i=arg->sl->head;i<arg->sl->tail;i++) {
	    	dispOrder(arg->sl->item[i]); fflush(stdout);
	    }

	}
}



void *ConsBuyMarket(argQueues *arg) {
	order temp;
	while (1) {
	    pthread_mutex_lock (arg->bm->mut);
	    while (arg->bm->empty) {
	      pthread_cond_wait (arg->bm->notEmpty, arg->bm->mut);
	    }
		
		// printf ("Processing order: ");
	    // dispOrder(arg->bm->item[arg->bm->head]); fflush(stdout);

	    if(arg->bm->item[arg->bm->head].type=='C') {
	   	//If true, then we have a canceled order, so we delete it from the queue, and go to the next
	    //market Buyer in queue.
	    	queueDel(arg->bm,&temp);
	    	printf("cancelled");
  		    pthread_mutex_unlock (arg->bm->mut);
	    	pthread_cond_signal (arg->bm->notFull);
	    	continue;
	    }

	    if((arg->sl->empty==0)&&((arg->sl->item[arg->sl->head].price<currentPriceX10)||arg->sm->empty==1)) {
	    	//if true, then we use the first order from the sellLimit queue for the Market Buyer.
	    	pthread_mutex_lock (arg->sl->mut);
	    	//printf("SellLimit order is preferable\n");
	    	//printf("Volume of BM order=%d and volume of SL order=%d\n", arg->bm->item[arg->bm->head].vol, arg->sl->item[arg->sl->head].vol);
	    	if(arg->sl->item[arg->sl->head].vol>arg->bm->item[arg->bm->head].vol)
	    	{
	    		queueDel(arg->bm,&temp);
	    	//	printf("BM order deleted\n");
	    		arg->sl->item[arg->sl->head].vol-=temp.vol;
	    	//	printf("New volume of SL order=%d\n", arg->sl->item[arg->sl->head].vol);
		    	pthread_mutex_unlock(arg->sl->mut);
		    	pthread_mutex_unlock (arg->bm->mut);
		    	pthread_cond_signal (arg->bm->notFull);
	    	}
	    	else if (arg->sl->item[arg->sl->head].vol<arg->bm->item[arg->bm->head].vol) {
	    		queueDel(arg->sl,&temp);
	    	//	printf("SL order deleted\n");
	    		arg->bm->item[arg->bm->head].vol-=temp.vol;
	    	//	printf("New volume of BM order=%d\n", arg->bm->item[arg->bm->head].vol);
		    	pthread_mutex_unlock(arg->sl->mut);
		    	pthread_mutex_unlock (arg->bm->mut);
		    	pthread_cond_signal (arg->sl->notFull);
	    	}
	    	else {
	    		queueDel(arg->bm,&temp);
	    		queueDel(arg->sl,&temp);
	    	//	printf("both deleted\n");
		    	pthread_mutex_unlock(arg->sl->mut);
		    	pthread_mutex_unlock (arg->bm->mut);
	    		pthread_cond_signal (arg->sl->notFull);
		    	pthread_cond_signal (arg->bm->notFull);
	    	}
	    	continue;
	    }
	    if(arg->sm->empty==0) {
	    	//if false, then we use the first order from the sellMarket queue for the Market Buyer.
	    	pthread_mutex_lock (arg->sm->mut);
	    	//printf("SellMarket order is preferable\n");
	    	//printf("Volume of BM order=%d and volume of SM order=%d\n", arg->bm->item[arg->bm->head].vol, arg->sm->item[arg->sm->head].vol);
	    	if(arg->sm->item[arg->sm->head].vol>arg->bm->item[arg->bm->head].vol)
	    	{
	    		queueDel(arg->bm,&temp);
	    	//	printf("BM order deleted\n");
	    		arg->sm->item[arg->sm->head].vol-=temp.vol;
	    	//	printf("New volume of SM order=%d\n", arg->sm->item[arg->sm->head].vol);
	    		pthread_mutex_unlock(arg->sm->mut);
				pthread_mutex_unlock (arg->bm->mut);
    			pthread_cond_signal (arg->bm->notFull);
	    	}
	    	else if (arg->sm->item[arg->sm->head].vol<arg->bm->item[arg->bm->head].vol) {
	    		queueDel(arg->sm,&temp);
	    	//	printf("SM order deleted\n");
	    		arg->bm->item[arg->bm->head].vol-=temp.vol;
	    	//	printf("New volume of BM order=%d\n", arg->bm->item[arg->bm->head].vol);
	    		pthread_mutex_unlock(arg->sm->mut);
				pthread_mutex_unlock (arg->bm->mut);
    			pthread_cond_signal (arg->sm->notFull);
	    	}
	    	else {
	    		queueDel(arg->bm,&temp);
	    		queueDel(arg->sm,&temp);
	    	//	printf("both deleted\n");
	    		pthread_mutex_unlock(arg->sm->mut);
				pthread_mutex_unlock (arg->bm->mut);
    			pthread_cond_signal (arg->sm->notFull);
    			pthread_cond_signal (arg->bm->notFull);
	    	}
	    	continue;
	    }
		pthread_mutex_unlock (arg->bm->mut);
    }
}

// void *ConsBuyLimit(argQueues *arg) {

	while (1) {
	    pthread_mutex_lock (arg->bl->mut);
	    while (arg->bl->empty) {
	      // printf ("*** Incoming Order Queue is EMPTY.\n"); fflush(stdout);
	      pthread_cond_wait (arg->bl->notEmpty, arg->bl->mut);
	    }

	    
	    
	}
}

// void *ConsSellMarket(argQueues *arg) {

// 	while (1) {
// 	    pthread_mutex_lock (arg->sm->mut);
// 	    while (arg->sm->empty) {
// 	      // printf ("*** Incoming Order Queue is EMPTY.\n"); fflush(stdout);
// 	      pthread_cond_wait (arg->sm->notEmpty, arg->sm->mut);
// 	    }

	    
	    
// 	}
// }

// void *ConsSellLimit(argQueues *arg) {

// 	while (1) {
// 	    pthread_mutex_lock (arg->sl->mut);
// 	    while (arg->sl->empty) {
// 	      // printf ("*** Incoming Order Queue is EMPTY.\n"); fflush(stdout);
// 	      pthread_cond_wait (arg->sl->notEmpty, arg->sl->mut);
// 	    }

	    
	    
// 	}
// }


void addAndSortLimitQueue(order ord, queue *l, int flag) {
	long i, previous, next, newTail=l->tail;
	order temp;
	pthread_mutex_lock (l->mut);
	queueAdd(l,ord);
	if(l->head>l->tail)
		newTail+=5000;
	//flag == 0 means we got a buyer queue to order, which means we use descending order.
	//Higher buying limit is more attractive to sellers, so we go from higher to lower prices.
	if(flag==0) {
		for(i=(newTail-1);i>=l->head;i--) {
		
			previous=i;
			next=i+1;
			if(previous>5000)
				previous-=5000;
			if(next>5000)
				next-=5000;
			
			if(l->item[previous].price<=l->item[next].price) {
				temp=l->item[previous];
				l->item[previous]=l->item[next];	
				l->item[next]=temp;
			}
			else {
				break;
			}
		}
	}
	//flag == 1 means we got a seller queue to order, so it's the opposite.		
	else {
		
		for(i=(newTail-1);i>=l->head;i--) {
		
			previous=i;
			next=i+1;
			if(previous>5000)
				previous-=5000;
			if(next>5000)
				next-=5000;
			
			if(l->item[previous].price>=l->item[next].price) {
				temp=l->item[previous];
				l->item[previous]=l->item[next];	
				l->item[next]=temp;
			}
			else {
				break;
			}
		}
	}
	pthread_mutex_unlock (l->mut);
	pthread_cond_signal (l->notEmpty);
}

void cancelOrder(order ord, argQueues *arg) {
	long i,a,newTail=arg->bm->tail;
	switch(ord.action) {
		case 'B' :
			if(arg->bm->tail<arg->bm->head)
				newTail+=5000;
			for(i=arg->bm->head;i<newTail;i++) {
				a=i;
				if(a>=5000)
					a-=5000;
				if(ord.oldid==arg->bm->item[a].id) {					
					arg->bm->item[a].type='C';
					break;
				}
			}
			if(arg->bl->tail<arg->bl->head)
				newTail+=5000;
			for(i=arg->bl->head;i<newTail;i++) {
				a=i;
				if(a>=5000)
					a-=5000;
				if(ord.oldid==arg->bl->item[a].id) {					
					arg->bl->item[a].type='C';
					break;
				}
				break;
			}
		case 'S' :
			if(arg->sm->tail<arg->sm->head)
				newTail+=5000;
			for(i=arg->sm->head;i<newTail;i++) {
				a=i;
				if(a>=5000)
					a-=5000;
				if(ord.oldid==arg->sm->item[a].id) {					
					arg->bm->item[a].type='C';
					break;
				}
			}
			if(arg->sl->tail<arg->sl->head)
				newTail+=5000;
			for(i=arg->sl->head;i<newTail;i++) {
				a=i;
				if(a>=5000)
					a-=5000;
				if(ord.oldid==arg->sl->item[a].id) {					
					arg->sl->item[a].type='C';
					break;
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
    printf("%c ", ord.action);
    printf("Cancel  %6ld        ", ord.oldid); break;
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
