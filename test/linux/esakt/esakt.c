/** \file
 * \brief Example code for Simple Open EtherCAT master
 *
 * Usage : simple_test [ifname1]
 * ifname is NIC interface, f.e. eth0
 *
 * This is a minimal test.
 *
 * (c)Arthur Ketels 2010 - 2011
 */

#include <stdio.h>
#include <string.h>
#include <inttypes.h>

#include "ethercat.h"

/*clpham:*/
#include <time.h>
#include <math.h>
/* #include <stdint.h> */
#include <stdlib.h>
#include <signal.h>
#include <locale.h> /*for digit grouping print*/
#include <unistd.h>

#define NSEC_PER_SEC 1000000000
#define EC_TIMEOUTMON 500

char IOmap[4096];
/* pthread_t thread1; */
OSAL_THREAD_HANDLE thread1;
int expectedWKC;
boolean needlf;
volatile int wkc;
boolean inOP;
uint8 currentgroup = 0;

/*clpham:*/
/* #define CYCLIC_INTERVAL_US 3000 */
#define CYCLIC_INTERVAL_US 5000
#define NUM_SUBOBJ_DO 13
#define NUM_SUBOBJ_AO 16
#define NUM_SUBOBJ_DI NUM_SUBOBJ_DO
#define NUM_SUBOBJ_AI 64
typedef struct PACKED{
	uint8 led;
	uint32 DO[NUM_SUBOBJ_DO];
	uint16 AO[NUM_SUBOBJ_AO];
}tPdoRx;
typedef struct PACKED{
	uint32 DI[NUM_SUBOBJ_DI];
	int32 Info;
	int16 AI[NUM_SUBOBJ_AI];
}tPdoTx;
static uint32 CyclicIntervalUs = CYCLIC_INTERVAL_US;
static boolean keepRunning = TRUE;
static boolean ShowStats = TRUE;
OSAL_THREAD_HANDLE thread2;
static int DeltaTimeNS = 0;

/*Cursor addressing*/
#define ESC 27
#define Clear()		printf("%c[2J", ESC)
#define CursorPos(row, col)		printf("%c[%d;%dH", ESC, row, col)
#define CursorSavePos()			printf("%c[s", ESC)
#define CursorRestorePos()		printf("%c[u", ESC)
#define CursorUp(row)			printf("%c[%dA\r", ESC, row)
#define CursorDown(row)			printf("%c[%dB\r", ESC, row)
//#define CursorHide()			printf("%c[?25l", ESC);
#define CursorHide()
#define CursorShow()			printf("%c[?25h", ESC);

/* extern void set_output_int8 (uint16 slave_no, uint8 module_index, int8 value); */
extern void setPdoRx(uint16 slave_no, uint8 module_index, tPdoRx *p);
extern void graceExit(void);
extern OSAL_THREAD_FUNC showStats( void *ptr );
extern void add_timespec(struct timespec *ts, int64 addtime);
extern struct timespec deltaTime(struct timespec start, struct timespec end);

