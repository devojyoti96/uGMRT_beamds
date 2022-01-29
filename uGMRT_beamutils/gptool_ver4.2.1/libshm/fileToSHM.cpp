#include "libshm.h"




# include <stdlib.h>
# include <iostream>
# include <fstream>
# include <string>
# include <string.h>
# include <unistd.h>
# include <stdio.h>
#include <sstream>
# include <sys/shm.h>
#include <time.h>
#include <signal.h>
# define BUFSIZE 5368709120
using namespace std;

int main(int argc, char *argv[])
{
	int arg = 1;
	long int shmsize;
	char* filepath;
	int nchan=2048;
	float sampling=0.08192e-3;
	int nSHMBuf;
	unsigned short int* rawdata;
	int counter=0;
	if(argc <= 1)
  	{
    		cout<<"Not enough arguments."<<endl;
    		exit(0);
  	}
  	while(arg < argc)
  	{
  		arg++;
  	}
  	arg=1;
  	while(arg < argc)
  	{
    		if(argv[arg][0]=='-')
    		{
        		   
      			switch(argv[arg][1])
     			{
        
        			case 'f':
        			case 'F':
        			{          
          				filepath = argv[arg+1];          				
          				arg+=2;
        			}
        			break;
			}
		}
	}
	ifstream datafile;
	datafile.open(filepath,ios::binary);	
	datafile.seekg(0,ios::beg);
	Correlator corrSHM(nchan,sampling);
	shmsize = corrSHM.initializeWriteSHM();
	cout<<shmsize<<endl;
	nSHMBuf=int(BUFSIZE/(shmsize));
	cout<<"Number of shared memory blocks in buffer: "<<nSHMBuf<<endl;
	cout<<"Buffering "<<nSHMBuf*shmsize*sampling/float(2*nchan)<<" seconds of data from file"<<endl;
	
	rawdata=new unsigned short int[nSHMBuf*shmsize];
	datafile.read((char *)rawdata,nSHMBuf*shmsize);
	cout<<"Buffering done.."<<endl;
	cout<<"Number of shared memory blocks in buffer: "<<nSHMBuf<<endl;
	datafile.close();
	clock_t start, end;
     	double cpu_time_used;
	double shmtime=shmsize*sampling/float(2.0*nchan);
	while(1)
	{
		
     
		start = clock();
 
    
		cout<<"Copying file buffer to SHM : "<<counter<<endl;
		corrSHM.writeToSHM(rawdata+counter*shmsize/2);
		counter=(counter+1)%nSHMBuf;
		do
		{
			end = clock();
			cpu_time_used = ((double) (end - start)) / CLOCKS_PER_SEC;
		}while(cpu_time_used<shmtime);
	}

}
