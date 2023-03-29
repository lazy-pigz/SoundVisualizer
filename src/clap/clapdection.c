#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <stdbool.h>
#include <pthread.h>
#include <stdbool.h>
#include "clapdection.h"
#include "mic.h"
#include "circlebuffer.h"
#include "../Utility.h"


double threshold = 50;
double upperthreshold = 300;
static pthread_t detector;
static bool run;
static pthread_mutex_t lock;
bool clap = false;
bool clapFeature = true;

static double average(double* arr, int size){
    double sum = 0;
    for(int i = 0; i < size; i++){
        sum += arr[i]; 
    }
    double avg = sum/size;
    return avg;
}

static void* dectectClap(){
    int ShortLen;
    int LongLen;
    bool clap1 = false;
   
    double* ShortTerm = NULL;
    double* LongTerm = NULL;
    double shortavg;
    double longavg;
    int buffertime = 4000;
    clock_t before;
    clock_t difference;
    int timedurationmsec;
    while(run){

        if(!clapFeature){
            sleepForMs(2500);

        }
        else{
           
            ShortTerm = Mic_getShortHistory(&ShortLen);
            
            LongTerm = Mic_getLongHistory(&LongLen);
            
            shortavg = average(ShortTerm,ShortLen);
            longavg = average(LongTerm,LongLen);
            if((shortavg)  > threshold + longavg && (shortavg) < upperthreshold + longavg ){ 
                if(!clap1){
                    printf("clap1,%f,%f,%f,%f\n",shortavg, longavg,threshold, upperthreshold   );
                    clap1 = true;
                    before = clock();
                    Mic_Longclear();
                }
                else{
                    clap1 = false;
                    printf("clap2,%f,%f,%f,%f\n",shortavg, longavg,threshold, upperthreshold   );
                    pthread_mutex_lock(&lock);
                    clap = true;
                    pthread_mutex_unlock(&lock);    
                    Mic_Longclear();
                }
                
            }

            if(clap1){
                difference = clock() - before;
                timedurationmsec = difference * 1000 / CLOCKS_PER_SEC;
                if(timedurationmsec > buffertime){
                    clap1 = false;
                }
            }
            free(ShortTerm);
            free(LongTerm);
        }
    }
    return NULL;
   
}

///////////////////////////////////////public functions//////////////////////////////////////////////////////////////////////

//starts the clap detection also creates a thread that samples that record mic values 
void startMicDetection(){
    pthread_mutex_init(&lock, NULL);
    run = true;
    Mic_startSampling();
    while(Mic_getNumSamplesTaken() < 500); // small busy wait to let the buffer fill up
    pthread_create(&detector,NULL,dectectClap,NULL);

}
//stop the clap dection thread and stops sampling values from the mic
void stopMicDectection(){
    run = false;
    pthread_join(detector,NULL);
    Mic_stopSampling();
    pthread_mutex_destroy(&lock);
}


bool isClap(){
    bool ret;
    pthread_mutex_lock(&lock);
    ret = clap;
    clap = false;
    pthread_mutex_unlock(&lock);
    return ret;
}

void clapOn(bool val){
    clapFeature = val;

}