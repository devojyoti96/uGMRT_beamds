/* icc -O3  -D_FILE_OFFSET_BITS=64 -D_LARGEFILE64_SOURCE=1 -D_LARGEFILE_SOURCE=1 -o read_PAbeam read_PAbeam.c

*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <sys/timeb.h>
#include <string.h>

#include "parallel_correlator.h"
#include "newcorr.h"

#include "gsb_unshuf.h"

//#undef POLAR
#undef CHAN_INT

#define POLAR 

int shmHId, shmBId;
DataHeader *dataHdr;
DataBufferIA *dataBuf;

void initialise () {
  shmHId = shmget( DasHeaderKey, sizeof( DataHeader ), SHM_RDONLY );
  shmBId = shmget( DasBufferKey, sizeof( DataBufferIA ), SHM_RDONLY );

  if( shmHId < 0 || shmBId < 0 ) {
    fprintf(stderr, "Error in attaching shared memory..\n");
    exit(-1);
  }

  dataHdr = (DataHeader *) shmat( shmHId, 0, 0 );
  dataBuf = (DataBufferIA *) shmat( shmBId, 0, 0 );
}

int main(int argc, char *argv[])
{
	//static short int data[IABeamBufferSize];
	unsigned int curRec = 0, curBlock;
	int fp_PA;
	int i, j, iteration = 0;
	struct timeval timestamp;
        FILE *ftsamp, *fp_data;
        double frac_sec;
        char time_string[40];
        struct tm* local_t;
	short int *data, *data1;
	unsigned char *data2;
	 if (argc <  7)
        {       fprintf(stderr, "Usage: %s <input data file name><output file name><observation duration in seconds><Frequency channel><input time resolution in sec><integration factor for PA beam><scaling factor>\n", argv[0]);
                exit(-1);
        }

        int loops = atoi(argv[3]); // time in seconds
	int channel = atoi(argv[4]); // No of frequency channel
	float tint = atof(argv[5]); // Input time resolution in sec
        int nint_pa = atoi(argv[6]); // integ factor for PA beam
	int scale = atoi(argv[7]); // scaling the 16bit to 8bit 
        //loops = loops * 4;

	system("date");
	int cnt = (int)((loops*1.0)/tint);
#ifdef POLAR
	int datacnt = (1.0*channel*cnt)/(1.0*BEAM_SIZE*16);	
#else
	int datacnt = (1.0*channel*cnt)/(1.0*BEAM_SIZE*4); // 8MB block	
#endif
	
	fp_PA = open(argv[2], O_CREAT|O_TRUNC|O_WRONLY|O_SYNC,S_IRUSR|S_IWUSR); // output file
	fp_data=fopen64(argv[1], "rb");  // input file
#ifdef POLAR
	data = (short int *)malloc((BEAM_SIZE)*32);
	data1 = (short int *)malloc((BEAM_SIZE)*32);
	data2 = (unsigned char *)malloc((BEAM_SIZE)*8);
#else
	data = (short int *)malloc((BEAM_SIZE)*8); // 16M * 16bit block
	data1 = (short int *)malloc((BEAM_SIZE)*8);
	data2 = (unsigned char *)malloc((BEAM_SIZE)*8);
#endif
	while(iteration != datacnt) {

#ifdef POLAR
	fread(data,2,(BEAM_SIZE*16),fp_data);     
	for (i=0;i<(BEAM_SIZE*16);i++)
	data1[i] = 0;
#else
	fread(data,2,(BEAM_SIZE*4),fp_data);  // 8M * 16bit read at a time   
	for (i=0;i<(BEAM_SIZE*4);i++)
	data1[i] = 0;
#endif
	
#ifdef POLAR
	for (i=0;i<((BEAM_SIZE*4)/channel);i++){
	for(j=0;j<4*channel;j=j+4)
        {        data1[j+(i/nint_pa)*4*channel] = (data1[j+(i/nint_pa)*4*channel] + data[j+i*4*channel]/nint_pa);  // Add two pols
                 data1[j+(i/nint_pa)*4*channel+1] = (data1[j+(i/nint_pa)*4*channel+1] + data[j+i*4*channel+1]/nint_pa);  // Add two pols
                 data1[j+(i/nint_pa)*4*channel+2] = (data1[j+(i/nint_pa)*4*channel+2] + data[j+i*4*channel+2]/nint_pa);  // Add two pols
                 data1[j+(i/nint_pa)*4*channel+3] = (data1[j+(i/nint_pa)*4*channel+3] + data[j+i*4*channel+3]/nint_pa);  // Add two pols
		 data2[j/4+(i/nint_pa)*channel] = (unsigned char)(((data1[j+(i/nint_pa)*4*channel] + data1[j+(i/nint_pa)*4*channel+2]/scale))); // stokes-I
        }

#ifdef CHAN_INT
	for(j=0;j<4*channel;j=j+8)	
	{	
		data1[j/2+(i/nint_pa)*2*channel] = (data1[j+(i/nint_pa)*4*channel+4] + data1[j+(i/nint_pa)*4*channel])/2; 
		data1[j/2+(i/nint_pa)*2*channel+1] = (data1[j+(i/nint_pa)*4*channel+4+1] + data1[j+(i/nint_pa)*4*channel+1])/2; 
		data1[j/2+(i/nint_pa)*2*channel+2] = (data1[j+(i/nint_pa)*4*channel+4+2] + data1[j+(i/nint_pa)*4*channel+2])/2; 
		data1[j/2+(i/nint_pa)*2*channel+3] = (data1[j+(i/nint_pa)*4*channel+4+3] + data1[j+(i/nint_pa)*4*channel+3])/2; 
	
	}
#endif
	}
	#ifdef CHAN_INT
	write(fp_PA,data1,sizeof(short int)*(BEAM_SIZE*8)/nint_pa);
	#else
	write(fp_PA,data2,sizeof(char)*(BEAM_SIZE*4)/nint_pa); // stokes-I only
	#endif
#else
        for (i=0;i<((BEAM_SIZE*4)/channel);i++)
	{ for(j=0;j<channel;j++)
                data1[j+(i/nint_pa)*channel] = (data1[j+(i/nint_pa)*channel] + data[j+i*channel]/nint_pa);  // Add two pols
	#ifdef CHAN_INT
	  for(j=0;j<channel;j=j+2)
		data1[j/2+(i/nint_pa)*(channel/2)] = (data1[j+(i/nint_pa)*channel] + data1[j+(i/nint_pa)*channel+1])/2;
	#endif
	}
	#ifdef CHAN_INT
	for (i=0;i<(BEAM_SIZE*2)/nint_pa;i++)
	data2[i] = (unsigned char)(data1[i]/scale); 
	write(fp_PA,data2,sizeof(char)*(BEAM_SIZE*2)/nint_pa);
	#else
	for (i=0;i<(BEAM_SIZE*4)/nint_pa;i++)
        data2[i] = (unsigned char)(data1[i]/scale);
        write(fp_PA,data2,sizeof(char)*(BEAM_SIZE*4)/nint_pa);
	#endif
#endif	
	
	iteration++;
	}
	close(fp_PA);
	fclose(fp_data);
        //fclose(ftsamp);
}	