void simpletest(char *ifname)
{
    int i, j, oloop, iloop, chk;
    needlf = FALSE;
    inOP = FALSE;

	struct timespec startTime, endTime;

   printf("Starting simple test\n");

   /* initialise SOEM, bind socket to ifname */
   if (ec_init(ifname))
   {
      printf("ec_init on %s succeeded.\n",ifname);
      /* find and auto-config slaves */


       if ( ec_config_init(FALSE) > 0 )
      {
         printf("%d slaves found and configured.\n",ec_slavecount);

#if 1
		 /*clpham: avoid using LRW, which is not support by the TI SDK v.1.0.6*/
		 for(i = 1; i<=ec_slavecount ; i++)
			 ec_slave[i].blockLRW = TRUE;
#endif

         ec_config_map(&IOmap);

         ec_configdc();

         printf("Slaves mapped, state to SAFE_OP.\n");
         /* wait for all slaves to reach SAFE_OP state */
         ec_statecheck(0, EC_STATE_SAFE_OP,  EC_TIMEOUTSTATE * 4);

         oloop = ec_slave[0].Obytes;
         if ((oloop == 0) && (ec_slave[0].Obits > 0)) oloop = 1;
         if (oloop > 8) oloop = 8;
         iloop = ec_slave[0].Ibytes;
         if ((iloop == 0) && (ec_slave[0].Ibits > 0)) iloop = 1;
         if (iloop > 8) iloop = 8;

         printf("segments : %d : %d %d %d %d\n",ec_group[0].nsegments ,ec_group[0].IOsegment[0],ec_group[0].IOsegment[1],ec_group[0].IOsegment[2],ec_group[0].IOsegment[3]);

         printf("Request operational state for all slaves\n");
         expectedWKC = (ec_group[0].outputsWKC * 2) + ec_group[0].inputsWKC;
         printf("Calculated workcounter %d\n", expectedWKC);
         ec_slave[0].state = EC_STATE_OPERATIONAL;
         /* send one valid process data to make outputs in slaves happy*/
         ec_send_processdata();
         ec_receive_processdata(EC_TIMEOUTRET);
         /* request OP state for all slaves */
         ec_writestate(0);
         chk = 40;
         /* wait for all slaves to reach OP state */
         do
         {
            ec_send_processdata();
            ec_receive_processdata(EC_TIMEOUTRET);
            ec_statecheck(0, EC_STATE_OPERATIONAL, 50000);
         }
         while (chk-- && (ec_slave[0].state != EC_STATE_OPERATIONAL));
         if (ec_slave[0].state == EC_STATE_OPERATIONAL )
         {
            printf("Operational state reached for all slaves.\n");
			printf("Sending Process Data every %'dus...\n", CyclicIntervalUs/1);
            inOP = TRUE;



struct timespec   ts, tleft;
int ht;
int64 cycletime;

clock_gettime(CLOCK_MONOTONIC, &ts);
ht = (ts.tv_nsec / 1000000) + 1; /* round to nearest ms */
ts.tv_nsec = ht * 1000000;
cycletime = CyclicIntervalUs * 1000; /* cycletime in ns */
int64 toff = 0;
/* dorun = 0; */


/*clpham: clear MaxAgeDO*/
char usdo[52*sizeof(uint32)];
memset(&usdo, 0, sizeof(usdo));
#if 1
ec_SDOwrite(
		1/*slave#*/,
		0xa000/*object index*/,
		1/*object subindex*/,
		TRUE/*CA*/,
		sizeof(usdo)/*psize*/,
		(void *)&usdo/*p*/,
		EC_TIMEOUTRXM);
#endif



                /* cyclic loop */
            /* for(i = 1; i <= 10000; i++) */
            for(i = 1;; i++)
            {
			
				clock_gettime(CLOCK_MONOTONIC, &startTime);
				
				/*Quick test on the Fab9 box*/
				static tPdoRx pdoRx =
				{	0,
					{0,0,0,0,0,0,0,0,0,0,0,0,0},
					{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}
				};
				tPdoTx pdoTx;
				tPdoRx *pPdoRx = &pdoRx;
				tPdoTx *pPdoTx = &pdoTx;
				static unsigned int i2=0;
				static boolean reverseDO[NUM_SUBOBJ_DO];
				static boolean reverseAO[NUM_SUBOBJ_AO];
#if 1
				/*DOs*/
				/* Bit walk the onboard LEDs and the DOs */
				//if(i>(i2+20)){/*Slow down the speed of the walking LEDs*/
				if(1){
					int brdIdx;
					union digitalOutTag{
						uint32 d32;
						struct bitFields{
							boolean bit0:1;
							boolean bit1:1;
							boolean bit2:1;
							uint32 reservedBits:29;
						}bits;
					}nxtDO, prevDI;
					uint32 prevDO;

					/*Re-init LED test values*/
					if(pPdoRx->led==0)
						pPdoRx->led = 1;

	/*clpham: DI latency measurements*/
	#if 1
					wkc = ec_receive_processdata(EC_TIMEOUTRET);
					pPdoTx = (tPdoTx *)ec_slave[0].inputs;
					int pdoIndx;
					/*DO*/
					for(pdoIndx=0; pdoIndx<NUM_SUBOBJ_DO; pdoIndx++){
						/*Toggle DO[pdoIndx]'s bit 0'*/
						prevDO = pPdoRx->DO[pdoIndx];
						nxtDO.d32 = prevDO;
						nxtDO.bits.bit0 = ~nxtDO.bits.bit0;

						/*Update DO[0][2] with DI[0][1]*/
						/* wkc = ec_receive_processdata(EC_TIMEOUTRET); */
						/* pPdoTx = (tPdoTx *)ec_slave[0].inputs; */
						prevDI.d32 = pPdoTx->DI[pdoIndx];
						nxtDO.bits.bit2 = prevDI.bits.bit0;
						pPdoRx->DO[pdoIndx] = nxtDO.d32;
					}
					/*AO*/
					uint16 prevAO, nxtAO, prevAI;
					/* for(pdoIndx=0; pdoIndx<NUM_SUBOBJ_AO; pdoIndx++){ */
					for(pdoIndx=0; pdoIndx<=8; pdoIndx+=8){
						/*Toggle AO0, AO8 0V<->10V*/
						prevAO = pPdoRx->AO[pdoIndx];

						/*Update AO1 (or AO9) with AI0 (or AI32)*/
						/* wkc = ec_receive_processdata(EC_TIMEOUTRET); */
						/* pPdoTx = (tPdoTx *)ec_slave[0].inputs; */
						nxtAO = pPdoTx->AI[pdoIndx*4];
						pPdoRx->AO[pdoIndx+1] = nxtAO;
						

						pPdoRx->AO[pdoIndx] = (prevAO==0) ? 0x7fff : 0;
					}

					setPdoRx(1, 0, pPdoRx);

					/* Walk to the next LED bit */
					pPdoRx->led <<=1; 
	#endif
	#if 0
					/*Re-init DO test values*/
					for(brdIdx=0; brdIdx<NUM_SUBOBJ_DO; brdIdx++){
						if(pPdoRx->DO[brdIdx]==0){
							pPdoRx->DO[brdIdx]=1;
							reverseDO[brdIdx] = FALSE;
						}
					}
	#endif

	#if 0
					/*Re-init AO test values*/
					for(brdIdx=0; brdIdx<NUM_SUBOBJ_AO; brdIdx++){
						if(pPdoRx->AO[brdIdx]==0){
							reverseAO[brdIdx] = FALSE;
						}
					}

					setPdoRx(1, 0, pPdoRx);

					/* Walk to the next LED bit */
					pPdoRx->led <<=1; 
	#endif
	#if 0
					/* Walk to the next DO bits */
					for(brdIdx=0; brdIdx<NUM_SUBOBJ_DO; brdIdx++){
						if(pPdoRx->DO[brdIdx]==2){
							pPdoRx->DO[brdIdx] = 1;
							reverseDO[brdIdx] = FALSE;
							continue;
						}

						if(!reverseDO[brdIdx])
							pPdoRx->DO[brdIdx]<<=2;
						else
							pPdoRx->DO[brdIdx]>>=2;

						if(pPdoRx->DO[brdIdx]==0){
							pPdoRx->DO[brdIdx] = 0x80000000;
							reverseDO[brdIdx] = TRUE;
						}
					}
	#endif

	#if 0
					/* Walk to the next AO values */
					for(brdIdx=0; brdIdx<NUM_SUBOBJ_AO; brdIdx++){
						/*Walk forward or backward(when reverse==TRUE) AOs*/
						int32 v = (int32)pPdoRx->AO[brdIdx];
						if(!reverseAO[brdIdx]){
							v += 100; /*was=100*/
							if(v>=0x8000){
								v = 0x7fff;
								reverseAO[brdIdx] = TRUE;
							}
						}
						else{
							v -= 100;
							if(v<0)
								v = 0;
						}
						pPdoRx->AO[brdIdx] = (uint16)v;
					}
					i2 = i;
	#endif
				}
#endif
			   /*Send the PDOs*/
               ec_send_processdata();
               wkc = ec_receive_processdata(EC_TIMEOUTRET);


                    /* osal_usleep(CyclicIntervalUs);#<{(|clpham: was=5000|)}># */
/* calculate next cycle start */
add_timespec(&ts, cycletime + toff);
/* wait to cycle start */
clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &ts, &tleft);

				clock_gettime(CLOCK_MONOTONIC, &endTime);
				DeltaTimeNS = (int)deltaTime(startTime, endTime).tv_nsec;

                }
                inOP = FALSE;
            }
            else
            {
                printf("Not all slaves reached operational state.\n");
                ec_readstate();
                for(i = 1; i<=ec_slavecount ; i++)
                {
                    if(ec_slave[i].state != EC_STATE_OPERATIONAL)
                    {
                        printf("Slave %d State=0x%2.2x StatusCode=0x%4.4x : %s\n",
                            i, ec_slave[i].state, ec_slave[i].ALstatuscode, ec_ALstatuscode2string(ec_slave[i].ALstatuscode));
                    }
                }
            }
            printf("\nRequest init state for all slaves\n");
            ec_slave[0].state = EC_STATE_INIT;
            /* request INIT state for all slaves */
            ec_writestate(0);
        }
        else
        {
            printf("No slaves found!\n");
        }
        printf("End simple test, close socket\n");
        /* stop SOEM, close socket */
        ec_close();
    }
    else
    {
        printf("No socket connection on %s\nExcecute as root\n",ifname);
    }
}

