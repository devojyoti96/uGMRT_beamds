/* Program for offline processing -- Written by Jayanta Roy (Last modified on 1 June 2007)*/
#ifndef CORRELATOR_H
#define CORRELATOR_H
/* header file correlator.h */

#include "newcorr.h"

/*
#define DEBUG
#undef DEBUG
#define GATHER
//#define AMP_PHAS

#define CHANNEL 512
#define FFTLEN (4*CHANNEL)
#define NCHAN 4
#define CORRLEN (NCHAN*FFTLEN)
#define NUM_CORR 8
#define NUM_PROCESSES 16
#define NNODECORR 8
#define FRNG_STEP 0.25
#define NSTEP 2880

#define NCORR (NUM_CORR*NCHAN*(NUM_CORR*NCHAN+1)/2)
//#define M_PI 3.141592654
#define ACQ_LEN (32*1024*1024)
#define UNPACK_LEN (32*1024*1024)
#define MPI_BUF_CHUNK (ACQ_LEN/NUM_CORR)
#define MPI_EXCESS_CHUNK (64*1024)
#define MPI_OVL_CHUNK (MPI_BUF_CHUNK+MPI_EXCESS_CHUNK)
#define ACQ_OVL (ACQ_LEN+MPI_EXCESS_CHUNK)

#define SWEEPS_PER_DUMP 1
#define CORR_SIZE (FFTLEN*NCHAN*NUM_CORR*(NCHAN*NUM_CORR+1)/4)

#define NTCHUNK 16
#define FFTBLOCK 64 */


static float corrbuf[2*CORR_SIZE];         //%2% CorrSize become double
static float corr_sum[2*CORR_SIZE];

static float phase_ti[2][NUM_ANT*NCHAN*POLS*FFTLEN/2];
static float dphase_t0[2][NUM_ANT*NCHAN*POLS*FFTLEN/2];
static short int iabuf[2][(BEAM_SIZE/4)*NUM_ACQ];   // 16*beam_len
static char pabuf[2][BEAM_SIZE];
static char pa_voltage[2][BEAM_SIZE*NUM_ANT];
static short int collect_pa[2][(BEAM_SIZE/4)*NUM_ACQ];
typedef float corrarray_t[NCORR][FFTLEN];
typedef float datarray_t[NCHAN*NUM_ANT*POLS][FFTLEN];

/*
typedef struct AntennaParType;
typedef struct CorrType;
typedef struct DasParType; 
typedef struct ModelParType;
typedef struct SourceParType;
*/

 CorrType corr;
 DasParType daspar;
 ModelParType mpar[MAX_SAMPS];  //%% ModelParType in old system ModelInfoType, calModelPar use this 'par'
 SourceParType source;
 ModelInfoType model ;  // Extra Added GSB %%

/* GSBC :: The declearation should be global to all and expected
to share among NUM_PROCESS=16, or NUM_PROCESS > 8 */

ProjectType Project[MAX_PROJECTS] ;
ScanInfoType ScanTab[MAX_SCANS] ;

 CorrType *Corr;    /*Defined in parallerl_correlator.h  'corr' */
 ModelInfoType *Model ; 
 DasHdrType  *dHdr ;
 DataBufType *dBuf ;
 int scan_info;

// DataBufType *dBuf ; /* GSBC: acq30 newcorr.h buf */

//AntennaParType antenna [30];

//datarray_t phase;
corrarray_t corrampphas;
corrarray_t corrbufavg;
corrarray_t corrbuf_acc;

// %1% Require to tranfer Data 
// %1% Temporary disabled typedef struct { float re, im ; } Complex ;
typedef  float Complex ; // To define corrbuf[CORRSIZE]
/*
static Complex *LtaBuf ;
static char *OutBuf=NULL ;
static float *LtaWt ;
static double *LtaTime ;
static DataParType *LtaPar, *ParBuf=NULL ;
static DataTabType *dTab;
static int dRec, pRec;  // dRec over MaxDataBuf, pRec: Physical MaxBlocks //
static float *Bufp; */

void corr_driver(signed char *buffer, int nint_corr, int nint_ia, int nint_pa, struct timeval timestamps, double *ch_freq, double BW, int w_buff, int fstop, int PAPols,int IAPols, int IASubPols, float step, short int *phase, int *iamask, short int *pamask, int ia_count,int pa_count,FILE *fp_data, FILE *fp_time,float *phastab,char *beam_mode1, char *beam_mode2,int mode);
void correlate(short int obuf[FFTLEN/FFTBLOCK][UNPACK_LEN/(FFTLEN*NCHAN*NUM_ANT)][NUM_CORR][NCHAN][FFTBLOCK], float corrbuf[FFTLEN/16][NCORR][2][4],short int outiabuf[BEAM_SIZE/4],short int outpabuf[BEAM_SIZE/4], char pabuf[BEAM_SIZE],int iamask[NCHAN*NUM_ACQ], short int pamask[NCHAN*NUM_ACQ], int nint_ia, int nint_pa, int ia_count,int pa_count);
void convcorr(float corrbuf[CORR_SIZE],float outcorrbuf[FFTLEN/8][NCORR][2][4]); //%2% Same Size! 
float current_time(void);
int send_data(int integrated, double time_ms, float *corr_buf);
void write_corr(float *corr_buf, struct tm* local_t, double time_ms, struct timeval timestamps,int iteration, FILE *fp_data,FILE *fp_time);
int corr_hdr(char *filename, CorrType *corr);
int corr_pre(char *filename, CorrType *corr);
void get_antenna(FILE *f, AntennaParType *antenna);
int get_sampler(FILE *f, CorrType *corr);
void get_freq_ch0(SourceParType *source);
double mjd_cal(struct tm *t);
double lmst(double mjd);
void calModelPar(double tm, CorrType *corr, ModelParType *mpar, SourceParType *source, int *antmask);
void gsbe_ScanInfo(CorrType *corr, ModelInfoType *model, ScanInfoType *) ; /* GSBC : Just getting info for each scan*/
#endif
