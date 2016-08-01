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
#define CYCLIC_INTERVAL_US 10000
#define NUM_SUBOBJ_DO 13
#define NUM_SUBOBJ_AO 16
#define NUM_SUBOBJ_DI 13
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

void simpletest(char *ifname)
{
    int i, j, oloop, iloop, chk;
    needlf = FALSE;
    inOP = FALSE;

   printf("Starting simple test\n");

   /* initialise SOEM, bind socket to ifname */
   if (ec_init(ifname))
   {
      printf("ec_init on %s succeeded.\n",ifname);
      /* find and auto-config slaves */


       if ( ec_config_init(FALSE) > 0 )
      {
         printf("%d slaves found and configured.\n",ec_slavecount);

#if 0
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
			printf("Sending Process Data every %d ms...\n", CyclicIntervalUs/1000);
            inOP = TRUE;
                /* cyclic loop */
            /* for(i = 1; i <= 10000; i++) */
            for(i = 1;; i++)
            {
				/*Quick test on the Fab9 box*/
				static tPdoRx pdoRx = {0, 0, 0};
				tPdoTx pdoTx;
				tPdoRx *pPdoRx = &pdoRx;
				tPdoTx *pPdoTx = &pdoTx;
				static unsigned int i2=0;
				static boolean reverseDO[NUM_SUBOBJ_DO];
				static boolean reverseAO[NUM_SUBOBJ_AO];
#if 0
				struct timespec ts;
				static uint32 pktIdx = 0;
				clock_gettime(CLOCK_REALTIME, &ts);
				static unsigned long ms =0;
				ms = (ts.tv_nsec/1000000) - ms;
				pPdoRx->DO[0] = (uint32)ms;
				pPdoRx->DO[1] = pktIdx++;
#endif

#if 0
				/*Write 0xD0D01111-0xD0D0DDDD to DO[31:0]-DO[415:384]*/
				int idx;
				for(idx=0; idx<NUM_SUBOBJ_DO; idx++){
					uint32 subi = idx+0;
					uint32 v= 0xd0d00000 + (subi<<3*4)+(subi<<2*4)+(subi<<1*4)+(subi<<0*4); /*=D0D01111:D0D0DDDD*/
					pPdoRx->DO[idx] =v;
					/* printf("DO[%2d]=0x%08x\n", idx, pPdoRx->DO[idx]); */
				}

				/*Write 0xA011-0xA0DD to AO0-AO15*/
				for(idx=0; idx<NUM_SUBOBJ_AO; idx++){
					uint16 subi = idx+0;
					uint16 v = 0xa000 + (subi<<1*4)+(subi<<0*4); /*=A011:A0DD*/
					pPdoRx->AO[idx] = v;
					/* printf("AO[%2d]=0x%04x\n", idx, pPdoRx->AO[idx]); */
				}
#endif

#if 1
				/*DOs*/
				/* Bit walk the onboard LEDs and the DOs */
				//if(i>(i2+20)){/*Slow down the speed of the walking LEDs*/
				if(1){
					int brdIdx;

					/*Re-init LED test values*/
					if(pPdoRx->led==0)
						pPdoRx->led = 1;

					/*Re-init DO test values*/
					for(brdIdx=0; brdIdx<NUM_SUBOBJ_DO; brdIdx++){
						if(pPdoRx->DO[brdIdx]==0){
							pPdoRx->DO[brdIdx]=1;
							reverseDO[brdIdx] = FALSE;
						}
					}

					/*Re-init AO test values*/
					for(brdIdx=0; brdIdx<NUM_SUBOBJ_AO; brdIdx++){
						if(pPdoRx->AO[brdIdx]==0){
							reverseAO[brdIdx] = FALSE;
						}
					}

					setPdoRx(1, 0, pPdoRx);

					/* Walk to the next LED bit */
					pPdoRx->led <<=1; 

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

					/* Walk to the next AO values */
					for(brdIdx=0; brdIdx<NUM_SUBOBJ_AO; brdIdx++){
						/*Walk forward or backward(when reverse==TRUE) AOs*/
						int32 v = (int32)pPdoRx->AO[brdIdx];
						if(!reverseAO[brdIdx]){
							v += 100;
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
				}
#endif
			   /*Send the PDOs*/
               ec_send_processdata();
               wkc = ec_receive_processdata(EC_TIMEOUTRET);


                    osal_usleep(CyclicIntervalUs);/*clpham: was=5000*/
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
        osal_usleep(1000);/*clpham:was=10000*/
    }
}

void ctrlCHandler(int dummy){
	keepRunning = FALSE;
}


int main(int argc, char *argv[])
{
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
		  CyclicIntervalUs = atoi(argv[2])*1000;
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
			printf("Processdata cycle %4d, WKC %d, T %"PRId64"\n", i, wkc, ec_DCtime);
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
		osal_usleep(10000);/*us*/
	}
}