OSAL_THREAD_FUNC ecatcheck( void *ptr )
{
    int slave;

struct timespec   ts, tleft;
int ht;
int64 cycletime;

clock_gettime(CLOCK_MONOTONIC, &ts);
ht = (ts.tv_nsec / 1000000) + 1; /* round to nearest ms */
ts.tv_nsec = ht * 1000000;
cycletime = 1000000; /* cycletime in ns */
int64 toff = 0;

    while(keepRunning)
    {
        if( inOP && ((wkc < expectedWKC) || ec_group[currentgroup].docheckstate))
        {
            if (needlf)
            {
               needlf = FALSE;
               printf("\n");
            }
            /* one ore more slaves are not responding */
            ec_group[currentgroup].docheckstate = FALSE;
            ec_readstate();
            for (slave = 1; slave <= ec_slavecount; slave++)
            {
               if ((ec_slave[slave].group == currentgroup) && (ec_slave[slave].state != EC_STATE_OPERATIONAL))
               {
                  ec_group[currentgroup].docheckstate = TRUE;
                  if (ec_slave[slave].state == (EC_STATE_SAFE_OP + EC_STATE_ERROR))
                  {
                     printf("ERROR : slave %d is in SAFE_OP + ERROR, attempting ack.\n", slave);
                     ec_slave[slave].state = (EC_STATE_SAFE_OP + EC_STATE_ACK);
                     ec_writestate(slave);
                  }
                  else if(ec_slave[slave].state == EC_STATE_SAFE_OP)
                  {
                     printf("WARNING : slave %d is in SAFE_OP, change to OPERATIONAL.\n", slave);
                     ec_slave[slave].state = EC_STATE_OPERATIONAL;
                     ec_writestate(slave);
                  }
                  else if(ec_slave[slave].state > 0)
                  {
                     if (ec_reconfig_slave(slave, EC_TIMEOUTMON))
                     {
                        ec_slave[slave].islost = FALSE;
                        printf("MESSAGE : slave %d reconfigured\n",slave);
                     }
                  }
                  else if(!ec_slave[slave].islost)
                  {
                     /* re-check state */
                     ec_statecheck(slave, EC_STATE_OPERATIONAL, EC_TIMEOUTRET);
                     if (!ec_slave[slave].state)
                     {
                        ec_slave[slave].islost = TRUE;
                        printf("ERROR : slave %d lost\n",slave);
                     }
                  }
               }
               if (ec_slave[slave].islost)
               {
                  if(!ec_slave[slave].state)
                  {
                     if (ec_recover_slave(slave, EC_TIMEOUTMON))
                     {
                        ec_slave[slave].islost = FALSE;
                        printf("MESSAGE : slave %d recovered\n",slave);
                     }
                  }
                  else
                  {
                     ec_slave[slave].islost = FALSE;
                     printf("MESSAGE : slave %d found\n",slave);
                  }
               }
            }
            if(!ec_group[currentgroup].docheckstate)
			{
               /* printf("OK : all slaves resumed OPERATIONAL.\n"); */
			}
        }
        /* osal_usleep(10000);#<{(|clpham:was=10000|)}># */
/* calculate next cycle start */
add_timespec(&ts, cycletime + toff);
/* wait to cycle start */
clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &ts, &tleft);
    }
}

