/*
Authors: Burak Yildirim, Duha Emir Ganioglu
ID: 72849, 71753
*/

#include <iostream>
#include <cstdlib>
#include <pthread.h>
#include <vector>
#include <queue>
#include <ctime>
#include <random>
#include "pthread_sleep.c"
#include <getopt.h>
#include <assert.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <cstring>

#include "project2_header.h"

using namespace std;

int secondToMili = 1000000;
int RoadNumber = 4;
int threadNumber = 5;

struct thread_data {
   int  thread_id;
   const char *message;
};

char directList[4] = {'N', 'E', 'S', 'W'};
int probsInit[4] = {0, 0, 0, 0};
double probsDirect[4];

void *police(void*);

void roadHandler(int); 
void carToTheRoad(int);
void *makeRoad(void *); 
char* adjustTime(time_t);
char* timeToString(int); 
void northSpecific(); 


int carId = 1; 
int totalCarQuantity = 0;
double p;
vector<queue<car> > roads(4, std::queue<car>());
pthread_mutex_t roadMutex;
pthread_mutex_t totalCarMutex;
pthread_cond_t honkCond;

// Converts the data into the terminal format and prints it
void intersectToString() {
   cout << "At " << adjustTime(time(NULL)) << ":" <<endl;
   cout << "   " << roads[0].size() << endl;
   cout << roads[3].size() <<"     " << roads[1].size() << endl;
   cout << "   " << roads[2].size() << endl;
}

// cmdline arguments: takes the user input as probability, simulation time and t 
void cmdline(int argc, char *argv[], double &p, int &s, int &t) {
   int choice;
   t = 0;
   p = 0.4;
   s  = 60;
    
   while ((choice = getopt(argc, argv, "n:e:k:w:p:s:t:")) != -1) {
      switch (choice) {
         case 'n':
            probsDirect[0] = atof(optarg);
            probsInit[0] = 1;
            cout << "North=> " << optarg << endl;
            break;
         case 'e':
            probsDirect[1] = atof(optarg);
            probsInit[1] = 1;
            cout << "East=> " << optarg << endl;
            break;
         case 'k':
            probsDirect[2] = atof(optarg);
            probsInit[2] = 1;
            cout << "South=>  " << optarg << endl;
            break;
         case 'w':
            probsDirect[3] = atof(optarg);
            probsInit[3] = 1;
            cout << "West=> " << optarg << endl;
            break;
         case 'p':
            p = atof(optarg);
            cout << "p=> " << optarg << endl;
            break;
         case 's':
            s = atoi(optarg);
            cout << "s=>  "<< optarg << endl;
            break;
         case 't':
            t = atof(optarg);
            cout << "t=> " << optarg << endl;
            break;
         default: 
            fprintf(stderr, "Invalid argument\n");
            exit(EXIT_FAILURE);
        }
   }
}

// Takes the fulltime as parameter and combines the string version
// Such as "%s:%s:%s"
char* adjustTime(time_t fullTime) {
   int seconds, minutes, hours;
   struct tm *ct = localtime(&fullTime);
   char* Time = (char*) malloc(sizeof(char) * 8);
   hours = ct->tm_hour;
   minutes = ct->tm_min; 
   seconds = ct->tm_sec;
   sprintf(Time, "%s:%s:%s", timeToString(hours), timeToString(minutes), timeToString(seconds));
   return Time;
}
// Takes the time and returns it as a string
char* timeToString(int timeAsNumber) {
   char* timeAsString = (char*) malloc(sizeof(char) * 5);
   if(timeAsNumber < 10) 
      sprintf(timeAsString, "0%d", timeAsNumber);
   else
      sprintf(timeAsString, "%d", timeAsNumber);
   return timeAsString;
}

// Creates the corresponding road
void *makeRoad(void *roadIdPtr) {
	int roadId = *((int*)roadIdPtr);
	pthread_mutex_lock(&roadMutex);
	carToTheRoad(roadId);
	pthread_mutex_unlock(&roadMutex);
	if(roadId == 0){
		northSpecific();
	} else {
		roadHandler(roadId);
	}
   return NULL;
}

// Takes the road id as parameter and handles the necessary operations
void roadHandler(int roadId) {
	pthread_sleep(1);
	pthread_mutex_lock(&roadMutex);
	double randNum = (double)rand() / (double)RAND_MAX;
   int newCar = 0 ;
   if((probsInit[roadId] == 1 && randNum < probsDirect[roadId])||randNum < p) {
	   newCar = 1;
	}
   if(newCar == 1) {
      carToTheRoad(roadId);
   }
	pthread_mutex_unlock(&roadMutex);
	roadHandler(roadId);
}

// roadHandler but for the north
void northSpecific() {
	pthread_sleep(1);
	pthread_mutex_lock(&roadMutex);
	double randNum = (double)rand() / (double)RAND_MAX;
   int newCar = 0 ;
   if((probsInit[0] == 1 && probsDirect[0]>randNum ) || p<randNum) {
      newCar = 1;
	} 
   if(newCar==1) {
      carToTheRoad(0);
      pthread_cond_signal(&honkCond);
		pthread_mutex_unlock(&roadMutex);
   } else {
		pthread_mutex_unlock(&roadMutex);
		pthread_sleep(20);
	}
	northSpecific();
}

