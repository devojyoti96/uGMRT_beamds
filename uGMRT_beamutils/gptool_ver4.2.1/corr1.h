# ifndef __CORR_H__
# define __CORR_H__
/* corr.h : Changed on 5 Feb, 1997.
   Fundamental Requirement of structures declared here is that
   they should have same sizes and alignment on all machines.
   DOS is exempted.
*/

enum { USB_130, USB_175, LSB_130, LSB_175, MAX_BANDS };
enum { RRLL, RRRL, RR__, MACMODES } ;
enum { arar_arar, alal_alal, brbr_brbr, blbl_blbl,
       aral_brbl, aral_alar, brbl_blbr, arbr_albl, DpcMuxVals} ;
enum { ANTNAMELEN=4, NAMELEN=32, DATELEN=32 };
enum { MAX_ANTS=30, MAX_SAMPS=60, MAX_FFTS=MAX_SAMPS, MAC_CARDS=33,
       MAX_MACS=16*MAC_CARDS, MAX_BASE=2*MAX_MACS, MAX_CHANS=256, POLS=2
     };
    /* be sure that  MAX_FFTS and MAX_SAMPS are both even numbers ! */
enum { MAX_ARRAYS=1 };
enum { LittleEndian=1, BigEndian=0 };

typedef struct 
{ int    ants, samplers, macs, channels, pols, sta, statime /* in usec */;
  unsigned char dpcmux,clksel,fftmode,macmode ;  /* replaces old dummy int */
  float  f_step, clock; /* Hz  */
  double t_unit;        /* Sec */
} CorrParType ;

typedef struct
{ char   object[NAMELEN], date_obs[DATELEN];
  char   observer[NAMELEN], project[NAMELEN], code[8] ;
  double ra_date, dec_date, mjd_ref, dra, ddec, period;
  int    antmask, bandmask, flag, seq , fold_flg;
         /* we use (daspar.antmask & scanpar.antmask) */
  float  integ, f_step , dm;
  int    rf[2], first_lo[2], bb_lo[2];  /*  Hz  */
  float  BW;
  int ref_ch, i_side_band;
} ScanParType;

typedef struct
{ int    antmask, samplers, macs, baselines, channels, lta;
  //short  bandmask, mode ;
  short  samp_num[MAX_SAMPS], mac_num [MAX_MACS ],
         base_num[MAX_BASE ], chan_num[MAX_CHANS];
  short  dummy[(2+MAX_SAMPS+MAX_MACS+MAX_BASE+MAX_CHANS) % 4]; /*double align*/
} DasParType ;

typedef struct { unsigned char ant_id, band, fft_id, array; } SamplerType;

typedef struct
{ unsigned char flag[4];
  //int           antmask;                   /* It's dummy, may be changed.  */
  char          version [NAMELEN   ];
  char          bandname[MAX_BANDS ][8];
  //SamplerType   sampler [MAX_SAMPS ];      /* ant, band vs. ffts           */
  //unsigned char mac     [MAX_MACS  ][4];   /* The 4 ffts, the mac scans    */
  CorrParType   corrpar;                   /* Max. enabled mac_params      */
  DasParType    daspar;                    /* Actually useful params       */
  ScanParType   scanpar [MAX_ARRAYS];      /* Observation dependent params */
} CorrType;

typedef struct { float phase, dp, delay, dslope; } DataParType;
           /* units:  phase in radian, dp in radian/sec,
                      delay in sec, dslope in radians/channel
           */

# define AntennaTypeSize     sizeof(AntennaType)
# define SamplerTypeSize     sizeof(SamplerType)
# define DataParTypeSize     sizeof(DataParType)
# define CorrTypeSize        sizeof(CorrType)
# define CorrSize            sizeof(CorrType)

typedef struct
{ int in0,in1,out0,out1 ;} IndexType ;

typedef struct 
{ double bxcd,bycd,bzsd,fixed,phase, bb_lo,freq, f_step ; 
} GeomType ;

/*About the shm collect_psr*/
# define DAS_H_KEY 1020
# define DAS_D_KEY 1021
# define DAS_P_KEY 1022
//# define DAS_BUFSIZE 4096000
//# define DAS_BUFSIZE 16908288

# define DAS_BUFSIZE 135266304 // 8 blocks
//# define DAS_BUFSIZE 67633152 // 4 blocks
# define DAS_HDRSIZE  200000

typedef struct
{ int s0,s1, card;  /* card points to relevant delay/FFT card */
   /* 
	    The two relevant samplers are given by 
	    daspar.samp_num[s0] and daspar.samp_num[s1]
	 */
  int delay, p0, pstep0,p1,pstep1, fstc, fstc_step ;  /* is int ok? */
  /* Do not plant delay difference between two streams here;
     the difference must be handled as static difference
     during initialisation
  */
  float p2fstc ;
} FftDelayParType ;

typedef struct
{ double clock, t_update  ;
  double pc_time ;
  double delay_step, fstc_scale, nco_tick_time ;
  double t_off;
  int cycle, seq, cards ;
            /*cycle = STA cycles between model update */
  unsigned char dpcmux,clksel,fftmode,macmode ;
  DataParType par[MAX_SAMPS] ;
	FftDelayParType fdpar[MAX_ANTS] ;
} ModelParType ;

typedef struct
{ int active, status, scan, scan_off;
  CorrType corr ;
  ModelParType model ;
  char buf[DAS_HDRSIZE];
} DasHdrType ;

typedef struct
{ int start_off, recl,  block, active;
  char buf[DAS_BUFSIZE] ;
} DasBufType ;

enum { BufMarked=1, BufReady=1<<1,  Rack0Bad=1<<2,Rack1Bad=1<<3,Rack2Bad=1<<4,
       Rack3Bad=1<<5,Rack4Bad=1<<6,Rack5Bad=1<<7,
       DelaySet=1<<8, FringeStop=1<<9, MakeFSTC=1<<10,
       MaxDataBuf=100
//       MaxDataBuf=20
     };
typedef struct
{ unsigned short flag, rec;
  int seqnum;
} DataTabType;
typedef struct
{ int flag, blocksize, maxblocks, cur_block, first_block, cur_rec;
  DataTabType dtab[MaxDataBuf];
  char buf[DAS_BUFSIZE] ;
} DataBufType ;

# endif