void ctrlCHandler(int dummy){
	keepRunning = FALSE;
}


int main(int argc, char *argv[])
{
	setlocale(LC_ALL, ""); /*for digit grouping*/

	/*Capture the ctrl-c*/
	signal(SIGINT, ctrlCHandler);

   /*Clear the terminal*/
   Clear();
   CursorPos(0, 0);

   printf("SOEM (Simple Open EtherCAT Master)\nSimple test\n");

   if (argc > 1)
   {
      /* create thread to handle slave error handling in OP */
     /* pthread_create( &thread1, NULL, (void *) &ecatcheck, (void*) &ctime); */
      osal_thread_create(&thread1, 128000, &ecatcheck, (void*) &ctime);



	  /*clpham:*/
	  if(argc>2){
		  CyclicIntervalUs = atoi(argv[2])*1; /*argv[2] in units of us*/
	  }else
		CyclicIntervalUs = CYCLIC_INTERVAL_US;

	  if(argc>3){
		  if(atoi(argv[3])!=0)
			  ShowStats = TRUE;
		  else
			  ShowStats = FALSE;
	  }
	  		

	/*clpham: thread to handle stats printing*/
	int ctime2;
	osal_thread_create(&thread2, 128000, &showStats, (void*) &ctime2);

      /* start cyclic part */
      simpletest(argv[1]);
   }
   else
   {
      printf("Usage: simple_test ifname1\nifname = eth0 for example\n");
   }

   printf("End program\n");
   CursorShow();
   return (0);
}