// Puts the car to the corresponding road
void carToTheRoad(int roadId) {
   car newCar = {carId++, directList[roadId], time(NULL), 0, 0};
   pthread_mutex_lock(&totalCarMutex);
   totalCarQuantity++;
   roads[roadId].push(newCar);
   if(totalCarQuantity == 1) {
      pthread_cond_signal(&honkCond);
   }
   pthread_mutex_unlock(&totalCarMutex);
}


int main (int argc, char *argv[]) {
   pthread_t threads[threadNumber];
   struct thread_data td[threadNumber];
   int rc;
   int s;
   int t;
   srand(time(0));
   
   if (pthread_mutex_init(&roadMutex, NULL) && pthread_mutex_init(&totalCarMutex, NULL))   { 
        printf("\nInitiliazing mutex error.\n"); 
        return 1; 
   } 
   pthread_cond_init (&honkCond, NULL);

   cmdline(argc, argv, p, s, t);
   time_t startTime = time(0);
   clock_t initTime = clock();
   clock_t previous = clock();
   int second = 0;
   double simRunTime = 0;
   
   cout << "Arguments:" << p <<" "<< s << endl;

   for( int i = 0; i < RoadNumber; i++ ) {
      int *index = (int*) malloc(sizeof(int));
      *index = i;
      rc = pthread_create(&threads[i], NULL, makeRoad, (void *)(index));
      if (rc) {
         cout << "Creating the thread failed," << rc << endl;
         exit(-1);
      }
   }
   rc = pthread_create(&threads[RoadNumber], NULL, police, NULL);
   while (totalCarQuantity < 4) {  
   }
   while(simRunTime < s) {
      pthread_mutex_lock(&roadMutex);
      if( clock() - previous > CLOCKS_PER_SEC) {
         previous = ++second * CLOCKS_PER_SEC; 
      }
      if(t == second) {
         t++; 
         intersectToString();
      }
      pthread_mutex_unlock(&roadMutex);
      simRunTime = (clock() - initTime ) / (double) CLOCKS_PER_SEC;
   }
   
   cout << "Simulation time: " << simRunTime << "\n";
   pthread_mutex_destroy(&roadMutex); 
   pthread_mutex_destroy(&totalCarMutex); 
   pthread_cond_destroy(&honkCond);

   exit(0);
}


// Handles the operations of the whole intersection.
void *police(void *) {
   FILE *carOut;
   FILE *policeOut;
   carOut = fopen("./car.log", "w");
   policeOut = fopen("./police.log", "w");
   fprintf(carOut, "CarID     Direction     Arrival-Time      Cross-Time       Wait-Time\n");
   fprintf(carOut, "_________________________________________\n");
   fprintf(policeOut, "Time    Event    \n");
   fprintf(policeOut, "__________________\n");
   
   int turnId = 0;
   
	while(true) {
      int maxCarNumber = 0;
      struct tm *currTimeData;
      struct tm *arrTimeData;

      pthread_mutex_lock(&roadMutex);
      int i = 0;
      while(i < RoadNumber) {
         int carQuantity = roads[i].size();
         if(carQuantity > maxCarNumber && (carQuantity > 4 || roads[turnId].empty())) {
            turnId = i;          	
         } else if(carQuantity > maxCarNumber){
            maxCarNumber = carQuantity;
         }
         i++;
      }

      pthread_mutex_lock(&totalCarMutex);
      if(maxCarNumber == 0 ) {
         pthread_mutex_unlock(&roadMutex);
         fprintf(policeOut, "%s \t %s \n", adjustTime(time(NULL)), "Cell Phone");
         while (0 >= totalCarQuantity) {
            pthread_cond_wait(&honkCond,&totalCarMutex);
         }
         fprintf(policeOut, "%s \t %s \n", adjustTime(time(NULL)), "Honk");
         pthread_sleep(3); 
         pthread_mutex_lock(&roadMutex);
      }
      pthread_mutex_unlock(&totalCarMutex);

      int waitUntil = 0;
  	   for(int i = 0; i < RoadNumber; i++) {
  	  	   if(!roads[i].empty()) {
		  	   car newCar = roads[i].front();
		  	   time_t currentTime = time(NULL);
		  	   int waitTime = ( currentTime - newCar.arrTime );
		  	   if(waitTime > waitUntil){
		  	   	waitUntil = waitTime;
		  	   	if(waitTime > 20){
		  	   		turnId = i;
		  	   	}
		  	   }
      	}
      }
      if(maxCarNumber != 0) {
         car passingThrough = (roads[turnId].front());
         roads[turnId].pop();
         pthread_mutex_lock(&totalCarMutex);
         totalCarQuantity--;
         pthread_mutex_unlock(&totalCarMutex);
         time_t currentTime = time(NULL);
         arrTimeData = localtime(&passingThrough.arrTime);
         currTimeData = localtime(&currentTime);
         int waitTime = ( currentTime - passingThrough.arrTime );
         maxCarNumber = 0;
         fprintf(carOut, "%d         %c    %s          %s         %d\n", passingThrough.carId, passingThrough.direction, adjustTime(passingThrough.arrTime), adjustTime(currentTime), waitTime);
         cout << "On the move: " << passingThrough.carId << "  " 
         << passingThrough.direction << "  " 
         << adjustTime(passingThrough.arrTime) << " " 
         << adjustTime(currentTime) << "  "
         << waitTime << "  "  << endl;
         pthread_mutex_unlock(&roadMutex);
         pthread_sleep(1);
      } else {
         pthread_mutex_unlock(&roadMutex);
      }
   }
   fclose(carOut);
   fclose(policeOut);
}