/*clpham:*/
void setPdoRx(uint16 slave_no, uint8 module_index, tPdoRx *p)
{
	uint8 *pTarget, *pSource;

	pTarget = ec_slave[slave_no].outputs;
	/* Move pointer to correct module index*/
	pTarget += module_index * 2;
	/* Write data to output */
	int i;
	pSource = (uint8 *)p;
	for(i=0; i<sizeof(tPdoRx); i++){
		*pTarget++ = *pSource++;
	}
}

void graceExit(void){
	CursorShow();
	printf("\r\n\r\nProgram ends due to ctrl-c!\n");
	CursorShow();
	sleep(1);
	exit(EXIT_SUCCESS);
}

OSAL_THREAD_FUNC showStats( void *ptr ){
	static tPdoRx pdoRx2 = {0, 0, 0};
	tPdoTx pdoTx;
	tPdoRx *pPdoRx = &pdoRx2;
	tPdoTx *pPdoTx = &pdoTx;
	int i;
	sleep(1);
	/* printf("\nStarting task 'showStats()...'\n"); */
	for(i = 1;; i++){
	    CursorHide();
		/*Print process data...*/
		if(ShowStats)
		/* if(wkc >= expectedWKC) */
		{
			pPdoRx = (tPdoRx *)ec_slave[0].outputs;
			pPdoTx = (tPdoTx *)ec_slave[0].inputs;

			static boolean cursorSavedFirst = FALSE;
			if(!cursorSavedFirst){
				CursorSavePos();
				cursorSavedFirst = TRUE;
			}

			CursorRestorePos();
			printf("Processdata cycle %4d, WKC %d, T %"PRId64", Cycle interval %'10dns\n",
					i, wkc, ec_DCtime, DeltaTimeNS/1);
			printf(" LEDs  =0x%02x", pPdoRx->led);

			if(1){
				int doi, dii, aoi, aii;
				for(doi=0, dii=0; doi<NUM_SUBOBJ_DO; doi++, dii++){
					CursorDown(1);
					if(doi<NUM_SUBOBJ_DO){
						printf(" DO[%2d]=0x%08x", doi, pPdoRx->DO[doi]);
					}else
						printf("                   ");

					if(dii<NUM_SUBOBJ_DI){
						printf("  DI[%2d]=0x%08x", dii, pPdoTx->DI[dii]);
					}
				}

				printf("\n --------");
				CursorDown(1);
				for(aoi=0, aii=0; aoi<NUM_SUBOBJ_AO; aoi++, aii++){
					printf(" AO[%2d]=0x%04x  AI[%2d]=0x%04x  AI[%2d]=0x%04x  AI[%2d]=0x%04x  AI[%2d]=0x%04x",
							aoi,pPdoRx->AO[aoi],
							aii, pPdoTx->AI[aii],
							aii+1*NUM_SUBOBJ_AO, pPdoTx->AI[aii+1*NUM_SUBOBJ_AO],
							aii+2*NUM_SUBOBJ_AO, pPdoTx->AI[aii+2*NUM_SUBOBJ_AO],
							aii+3*NUM_SUBOBJ_AO, pPdoTx->AI[aii+3*NUM_SUBOBJ_AO]);
					CursorDown(1);
				}

			}
		}
		if(!keepRunning)
			graceExit();

		CursorShow();
		osal_usleep(50000);/*us*/
	}
}

/* add ns to timespec */
void add_timespec(struct timespec *ts, int64 addtime)
{
   int64 sec, nsec;

   nsec = addtime % NSEC_PER_SEC;
   sec = (addtime - nsec) / NSEC_PER_SEC;
   ts->tv_sec += sec;
   ts->tv_nsec += nsec;
   if ( ts->tv_nsec > NSEC_PER_SEC )
   {
      nsec = ts->tv_nsec % NSEC_PER_SEC;
      ts->tv_sec += (ts->tv_nsec - nsec) / NSEC_PER_SEC;
      ts->tv_nsec = nsec;
   }
}

struct timespec deltaTime(struct timespec start, struct timespec end){
	struct timespec t;
	if((end.tv_nsec - start.tv_nsec) <0){
		t.tv_sec = end.tv_sec - start.tv_sec-1;
		t.tv_nsec = 1000000000+end.tv_nsec-start.tv_nsec;
	}else{
		t.tv_sec = end.tv_sec - start.tv_sec;
		t.tv_nsec = end.tv_nsec - start.tv_nsec;
	}
	return t;
}
