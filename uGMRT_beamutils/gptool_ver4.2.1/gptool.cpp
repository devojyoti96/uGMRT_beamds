/*************************************************************
The GMRT Pulsar Tool (gptool)

The tool has its foundation in pmon and then wpmon. It was 
written from scratch keeping in mind the need for it to run 
real time with the high data flow rate of the upgraded GMRT. 
This was done in consultation with Yashwant Gupta. This version
uses c++ constructs such as classes to steamline to code and
to make feature expansions easier. If you're working on it 
please keep in mind these two guidelines:

1> Continue using similar naming conventions.
2> This programs owes a lot of its speed to how it handles array
   access. Use pointers to access arrays instead of indexing 
   wherever an array is being accessed repeatedly in a loop. 
   You can find multiple examples of this throughout the code.

			-Aditya Chowdhury, 14th March 2016

************************************************************/

# include "cpgplot.h"
# include <stdlib.h>
# include <iostream>
# include <fstream>
# include <math.h>
# include <string>
# include <string.h>
# include <unistd.h>
# include <stdio.h>
#include <sstream>
#include <omp.h>
# include <sys/shm.h>
#include <signal.h>
#include <iomanip>
#include <ctime>
#include <sched.h>
//Includes for attaching to shared memory
# include "gmrt_newcorr.h"
//# include "acqpsr.h"
# include "protocol.h"
# include "libshm.h"
using namespace std;

/*******************************************************************
*CLASS:	Information
*Stores all the input parameters to gptool. At run-time there is one 
*instance of this class being shared by all other objects that does 
*various operations on the data. 

*Also has an member function that read from a gptool.in file and 
*intiliazes their variables. 
*******************************************************************/
class Information
{	
	
	public:
	
	//variables
	string			pulsarName;
	string			modeOperation; 		//PA or IA
	float			lowestFrequency;
	float			bandwidth;
	int			noOfChannels;
	int 			startChannel; 		//Channels before startChannel are ignored by the program.
	int 			stopChannel;		//Channels after stopChannel are ignored by the program.
	double			samplingInterval;	//In seconds
	double			periodInMs;		//Pulsar period
	int			periodInSamples;
	float			profileOffset;		//Phase (0 to 1) by which the profile will be shifted
	float 			dispersionMeasure;	//in pc/cc
	float			blockSizeSec;		//Size of each window in seconds
	long int		blockSizeSamples;	//Size of each window in number of samples
	int			sampleSizeBytes;	//Stores the size in bytes of each sample. Usual GMRT data is sampled to 2byte integers.
	char			doFixedPeriodFolding;	//0-> use polyco based folding 1-> use fixed period to fold
	int*			badChanBlocks;		//manually entered list of bad channel blocks
	int			nBadChanBlocks;		//number of such bad blocks
	unsigned short		meanval;
	int			shmID;			//1-> standard correlator shm; 2-> gptool file shm; 3-> gptool inline shm
	//Parameters for polyco folding
	char*			psrcatdbPath;		//Path to psrcat database.
	double			MJDObs;			//Mean Julian Day at the start of observation
	int			spanPolyco,maxHA; 	//time span (in minutes) of each polyco set
	int			nCoeffPolyco;		//number of coefficients in each polyco set
	double*			polycoTable;		//relevent parameters from polyco.dat
	int 			polycoRowIndex;		//index number of correct polyco set.
	char			sidebandFlag;		//A sideband flag of 0 indicates frequencies decreasing with channel number. 1 is for the reverse scenario.
	char*			filepath;		//In offline mode, the path of the raw data file.
	string		outputfilepath;	//path where filtered raw data is written out
	string			filename;		//Name of raw data file in offline mode
	char			noOfPol;		//Number of polarization channels. Usually 4.
	char			polarChanToDisplay;	//Number of polarization channels to display.
	//Various flags:
	
	char			doReadFromFile;		//0-> Real time processing;	1-> Reading from file	
	char			doManualMode;		//0-> Automatic mode;		1-> Manual mode
	char			doWindowDelay;		//0-> No delay processing	1-> Adds artificial waiting time after each window to emulate real time 
	char			doWriteFullDM;		//1-> Write dedispersed series
	char			doWriteFiltered2D;	//1-> Write filtered 2D data
	char			doPolarMode;		//1-> Polarization data
	char			doReplaceByMean;	//1-> Flagged samples replaced by value of central tendency (either median or mode depending on algorithm chosen)
	char			doFilteringOnly;	//1-> Will perform filtering and write out filtered 2D time freq data	
	char			refFrequency;		//Dedisperse w.r.t 1-> Highest frequency 0-> Lowest frequency
	char			doUseTempo2;		//Sets the flag to decide whether to use tempo2 or tempo1
	char			isInline;		//inline mode of gptool - read and write to SHM
	//Time filtering information:
	char			doTimeFlag;		//0-> Time flagging off		1-> Time flagging on
	char			timeFlagAlgo;		//1-> Histogram based		2-> MAD based
	char			doTimeClip;		//0-> Time sample clipping off	1-> On
	char			doMultiPointFilter;	//0-> single point filter	1-> Multi-point filter
	char			doUseNormalizedData;	//0-> w/o normalization 	1-> w normalizations
	float			timeCutOffToRMS;	//Cutoff to rms ratio
	float*			cutoff;			//multi point rms cutoffs for time filtering stored here
	char			doWriteTimeFlags;	//0-> Flag file not written	1-> File flag written
	float			smoothFlagWindowLength;
	float			concentrationThreshold;
	//Channel filtering information:
	char			doChanFlag;		//0-> Chan flagging off		1-> Channel flagging on
	char			chanFlagAlgo;		//1-> Histogram based		2-> MAD based
	char			doChanClip;		//0-> Channel clipping off	1-> On
	char			bandshapeToUse;		//1-> Mean Bandshape		2-> Normalized bandshape	3->Mean-to-rms bandshape 4-> Both 2 and 3
	char			doZeroDMSub;		//0-> No zero DM subtraction 1-> On
	char			doRunFilteredMode;	//0-> Off 1-> Filtered mode run with no filtering
	float			chanCutOffToRMS;	//Cutoff to rms ratio
	char			doWriteChanFlags;	//0-> Flag file not written	1-> File flag written
	int			smoothingWindowLength;	//The window length of moving median smoothing.
	char			flagOrder; 		//1->channel first;2->time first;3->independent	
	char			normalizationProcedure; //1-> Cumulative bandshape, 2-> Externally supplied bandshape.dat
	double 			startTime;		//For file read the start time (used to skip some initial data blocks
	double			startBlockIndex;	//Block index corresponding to the start time
	//Functions:
	double stringToDouble(const std::string& s);	//Converts strings to double, used to take input from .in file
	void readGptoolInputFile();			//Function to read from .in file
	short int checkGptoolInputFileVersion();	//Checks if gptool.in has the latest version formatting.
	void reformatGptoolInputFile();			//reformats gptool.in file to latest version
	void cutOffCalculations();			//calculates the double,triple and quadrapule point cutoffs
	void display();					//Displays all input information to the user
	void writeWpmonIn();				//Writes out a gptool.in sample when it is not found.
	void writeInfFile();				//Writes out INF file to be used by presto
	void displayNoOptionsHelp();			//Displays possible ways to run gptool
	void genMJDObs();				//Generates mean julian day at the start of observation
	void getPsrcatdbPath();				//Gets the location of psrcat database.
	void genPolycoTempo1();				//Fires tempo1 to generate polyco.dat
	void genPolycoTempo2();				//Fires tempo2 to generate polyco.dat
	void loadPolyco();				//loads information from polyco.dat to double* polycoTable
	void checkPulsarName();				//Verifies if the pulsar name is valid
	void autoDM();					//Gets dispersion measure from psr cat
	void calculateCutoff();
	void errorChecks(); 		//Checks for error in the .in file
	void fillParams();
	void parseManFlagList(std::string& s);	//parses list of bad sub-bands
};

//implementation of Information class methods

/*******************************************************************
*FUNCTION: double Information::stringToDouble(const std::string& s)
*string& s		: string to convert from
*returns *double x 	: the converted double
*******************************************************************/
double Information::stringToDouble(const std::string& s)
{
	istringstream i(s);
	double x;
	if (!(i >> x))
		return 0;
	return x;
 }

/*
void Information::reformatGptoolInputFile_old()
{
	system("cp gptool.in gptool.in.oldver");
	char* fileparstr = "gptool.in";
	string line;
	string lineTimeFlags,lineChanFlags,linefullDM;
	int k=1,CompleteFlag = 0;
	ifstream filepar(fileparstr,ios::in);
	stringstream fileOutput;
	fileOutput<<"#*#*#gptool input file v1.5#*#*#"<<endl;
	fileOutput<<"-------------------------------------------------"<<endl;

	while(getline(filepar,line))
	{
*/
		/*******************************************************************
		 *tempStr stores the current parameter read as string 
		 *This is then converted to the required data type of the parameters
		 *according to the line number. 
	 	 *******************************************************************/
/*		int c=0;
		int e=0;
	  	string tempStr;
		while(line[e]!=':' && line[e]!='\0')
		  	e++;		
		while(line[c]!=' ' && line[c]!='\0' && line[c]!='\t' && c<e)
		{
		  	tempStr+=line[c];
		  	c++;
		} 
		if(k<=35 || (k>36 && k<43) || (k>43 && k<48))
			fileOutput<<line<<endl;
		
		if(k==18)
			fileOutput<<"-1\t\t: Number of bins in folded profile (-1 for native resolution)"<<endl;
		if(k==36)
			lineChanFlags=line;
		if(k==43)
			lineTimeFlags=line;
		if(k==48)
			linefullDM=line;
		k++;
	}
	fileOutput<<"0\t\t: Replace by median values (0-> Ignore flagged samples, 1-> Replace flagged samples by window median)"<<endl;
	fileOutput<<"-------------------------------------------------"<<endl;
	fileOutput<<"#****I/O options****#"<<endl;
	fileOutput<<lineChanFlags<<endl;
	fileOutput<<lineTimeFlags<<endl;
	fileOutput<<"0\t\t: write out filtered 2D raw data (0-> no, 1-> yes)"<<endl;
	fileOutput<<linefullDM<<endl;
	filepar.close();

	ofstream newInFile(fileparstr,ios::out);
	newInFile<<fileOutput.str().c_str();
	newInFile.close();
}	
*/
/*******************************************************************
*FUNCTION: void Information::reformatGptoolInputFile()
*Checks if the gptool.in file is latest version or not.
*returns 1 if latest version otherwise returns 0.
*******************************************************************/
void Information::reformatGptoolInputFile()
{
	system("cp gptool.in gptool.in.oldver");
	char* fileparstr = "gptool.in";
	string line;
	string lineTimeFlags,lineChanFlags,linefullDM;
	int k=1,CompleteFlag = 0;
	ifstream filepar(fileparstr,ios::in);
	stringstream fileOutput;
	fileOutput<<"#*#*#gptool input file v2.0#*#*#"<<endl;
	fileOutput<<"-------------------------------------------------"<<endl;
	while(getline(filepar,line))
	{
		if((k>2 && k<56))
			fileOutput<<line<<endl;
		
		k++;
	}
	filepar.close();
	fileOutput<<"-------------------------------------------------"<<endl;
	fileOutput<<"#****manual flagging options****#"<<endl;
	fileOutput<<"0		: Number of bad channel blocks"<<endl;
	fileOutput<<"List: 		#in next line, example: [200,400],[1200,1400]"<<endl;
	fileOutput<<""<<endl;	

	ofstream newInFile(fileparstr,ios::out);
	newInFile<<fileOutput.str().c_str();
	newInFile.close();
}	
/*******************************************************************
*FUNCTION: short int Information::checkGptoolInputFileVersion()
*Checks if the gptool.in file is latest version or not.
*returns 1 if latest version otherwise returns 0.
*******************************************************************/
short int Information::checkGptoolInputFileVersion()
{
	char* fileparstr = "gptool.in";
	string firstline;
	string newfirstline="#*#*#gptool input file v2.0#*#*#";
	int k=1,CompleteFlag = 0;
	ifstream filepar(fileparstr,ios::in);
	if(!filepar.is_open())
	{
		cout<<"gptool.in not found!"<<endl;
	 	cout<<"A sample gptool.in file has been written to the current directory."<<endl;
		writeWpmonIn();
	  	exit(0);
	}
	getline(filepar,firstline);
	if(firstline.compare(newfirstline)==0)
		return 1;
	else
		return 0;

}

/*******************************************************************
*FUNCTION: void Information::readGptoolInputFile()
*Reads data from gptool.in file.
*******************************************************************/
void Information::readGptoolInputFile()
{
	  char* fileparstr = "gptool.in";
	  string line;
	  int k=1,CompleteFlag = 0;
	  ifstream filepar(fileparstr,ios::in);
	  if(!filepar.is_open())
	  {
	  	cout<<"gptool.in not found!"<<endl;
	 	cout<<"A sample gptool.in file has been written to the current directory."<<endl;
		writeWpmonIn();
	  	exit(0);
	  }

	  while(getline(filepar,line))
	  {
		/*******************************************************************
		 *tempStr stores the current parameter read as string 
		 *This is then converted to the required data type of the parameters
		 *according to the line number. 
	 	 *******************************************************************/
		int c=0;
		int e=0;
	  	string tempStr;
		while(line[e]!=':' && line[e]!='\0' && line[e]!='\n')
		  	e++;		
		while(line[c]!=' ' && line[c]!='\0' && line[c]!='\t' && c<e)
		{
		  	tempStr+=line[c];
		  	c++;
		} 
		switch(k)
	    	{
	    		
	      		case 1:
				case 2:
				case 3:
	      			break;
	      		case 4:
	      		{
					modeOperation=tempStr;
					break;
	      		}
				case 5:
	      		{
					doPolarMode=char(int(stringToDouble(tempStr)));
					if(doPolarMode)
						noOfPol=4;
					else
						noOfPol=1;
					break;
	      		}			
				case 6:
	      		{
	       			sampleSizeBytes=int(stringToDouble(tempStr));
					break;
	      		}
				case 7:
				case 8:
					break;
				case 9:
				{
					lowestFrequency=stringToDouble(tempStr);	
					break;
			  	} 
				case 10:
			  	{
					bandwidth=stringToDouble(tempStr);  
					break;
			  	}
				case 11:
			  	{
			  			int temp=int(stringToDouble(tempStr));
			  			if(temp==-1)
			  				sidebandFlag=0;
			  			else if(temp==1)
			  				sidebandFlag=1;
			  			else
			  				sidebandFlag=5; //error case
					break;
			  	}
				case 12:
			  	{
					noOfChannels=int(stringToDouble(tempStr));
					break;
			  	}
				case 13:
			  	{
					samplingInterval=stringToDouble(tempStr)/1000.0; //Converting milliseconds to seconds
					break;
			  	}
				case 14:
				case 15:
					break;
				case 16:
			  	{
					pulsarName=tempStr;
					break;
				}			
	      		case 17:
	      		{
	       			periodInMs=stringToDouble(tempStr);
	       			if(periodInMs==-1)
	       				doFixedPeriodFolding=0;
	       			else
	       				doFixedPeriodFolding=1;
				break;
	      		}
	      		case 18:
	      		{
	       			dispersionMeasure=stringToDouble(tempStr);
				break;
	      		}
	      		case 19:
				case 20:
					break; 
				case 21:
	      		{				
	       			periodInSamples=(int)stringToDouble(tempStr);
					break;
	      		}
				case 22:
	      		{				
	       			profileOffset=stringToDouble(tempStr);
				break;
	      		}
				case 23:
	      		{
	      			nCoeffPolyco=int(stringToDouble(tempStr));
	      			break;	      			
	      		}
	      		case 24:
	      		{
	      			spanPolyco=int(stringToDouble(tempStr));
	      			break;
	      		}
				case 25:
	      		{
	      			maxHA=int(stringToDouble(tempStr));
	      			break;
	      		}
				case 26:
				case 27:
					break;
				case 28:
				{
					polarChanToDisplay=char(int(stringToDouble(tempStr)));
				}
				case 29:
				{
					blockSizeSec=stringToDouble(tempStr);
					break;
				}
				case 30:      		
	      		{
	      			doManualMode=char(int(stringToDouble(tempStr)));        		
	       			break;
	      		}
	      		case 31:
	      		{
	      			doWindowDelay=char(int(stringToDouble(tempStr))); 
	       			break;
	      		}
				case 32:
				case 33:
					break;
	      		case 34:
	      		{
				startChannel=int(stringToDouble(tempStr));
	      			break;
	      		}
	      		case 35:
	      		{
	       			stopChannel=noOfChannels-int(stringToDouble(tempStr));
	      			break;        
	      		}	      		
	      		case 36:
	      		{
	       			doChanFlag=char(int(stringToDouble(tempStr)));
				break;
	      		}
	      		case 37:
	      		{
	       			bandshapeToUse=char(int(stringToDouble(tempStr)))+1;			//Plus one done to skip the now obsolete option of using mean bandshape to filter.
					break;
	      		}
	      		case 38:
	      		{
	       			chanCutOffToRMS=stringToDouble(tempStr);
	       			break;
	      		}	      		
				case 39:
				case 40:
				break;
	      		case 41:
	     		{
					doTimeFlag=char(int(stringToDouble(tempStr)));
					break;
	      		}
				case 42:
	     		{
				doUseNormalizedData=char(int(stringToDouble(tempStr)));
				break;
	      		}
	      		case 43:
	      		{
				timeFlagAlgo=char(int(stringToDouble(tempStr)));
	       			break;
	      		}	      		
	      		case 44:
	      		{
	       			timeCutOffToRMS=stringToDouble(tempStr);
				break;
	      		}   
				case 45:
				case 46:
					break;   
				case 47:
				{
					smoothingWindowLength=int(stringToDouble(tempStr));
					break;
				}
				case 48:
				{
					normalizationProcedure=char(int(stringToDouble(tempStr)));
					break;
				} 	
				case 49:
				{
					doReplaceByMean=char(int(stringToDouble(tempStr)));
					break;
				} 	
				case 50:
				case 51:
					break; 
				case 52:
	      		{
	      			doWriteChanFlags=char(int(stringToDouble(tempStr)));				
	       			break;
	      		}
				case 53:
	      		{
	      			doWriteTimeFlags=char(int(stringToDouble(tempStr)));
	       			break;
	      		}
				case 54:
	      		{
	      			doWriteFiltered2D=char(int(stringToDouble(tempStr)));
	       			break;
	      		}
	      		case 55:      		
	      		{
	      			doWriteFullDM=char(int(stringToDouble(tempStr)));        		
	       			break;
	      		}
			case 56:
			case 57:
				break;
			case 58:      		
	      		{
	      			nBadChanBlocks=char(int(stringToDouble(tempStr)));  
	       			break;
	      		}
			case 59:
				break;
			case 60:      		
	      		{
	      			parseManFlagList(tempStr);        		
	       			break;
	      		}
	      			      		
	      		default:
	      		{
				CompleteFlag = 1;
				break;
			}        	
		}
	    	k++;
	    	if(CompleteFlag == 1)
	      		break;
	}
	//Manually filling obsolete options (removed from .in file)
	smoothFlagWindowLength=-1;
	refFrequency=0;
	flagOrder=1;
	chanFlagAlgo=2;
	doMultiPointFilter=0;
	if(flagOrder==2 && doUseNormalizedData) //Channel filtering cannot be done after time filtering if data is normalized
	{
		cout<<"Error in line of gptool.in"<<endl;
		cout<<"Channel filtering cannot be done after time filtering if data is normalized."<<endl;
		cout<<"Recommended option is to do channel filtering first."<<endl;
		exit(0);
	}
	
//	calculateCutoff();	
	errorChecks();
}
void Information::parseManFlagList(std::string& s)
{
	int p=0;
	int i=0;
	int p1,p2,p3;
	int slen=s.length();
	badChanBlocks=new int[nBadChanBlocks*2];
	while(p<slen)
	{
		if(i>=nBadChanBlocks)
		{
			cout<<"Error in line 60 of gptool.in"<<endl;
			cout<<"Expected exactly "<<nBadChanBlocks<<" list of bad sub-bands"<<endl;
			exit(0);
		}
		if(s[p]=='[')
			p1=p;
		else
		{
			cout<<"Error in line 60 of gptool.in"<<endl;
			cout<<"Expected [ to mark the start of a bad sub-band"<<endl;
			exit(0);
		}
		while(s[++p]!=',' && s[p]!='[' && s[p]!=']' && p<slen);
		if(s[p]==',')
			p2=p;
		else
		{
			cout<<"Error in line 60 of gptool.in"<<endl;
			cout<<"Expected ,"<<endl;
			exit(0);
		}
		while(s[++p]!=']' && p<slen);
		if(s[p]==']')
			p3=p;
		else
		{
			cout<<"Error in line 60 of gptool.in"<<endl;
			cout<<"Expected ] to mark the start of a bad sub-band"<<endl;
			exit(0);
		}
		if(++p<slen && s[p]!=',')
		{
			cout<<"Error in line 60 of gptool.in"<<endl;
			cout<<"Expected , between list of sub-bands"<<endl;
			exit(0);
		}
		p++;
		badChanBlocks[i*2]=int(stringToDouble(s.substr(p1+1,p2-p1-1)));
		badChanBlocks[i*2+1]=int(stringToDouble(s.substr(p2+1,p3-p2-1)));
		i++;
		
	}
	if(i<nBadChanBlocks)
	{
		cout<<"Error in line 60 of gptool.in"<<endl;
		cout<<"Expected exactly "<<nBadChanBlocks<<" list of bad sub-bands"<<endl;
		exit(0);
	}
}
void Information::fillParams()
{	
	if(doRunFilteredMode)	//rerun of .gpt output file. No filtering/
	{
		doChanFlag=0;
		doTimeFlag=0;
		bandshapeToUse=1;
		doUseNormalizedData=0;
		doReplaceByMean=0;
		doWriteFiltered2D=0;
		
	}
	if(!doFilteringOnly)
	{
		if(!doFixedPeriodFolding)	//Perform actions to use polyco based folding
		{
			genMJDObs();
			if(doUseTempo2)
				genPolycoTempo2();
			else
	 			genPolycoTempo1();
	 		loadPolyco();
	 		polycoRowIndex= (int)(((MJDObs-polycoTable[0])*1440)+spanPolyco/2.0)/spanPolyco;
			double freq=polycoTable[polycoRowIndex*(3+nCoeffPolyco)+2]*60.0;
			double dt=(MJDObs-polycoTable[polycoRowIndex*(3+nCoeffPolyco)])*1440;
			/***********************************************************************************************
			*The following while loop is due to an ill understood issue with the polyco.dat file (generated by
			*tempo 1) where a particular set(s) in the file has a dataspan less than that specified by spanPolyco
			*(usually half of it, for example if spanPolyco is 60mins, there maybe a 30mins set in the file. The
			*reason is not yet understood (8th May '16)
			************************************************************************************************/
			while(dt>spanPolyco/2.0)
			{
				polycoRowIndex++;
				dt=(MJDObs-polycoTable[polycoRowIndex*(3+nCoeffPolyco)])*1440;
			}
			/***********************************************************************************************
			*The next loop is for the yet unecountered case of set(s) having more span than spanPolyco
			************************************************************************************************/
			while(dt<-spanPolyco/2.0)
			{
				polycoRowIndex--;
				dt=(MJDObs-polycoTable[polycoRowIndex*(3+nCoeffPolyco)])*1440;
			}
			for(int i=1;i<nCoeffPolyco;i++)  	
	    			freq += i*polycoTable[polycoRowIndex*(3+nCoeffPolyco)+3+i] * pow(dt,i-1);
	 		periodInMs = (1000.0*60.0)/freq;
	 	} 	

		if(periodInSamples==-1)
			periodInSamples=(int)(periodInMs/(samplingInterval*1000.0)+0.5);	//Calculating the period in number of samples
		if(dispersionMeasure<0) //Get DM from psrcat or polyco.dat
			autoDM();
	}

	if(blockSizeSec==0)						//Setting blocksize equal to pulsar period
		blockSizeSec=periodInMs/1000.0;
	blockSizeSamples=blockSizeSec/samplingInterval;
	startBlockIndex=floor(startTime/blockSizeSec);
	if(!doFilteringOnly)
		writeInfFile();
	if(doReadFromFile)
	{
		string temp=filepath;
		int slashpos=temp.find_last_of("/");
		filename=temp.substr(slashpos+1);
	}
	if(doWriteFiltered2D)
	{
		if(doReadFromFile)
		{
			stringstream hdrCpyCommand;
			if(outputfilepath.empty())
				outputfilepath=filepath;
			else
			{
				stringstream filenameStream;
				filenameStream<<outputfilepath<<"/"<<filename;	
				outputfilepath=filenameStream.str().c_str();
			}
			hdrCpyCommand<<"cp "<<filepath<<".hdr "<<outputfilepath<<".gpt.hdr"<<endl;
			system(hdrCpyCommand.str().c_str());
		}
		else
		{
			stringstream filenameStream;
			filenameStream<<outputfilepath;	
			outputfilepath=filenameStream.str().c_str();
		}
	}
}
void Information::errorChecks()
{
	char erFlag=0;
	if(doReadFromFile==1)
	{
		ifstream testExistance;
		testExistance.open(filepath);
		if(!testExistance.is_open())
  		{
    			cout<<"No file with name: "<< filepath<<endl;
    			erFlag=1;
 		}
	}
	if(doPolarMode != 0 && doPolarMode != 1)
	{
		cout<<"Error in line 5 of gptool.in:"<<endl<<"Polarization mode must be either 0 for for Intensity or 1 for full stokes"<<endl;
		erFlag=1;
	}
	if(doPolarMode && polarChanToDisplay>3)
	{
		cout<<"Error in line 28 of gptool.in:"<<endl;
		cout<<"Polarization channel to display must be between 0 and 3 OR -1 to display all four channels"<<endl;
		erFlag=1;
	}
	if(sampleSizeBytes !=1 && sampleSizeBytes !=2 && sampleSizeBytes != 4)
	{
		cout<<"Error in line 6 of gptool.in:"<<endl<<"The tool can only process 1 or 2 byte integer or 4 byte float."<<endl;
		erFlag=1;
	}
	if(sidebandFlag !=0 && sidebandFlag != 1)
	{
		cout<<"Error in line 11 of gptool.in:"<<endl<<"-1 for decreasing channel ordering, +1 for increasing."<<endl;
		erFlag=1;
	}
	if(!doFixedPeriodFolding || dispersionMeasure<0)
	{
		getPsrcatdbPath();
		checkPulsarName();
	}
	if(periodInSamples==0)
	{
		cout<<"Error in 21 of gptool.in:"<<endl<<"Number of bins in folded profile must be positive definite."<<endl;
		erFlag=1;
	}
	if(profileOffset >1 || profileOffset<0)
	{
		cout<<"Error in 22 of gptool.in:"<<endl<<"Pulsar phase offset must be between 0 and 1"<<endl;
		erFlag=1;
	}
	if(doManualMode !=0 && doManualMode !=1)
	{
		cout<<"Error in line 30 of  gptool.in:"<<endl<<"0 for automatic update and 1 for manual update."<<endl;
		erFlag=1;
	}
	if(startChannel>noOfChannels)
	{
		cout<<"Error in line 34 of  gptool.in:"<<endl<<"Channels to skip cannot be greater than number of channels"<<endl;
		erFlag=1;
	}
	
	if(stopChannel<startChannel)
	{
		cout<<"Error in line 35 of  gptool.in:"<<endl<<"The sum of channels to skip in the beginning and end exceeds the number of channels"<<endl;
		erFlag=1;
	}
	
	
	if(doChanFlag !=0 && doChanFlag !=1)
	{
		cout<<"Error in line 36 of  gptool.in:"<<endl<<"0 for no frequency domain filtering and 1 for filtering on."<<endl;
		erFlag=1;
	}
	
	if(bandshapeToUse !=2 && bandshapeToUse !=3 && bandshapeToUse !=4)
	{
		cout<<"Error in line 37 of  gptool.in:"<<endl<<"1 for normalized bandshape; 2 for mean to rms bandshape and 3 for both."<<endl;
		erFlag=1;
	}
	
	if(chanCutOffToRMS <=0)
	{
		cout<<"Error in line 38 of  gptool.in:"<<endl<<"Cutoff must be positive."<<endl;
		erFlag=1;
	}
	
	if(doWriteChanFlags!=0 && doWriteChanFlags!=1)
	{
		cout<<"Error in line 52 of  gptool.in:"<<endl<<"1 to write channel flags, 0 otherwise."<<endl;
		erFlag=1;
	}
	
	if(doTimeFlag !=0 && doTimeFlag !=1)
	{
		cout<<"Error in line 41 of  gptool.in:"<<endl<<"0 for no time domain filtering and 1 for filtering on."<<endl;
		erFlag=1;
	}
	
	
	if(doUseNormalizedData!=0 && doUseNormalizedData!=1)
	{
		cout<<"Error in line 42 of  gptool.in:"<<endl<<"1 to normalize data, 0 otherwise."<<endl;
		erFlag=1;
	}
	
	if(timeFlagAlgo!=2 && timeFlagAlgo!=1)
	{
		cout<<"Error in line 43 of  gptool.in:"<<endl<<"1 to use histogram based, 2 to use MAD basesd."<<endl;
		erFlag=1;
	}
	
	if(timeCutOffToRMS <=0)
	{
		cout<<"Error in line 44 of  gptool.in:"<<endl<<"Cutoff must be positive."<<endl;
		erFlag=1;
	}
	
	if(doWriteTimeFlags!=0 && doWriteTimeFlags!=1)
	{
		cout<<"Error in line 53 of  gptool.in:"<<endl<<"1 to write time flags, 0 otherwise."<<endl;
		erFlag=1;
	}
	
	if((doUseNormalizedData==1 || (doChanFlag==1 && bandshapeToUse==2)) && normalizationProcedure ==1 && (smoothingWindowLength<=0 || smoothingWindowLength>=noOfChannels))
	{
		cout<<"Error in line 47 of  gptool.in:"<<endl<<"Smooth window length must be positive and less than the number of channels."<<endl;
		erFlag=1;
	}
	
	if(normalizationProcedure!=2 && normalizationProcedure!=1)
	{
		cout<<"Error in line 48 of  gptool.in:"<<endl<<"1 to use cumulative smooth bandshape, 2 to use externally supplied bandshape.dat."<<endl;
		erFlag=1;
	}
	if(doReplaceByMean!=0 && doReplaceByMean!=1 &&  doReplaceByMean!=2)
	{
		cout<<"Error in line 49 of  gptool.in:"<<endl<<"0 to replace by zeros, 1 to replace flagged points by modal (median) value, 2 to replace by smooth bandshape"<<endl;
		erFlag=1;
	}
	if(doReplaceByMean==1 && doUseNormalizedData!=1)
	{
		cout<<"Invalid combination of choices in line 42 and 49."<<endl<<"Data must be normalized to replace by modal (median) values.";
		erFlag=1;
	}
	if(doReplaceByMean==1 && doTimeFlag!=1)
	{
		cout<<"Invalid combination of choices in line 41 and 49."<<endl<<"Time filtering must be on to replace by modal (median) values.";
		erFlag=1;
	}
	if(normalizationProcedure==2)
	{
		ifstream testExistance;
		testExistance.open("bandshape.dat");
		if(!testExistance.is_open())
  		{
    			cout<<"No bandshape.dat file found."<<endl;
    			erFlag=1;
 		}
	}
	if(doWriteFullDM!=0 && doWriteFullDM!=1)
	{
		cout<<"Error in line 55 of  gptool.in:"<<endl<<"1 to write dedispersed time series, 0 otherwise."<<endl;
		erFlag=1;
	}
	if(doWriteFiltered2D!=0 && doWriteFiltered2D!=1)
	{
		cout<<"Error in line 54 of  gptool.in:"<<endl<<"1 to write 2D time-frequency data, 0 otherwise."<<endl;
		erFlag=1;
	}
	if(nBadChanBlocks!=0)
	{
		for(int i=0;i<nBadChanBlocks;i++)	
		{
			if(badChanBlocks[i*2]<0 || badChanBlocks[i*2+1]<0 )
			{
				cout<<"Error in line 60 of gptool.in"<<endl;
				cout<<"In sub-band "<<i+1<<": channel number cannot be less than zero."<<endl;
				erFlag=1;
			}
			if(badChanBlocks[i*2]>noOfChannels || badChanBlocks[i*2+1]>noOfChannels )
			{
				cout<<"Error in line 60 of gptool.in"<<endl;
				cout<<"In sub-band "<<i+1<<": channel number cannot be greater than the number of channels."<<endl;
				erFlag=1;
			}
			if(badChanBlocks[i*2]>=badChanBlocks[i*2+1])
			{
				cout<<"Error in line 60 of gptool.in"<<endl;
				cout<<"In sub-band "<<i+1<<": end channel must be strictly greater than start channel."<<endl;
				erFlag=1;
			}
		}			

	}
	if(erFlag==1)
	{
		exit(0);
	}	
	
}
void Information::calculateCutoff()
{
	float table[91][5]={{1., 0.559349, 0.39004, 0.299751, 0.243506}, {1.1, 0.62054, 0.434484,
   0.334715, 0.272346}, {1.2, 0.68246, 0.479754, 0.370477, 
  0.301926}, {1.3, 0.745061, 0.525809, 0.407005, 0.332225}, {1.4, 
  0.808296, 0.57261, 0.444271, 0.363221}, {1.5, 0.872124, 0.62012, 
  0.482245, 0.39489}, {1.6, 0.936504, 0.668302, 0.520897, 
  0.427211}, {1.7, 1.0014, 0.717119, 0.560199, 0.460161}, {1.8, 
  1.06677, 0.766538, 0.600123, 0.493716}, {1.9, 1.13259, 0.816526, 
  0.64064, 0.527854}, {2., 1.19882, 0.867051, 0.681723, 
  0.562553}, {2.1, 1.26543, 0.918084, 0.723347, 0.59779}, {2.2, 
  1.33241, 0.969594, 0.765486, 0.633543}, {2.3, 1.39971, 1.02156, 
  0.808114, 0.669793}, {2.4, 1.46733, 1.07394, 0.851208, 
  0.706516}, {2.5, 1.53523, 1.12673, 0.894744, 0.743694}, {2.6, 
  1.6034, 1.17989, 0.938702, 0.781306}, {2.7, 1.67182, 1.23341, 
  0.983059, 0.819334}, {2.8, 1.74046, 1.28725, 1.02779, 
  0.857758}, {2.9, 1.80933, 1.34142, 1.07289, 0.896561}, {3., 1.87839,
   1.39587, 1.11832, 0.935725}, {3.1, 1.94763, 1.45061, 1.16408, 
  0.975234}, {3.2, 2.01705, 1.5056, 1.21015, 1.01507}, {3.3, 2.08662, 
  1.56084, 1.2565, 1.05522}, {3.4, 2.15635, 1.61631, 1.30313, 
  1.09567}, {3.5, 2.22621, 1.67199, 1.35002, 1.1364}, {3.6, 2.2962, 
  1.72787, 1.39715, 1.1774}, {3.7, 2.36631, 1.78395, 1.44451, 
  1.21866}, {3.8, 2.43653, 1.8402, 1.4921, 1.26016}, {3.9, 2.50685, 
  1.89662, 1.53989, 1.3019}, {4., 2.57727, 1.9532, 1.58788, 
  1.34386}, {4.1, 2.64777, 2.00992, 1.63605, 1.38602}, {4.2, 2.71836, 
  2.06679, 1.6844, 1.42839}, {4.3, 2.78902, 2.12378, 1.73292, 
  1.47094}, {4.4, 2.85975, 2.18089, 1.78159, 1.51368}, {4.5, 2.93055, 
  2.23812, 1.83041, 1.55659}, {4.6, 3.00141, 2.29545, 1.87936, 
  1.59965}, {4.7, 3.07233, 2.35289, 1.92845, 1.64288}, {4.8, 3.1433, 
  2.41042, 1.97767, 1.68625}, {4.9, 3.21432, 2.46804, 2.027, 
  1.72975}, {5., 3.28538, 2.52573, 2.07644, 1.77339}, {5.1, 3.35648, 
  2.58351, 2.12599, 1.81716}, {5.2, 3.42763, 2.64136, 2.17564, 
  1.86104}, {5.3, 3.4988, 2.69928, 2.22537, 1.90504}, {5.4, 3.57001, 
  2.75726, 2.2752, 1.94914}, {5.5, 3.64126, 2.8153, 2.32511, 
  1.99334}, {5.6, 3.71253, 2.8734, 2.3751, 2.03764}, {5.7, 3.78382, 
  2.93154, 2.42516, 2.08203}, {5.8, 3.85514, 2.98974, 2.47529, 
  2.12651}, {5.9, 3.92648, 3.04798, 2.52549, 2.17106}, {6., 3.99784, 
  3.10627, 2.57575, 2.2157}, {6.1, 4.06923, 3.16459, 2.62606, 
  2.26041}, {6.2, 4.14062, 3.22296, 2.67644, 2.30518}, {6.3, 4.21204, 
  3.28135, 2.72686, 2.35003}, {6.4, 4.28346, 3.33978, 2.77733, 
  2.39493}, {6.5, 4.3549, 3.39824, 2.82785, 2.4399}, {6.6, 4.42635, 
  3.45673, 2.87841, 2.48492}, {6.7, 4.49782, 3.51524, 2.92901, 
  2.52999}, {6.8, 4.56929, 3.57378, 2.97964, 2.57511}, {6.9, 4.64077, 
  3.63233, 3.03032, 2.62028}, {7., 4.71226, 3.69091, 3.08103, 
  2.6655}, {7.1, 4.78375, 3.74951, 3.13176, 2.71076}, {7.2, 4.85525, 
  3.80813, 3.18253, 2.75605}, {7.3, 4.92676, 3.86676, 3.23333, 
  2.80139}, {7.4, 4.99827, 3.92541, 3.28415, 2.84676}, {7.5, 5.06978, 
  3.98408, 3.33499, 2.89216}, {7.6, 5.1413, 4.04275, 3.38586, 
  2.9376}, {7.7, 5.21282, 4.10144, 3.43675, 2.98306}, {7.8, 5.28435, 
  4.16014, 3.48766, 3.02856}, {7.9, 5.35587, 4.21884, 3.53859, 
  3.07408}, {8., 5.4274, 4.27756, 3.58953, 3.11962}, {8.1, 5.49893, 
  4.33628, 3.64049, 3.16519}, {8.2, 5.57046, 4.39502, 3.69147, 
  3.21078}, {8.3, 5.64198, 4.45375, 3.74246, 3.25639}, {8.4, 5.71351, 
  4.5125, 3.79346, 3.30202}, {8.5, 5.78504, 4.57125, 3.84447, 
  3.34767}, {8.6, 5.85657, 4.63, 3.89549, 3.39334}, {8.7, 5.92809, 
  4.68876, 3.94653, 3.43902}, {8.8, 5.99962, 4.74752, 3.99757, 
  3.48472}, {8.9, 6.07114, 4.80628, 4.04862, 3.53043}, {9., 6.14266, 
  4.86504, 4.09968, 3.57615}, {9.1, 6.21418, 4.92381, 4.15075, 
  3.62189}, {9.2, 6.2857, 4.98258, 4.20182, 3.66764}, {9.3, 6.35721, 
  5.04135, 4.2529, 3.7134}, {9.4, 6.42872, 5.10012, 4.30398, 
  3.75916}, {9.5, 6.50023, 5.15889, 4.35507, 3.80494}, {9.6, 6.57174, 
  5.21766, 4.40616, 3.85073}, {9.7, 6.64324, 5.27643, 4.45725, 
  3.89652}, {9.8, 6.71474, 5.3352, 4.50835, 3.94232}, {9.9, 6.78624, 
  5.39397, 4.55945, 3.98813}, {10., 6.85773, 5.45273, 4.61055, 
  4.03394}};
  
  cutoff=new float[5];
  cutoff[0]=timeCutOffToRMS;
  
  int i1,i2;
  i1=floor((cutoff[0]-1)/0.1);
  i2=ceil((cutoff[0]-1)/0.1);
  
  for(int j=1;j<=4;j++)
    	cutoff[j]=table[i1][j]+((table[i2][j]-table[i1][j])/0.1)*(cutoff[0]-table[i1][0]);
  
  
}
/*******************************************************************
*FUNCTION: void Information::genMJDObs()
*Generates Modified Julian Day (MJD) at the start of observation.
*In case of offline data it reads relevent parameters from the header
*file and in case of online data it converts the system time to MJD.
*******************************************************************/
void Information::genMJDObs()
{
	int YYYY,MM,DD,HH,mm,SS,ss;
  	stringstream convertTime;
  	long int nanoseconds;
  	double sec;
  	string command,linehdr;
	ostringstream  headerName;
  	if(doReadFromFile==1) //Read from file
    		headerName<<filepath<<".hdr";
  	else //Real Time
  		headerName<<"timestamp.gpt";
    	ifstream headerFile(headerName.str().c_str(),ios::in);
    	if(!headerFile.is_open())
    	{
      		cout<<headerName<<":header file not found!"<<endl;
      		exit(1);
    	}

    	//Extract date and time information and check for sanity
    	getline(headerFile,linehdr); //Ignore first line
    	getline(headerFile,linehdr); //Get the second line and process it
    	sscanf(linehdr.c_str(),"%*s %*s %d:%d:%lf",&HH,&mm,&sec);
    	getline(headerFile,linehdr);
    	sscanf(linehdr.c_str(),"%*s %d:%d:%d",&DD,&MM,&YYYY);
    	headerFile.close();

    	// Checking the sensibility of the IST Date and time Acquired:
    	if(YYYY < 0 || MM < 1 || MM > 12 || DD < 1 || DD > 31 || HH < 0 || HH > 23 || mm < 0 || mm > 59 || sec < 0.0 || sec >= 60.0 )
    	{
      		cout<<"\n\nERROR ! Awkward or invalid format of IST time and Date in the raw.hdr file.\n";
      		cout<<"\t'raw.hdr' file should STRICTLY contain the IST time and IST date of Observation in this format\n";
      		cout<<"#Start time and date\n";
      		cout<<"IST Time: HH:mm:SS.ssss\n";
      		cout<<"Date: DD:MM:YYYY\n\n";
      		exit(1);
    	}

    	if( (MM==4 || MM==6 || MM==9|| MM==11) && DD>30)
    	{
      		cout<<"\n\nERROR ! Given Month can not have Days > 30.\n";
      		cout<<"\t'raw.hdr' file should STRICTLY contain the IST time and IST date of Observation in this format\n";
      		cout<<"#Start time and date\n";
     		cout<<"IST Time: HH:mm:SS.ssss\n";
      		cout<<"Date: DD:MM:YYYY\n\n";
      		exit(1);
    	}

    	// The following line is to check for whether current year is leap year 
    	// The logic is if(Div_by_4 And (Not_div_by_100 Or Div_by_400)) then Leap year else not.
    	if((YYYY % 4 == 0) && ((YYYY % 100 != 0) || (YYYY % 400 == 0)))
    	{
      		if(MM==2 && DD>29)
      		{
        		cout<<"\n\nERROR ! In the file raw.hdr : Leap-year Month of Feb can not have Days > 29.\n";
        		exit(1);
      		}
    	}
	else
    	if(MM==2 && DD >28)
    	{
     		cout<<"\n\nERROR ! In the file raw.hdr : In non-Leap-year Month of Feb can not have Days > 28.\n";
		exit(1);
    	}

    		
    
    	//Convert IST to UTC
    	convertTime.str("");
    	convertTime<<"TZ=\"UTC\" date \"+%d %m %Y %H %M %S %N\" -d \""<<YYYY<<"-"<<MM<<"-"<<DD<<" "<<HH<<":"<<mm<<":"<<setprecision(12)<<sec<<" IST\""<<"> UTC"<<endl;
    	system(convertTime.str().c_str());
  	

  	//Read UTC File and Get UTC Time
  	ifstream UTCfile("UTC",ios::in);
  	UTCfile>>DD>>MM>>YYYY>>HH>>mm>>SS>>nanoseconds;
  	sec = SS+((double)nanoseconds/1000000000.0);
  	UTCfile.close();
  	system("rm -rf UTC");
  	//Calculate the MJD
  	stringstream calenderToMJD;
  	calenderToMJD<<"cal2mjd "<<YYYY<<" "<<MM<<" "<<DD<<" "<<HH<<" "<<mm<<" "<<setprecision(12)<<sec<<" >calenderToMJDoutput";
  	system(calenderToMJD.str().c_str());
  	ifstream mjd("calenderToMJDoutput",ios::in);
  	string mjdTemp;
  	getline(mjd,mjdTemp);
  	getline(mjd,mjdTemp);
  	sscanf(mjdTemp.c_str(),"%*s %*s %lf",&MJDObs);
  	system("rm -rf calenderToMJDoutput");
  	mjd.close();
}
/*******************************************************************
*FUNCTION: void Information::getPsrcatdbPath()
*Locates the psrcat database file.
*******************************************************************/
void Information::getPsrcatdbPath()
{
	stringstream checkPsrcatCommand;
	checkPsrcatCommand<<"echo $PSRCAT_FILE | awk 'FNR ==1 {print}' > dbpath";
	system(checkPsrcatCommand.str().c_str());
	
	ifstream* path= new ifstream("dbpath",ios::in);
	path->seekg(0,ios::end);
	size_t eof=path->tellg();
	if((int)eof<=2)
	{	
		path->close();
		system("rm -rf dbpath");
		
		stringstream dbPathCommand;
	    	dbPathCommand<<"locate \"psrcat.db\" | awk 'FNR == 1 {print}' > dbpath";
	    	system(dbPathCommand.str().c_str());
		path=new ifstream("dbpath",ios::in);
		path->seekg(0,ios::end);
		eof=path->tellg();
		if((int)eof==0)
		{
			cout<<"No psrcat.db found!"<<endl;
			exit(0);
		}
	}
	psrcatdbPath=new char[(int)eof];
	path->seekg(0,ios::beg);
	path->getline (psrcatdbPath,eof);
	path->close();
	system("rm -rf dbpath");
		
}
/*******************************************************************
*FUNCTION: void Information::checkPulsarName()
*Verify pulsar name with psr cat and see if its valid. 
*In case B name is specified this routine also gets the correspoding
*J name.
*******************************************************************/
void Information::checkPulsarName()
{
	//Generate JName and check if source name is valid
    	stringstream JNameGen;
    	JNameGen<<"psrcat -db_file "<<psrcatdbPath<<" -e "<<pulsarName<<" | grep PSRJ | awk '{print $2}' > JNamePulsar";
    	system(JNameGen.str().c_str());
    	ifstream JName("JNamePulsar",ios::in);
    	JName.seekg(0,ios::end);
    	if(int(JName.tellg())==0)
    	{
        	cout<<"Error in line 14 of gptool.in:"<<endl;
        	cout<<"Incorrect pulsar name."<<endl;
        	exit(1);
    	}
    	JName.seekg(0,ios::beg);
    	JName>>pulsarName;    	
    	JName.close();
    	system("rm -rf JNamePulsar");
}
/*******************************************************************
*FUNCTION: void Information::autoDM()
*Gets DM value from psrcat in case its not specified by the user
*******************************************************************/
void Information::autoDM()
{	
  	stringstream DMGet;
  	if(!doFixedPeriodFolding)
  	{
		ifstream *polyco;
		if(!doUseTempo2)
  			polyco=new ifstream("polyco.dat",ios::in);
		else
			polyco=new ifstream("polyco_new.dat",ios::in);
    		string temp;
    		*polyco>>temp>>temp>>temp>>temp>>dispersionMeasure;
		delete polyco;
    	}
    	else
    	{
    		DMGet<<"psrcat -db_file "<<psrcatdbPath<<" -e "<<pulsarName<<" | grep DM | awk '{print $2}' > DMDMDM";
    		system(DMGet.str().c_str());  	
  		ifstream DMGet("DMDMDM",ios::in);
  		DMGet>>dispersionMeasure;
  		DMGet.close();
  		system("rm -rf DMDMDM");
  	}
}
/*******************************************************************
*FUNCTION: void Information::genPolycoTempo1()
*Fires tempo1 to generate polyco.dat file
*Refer to tempo manual for format of the input file tz.in
*******************************************************************/
void Information::genPolycoTempo1()
{
	//Generate tz.in
	ofstream tz("tz.in",ios::out);
	tz<<"r 12 60 12 325\n\n\n";
	string tempString = pulsarName.substr(1);
	float freqTz;
	if(sidebandFlag == 0)
        	freqTz = (lowestFrequency+bandwidth-(bandwidth/noOfChannels)/2.0);
    	else 
      		freqTz = (lowestFrequency+(bandwidth/noOfChannels)/2.0);
    	tz<<tempString<<setprecision(12)<<" "<<spanPolyco<<" "<<nCoeffPolyco<<" "<<maxHA<<" "<<freqTz<<endl;
   	tz.close();
    	//gets parameter file from psr catalogue
    	stringstream parCommand;
	parCommand<<"psrcat -db_file "<<psrcatdbPath<<" -e "<<pulsarName<<" > parfile_used";
    	system(parCommand.str().c_str());
    	//fire tempo to get polycos    	
    	float MJD1=MJDObs-1.0;
    	float MJD2=MJDObs+1.0;
        stringstream tempoCommand;
        //tempoCommand<<"echo "<<MJD1<<" "<<MJD2<<"| tempo -z -f parfile_used > /dev/null";
	tempoCommand<<"echo "<<setprecision(12)<<MJD1<<" "<<setprecision(12)<<MJD2<<"| tempo -z -f parfile_used> /dev/null";
	system(tempoCommand.str().c_str());   
	fstream *fin;
	if(!doUseTempo2)
		fin=new fstream("polyco.dat", fstream::in);
	else
		fin=new fstream("polyco_new.dat", fstream::in);
	
	fin->seekg(0, ios_base::end);
	if(fin->tellg()==0)
	{		
		tempoCommand.str("");
		tempoCommand.clear();
		tempoCommand<<"echo "<<setprecision(12)<<MJD1<<" "<<setprecision(12)<<MJD2<<"| tempo -z> /dev/null";
		system(tempoCommand.str().c_str());   
	}  
	delete fin;
}

/*******************************************************************
*FUNCTION: void Information::genPolycoTempo2()
*Fires tempo2 to generate polyco.dat file
*Refer to tempo manual for format of the input file tz.in
*******************************************************************/
void Information::genPolycoTempo2()
{
	
	string tempString = pulsarName.substr(1);
	float freqTz;
	if(sidebandFlag == 0)
        freqTz = (lowestFrequency+bandwidth-(bandwidth/noOfChannels)/2.0);
    else 
      	freqTz = (lowestFrequency+(bandwidth/noOfChannels)/2.0);
 
    //gets parameter file from psr catalogue
    stringstream parCommand;
	parCommand<<"psrcat -db_file "<<psrcatdbPath<<" -e "<<pulsarName<<" >"<<pulsarName<<".par";
    system(parCommand.str().c_str());
    //fire tempo to get polycos    	
    float MJD1=MJDObs-1.0;
    float MJD2=MJDObs+1.0;
    stringstream tempoCommand;
	tempoCommand<<"tempo2 -f "<<pulsarName<<".par"<<" -polyco \""<<MJD1<<" "<<MJD2<<" "<<spanPolyco<<" "<<nCoeffPolyco<<" "<<maxHA<<" gmrt "<<freqTz<<"\" -tempo1> /dev/null";
	system(tempoCommand.str().c_str());   
	/**ifstream fin("polyco.dat", fstream::in);
	fin.seekg(0, ios_base::end);
	if(fin.tellg()==0)
	{		
		tempoCommand.str("");
		tempoCommand.clear();
		tempoCommand<<"echo "<<setprecision(12)<<MJD1<<" "<<setprecision(12)<<MJD2<<"| tempo -z> /dev/null";
		system(tempoCommand.str().c_str());   
	}  **/
}


/*******************************************************************
*FUNCTION: void Information::loadPolyco()
*Loads relevent parameters from polyco.dat file to an array
*Refer to tempo manual for format of polyco.dat
*******************************************************************/
/*Polyco array structure
1 row => 1 set
polyco[row][0] => MJD (line 1)
           [1] => RPHASE (line 2)
           [2] => Ref rotation frequency (F0) (line 2)
           [>3] => polycos (3 per line from line 3)*/
void Information::loadPolyco()
{
	string line;    
	ifstream *polyco;
	if(!doUseTempo2)
  		polyco=new ifstream("polyco.dat",ios::in);
	else
		polyco=new ifstream("polyco_new.dat",ios::in);

    	int numberOfSets=ceil(72*60.0/spanPolyco); //3 MJD corresponds to 72 hours
    	polycoTable=new double[(3+nCoeffPolyco)*numberOfSets];
    	
    	double *ptrPolycoTable=polycoTable;
    	//Calculates the number of coefficient lines. Each line in polyco.dat contains upto 3 coefficients
    	int nCoeffLines=nCoeffPolyco/3;
    	if(nCoeffPolyco%3!=0)
    		nCoeffLines++;
    	
    	for(int i=0;i<numberOfSets;i++) 
    	{
        	getline(*polyco,line);        	
            	//Check the line and get information to load into polycos
            	sscanf(line.c_str()," %*s %*s %*s %lf",ptrPolycoTable++);	//MJD at the middle of current span
            	getline(*polyco,line);  
              	sscanf(line.c_str()," %lf %lf",(ptrPolycoTable),(ptrPolycoTable+1)); //RPHASE & REF ROTATION FREQ for the current span
		ptrPolycoTable+=2;
		//loading coefficients
              	int coeffLoaded=0;              	
                for(int j=1;j<=nCoeffLines;j++)
            	{
            		getline(*polyco,line);            		
              		if(nCoeffPolyco - coeffLoaded == 1)
              		{
                  		sscanf(line.c_str()," %le",ptrPolycoTable);
                  		coeffLoaded++;
                  		ptrPolycoTable++;
              		}
              		else if(nCoeffPolyco - coeffLoaded == 2)
              		{
                  		sscanf(line.c_str()," %le %le",ptrPolycoTable,ptrPolycoTable+1);
                  		ptrPolycoTable+=2;
                  		coeffLoaded+=2;
              		}
              		else
              		{
                 		sscanf(line.c_str()," %le %le %le ", ptrPolycoTable,ptrPolycoTable+1,ptrPolycoTable+2);
                  		ptrPolycoTable+=3;
                  		coeffLoaded += 3;
              		}
      		}
            
    	}    
    	polyco->close(); 
	delete polyco;
}

/*******************************************************************
*FUNCTION: void Information::display()
*Displays all run parameters to the terminal
*******************************************************************/
void Information::display()
{
	
	stringstream displays;
	displays<<endl<<"gptool v 4.2.1"<<endl;
	if(doFilteringOnly)
		displays<<"FILTERING ONLY MODE"<<endl<<endl;
	if(isInline)
		displays<<"INLINE MODE - read from and write to a shared memory"<<endl;
	displays<<"Mode of operation: "<<modeOperation<<endl;
	if(doPolarMode)
	{
		displays<<"Polarization mode ON"<<endl;
		if(polarChanToDisplay==-1)
			displays<<"All polarizations will be displayed"<<endl;
		else
			displays<<"Only polarization chan "<<int(polarChanToDisplay)<<" will be displayed"<<endl;
	}
	switch(sampleSizeBytes)
	{
		case 1:
			displays<<"Input data is sampled as 1 byte integers"<<endl;
			break;
		case 2:
			displays<<"Input data is sampled as 2 byte integers"<<endl;
			break;
		case 4:
			displays<<"Input data is sampled as 4 byte floating point numbers."<<endl;
			break;
	}
	
	displays<<endl<<"Lowest frequency :"<<lowestFrequency<<" MHz"<<endl;
	displays<<"Bandwidth: "<<bandwidth<<" MHz"<<endl;
	
	if(sidebandFlag)
		displays<<"Sideband flag: 1. Frequency of first channel is "<<lowestFrequency<<" MHz"<<endl;
	else
		displays<<"Sideband flag: -1. Frequency of first channel is "<<lowestFrequency+bandwidth<<" MHz"<<endl;
	displays<<"Number of channels: "<<noOfChannels<<endl;	
	displays<<"Sampling interval :"<<samplingInterval*1000.0<<" ms"<<endl;

	if(!doFilteringOnly)
	{
	
		displays<<endl<<"Pulsar: "<<pulsarName<<endl;
		
		displays<<"Pulsar period: "<<setprecision(12)<<periodInMs<<" ms"<<endl;
		displays<<"Dispersion measure: "<<setprecision(10)<<dispersionMeasure<<" pc/cc"<<endl;	
	
	
	
		/*if(refFrequency)
			cout<<endl<<"Dedispersion will be done with respect to the highest frequency."<<endl;
		else
			cout<<"Dedispersion will be done with respect to the lowest frequency."<<endl; */
		displays<<"Number of bins in folded profile: "<<periodInSamples<<endl;
		displays<<"Phase of pulsar profile will be offset by "<<profileOffset<<endl;
		if(!doFixedPeriodFolding)
		{
			if(doUseTempo2)
				displays<<"Polyco based folding to be performed using tempo2."<<endl;	
			else
				displays<<"Polyco based folding to be performed using tempo1."<<endl;	
			displays<<"MJD of observation is "<<setprecision(20)<<MJDObs<<endl;
			displays<<"Number of coefficients generated for each span: "<<nCoeffPolyco<<endl;
			displays<<"Validity of each span: "<<spanPolyco<<" mins"<<endl;
			displays<<"Maximum Hour Angle: "<<maxHA<<endl;
		}
		else
			displays<<"Fixed period folding to be performed."<<endl;
	}
	displays<<endl;
	if(isInline)
		displays<<"Display window size has been adjusted to accomodate integer multiples of shared memory buffer"<<endl;
	
	displays<<"Display window size :"<<blockSizeSec<<" sec"<<endl;
	if(doManualMode && doReadFromFile)
		displays<<"Window update will happen on key press."<<endl;
	if(doWindowDelay && doReadFromFile)
		displays<<"gptool will spend atleast "<<blockSizeSec<<" seconds on each window."<<endl;

	displays<<endl<<"Number of channels excluded at start: "<<startChannel<<endl;
	displays<<"Number of channels excluded at end: "<<noOfChannels-stopChannel<<endl;
	if(nBadChanBlocks!=0)
	{
		displays<<endl<<"Number of bad subbands to exclude: "<<nBadChanBlocks<<endl;
		for(int i=0;i<nBadChanBlocks;i++)				
			displays<<"Sub-band "<<i+1<<" : chan # "<<badChanBlocks[i*2]<<" to "<<badChanBlocks[i*2+1]<<endl;		
		
	}
	
	
	if(doChanFlag)
	{
		displays<<endl<<"Channel flagging turned on. Details of procedure: "<<endl;
		switch(bandshapeToUse)
		{
			case 1:
				displays<<"\tMean bandshape will be used to detect RFI."<<endl;
				break;
			case 2:
				displays<<"\tBandhshape will be smoothened and then normalized.\n\tNormalized bandshape will be used to detect RFI."<<endl;
				if(normalizationProcedure==1)
					displays<<"\t"<<smoothingWindowLength<<" channels will be used as window length for smoothing"<<endl;
				else
					displays<<"\t"<<"Data from bandshape.dat will be used to normalize"<<endl;
				break;
			case 3:
				displays<<"\tMean to rms bandshape will be used to detect RFI."<<endl;
				break;
			case 4:
				displays<<"\tMean to rms bandshape as well as normalized bandshape will be used to detect RFI."<<endl;
				break;
		}
		/*
		if(chanFlagAlgo==1)
			cout<<"\tHistogram based algorithm selected."<<endl;		
		else if(chanFlagAlgo==2)
			cout<<"\tMAD based algorithm selected."<<endl;	
		*/	
		displays<<"\tCutOff to RMS ratio: "<<chanCutOffToRMS<<endl;				
	}
	else
	{
		displays<<endl<<"No channel flagging will be performed."<<endl;
	}
	if(doTimeFlag)
	{
		displays<<endl<<"Time flagging turned on. Details of procedure: "<<endl;	
		if(doUseNormalizedData)
		{
			displays<<"\t2-D time-frequency data will be normalized using smoothened bandshape before filtering"<<endl;	
			if(normalizationProcedure==1)
				displays<<"\t"<<smoothingWindowLength<<" channels will be used as window length for smoothing"<<endl;
			else
				displays<<"\t"<<"Data from bandshape.dat will be used to normalize"<<endl;
		}
		if(timeFlagAlgo==1)
			displays<<"\tHistogram based algorithm selected."<<endl;
		else if(timeFlagAlgo==2)
			displays<<"\tMAD based algorithm selected."<<endl;
		
		if(doMultiPointFilter)
		{
			displays<<"\tMultipoint flagging ON. CutOff to rms : "<<endl;
			for(int j=0;j<3;j++)
				cout<<"\t"<<cutoff[j]<<" ";
			displays<<endl;
		}
		else
			displays<<"\tCutOff to RMS ratio: "<<timeCutOffToRMS<<endl; 
		displays<<"\tBad samples to be ignored."<<endl;		
		if(smoothFlagWindowLength>0)
		{
			displays<<"\tAny time block of length "<<smoothFlagWindowLength<<" seconds will be rejected if it has more than "<<concentrationThreshold<<" % of corrupted data"<<endl;
		}				
	}
	else
	{
		displays<<endl<<"No time flagging will be performed."<<endl;
	}

	if(doTimeFlag && doChanFlag)
	{	
		if(doReplaceByMean)
		{
			if(timeFlagAlgo==1)
				displays<<endl<<"Flagged samples will be replaced by modal value of zero DM time series.";
			else if(timeFlagAlgo==2)
				displays<<endl<<"Flagged samples will be replaced by median value of zero DM time series.";
		}
		else
			displays<<endl<<"Flagged samples will be ignored"<<endl;
	}
	if(doChanFlag && doWriteChanFlags)
		displays<<"Flag output to chanflag.gpt"<<endl;	
	if(doTimeFlag && doWriteTimeFlags)
		displays<<"Flag output to timeflag.gpt"<<endl;	
	if(doZeroDMSub==1)
		displays<<"Appropiately scaled version of the zero DM time series will be subtracted to minimize the rms of each spectral channel."<<endl;	
	if(doWriteFiltered2D)
	{
			displays<<endl<<"Filtered 2D raw data output to "<<outputfilepath<<".gpt"<<endl;
			displays<<endl<<"Filtered file will have a rescaled mean of "<<meanval<<endl;
	}
	/*if(doTimeFlag && doChanFlag)
	{
		if(flagOrder==1)
			cout<<endl<<"Channel filtering will be done first."<<endl;
		else if(flagOrder==2)
			cout<<"Time filtering will be done first."<<endl;
		else if(flagOrder==3)
			cout<<"Time & channel filtering will be done independent of each other."<<endl;
		cout<<endl;
	}*/
	
	
	if(doWriteFullDM)
		displays<<"Dedispersed time series will be written out"<<endl<<endl;

	if(doRunFilteredMode)
		displays<<endl<<"WARNING : gptool run in readback mode. Inputs in lines 36,41,42,48 & 54 were overridden."<<endl<<endl;
	if(!doReadFromFile)
		displays<<"Taking data from shared memory"<<endl;
	else
		displays<<"Raw file path: "<<filepath<<endl;
	cout<<displays.str().c_str();
	time_t now = time(0);   
  	 //convert now to string form
   	char* dt = ctime(&now);
	ofstream f("log.gpt",ios::app);
	f<<"Time of run: "<<dt<<endl;
	f<<displays.str().c_str()<<endl<<endl;
	f<<"--------------------------------------------"<<endl;
	
}
/*******************************************************************
*FUNCTION: void Information::e()
*Displays error message when no option is given while running wpmon.
*******************************************************************/
void Information::displayNoOptionsHelp()
{
	cout<<"gptool -f [filename] -r -shmID [shm_ID] -s [start_time_in_sec] -o [output_2d_filtered_file] -m [mean_value_of_2d_op] -tempo2 -nodedisp  -zsub -inline -gfilt"<<endl<<endl;
	cout<<"-f [filename] \t\t\t :Read from GMRT format file [filename]"<<endl;
	cout<<"-r  \t\t\t\t :Attach to shared memory"<<endl;
	cout<<"-shmID [shm_ID] \t\t :shm_ID = \t1 -> Standard correlator shm \n\t\t\t\t\t\t2-> File simulator shm \n\t\t\t\t\t\t3-> Inline gptool shm"<<endl; 
	cout<<"-s [start_time_in_sec] \t\t :start processing the file after skipping some time"<<endl;
	cout<<"-o [output_2d_filtered_file] \t :path to GMRT format filtered output file"<<endl;
	cout<<"-m [mean_value_of_2d_op] \t :mean value of output filtered file \n \t\t\t\t this is the value by which the gptool normalized data \n\t\t\t\t is scaled by before writing out filtered file"<<endl;
	cout<<"-tempo2 \t\t\t :use tempo2 instead of tempo1 to get polycos"<<endl;
	cout<<"-nodedisp \t\t\t :just perform RFI filtering without further processing"<<endl;
	cout<<"-zsub \t\t\t\t :optimized zero DM subtraction. \n\t\t\t\t Experimental feature, use with caution"<<endl;
	cout<<"-inline \t\t\t :inline mode of gptool, read from a shared memory \n\t\t\t\t write filtered output to another shared memory"<<endl;
	cout<<"-gfilt \t\t\t\t :turns off all filtering options, \n\t\t\t\t will over-ride gptool.in inputs"<<endl;
	
}

/*******************************************************************
*FUNCTION: void Information::generateInfFile()
*Writes a fullDM_filtered.inf to be used by presto to process the
*dedispersed series generated
*******************************************************************/
void Information::writeInfFile()
{
	//Getting RA and DEC from atnf psrcat
	string RA;
	string DEC;
	if(psrcatdbPath!=NULL)
	{
		stringstream RACommand;
	    	RACommand<<"psrcat -db_file "<<psrcatdbPath<<" -e "<<pulsarName<<" | grep RAJ | awk '{print $2}' > RAJ";
	    	system(RACommand.str().c_str());
	    	ifstream RAJ("RAJ",ios::in);    	
	    	RAJ>>RA;    	
	    	RAJ.close();
	    	system("rm -rf RAJ");
	
		stringstream DECCommand;
	    	DECCommand<<"psrcat -db_file "<<psrcatdbPath<<" -e "<<pulsarName<<" | grep DECJ | awk '{print $2}' > DECJ";
	    	system(DECCommand.str().c_str());
	    	ifstream DECJ("DECJ",ios::in);    	
	    	DECJ>>DEC;    	
	    	DECJ.close();
	    	system("rm -rf DECJ");
	}
	else
	{
		RA="";
		DEC="";
	}
	stringstream fileOutput;
	fileOutput<<"Telescope used\t\t\t\t= "<<"GMRT"<<endl;
 	fileOutput<<"Instrument used\t\t\t\t= "<<"GWB"<<endl;
	fileOutput<<"Object being observed\t\t\t= "<<&pulsarName[1]<<endl;
	fileOutput<<"J2000 Right Ascension (hh:mm:ss.ssss)\t= "<<RA<<endl;
	fileOutput<<"J2000 Declination     (dd:mm:ss.ssss)\t= "<<DEC<<endl;
	fileOutput<<"Data observed by\t\t\t= "<<"GMRT"<<endl;
 	fileOutput<<"Epoch of observation (MJD)\t\t= "<<setprecision(12)<<MJDObs<<endl;
	fileOutput<<"Barycentered?           (1=yes, 0=no)\t= "<<"0"<<endl;
	fileOutput<<"Number of bins in the time series\t= "<<endl;
	fileOutput<<"Width of each time series bin (sec)\t= "<<setprecision(12)<<samplingInterval<<endl;  
 	fileOutput<<"Any breaks in the data? (1=yes, 0=no)\t= "<<"0"<<endl;
	fileOutput<<"Type of observation (EM band)\t\t= "<<"Radio"<<endl;
	fileOutput<<"Beam diameter (arcsec)\t\t\t= "<<endl;
 	fileOutput<<"Dispersion measure (cm-3 pc)\t\t= "<<dispersionMeasure<<endl;
	fileOutput<<"Central freq of low channel (Mhz)\t= "<<lowestFrequency<<endl; 
 	fileOutput<<"Total bandwidth (Mhz)\t\t\t= "<<bandwidth<<endl;
 	fileOutput<<"Number of channels\t\t\t= "<<1<<endl;
	fileOutput<<"Channel bandwidth (Mhz)\t\t\t= "<<bandwidth/noOfChannels<<endl;
	fileOutput<<"Data analyzed by\t\t\t= "<<"gptool"<<endl;

	ofstream inFilteredFile("fullDM_filtered.inf",ios::out);
	inFilteredFile<<"Data file name without suffix\t\t= "<<"fullDM_filtered"<<endl;
	inFilteredFile<<fileOutput.str().c_str();
	inFilteredFile.close();

	ofstream inUnFilteredFile("fullDM_unfiltered.inf",ios::out);
	inUnFilteredFile<<"Data file name without suffix\t\t= "<<"fullDM_unfiltered"<<endl;
	inUnFilteredFile<<fileOutput.str().c_str();
	inUnFilteredFile.close();
}
/*******************************************************************
*FUNCTION: void Information::writeWpmonIn()
*Writes a sample gptool.in when it is not found.
*******************************************************************/
void Information::writeWpmonIn()
{
	ofstream inFile("gptool.in",ios::out);
	inFile<<"#*#*#gptool input file v2.0#*#*#"<<endl;
	inFile<<"-------------------------------------------------"<<endl;
	inFile<<"#****Mode of observation****#"<<endl;
	inFile<<"PA\t\t: Beam mode"<<endl;
	inFile<<"0\t\t: Polarization mode (0-> intesity, 1-> stokes data)"<<endl;
	inFile<<"2\t\t: Sample size of data (in bytes, usually 2)"<<endl;
	inFile<<"-------------------------------------------------"<<endl;
	inFile<<"#****Observation Paramaters****#"<<endl;
	inFile<<"1030\t\t: Frequency band (lowest value in Mhz)"<<endl;
	inFile<<"200\t\t: Bandwidth(in Mhz)"<<endl;
	inFile<<"-1\t\t: Sideband flag (-1-> decreasing +1-> increasing)"<<endl;
	inFile<<"2048\t\t: Number of channels"<<endl;
	inFile<<"1.31072\t\t: Sampling Interval (in ms)"<<endl;
	inFile<<"-------------------------------------------------"<<endl;
	inFile<<"#****Pulsar Parameters****#"<<endl;
	inFile<<"J1807-0847\t: Pulsar name"<<endl;
	inFile<<"-1\t\t: Pulsar period (in milliseconds)"<<endl;
	inFile<<"-1\t\t: DM (in pc/cc)"<<endl;
	inFile<<"-------------------------------------------------"<<endl;
	inFile<<"#****Dedispersion & Folding parameters****#"<<endl;
	inFile<<"-1\t\t: Number of bins in folded profile (-1 for native resolution)"<<endl;
	inFile<<"0\t\t: Phase offset for folding"<<endl;
	inFile<<"12\t\t: Number of coefficients for each polyco span (nCoeff)"<<endl;
	inFile<<"60\t\t: Validity of each span (nSpan in mins)"<<endl;
	inFile<<"12\t\t: Maximum hour angle"<<endl;
	inFile<<"-------------------------------------------------"<<endl;
	inFile<<"#****Display Parameters****#"<<endl;
	inFile<<"0\t\t: Polarization channel to display (0-3 or -1 for all four)"<<endl;
	inFile<<"1\t\t: Display window size (seconds, 0-> pulsar period)"<<endl;
	inFile<<"0\t\t: Update mode	    	(0-> automatic, 1-> manual)"<<endl;
	inFile<<"0\t\t: Time delay between window updates (0-> no delay, 1-> emulate real time)"<<endl;
	inFile<<"-------------------------------------------------"<<endl;
	inFile<<"#****Spectral line RFI mitigation options****#"<<endl;
	inFile<<"50\t\t: Number of channels to flag at band beginning"<<endl;
	inFile<<"50\t\t: Number of channels to flag at band end"<<endl;
	inFile<<"1\t\t: Frequency flagging options (0-> no flagging, 1-> real time calculation)"<<endl;
	inFile<<"1\t\t: Bandshape to use for frequency flagging (1-> normalized bandshape, 2-> mean-to-rms bandshape, 3-> Both)"<<endl;
	inFile<<"2.5\t\t: Threshold for frequency flagging (in units of RMS deviation)"<<endl;				
	inFile<<"-------------------------------------------------"<<endl;
	inFile<<"#****Time domain impulsive RFI mitigation options****#"<<endl;
	inFile<<"1\t\t: Time flagging options 	(0-> no flagging, 1-> real time calculation)"<<endl;
	inFile<<"1\t\t: Data normalization before filtering (0-> no, 1-> yes)"<<endl;
	inFile<<"1\t\t: Time flagging algorithm	(1-> histogram based, 2-> MAD based)"<<endl;
	inFile<<"3\t\t: Threshold for time flagging (in units of RMS deviation)"<<endl;
	inFile<<"-------------------------------------------------"<<endl;
	inFile<<"#****Other options****#"<<endl;
	inFile<<"20\t\t: Smoothing window size for bandshape normalization (in number of channels)"<<endl;
	inFile<<"1\t\t: Normalization procedure (1-> cumulative smooth bandshape, 2-> externally supplied bandshape.dat)"<<endl;
	inFile<<"0\t\t: Replace by median values (0-> Ignore flagged samples, 1-> Replace flagged samples by window median, 2-> Replace by smooth bandshape)"<<endl;
	inFile<<"-------------------------------------------------"<<endl;
	inFile<<"#****I/O options****#"<<endl;
	inFile<<"0\t\t: Write channel flag file (0-> no,1-> yes)"<<endl;	
	inFile<<"0\t\t: Write time flag file (0-> no, 1-> yes)"<<endl;
	inFile<<"0\t\t: write out filtered 2D raw data (0-> no, 1-> yes)"<<endl;
	inFile<<"0\t\t: Write out fullDM.raw	(0-> no, 1-> yes)"<<endl;
	inFile<<"-------------------------------------------------"<<endl;
	inFile<<"#****manual flagging options****#"<<endl;
	inFile<<"0\t\t: Number of bad channel blocks"<<endl;
	inFile<<"List\t\t: #in next line, example: [200,400],[1200,1400]"<<endl;
	inFile<<""<<endl;
}
//end of information class methods implementations


/*******************************************************************
*CLASS:	AquireData
*Contains functions to read data either from a file or SHM (in case 
*of real time operation). It also has a functions to split read data
*into four polarizations. 
*******************************************************************/
class AquireData
{
	public:
	//Static variables:	
	static	Information	info;
	static long int		eof;  			//Length of file in bytes
	static double		totalError;		//Cumulative uncorrected difference between true window width and required width
	static long int		curPos;			//Total number of samples read.
	//Static SHM Variables:
	static DasHdrType*	dataHdr;
	static DataBufType*	dataBuffer;
	static DataTabType*	dataTab;
	static unsigned short*  zeros;
	static int		recNum;
	static int		currentReadBlock;
	static int		remainingData;
	static timeval*		startTimeStamp;
	static float		buffSizeSec;
	static int		nbuff;
	//Variables:
	unsigned char*		rawDataChar;		//1-byte integer read data is stored here
	char*			rawDataCharPolar;	//1-byte integer read data is stored here
	unsigned short*		rawData;		//2-byte integer read data is stored here
	short*			rawDataPolar;		//2-byte integer read data is stored here
	float*			rawDataFloat;		//4-byte floating point read data
	float**			splittedRawData;	//Pointers to 4 polarization data in case of polar modeIndex.
							//In normal run it just has one pointer to rawData.
	char			hasReachedEof;		//Flag to let the program know when end of file has been reached
	long int		blockLength;		//Length(in samples) of the current block
	int			blockIndex;		//Current window number. (starting from 0)
	char*			headerInfo;
	int			nBuffTaken;
	//Functions:
	
	AquireData(Information _info);			/*Constructor for first time intialization, the static variables 
							 *are initialized here.*/
	AquireData(int _blockIndex);			//Constructor to intialize non-static me
			
	~AquireData();
	void readData();				/*This method is a wrapper that checks if the data is being read
							 *from file or from SHM and according calls the	appropiate
							 *function.*/
	void readDataFromFile();			//Reads from file
	void initializeSHM();				//Attaches to SHM
	int readFromSHM();				//Reads from SHM
	void splitRawData();
};
//Declaring static variables:
Information 	AquireData::info;
long int 	AquireData::eof;
DasHdrType*	AquireData::dataHdr;
DataBufType*	AquireData::dataBuffer;
DataTabType*	AquireData::dataTab;
unsigned short* AquireData::zeros;
int		AquireData::recNum=0;
int		AquireData::remainingData=0;
int		AquireData::currentReadBlock=0;
long int	AquireData::curPos=0;
double		AquireData::totalError=0.0;
float		AquireData::buffSizeSec;
int		AquireData::nbuff;		
struct timeval*	AquireData::startTimeStamp;
/*******************************************************************
*CONSTRUCTOR: AquireData::AquireData(Information _info)
*Information _info : contains all parameters.
*Invoked on first creation of an object of this type.
********************************************************************/
AquireData::AquireData(Information _info)
{
	 info=_info;
	 blockIndex=0;	 
	 hasReachedEof=0;
	 totalError=0.0;
	 remainingData=0;
	 //finding length of data file or initializing SHM according to the mode of operation
	 if(info.doReadFromFile)
	 {
	 	ifstream datafile;
	 	datafile.open(info.filepath,ios::binary);
		if(!datafile.is_open())
  		{
    			cout<<"Raw data file not found!"<<endl;
    			exit(1);
 		}
	 	datafile.seekg(0,ios::end);
	 	eof=datafile.tellg();
	 	datafile.close();
		if(eof==0)
		{
			cout<<"DATA FILE EMPTY"<<endl;
			exit(1);
		}
	 }
	// else
	//	initializeSHM();
	
	 
}
/*******************************************************************
*CONSTRUCTOR: AquireData::AquireData(int _blockIndex)
*int _blockIndex : current window number.
********************************************************************/
AquireData::AquireData(int _blockIndex)
{
	blockIndex=_blockIndex; 
	hasReachedEof=0;
	rawData=NULL;
	rawDataFloat=NULL;
	rawDataPolar=NULL;
	splittedRawData=NULL;
	nBuffTaken=0;
	if(info.isInline)
		headerInfo=new char[4096*nbuff];
	headerInfo=NULL;
}
/*******************************************************************
*DESTRUCTOR: AquireData::~AquireData()
*Free all memory
********************************************************************/
AquireData::~AquireData()
{	
	switch(info.sampleSizeBytes)
	{
		case 1:
			if(!info.doPolarMode)
			{
				delete[] rawDataChar;
			}
			else
				delete[] rawDataCharPolar;
			break;
		case 2:
			if(!info.doPolarMode)
				delete[] rawData;
			else
				delete[] rawDataPolar;
			break;
		case 4:		
			delete[] rawDataFloat;
			break;
	}
	
	//delete[] splittedRawData;
	
}
/*******************************************************************
*FUNCTION: void BasicAnalysis::readData()
*Wrapper to decide which method to call to aquire data
*******************************************************************/
void AquireData::readData()
{
	double er=info.blockSizeSec/info.samplingInterval;
	er=er-(long)er;
	totalError+=er;	
	blockLength = info.blockSizeSamples;	
  	if(!info.isInline && totalError>=1.0)
  	{
  		blockLength++;
  		totalError--;
  	}	
	if(info.doReadFromFile)
		readDataFromFile();
	else
		readFromSHM();
}
/*******************************************************************
*FUNCTION: void AquireData::initializeSHM()
*Initializes collect_psr shared memory of GSB/GWB
*******************************************************************/
void AquireData::initializeSHM()
{
  	int iddataHdr,idDataBuffer;
	switch(info.shmID)
	{
		case 1: //standard correlator shm
			iddataHdr = shmget(DAS_H_KEY,sizeof(DasHdrType),SHM_RDONLY);
			idDataBuffer = shmget(DAS_D_KEY,sizeof(DataBufType),SHM_RDONLY);
		break;
		case 2: //file shm
			iddataHdr = shmget(DAS_H_KEY_GPTOOL,sizeof(DasHdrType),SHM_RDONLY);
			idDataBuffer = shmget(DAS_D_KEY_GPTOOL,sizeof(DataBufType),SHM_RDONLY);
		break;
		case 3: //inline gptool shm
			iddataHdr = shmget(DAS_H_KEY_GPTOOL_INLINE,sizeof(DasHdrType),SHM_RDONLY);
			idDataBuffer = shmget(DAS_D_KEY_GPTOOL_INLINE,sizeof(DataBufType),SHM_RDONLY);
		break;
	}
	
	cout<<"iddataHdr="<<iddataHdr<<endl<<"idDataBuffer="<<idDataBuffer<<endl;
	
	if(iddataHdr < 0 || iddataHdr < 0)
	{
		exit(1);
	}
	dataHdr = (DasHdrType*)shmat(iddataHdr,0,SHM_RDONLY);
  	dataBuffer = (DataBufType*)shmat(idDataBuffer,0,SHM_RDONLY);
	if((dataBuffer)==(DataBufType*)-1)
	{
		cout<<"Cannot attach to shared memory!"<<endl;
		exit(1);
	}
	cout<<"Attached to shared memory:"<<dataHdr<<","<<dataBuffer<<endl;
	dataTab = dataBuffer-> dtab;
	cout<<"Max no of blocks="<<dataBuffer->maxblocks<<endl;
	zeros=new unsigned short[dataBuffer->blocksize-4096];
	if(info.isInline)
	{	
		buffSizeSec=(info.samplingInterval*(dataBuffer->blocksize-4096)/(info.sampleSizeBytes*info.noOfChannels));
		nbuff=(int)floor(info.blockSizeSec/buffSizeSec);
		info.blockSizeSec=nbuff*buffSizeSec;
		info.blockSizeSamples=(int)round(info.blockSizeSec/info.samplingInterval);

	}
	 /*   find a block, two blocks before the current block of the shm for reading data */
	//if(dataBuffer->cur_rec > (dataBuffer->maxblocks)/2)
	recNum = (dataBuffer->cur_rec+MaxDataBuf)%MaxDataBuf;
	currentReadBlock = dataTab[recNum].seqnum;

	int timeOff=sizeof(double)+sizeof(int)+sizeof(int)+sizeof(int);
	int blk_nano_off=timeOff+sizeof(struct timeval)+sizeof(int);
	startTimeStamp=(struct timeval*)(dataBuffer->buf+dataTab[recNum].rec*(dataBuffer->blocksize)+timeOff);
	
	int usec=0;
  	double nano_usec;
	struct tm *local_t = localtime(&startTimeStamp->tv_sec);
//  usec = 1000000*(timestamp_gps->tv_usec);
	double *blk_nano = (double *)(dataBuffer->buf+dataTab[recNum].rec*(dataBuffer->blocksize) + blk_nano_off);
  	usec = (startTimeStamp->tv_usec);
 	nano_usec = local_t->tm_sec; // putting int value in double:CAUTION
  	nano_usec += (startTimeStamp->tv_usec)/1e6;
  	nano_usec += (*blk_nano)/1e6;
	FILE* FATM=fopen("timestamp.gpt","w");
        char time_string[40];
        fprintf(FATM,"#Start time and date\n");
        fprintf(FATM,"IST Time: %02d:%02d:%012.9lf\n",local_t->tm_hour,local_t->tm_min, nano_usec);
        strftime (time_string, sizeof (time_string), "%d:%m:%Y", local_t);
        fprintf(FATM,"Date: %s\n",time_string);
        fclose(FATM);

}
double timeWaitTime=0.0; //Benchmark
/*******************************************************************
*FUNCTION: AquireData::readFromSHM()
*Reads from collect_psr shared memory of GSB/GWB
*******************************************************************/
int AquireData::readFromSHM()
{  	
	int DataOff=4096;
  	int bufferBlockLength=(dataBuffer->blocksize-DataOff)/info.sampleSizeBytes;  		
	long samplesToTake=info.noOfChannels*info.noOfPol*blockLength* info.sampleSizeBytes;	
	rawData=new unsigned  short int[blockLength*info.noOfPol*info.noOfChannels];	
	long int fetched=0;
	ofstream meanFile;
	meanFile.open("realTimeWarning.gpt",ios::app);
	ofstream recordFile;
	recordFile.open("blockLossRecord.gpt",ios::app);
	while(fetched<samplesToTake)
	{
  		timeWaitTime+=omp_get_wtime();
		int flag=0;
		while((dataHdr->status == DAS_START) && (dataTab[recNum].flag &BufReady) == 0)
		{
			usleep(2000);
			if(flag==0)
			{
				cout<<"Waiting"<<endl;
				flag=1;
			}
		}
		if(flag==1)
			cout<<"Ready"<<endl;
		timeWaitTime-=omp_get_wtime();
		if(dataHdr->status != DAS_START)
		{
			if ((dataTab[recNum].flag & BufReady) == 0)
			{
				cout<<"DAS not in START mode!!"<<endl;
				return -1;
			}
		}
		currentReadBlock = dataTab[recNum].seqnum;
		if(!info.isInline)
		{			
			cout<<endl<<"recNum "<<recNum<<endl;
			cout<<"dataBuffer->cur_rec "<<dataBuffer->cur_rec<<endl;
			cout<<"MaxDataBuf "<<MaxDataBuf<<endl;
			cout<<"dataBuffer->cur_block "<<dataBuffer->cur_block<<endl;
			cout<<"currentReadBlock "<<currentReadBlock<<endl;
			cout<<"dataBuffer->maxblocks "<<dataBuffer->maxblocks<<endl<<endl;
		}
		//memcpy(&currentReadBlock, &dataTab[recNum].seqnum, sizeof(int));
		//dataBuffer->maxblocks-1
		if(dataBuffer->cur_block - currentReadBlock >=dataBuffer->maxblocks-1)
		{
			meanFile<<"Lag in block blockIndex="<<blockIndex<<endl;
			meanFile<<"recNum = "<<recNum<<", Reading Sequence: "<<currentReadBlock<<", Collect's Sequence: "<<(dataBuffer->cur_block-1)<<endl;
			cout<<"Processing lagged behind..."<<endl;
			//recordFile<<blockIndex
			/*while(dataBuffer->cur_block - currentReadBlock >=dataBuffer->maxblocks-1)
			{
				if(samplesToTake-fetched>dataBuffer->blocksize-DataOff-remainingData)
				{
					memcpy(rawData+fetched/sizeof(unsigned short), zeros+remainingData, dataBuffer->blocksize-DataOff-remainingData);
  				fetched+=(dataBuffer->blocksize-DataOff-remainingData);
  				recNum=(recNum+1)%MaxDataBuf;
				remainingData=0;
  				}
  				else
  				{
					memcpy(rawData+fetched/sizeof(unsigned short), zeros+remainingData,samplesToTake-fetched);				
					remainingData=(samplesToTake-fetched);
					curPos+=samplesToTake;
					meanFile.close();
  					return -1;
  				}
				currentReadBlock = dataTab[(recNum)].seqnum;
			}*/
			cout<<"recNum = "<<recNum<<", Reading Sequence: "<<currentReadBlock<<", Collect's Sequence: "<<(dataBuffer->cur_block-1)<<" blockIndex = "<<blockIndex<<"\nRealiging...\n";
			recNum = (dataBuffer->cur_rec-1-2+MaxDataBuf)%MaxDataBuf;
			currentReadBlock = dataTab[(recNum)].seqnum;
		}
		//cout<<"recNum = "<<recNum<<", Reading Sequence: "<<currentReadBlock<<", Collect's Sequence: "<<dataBuffer->cur_block-1<<endl;
		//cout<<"Blocksize="<<dataBuffer->blocksize<<endl;
		if(samplesToTake-fetched>dataBuffer->blocksize-DataOff-remainingData)
		{	
			if(info.isInline)
			{
				memcpy(headerInfo+nBuffTaken*DataOff, dataBuffer->buf+dataTab[recNum].rec*(dataBuffer->blocksize), DataOff);
				nBuffTaken++;
			}
  			memcpy(rawData+fetched/sizeof(short), dataBuffer->buf+dataTab[recNum].rec*(dataBuffer->blocksize)+DataOff+remainingData, dataBuffer->blocksize-DataOff-remainingData);
  			fetched+=(dataBuffer->blocksize-DataOff-remainingData);
  			recNum=(recNum+1)%MaxDataBuf;
			remainingData=0;
  		}
  		else
  		{
  			memcpy(rawData+fetched/sizeof(short), dataBuffer->buf+dataTab[recNum].rec*(dataBuffer->blocksize)+DataOff+remainingData, samplesToTake-fetched);
			remainingData=(samplesToTake-fetched);
  			break;
  		}
  	}
  	curPos+=samplesToTake;
	meanFile.close();
	return 1;
}
/*******************************************************************
*FUNCTION: void AquireData::readDataFromFile()
*In offline mode reads raw data from a file
*******************************************************************/
void AquireData::readDataFromFile()
{
	int 	 c,i;
	long int blockSizeBytes= info.noOfChannels*info.noOfPol* blockLength* info.sampleSizeBytes; //Number of bytes to read in eac block
							//Number of bytes that have already been read
	
	ifstream datafile;
	datafile.open(info.filepath,ios::binary);	
	datafile.seekg(curPos,ios::beg);
	//logic to handle reading last block
	if(curPos+blockSizeBytes> eof)
	{
		blockSizeBytes=eof-curPos;
		blockLength=blockSizeBytes/(info.sampleSizeBytes*info.noOfChannels*info.noOfPol);		//The number of time samples 
		hasReachedEof=1;
	}
	/*******************************************************************
	*Handles different data types. GMRT data is mostly of type short while 
	*certain processed data maybe floating point.
	*******************************************************************/
	switch(info.sampleSizeBytes)
	{
		case 1:
			if(!info.doPolarMode)
			{
				rawDataChar=new unsigned char[blockSizeBytes/info.sampleSizeBytes];
				datafile.read((char*)rawDataChar,blockSizeBytes);
			}	
			else
			{
				rawDataCharPolar=new char[blockSizeBytes/info.sampleSizeBytes];
				datafile.read((char*)rawDataCharPolar,blockSizeBytes);
			
			}
			break;
		case 2:
			if(!info.doPolarMode)
			{
				rawData=new unsigned short[blockSizeBytes/info.sampleSizeBytes];
				datafile.read((char*)rawData,blockSizeBytes);			
			}
			else
			{
				rawDataPolar=new short[blockSizeBytes/info.sampleSizeBytes];
				datafile.read((char*)rawDataPolar,blockSizeBytes);	
			}

			break;
		case 4:
			rawDataFloat=new float[blockSizeBytes/info.sampleSizeBytes];
			datafile.read((char*)rawDataFloat,blockSizeBytes);
			break;
	}
	datafile.close();		
	curPos+=blockSizeBytes;
}
float u16tofloat(short x)
{
    union { float f; int i; } u; u.f = 0.00f; u.i |= x;
    return u.f ; 
}
/**********************************************************************
*FUNCTION: void AquireData::splitRawData()
*If on polar mode, splitting of raw data into four polarization channels
*is done here. GMRT polarization data is read out in the following fomat:
T1_C1_P T1_C1_Q T1_C1_R T1_C1_S T1_C2_P T1_C2_Q T1_C2_R T1_C2_S...
T1_CN_S T2_C1_P ...........TN_CN_S
where P,Q,R,S are the four polarizations, Cn denotes the nth channel 
and Tn the nth time sample. So T15_C25_Q means the Q-polarization data
of channel 25 of the 15th time sample.
**********************************************************************/
void AquireData::splitRawData()
{
	
	splittedRawData=new float*[info.noOfPol];
	float **ptrSplittedRawData=new float*[info.noOfPol];
	long int length=blockLength*info.noOfChannels;	
	for(int k=0;k<info.noOfPol;k++)
	{
		splittedRawData[k]=new float[length];
		ptrSplittedRawData[k]=splittedRawData[k];
	}
	/*******************************************************************
	*Handles different data types. GMRT data is mostly of type short while 
	*certain processed data maybe floating point.
	*******************************************************************/
	
	float* ptrRawDataFloat=rawDataFloat;
	switch(info.sampleSizeBytes)
	{
		case 1:		
			if(!info.doPolarMode)
			{
				unsigned char* ptrRawData=rawDataChar;	
				for(int j=0;j<blockLength;j++)		//Refer to the GMRT polarization data format in Function description.
				{		
					for(int i=0;i<info.noOfChannels;i++,ptrRawData++,ptrSplittedRawData[0]++)
					{
							*(ptrSplittedRawData[0])=(*ptrRawData);
					}
				}
			}
			else
			{
				char* ptrRawData=rawDataCharPolar;	
				for(int j=0;j<blockLength;j++)		//Refer to the GMRT polarization data format in Function description.
				{		
					for(int i=0;i<info.noOfChannels;i++)
					{
							*(ptrSplittedRawData[0]++)=(*ptrRawData++);
							*(ptrSplittedRawData[1]++)=(*ptrRawData++);
							*(ptrSplittedRawData[2]++)=(*ptrRawData++);
							*(ptrSplittedRawData[3]++)=(*ptrRawData++);

					}
				}
			}
			break;
		case 2:		
			if(!info.doPolarMode)
			{
				unsigned short int* ptrRawData=rawData;	
				for(int j=0;j<blockLength;j++)		//Refer to the GMRT polarization data format in Function description.
				{		
					for(int i=0;i<info.noOfChannels;i++,ptrRawData++,ptrSplittedRawData[0]++)
					{
							*(ptrSplittedRawData[0])=(*ptrRawData);
					}
				}
			}
			else
			{
				short int* ptrRawData=rawDataPolar;	
				for(int j=0;j<blockLength;j++)		//Refer to the GMRT polarization data format in Function description.
				{		
					for(int i=0;i<info.noOfChannels;i++)
					{
							*(ptrSplittedRawData[0]++)=(*ptrRawData++);
							*(ptrSplittedRawData[1]++)=(*ptrRawData++);
							*(ptrSplittedRawData[2]++)=(*ptrRawData++);
							*(ptrSplittedRawData[3]++)=(*ptrRawData++);

					}
				}
			}
			break;
		case 4:					
			for(int j=0;j<blockLength;j++)		//Refer to the GMRT polarization data format in Function description.
			{		
				for(int i=0;i<info.noOfChannels;i++)
				{
					for(int k=0;k<info.noOfPol;k++,ptrRawDataFloat++)
					{				
						(*ptrSplittedRawData[k])=(*ptrRawDataFloat);	
						ptrSplittedRawData[k]++;
					}	
				}
			}
			break;
	}
	delete[] ptrSplittedRawData;
}
//End of AquireData implementation.

/*******************************************************************
*CLASS: BasicAnalysis
*Performs basic operations like bandshape and zeroDM time series 
*computation for a particular window and a particular polarization.
*These quantities are then used by objects of RFIFiltering class 
*to find and filter Radio Frequency Interference (RFI).
*******************************************************************/
class BasicAnalysis
{
	public:
	//static variables:
	
	static	Information	info;			//contains all input parameters
	static double**		smoothSumBandshape;	//Cumulative smooth bandshape for each polarization
	static double** 	sumBandshape;		//Cumulative bandshapes for each polaization
	static double**		squareBandshape;	//Cumulative square of bandshape for each polarization
	static long long int** 	countBandshape;		//Number of sample that goes into each frequency bin (Cumulative) for each polaization
	static float**		externalBandshape; 	//Externally loaded bandshape
	long int		blockLength;	
	
	//variables:
	float 			cumBandshapeScale;		//Used to scale down the cumulative bandshape to avoid overflow issues
	int			polarIndex;			//Index of polarization to process.
	float			*rawData;			//The 2D time-frequency data 	
	short int	*filteredRawData;		//The Filtered 2D time-frequency data 
	float			*zeroDM;			//Time series obtained by collapsing all frequency channels (Without dedispersion)
	float			*zeroDMUnfiltered;		//Time series obtained by collapsing all frequency channels (Without dedispersion), without filtering
	float			*bandshape;			//Mean bandshape obtained by collapsing all time samples.
	float			*meanToRmsBandshape;		//Mean to rms computed for each channel. (rms of time series for that channel)
	float			*smoothBandshape;		//smoothened bandshape obtained by moving mean or median
	float			*normalizedBandshape;		//bandshape normalized using smoothBandshape.
	float			*correlationBandshape;
	char			*headerInfo;			//corresponding header information - used only in INLINE mode
	//Minimum and maximum of each array. Used in plotting.	
	float 		minZeroDM;		
	float		maxZeroDM;
	float		maxBandshape;
	float		minBandshape;
	float		maxMeanToRmsBandshape;
	float		minMeanToRmsBandshape;
	float		minNormalizedBandshape;
	float		maxNormalizedBandshape;
	int 		count;				//No of time series in current bandshape;
	//Functions:
	BasicAnalysis(Information _info);
	BasicAnalysis(float* _rawData,int polarIndex_,long int _blockLength);
	~BasicAnalysis();
	void computeZeroDM(char* freqFlags);							//Computes zeroDM with given frequency flags
	void computeBandshape();								//Computes bandshape
	void computeBandshape(char* timeFlags);							//Computes bandshape with given time flags
	void calculateCumulativeBandshapes();							//Computes the global cumulative bandshapes
	void quicksort(float* x,long int first,long int last);					//Used to compute smooth bandshape using moving median 
	void smoothAndNormalizeBandshape(); 							//Smoothens and normalizes the bandshape
	void normalizeBandshape();								// Normalize bandshape using externally supplied file
	void normalizeData();									//normalizes 2-D data
	void getFilteredRawData(char* timeFlags,char* freqFlags,float replacementValue);	//gets Filtered Raw Data.
	void getFilteredRawDataSmoothBshape(char* timeFlags,char* freqFlags);
	void subtractZeroDM(char* freqFlags,float centralTendency);
	void writeBandshape(const char*  filename);						//Writes out the cumulative mean and rms a bandshape	
	void writeCurBandshape(const char* filename);						//Writes out current bandshape
	void writeFilteredRawData(const char*  filename);					//Writes out filtered 2D data
		
};
//implementation of BasicAnalysis methods begins
//Declaration of static variables

Information	BasicAnalysis::info;
double**	BasicAnalysis::smoothSumBandshape;
double** 	BasicAnalysis::sumBandshape;
double**	BasicAnalysis::squareBandshape;
float**		BasicAnalysis::externalBandshape;
long long int** BasicAnalysis::countBandshape;


/*******************************************************************
*CONSTRUCTOR: BasicAnalysis::BasicAnalysis(Information _info)
*Information _info: All input parameters are contained in this object
*Initialization of static variables is done in this constructor
*******************************************************************/
BasicAnalysis::BasicAnalysis(Information _info)
{
	info=_info;
	//initializing other static variables
	smoothSumBandshape=new double*[info.noOfPol];
	sumBandshape=new double*[info.noOfPol];
	squareBandshape=new double*[info.noOfPol];
	countBandshape=new long long int*[info.noOfPol];
	externalBandshape=new float*[info.noOfPol];
	for(int k=0;k<info.noOfPol;k++)
	{
		sumBandshape[k]=new double[info.noOfChannels];
		smoothSumBandshape[k]=new double[info.noOfChannels];
		squareBandshape[k]=new double[info.noOfChannels];
		countBandshape[k]=new long long int[info.noOfChannels];
		externalBandshape[k]=new float[info.noOfChannels];
		for(int i=0;i<info.noOfChannels;i++)
			sumBandshape[k][i]=smoothSumBandshape[k][i]=squareBandshape[k][i]=countBandshape[k][i]=0;
	}
	if(info.normalizationProcedure==2)
	{
		int count=0;
		std::ifstream f("bandshape.dat");
		std::string line;
		while (std::getline(f, line)) {
			count++;
			int index;
			float val;
			std::istringstream ss(line);
			ss >> index >> val;
			if(val==0)
				val=-1;
			externalBandshape[0][index]=val;
		}
		if(count<<info.noOfChannels)
		{
			cout<<"bandshape.dat does not contain "<<info.noOfChannels<<" number of channels"<<endl;
		}
	}
	filteredRawData=NULL;
	
}
/*******************************************************************
*FUNCTION: BasicAnalysis::BasicAnalysis(float* _rawData,int _blockIndex)
*float *_rawData 		:the 2-D raw data to process in the current block
*int _polarIndex		:index of polarization to process
*long int _blockLength		:length of current block
*Performs initialization of variables for current block
*******************************************************************/
BasicAnalysis::BasicAnalysis(float* _rawData,int _polarIndex,long int _blockLength)
{
	polarIndex=_polarIndex;
	blockLength=_blockLength;
	rawData=_rawData;
	bandshape=new float[info.noOfChannels];	
	correlationBandshape=new float[info.noOfChannels];	
	meanToRmsBandshape=new float[info.noOfChannels];	
	normalizedBandshape=new float[info.noOfChannels];	
	zeroDM=new float[blockLength]; 
	zeroDMUnfiltered=new float[blockLength]; 
	cumBandshapeScale=1000.0;
	if(info.normalizationProcedure==2)
		smoothBandshape=externalBandshape[polarIndex];
	else
		smoothBandshape=new float[info.noOfChannels];
	filteredRawData=NULL;
	headerInfo=NULL;
	
}
/*******************************************************************
*DESTRUCTOR: BasicAnalysis::~BasicAnalysis()
*Frees memory occupied by various arrays
*******************************************************************/
BasicAnalysis::~BasicAnalysis()
{
	delete[] rawData;
	delete[] zeroDM;
	delete[] zeroDMUnfiltered;
	delete[] bandshape;
	delete[] correlationBandshape;
	delete[] meanToRmsBandshape;
	delete[] normalizedBandshape;
	if(info.normalizationProcedure!=2)
		delete[] smoothBandshape;
	if(filteredRawData!=NULL)
		delete[] filteredRawData;
	if(headerInfo!=NULL)
		delete[] headerInfo;
}


/*******************************************************************
*FUNCTION: void BasicAnalysis::computeZeroDM(char* freqFlags)
*char* freqFlags : The channels marked 1 in this array is not 
*included while computing zeroDM (or are clipped)
*For no flagging, a char* variable with all 0's is passed on to this 
*function
*Computes the time series by collapsing all frequency channels with
*no dedispersion (hence the name "zeroDM" series). While collapsing 
*the flagged channels are ignored or clipped.
*******************************************************************/
void BasicAnalysis::computeZeroDM(char* freqFlags)
{
	float*	ptrZeroDM;
	float*	ptrZeroDMUnfiltered;
	float* 	ptrRawData;
	float 	count=0;					//Stores the number of channels added to get each time sample	
	int 	startChannel=info.startChannel;
	int 	nChan= info.stopChannel-startChannel;		//Number of channels to use
	int 	endExclude=info.noOfChannels-info.stopChannel;	//Number of channels to exclude from the end of the band
	int 	l= blockLength;
	for(int i=0;i<nChan;i++)
		if(!freqFlags[i])
			count++;
	ptrRawData=rawData;
	ptrZeroDM=zeroDM;
	ptrZeroDMUnfiltered=zeroDMUnfiltered;
	maxZeroDM=0;
	minZeroDM=10000*nChan;					//This is done because there is no sample computed yet.		
	for(int i=0;i<l;i++,ptrZeroDM++,ptrZeroDMUnfiltered++)
	{		
		*ptrZeroDM=0;	
		*ptrZeroDMUnfiltered=0;		
		ptrRawData+=startChannel;			//startChannel number of channels skipped at the start of the band
		for(int j=0;j<nChan;j++,ptrRawData++)
		{
			*ptrZeroDMUnfiltered+=(*ptrRawData);
			if(!freqFlags[j])
				(*ptrZeroDM)+=(*ptrRawData);
		}
		ptrRawData+=endExclude;				//endExclude number of channels skipped at the end of the band
		(*ptrZeroDM)/=(float)count;			//Each sample averaged 
		(*ptrZeroDMUnfiltered)/=(float)nChan;
		//Calculating of minimum and maximum of zeroDM series
		if(*ptrZeroDM>maxZeroDM)
			maxZeroDM=*ptrZeroDM;
		if(*ptrZeroDM<minZeroDM)
			minZeroDM=*ptrZeroDM;
	}
}
/*******************************************************************
*FUNCTION: void BasicAnalysis::computeBandshape()
*Computes mean and mean-to-rms bandshape for the current block .
*It also calculates quantities to find the cumulative mean and 
*rms of the bandshape.
*******************************************************************/
void BasicAnalysis::computeBandshape()
{
	float	*ptrBandshape,*ptrMeanToRmsBandshape;
	float	*ptrRawData;
	int 	startChannel=info.startChannel;
	int 	nChan= info.stopChannel-startChannel;				//Number of channels to use
	int 	endExclude=info.noOfChannels-info.stopChannel;			//Number of channels to exclude from the end
	int 	l= blockLength;
	ptrRawData=rawData;
	ptrBandshape=bandshape;
	ptrMeanToRmsBandshape=meanToRmsBandshape;
	//Intialization of bandshape
	for(int j=0;j<info.noOfChannels;j++,ptrBandshape++,ptrMeanToRmsBandshape++)
		*ptrBandshape=*ptrMeanToRmsBandshape=0;	
	for(int i=0;i<l;i++)
	{		
		ptrBandshape=&bandshape[startChannel]; 				//startChannel number of channels skipped at the start
		ptrMeanToRmsBandshape=&meanToRmsBandshape[startChannel];
		ptrRawData+=startChannel;
		for(int j=0;j<nChan;j++,ptrRawData++,ptrBandshape++,ptrMeanToRmsBandshape++)
		{
			(*ptrBandshape)+=(*ptrRawData);
			*ptrMeanToRmsBandshape+=(*ptrRawData)*(*ptrRawData);
		}
		ptrRawData+=endExclude;						//endExclude number of channels skipped at the end
		ptrBandshape+=endExclude;
		ptrMeanToRmsBandshape+=endExclude;
	}
	count=blockLength;
}


/*******************************************************************
*void BasicAnalysis::computeBandshape(char* timeFlags)
*char* timeFlags : Excludes the time samples for which timeFlag is 1
*This function is similar to void BasicAnalysis::computeBandshape(),
*the only difference being that timeFlags is used to ignore (or clip)
*bad time samples.
*******************************************************************/
void BasicAnalysis::computeBandshape(char* timeFlags)
{
	float *ptrBandshape,*ptrMeanToRmsBandshape;
	float* 	ptrRawData;
	char*  	ptrTimeFlags;
	int 	startChannel=info.startChannel;
	int 	nChan= info.stopChannel-startChannel;
	int 	endExclude=info.noOfChannels-info.stopChannel;
	int 	l= blockLength;
	ptrRawData=rawData;
	ptrBandshape=bandshape;
	ptrTimeFlags=timeFlags;
	
	//Initializing bandshape
	for(int j=0;j<info.noOfChannels;j++,ptrBandshape++,ptrMeanToRmsBandshape++)
		*ptrBandshape=*ptrMeanToRmsBandshape=0;		
			
	ptrTimeFlags=timeFlags;	
	
	for(int i=0;i<l;i++,ptrTimeFlags++)
	{		
		ptrBandshape=&(bandshape[startChannel]);			//startChannel number of channels skipped at the start
		ptrMeanToRmsBandshape=&meanToRmsBandshape[startChannel];
		if(!(*ptrTimeFlags))
		{
			ptrRawData+=startChannel;			
			for(int j=0;j<nChan;j++,ptrRawData++,ptrBandshape++,ptrMeanToRmsBandshape++)
			{
				(*ptrBandshape)+=(*ptrRawData);	
				*ptrMeanToRmsBandshape+=(*ptrRawData)*(*ptrRawData);	
					
			}
			ptrRawData+=endExclude;					//endExclude number of channels skipped at the end
			ptrBandshape+=endExclude;
			ptrMeanToRmsBandshape+=endExclude;
		}
		else
			ptrRawData+=info.noOfChannels;				//skips entire time sample if flagged
	}
	
	//Finding number of time samples added to each channel bin
	for(int i=0;i<l;i++,ptrTimeFlags++)
		if(!(*ptrTimeFlags))		
			count++;
	
}
void BasicAnalysis::calculateCumulativeBandshapes()
{
	float *ptrBandshape,*ptrMeanToRmsBandshape;
	int nChan= info.stopChannel-info.startChannel;
	ptrBandshape=&bandshape[info.startChannel];	
	ptrMeanToRmsBandshape=&meanToRmsBandshape[info.startChannel];
	minMeanToRmsBandshape=minBandshape=1e8;
	maxMeanToRmsBandshape=maxBandshape=0;

	
	double *ptrSumBandshape=&sumBandshape[polarIndex][info.startChannel];
	double *ptrSquareBandshape=&squareBandshape[polarIndex][info.startChannel];
	long long int *ptrCountBandshape=&countBandshape[polarIndex][info.startChannel];
	/*************************************************************************************
	*Calculation of cumulative values, current mean bandshape, mean-to-rms bandshape.
	*Also computes the max and min of each of these bandshapes.
	*These max-min values are used for plotting purposes.
	*The cumulative values are used to calculate the mean and the mean-to-rms bandshape
	*over the entire run of the program.
	**************************************************************************************/
	for(int j=0;j<nChan;j++,ptrBandshape++,ptrSumBandshape++,ptrSquareBandshape++,ptrCountBandshape++,ptrMeanToRmsBandshape++)
	{
		//Computation of cumulative values: 
		*ptrSumBandshape+=*ptrBandshape/cumBandshapeScale;
		*ptrSquareBandshape+=*ptrMeanToRmsBandshape/(cumBandshapeScale*cumBandshapeScale);
		*ptrCountBandshape+=count;	
		
		//Computation of mean bandshape:
		*ptrBandshape/=(float)count;
		if(*ptrBandshape>maxBandshape)
			maxBandshape=*ptrBandshape;
		if(*ptrBandshape<minBandshape)		
			minBandshape=*ptrBandshape;
		if(*ptrBandshape==0)
		{
			*ptrMeanToRmsBandshape=0;
			minMeanToRmsBandshape=0;
			continue;
		}
		//Computation of mean-to-rms bandshape:
		*ptrMeanToRmsBandshape=(*ptrBandshape)/sqrt(*ptrMeanToRmsBandshape/count-pow(*ptrBandshape,2));
		
		
		if(*ptrMeanToRmsBandshape>maxMeanToRmsBandshape)
			maxMeanToRmsBandshape=*ptrMeanToRmsBandshape;
		if(*ptrMeanToRmsBandshape<minMeanToRmsBandshape)		
			minMeanToRmsBandshape=*ptrMeanToRmsBandshape;
		
	}
	if((int)info.bandshapeToUse==2 || info.doUseNormalizedData)
	{
		if(info.normalizationProcedure==1)
			smoothAndNormalizeBandshape();
		else
			normalizeBandshape();
	}
			
}
void BasicAnalysis::quicksort(float* x,long int first,long int last)
{
    int pivot,j,i;
    float temp;

     if(first<last){
         pivot=first;
         i=first;
         j=last;

         while(i<j){
             while(x[i]<=x[pivot]&&i<last)
                 i++;
             while(x[j]>x[pivot])
                 j--;
             if(i<j){
                 temp=x[i];
                  x[i]=x[j];
                  x[j]=temp;
             }
         }

         temp=x[pivot];
         x[pivot]=x[j];
         x[j]=temp;
         quicksort(x,first,j-1);
         quicksort(x,j+1,last);

	}
}
/*******************************************************************
*FUNCTION: void BasicAnalysis::smoothAndNormalizeBandshape()
*Smoothens the mean bandshape and uses it to find the normalized 
*bandshape.
*The smooth bandshape here is used as an approximation to filtering
*response where smoothing ensures that the impact of RFI is minimized.
*Normilization inverts the effect of filter response to bring the power
*levels of each channel to roughly 1.
*Normalization helps in both time and frequency domain filtering.
*Refer to documentations on it for more details.
*******************************************************************/
void BasicAnalysis::smoothAndNormalizeBandshape() 
{
	int startChannel=info.startChannel;
	int nChan= info.stopChannel-startChannel;
	double *ptrSmoothSumBandshape=&smoothSumBandshape[polarIndex][startChannel];
	float *ptrSmoothBandshape=&smoothBandshape[startChannel];
	float *ptrNormalizedBandshape=&normalizedBandshape[startChannel];
	float *ptrBandshape=&bandshape[startChannel];	
	long long *ptrCountBandshape=&countBandshape[polarIndex][startChannel];
	for(int i=0;i<info.noOfChannels;i++)
		smoothBandshape[i]=0;
	int wSize=info.smoothingWindowLength/2;
	float* tempArray=new float[info.smoothingWindowLength];
	minNormalizedBandshape=maxNormalizedBandshape=0;
	
	for(int j=0;j<nChan;j++,ptrBandshape++,ptrSmoothBandshape++,ptrNormalizedBandshape++,ptrSmoothSumBandshape++,ptrCountBandshape++)
	{
		int tempCnt=0;	
		for(int i=-1*wSize;i<wSize;i++)
		{
			if(i+j>=nChan)
				break;
			if(i+j>=0)
			{
				tempArray[tempCnt]=*(ptrBandshape+i);
				tempCnt++;	
			}
		}
		quicksort(tempArray,0,tempCnt-1);
		*ptrSmoothBandshape=tempArray[tempCnt/2];
		
		*ptrSmoothSumBandshape+=(*ptrSmoothBandshape)*blockLength/cumBandshapeScale;
		*ptrSmoothBandshape=(*ptrSmoothSumBandshape)*cumBandshapeScale/(*ptrCountBandshape);
		*ptrNormalizedBandshape=*ptrBandshape/(*ptrSmoothBandshape);
		if(*ptrNormalizedBandshape<minNormalizedBandshape)
			minNormalizedBandshape=*ptrNormalizedBandshape;
		if(*ptrNormalizedBandshape>maxNormalizedBandshape)
			maxNormalizedBandshape=*ptrNormalizedBandshape;
		if(*ptrSmoothBandshape==0)
		{
			*ptrSmoothBandshape=1;
			*ptrNormalizedBandshape=0;
		}
	}
	delete[] tempArray;
}

/*******************************************************************
*FUNCTION: void BasicAnalysis::normalizeBandshape()
*Normalizes bandshape using externally supplied bandshape
*Normalization inverts the effect of filter response to bring the power
*levels of each channel to roughly 1.
*Normalization helps in both time and frequency domain filtering.
*Refer to documentations on it for more details.
*******************************************************************/
void BasicAnalysis::normalizeBandshape() 
{
	
	int nChan= info.stopChannel-info.startChannel;
	float *ptrSmoothBandshape=&smoothBandshape[info.startChannel];
	float *ptrNormalizedBandshape=&normalizedBandshape[info.startChannel];
	float *ptrBandshape=&bandshape[info.startChannel];
	for(int j=0;j<nChan;j++,ptrBandshape++,ptrSmoothBandshape++,ptrNormalizedBandshape++)
	{
		if(*ptrSmoothBandshape==0)
			*ptrNormalizedBandshape=0;
		*ptrNormalizedBandshape=*ptrBandshape/(*ptrSmoothBandshape);
		if(*ptrNormalizedBandshape<minNormalizedBandshape)
			minNormalizedBandshape=*ptrNormalizedBandshape;
		if(*ptrNormalizedBandshape>maxNormalizedBandshape)
			maxNormalizedBandshape=*ptrNormalizedBandshape;
	}
}

/*******************************************************************
*FUNCTION: void BasicAnalysis::NormalizeData()
*Normalizes data based on smooth bandshape.
*******************************************************************/
void BasicAnalysis::normalizeData()
{	
	int startChannel=info.startChannel;
	int nChan= info.stopChannel-startChannel;
	int endExclude=info.noOfChannels-info.stopChannel;
	float *ptrSmoothBandshape=&smoothBandshape[startChannel];
	float *ptrRawData=rawData;	
	for(int i=0;i<blockLength;i++)
	{							
		ptrRawData+=startChannel;
		ptrSmoothBandshape=&smoothBandshape[startChannel];
		for(int j=0;j<nChan;j++,ptrRawData++,ptrSmoothBandshape++)
			(*ptrRawData)=(*ptrRawData)/(*ptrSmoothBandshape);		
		ptrRawData+=endExclude;
		
	}
}

/*******************************************************************
*FUNCTION: void BasicAnalysis::computeZeroDM(char* freqFlags)
*char* freqFlags : The channels marked 1 in this array is not 
*included while computing zeroDM (or are clipped)
*For no flagging, a char* variable with all 0's is passed on to this 
*function
*Subtracts the zero DM time series from each channel
*******************************************************************/
void BasicAnalysis::subtractZeroDM(char* freqFlags,float centralTendency)
{
	float*	ptrZeroDM;
	float*	ptrCorrelationBandshape;
	float*	ptrMeanToRmsBandshape;
	float*  ptrNormalizedBandshape;
	float* 	ptrRawData;
	float 	count=0;					//Stores the number of channels added to get each time sample	
	int 	startChannel=info.startChannel;
	int 	nChan= info.stopChannel-startChannel;		//Number of channels to use
	int 	endExclude=info.noOfChannels-info.stopChannel;	//Number of channels to exclude from the end of the band
	int 	l= blockLength;
	float 	zeroDMMean,zeroDMRMS;
	for(int i=0;i<nChan;i++)
		if(!freqFlags[i])
			count++;
	ptrZeroDM=zeroDM;
	zeroDMMean=0.0;	
	zeroDMRMS=0.0;
	for(int i=0;i<l;i++,ptrZeroDM++)
	{
		zeroDMMean+=*ptrZeroDM;
		zeroDMRMS+=(*ptrZeroDM)*(*ptrZeroDM);
	}
	zeroDMMean/=l;
	zeroDMRMS=zeroDMRMS/l-zeroDMMean*zeroDMMean;
	ptrZeroDM=zeroDM;
	ptrRawData=rawData;
	ptrCorrelationBandshape=correlationBandshape;
	for(int j=0;j<info.noOfChannels;j++,ptrCorrelationBandshape++)		
		*ptrCorrelationBandshape=0;
	for(int i=0;i<l;i++,ptrZeroDM++)
	{	
		ptrRawData+=startChannel;			//startChannel number of channels skipped at the start of the band
		ptrCorrelationBandshape=correlationBandshape+startChannel;
		ptrNormalizedBandshape=normalizedBandshape+startChannel;
		for(int j=0;j<nChan;j++,ptrRawData++,ptrCorrelationBandshape++,ptrNormalizedBandshape++)		
			*ptrCorrelationBandshape+=(*ptrZeroDM-zeroDMMean)*(*ptrRawData-*ptrNormalizedBandshape);
		ptrRawData+=endExclude;				//endExclude number of channels skipped at the end of the band
	}
	ptrZeroDM=zeroDM;
	ptrRawData=rawData;
	for(int i=0;i<l;i++,ptrZeroDM++)
	{	
		ptrRawData+=startChannel;			//startChannel number of channels skipped at the start of the band
		ptrCorrelationBandshape=correlationBandshape+startChannel;
		ptrNormalizedBandshape=normalizedBandshape+startChannel;
		ptrMeanToRmsBandshape=meanToRmsBandshape+startChannel;
		for(int j=0;j<nChan;j++,ptrRawData++,ptrNormalizedBandshape++,ptrCorrelationBandshape++,ptrMeanToRmsBandshape++)		
			*ptrRawData=(*ptrRawData-*ptrNormalizedBandshape)-(*ptrZeroDM-zeroDMMean)*(*ptrCorrelationBandshape/l)/zeroDMRMS+*ptrNormalizedBandshape;
		ptrRawData+=endExclude;				//endExclude number of channels skipped at the end of the band
	}
}
/*******************************************************************
*FUNCTION: void BasicAnalysis::getFilteredRawData(float replacementValue)
*float replacementValue - 0 or median/mode of zeroDM series
*Replaces flagged values by zero or median when 2d time-freq data
*is to be written out
*******************************************************************/
void BasicAnalysis::getFilteredRawData(char* timeFlags,char* freqFlags,float replacementValue)
{
	float* ptrRawData=rawData;
	char* ptrTimeFlags=timeFlags;
	char* ptrFreqFlags;
	
	long int pos;

	int startChannel=info.startChannel;
	int stopChannel=info.stopChannel;
	int totalChan=info.noOfChannels;
	int endExclude=info.noOfChannels-stopChannel;
	filteredRawData=new short int[blockLength*totalChan];
	short int* ptrFilteredRawData=filteredRawData;
	for(long int i=0;i<blockLength;i++,ptrTimeFlags++)
	{
			ptrFreqFlags=freqFlags;	
			for(int j=0;j<startChannel;j++,ptrRawData++,ptrFilteredRawData++)
				*ptrFilteredRawData=(short int)(replacementValue*info.meanval);

			for(int j=startChannel;j<stopChannel;j++,ptrRawData++,ptrFreqFlags++,ptrFilteredRawData++)
			{

				if((!*ptrTimeFlags)&(!*ptrFreqFlags))
					*ptrFilteredRawData=(short int)((*ptrRawData)*info.meanval);
				else
					*ptrFilteredRawData=(short int)(replacementValue*info.meanval);					
			}

			for(int j=stopChannel;j<totalChan;j++,ptrRawData++,ptrFilteredRawData++)
				*ptrFilteredRawData=(short int)(replacementValue*info.meanval);
	}
}
/*******************************************************************
*FUNCTION: void BasicAnalysis::getFilteredRawDataSmoothBshape(char* timeFlags,char* freqFlags)
*Replaces flagged values by the smooth bandshape value for that channel
*******************************************************************/
void BasicAnalysis::getFilteredRawDataSmoothBshape(char* timeFlags,char* freqFlags)
{
	float* ptrRawData=rawData;
	char* ptrTimeFlags=timeFlags;
	char* ptrFreqFlags;
	
	long int pos;

	int startChannel=info.startChannel;
	int stopChannel=info.stopChannel;
	int totalChan=info.noOfChannels;
	int endExclude=info.noOfChannels-stopChannel;
	filteredRawData=new short int[blockLength*totalChan];
	short int* ptrFilteredRawData=filteredRawData;
	float *ptrSmoothBandshape;
	for(long int i=0;i<blockLength;i++,ptrTimeFlags++)
	{
			ptrFreqFlags=freqFlags;	
			ptrSmoothBandshape=smoothBandshape;
			for(int j=0;j<startChannel;j++,ptrRawData++,ptrFilteredRawData++,ptrSmoothBandshape++)
				*ptrFilteredRawData=*ptrSmoothBandshape;

			for(int j=startChannel;j<stopChannel;j++,ptrRawData++,ptrFreqFlags++,ptrFilteredRawData++,ptrSmoothBandshape++)
			{

				if((!*ptrTimeFlags)&(!*ptrFreqFlags))
					*ptrFilteredRawData=(short int)(*ptrRawData);
				else
					*ptrFilteredRawData=(short int)(*ptrSmoothBandshape);					
			}

			for(int j=stopChannel;j<totalChan;j++,ptrRawData++,ptrFilteredRawData++,ptrSmoothBandshape++)
				*ptrFilteredRawData=*ptrSmoothBandshape;
	}
}
/*******************************************************************
*FUNCTION: void BasicAnalysis::writeFilteredRawData(const char*  filename)
*const char*  fileName: Filename to write to.
*Writes out filtered rawdata where flagged samples have been replaced by 
*zeros
*******************************************************************/
void BasicAnalysis::writeFilteredRawData(const char*  filename)
{
	FILE* filteredRawDataFile;    
	filteredRawDataFile = fopen(filename, "ab");
	fwrite(filteredRawData, sizeof(short int), blockLength*info.noOfChannels, filteredRawDataFile); 
	fclose(filteredRawDataFile); 
}
/*******************************************************************
*FUNCTION: void BasicAnalysis::writeBandshape(char* filename)
*char* filename: Name of the file to which mean, mean smooth and rms bandshape will 
*be written out.
*Writes out the mean, mean smooth and rms bandshape (over the entire run of the program)
*to the specified file.
*******************************************************************/
void BasicAnalysis::writeBandshape(const char* filename)
{
	ofstream meanFile;
	meanFile.open(filename);
	double sMean,mean,rms;
	meanFile<<"# chan_no \t smooth_bshape \t mean_bshape \t rms_bshape"<<endl;
	for(int i=0;i<info.noOfChannels;i++)
	{
		if(countBandshape[polarIndex][i]!=0)		
		{
			sMean=smoothSumBandshape[polarIndex][i]/countBandshape[polarIndex][i];	
			mean=sumBandshape[polarIndex][i]/countBandshape[polarIndex][i];	
			rms=sqrt((squareBandshape[polarIndex][i]/countBandshape[polarIndex][i])-pow(mean,2));	
		}		
		else
		{
			sMean=0;
			mean=0;
			rms=-1;
		}
		meanFile<<i<<" "<<sMean*cumBandshapeScale<<" "<<mean*cumBandshapeScale<<" "<<rms*cumBandshapeScale<<endl;	
			
	}
	meanFile.close();
}

/*******************************************************************
*FUNCTION: void BasicAnalysis::writeBandshape(char* filename)
*char* filename: Name of the file to which mean bandshape will 
*be written out.
*Writes out the mean bandshape (for current block)
*to the specified file.
*******************************************************************/
void BasicAnalysis::writeCurBandshape(const char* filename)
{
	ofstream meanFile;
	meanFile.open(filename,ios::app);
	meanFile<<blockLength<<" ";
	if((int)info.bandshapeToUse==2 || info.doUseNormalizedData)
		for(int i=0;i<info.noOfChannels;i++)
		{			
			if(smoothBandshape[i]==0)
				meanFile<<0.0<<" ";	
			else
				meanFile<<bandshape[i]/smoothBandshape[i]<<" ";			
		}	
	else	
		for(int i=0;i<info.noOfChannels;i++)
		{			
				meanFile<<bandshape[i]<<" ";				
		}	
	
	meanFile<<endl;
	meanFile.close();
}

//implementation of BasicAnalysis methods ends

/*******************************************************************
CLASS: RFIFiltering
*Handles detecting and flagging Radio Frequency Interference (RFI) in
*input data.
*Contains methods to find the central tendency as well as the rms of 
*the current block. Can be expanded to include multiple algorithms
*to do so.
*It also generates and writes flags.
*******************************************************************/
class RFIFiltering
{
	
	public:
	float histogramInterval;
	float*	input;			//Input array from which outliers are to be detected
	int	inputSize;		//Size of input array
	float	inputMax;		//Maximum and minimum elements of the array. Used by certain algorithms.
	float	inputMin;		
	float	centralTendency;	//Computed central tendency of the input data.
	float	rms;			//Computer rms of the input data.
	float	cutoff;			//Threshold above which samples are flagged.
	float	cutoffToRms;		//Threshold to rms ratio.
	char*	flags;			//Array where flags are stored. 1 implies outlier. 0 implies normal.
	float*  sFlags;
	//These variables are used when histogram based filtering is done
	float*	histogram;
	float*	histogramAxis;
	int 	histogramSize;
	int	histogramMax;
	
	
	RFIFiltering(float* input_,int inputSize_);			//Constructor	
	~RFIFiltering();						//Destructor
	void computeStatistics(int algorithmCode);			//Wrapper to call appropiate function to find the central tendency and rms
	void smoothFlags(int windowLength,float threshold);
	void flagData();						//Function to generate flags once rms and central tendency has been found
	void multiPointFlagData(float* multiCutoff);
	void generateBlankFlags();					//Generates blank flags in case of no flagging
	void writeFlags(const char* fileName);				//Writes out flags to a file
	void writeFlags(const char* filename,char* startFlags,int nStartFlags,char* endFlags,int nEndFlags); //Writes out flags to a file with startFlags and endFlags appeneded at the beginning and end respectively.
	void generateManualFlags(int nBadChanBlocks,int* badChanBlocks,int offset);	//flags user specified blocks
	private:
	void histogramBased();						//Finds mode for central tendency and rms based on finding the distribution
	void MADBased();						//Finds median for central tendency and median absolute deviation for rms
	void quicksort(float* x,long int first,long int last);		//Sorting routine used by MADBased() to sort the input
	
};
//implementation of RFIFiltering methods

/*******************************************************************
*CONSTRUCTOR: RFIFiltering::RFIFiltering(float* input_,int inputSize_)
*float* input_  : Array from which outliers are to be detected.
*int inputSize_ : Size of the input array
*******************************************************************/
RFIFiltering::RFIFiltering(float* input_,int inputSize_)
{
	input=input_;
	inputSize=inputSize_;
	flags=new char[inputSize];
	sFlags=new float[inputSize];
	histogram=NULL;
	histogramAxis=NULL;
	generateBlankFlags();  	
}


/*******************************************************************
*DESTRUCTOR: RFIFiltering::~RFIFiltering()
*frees used memory
*******************************************************************/
RFIFiltering::~RFIFiltering()
{
	if(histogram!=NULL)
	{
		delete[] histogram;
		delete[] histogramAxis;
	}
	delete[] flags;
	delete[] sFlags;
}
/*******************************************************************
*FUNCTION: void RFIFiltering::computeStatistics(int algorithmCode)
*int algorithmCode: algorithm to use 
*Wrapper to select algorithm to find central tendency and rms.
*******************************************************************/
void RFIFiltering::computeStatistics(int algorithmCode)
{
	switch(algorithmCode)
	{
		case 1:
			histogramBased();
			break;
		case 2:
			MADBased();
			break;
	}
}
/*******************************************************************
*FUNCTION: void RFIFiltering::quicksort(float* x,long int first,long int last)
*sorts a given array using quicksort algorithm.
/*******************************************************************/
void RFIFiltering::quicksort(float* x,long int first,long int last)
{
    int pivot,j,i;
    float temp;

     if(first<last){
         pivot=first;
         i=first;
         j=last;

         while(i<j){
             while(x[i]<=x[pivot]&&i<last)
                 i++;
             while(x[j]>x[pivot])
                 j--;
             if(i<j){
                 temp=x[i];
                  x[i]=x[j];
                  x[j]=temp;
             }
         }

         temp=x[pivot];
         x[pivot]=x[j];
         x[j]=temp;
         quicksort(x,first,j-1);
         quicksort(x,j+1,last);

    }
}
/*******************************************************************
*FUNCTION: void RFIFiltering::MADBased()
*Median Absolute Deviation (MAD) based algorithm to calculate
*mean and rms of the underlying gaussian from which the dataset 
*is derived. It uses median to estimate the central tendency and
*MAD to estimate the variance. 
*******************************************************************/
void RFIFiltering::MADBased()
{
	float *ptrInput=input;
	float *tempInput=new float[inputSize];
	float *ptrTempInput=tempInput;
	/*Copies the input array to another array for sorting.
	*This is done to avoid scrambling the original array 
	*which may be in use by other objects*/
	for(long int i=0;i<inputSize;i++,ptrInput++,ptrTempInput++)
		*ptrTempInput=*ptrInput;

	quicksort(tempInput,0,inputSize-1); //sorts the entire array
	//median is calculated
	if(inputSize%2==0)
		centralTendency=(tempInput[inputSize/2-1]+tempInput[inputSize/2])/2.0;
	else
		centralTendency=tempInput[inputSize/2];
	ptrTempInput=tempInput;
	//Deviations from the median is calculated and its median used to estimate rms
	for(long int i=0;i<inputSize;i++,ptrTempInput++)
		*ptrTempInput=fabs(*ptrTempInput-centralTendency);
	quicksort(tempInput,0,inputSize-1);
	
	if(inputSize%2==0)
		rms=(tempInput[inputSize/2-1]+tempInput[inputSize/2])/2.0;
	else
		rms=tempInput[inputSize/2];
	rms=rms*1.4826;
	cutoff=rms*cutoffToRms;
	delete[] tempInput;
}
/*******************************************************************
*FUNCTION: void RFIFiltering::histogramBased()
*This algorithm bins datapoints to form a histogram of size histogramSize.
*It then uses the modal point of the histogram to estimate the central 
*tendency and then uses it along with the width at half maximum to find
*variance.
*******************************************************************/
void RFIFiltering::histogramBased()
{	
	float *ptrInput,interval;
	int modeHeight,modeIndex;
	int p;
	interval=histogramInterval;
	histogramSize=(inputMax-inputMin)/(interval);
	histogramSize++;
	histogram=new float[histogramSize];
	histogramAxis=new float[histogramSize];
	for (int i=0;i<histogramSize;i++)
		 histogram[i]=0;
	ptrInput=&(input[0]);
	for(int i=0;i< inputSize;i++,ptrInput++)
		 histogram[(int)((*ptrInput-inputMin)/interval)]++; //Computing histogram


	modeHeight=0;
	modeIndex=0;
	//finding modal point
	for (int i=0;i<histogramSize;i++)
	{
		if(histogram[i]>modeHeight)
		{
			modeHeight=histogram[i];
			modeIndex=i;
		}
	}
	if(modeHeight==0) 
		return;
	histogramMax=modeHeight;
	p=0;	
	while(histogram[modeIndex+p]>=histogram[modeIndex]/2.0) //finding index of half on the right
		p++;
	int q=0;
	while(histogram[modeIndex+q]>=histogram[modeIndex]/2.0) //finding index of half on the left
		q--;
	
	centralTendency=(modeIndex+0.5)*interval+inputMin;
	/*rms=(p*interval)/(sqrt(2*log(histogram[modeIndex]/(float)histogram[modeIndex+p])));
	rms+=(p*interval)/(sqrt(2*log(histogram[modeIndex]/(float)histogram[modeIndex-p])));
	rms/=2.0;*/
	rms=(p-q)*interval/2.355;
	for (int i=0;i<histogramSize;i++)
		histogramAxis[i]=(i*interval+inputMin-centralTendency)/rms;
	
	cutoff=rms*cutoffToRms;
	histogramInterval=(4.0*rms)/(pow(inputSize,1/3.0));	
}
/*******************************************************************
*FUNCTION: void RFIFiltering::smoothFlags()
*Generates a smooth version of the flags
*******************************************************************/
void RFIFiltering::smoothFlags(int windowLength,float threshold)
{
	float* ptrSmoothFlags;
	char* ptrFlags;
	ptrFlags=flags;	 
	ptrSmoothFlags=sFlags;
	float sum;
	int count;
	int s=windowLength/2;
	
	for(int i=0;i< inputSize;i++,ptrFlags++,ptrSmoothFlags++)
	{
		sum=0;
		count=0;
	       for(int j=-s;j<=s && i+j<inputSize;j++)	
	       {
	       		if(i+j<0)
	       			continue;
	       		sum+=(*(ptrFlags+j));
	       		count++;
	       }
	       *ptrSmoothFlags=1.0-(sum/count);
	}
	ptrFlags=flags;	 
	ptrSmoothFlags=sFlags;
	for(int i=0;i< inputSize;i++,ptrFlags++,ptrSmoothFlags++)
	{
		if(*ptrSmoothFlags<threshold)
		{
		 	for(int j=-s;j<=s && i+j<inputSize;j++)	
	       		{
	       			if(i+j<0)
	       				continue;
	       			*(ptrFlags+j)=1;
	       		}
	       		
	       	}
	       
	}
	
}
/*******************************************************************
*FUNCTION: void RFIFiltering::generateManualFlags(int nBadChanBlocks,int* badChanBlocks)
*Flags blocks specified by user (usually blocks of channels)
*******************************************************************/
void RFIFiltering::generateManualFlags(int nBadChanBlocks,int* badChanBlocks,int offset)
{
	for(int i=0;i<nBadChanBlocks;i++)
	{		
		for(int j=badChanBlocks[i*2];j<badChanBlocks[i*2+1];j++)
		{	
			if(j-offset<0 or j-offset>=inputSize)	
				continue;	
			flags[j-offset]=1;
		}
	}
}
/*******************************************************************
*FUNCTION: void RFIFiltering::generateBlankFlags()
*In cases when an array containing blank flags (all zeroes,no flagging)
*is required by other functions.
*******************************************************************/
void RFIFiltering::generateBlankFlags()
{
	float* ptrInput;
	char* ptrFlags;
	ptrFlags=flags;	 
	for(int i=0;i< inputSize;i++,ptrFlags++)
	       *ptrFlags=0;	
}
/*******************************************************************
*FUNCTION: void RFIFiltering::generateBlankFlags()
*Flagging of data based on its deviation from central tendency being
*above a certain threshold.
*******************************************************************/
void RFIFiltering::flagData()
{
	float* ptrInput;
	char* ptrFlags;
	ptrInput=input;
	ptrFlags=flags;
	for(int i=0;i< inputSize;i++,ptrInput++,ptrFlags++)
	{
		if(fabs(*ptrInput-centralTendency)>cutoff)
			*ptrFlags=1;
	}
}
/*******************************************************************
*FUNCTION: void RFIFiltering::generateBlankFlags()
*Flagging of data based on its deviation from central tendency being
*above a certain threshold.
*******************************************************************/
/*void RFIFiltering::multiPointFlagData(float* multiCutoff)
{
	float* ptrInput;
	char* ptrFlags;
	char thisFlag=0;
	short last=3;
	ptrInput=input;
	ptrFlags=flags;
	for(int i=0;i< inputSize;i++,ptrFlags++)
		*ptrFlags=0;
	ptrFlags=flags;
	for(int i=0;i< inputSize;i++,ptrInput++,ptrFlags++)
	{
		last=3;
		if(i+last>inputSize)
			last=inputSize-i;
		for(int k=0;k<last;k++)
		{
			thisFlag=1;
			for(int p=0;p<k;p++)
				if(thisFlag && (fabs(*(ptrInput+p)-centralTendency)>multiCutoff[k]*rms))
					thisFlag=1;
				else
					thisFlag=0;
			for(int p=0;p<k;p++)
			{
				if(thisFlag)
					*(ptrFlags+p)=1;
			}
		}
	}

}*/
void RFIFiltering::multiPointFlagData(float* multiCutoff)
{
	float* ptrInput;
	char* ptrFlags;
	ptrInput=input;
	ptrFlags=flags;
	for(int i=0;i< inputSize;i++,ptrFlags++)
		*ptrFlags=0;
	ptrFlags=flags;
	for(int i=0;i< inputSize-1;i++,ptrInput++,ptrFlags++)
	{
		if(fabs(*(ptrInput)-centralTendency)>multiCutoff[1]*rms && fabs(*(ptrInput+1)-centralTendency)>multiCutoff[1]*rms)
			*(ptrFlags)=*(ptrFlags+1)=1;
	}

}

/*******************************************************************
*FUNCTION: void RFIFiltering::writeFlags(const char* filename)
*const char* filename: Name of file to which flags are written.
*******************************************************************/
void RFIFiltering::writeFlags(const char* filename)
{
	FILE* flagFile;    
    	flagFile = fopen(filename, "ab");
        fwrite(flags, sizeof(char), inputSize, flagFile);    
    	fclose(flagFile);    	
}

/*******************************************************************
*FUNCTION: void RFIFiltering::writeFlags(const char* filename)
*const char* filename: Name of file to which flags are written.
*char* startFlags: Initial flags at the beginning 
*int nStartFlags: number of flags (1s) to write before flags
*char* endFlags: flags appeneded at the end
*int nEndFlags: number of flags (1s) to write after flags
*This is used to write channel flag file where some channels
*at the start and the end have been marked bad by the user. 
*******************************************************************/
void RFIFiltering::writeFlags(const char* filename,char* startFlags,int nStartFlags,char* endFlags,int nEndFlags)
{
	FILE* flagFile;    
    	flagFile = fopen(filename, "ab");
	fwrite(startFlags, sizeof(char), nStartFlags, flagFile);    
        fwrite(flags, sizeof(char), inputSize, flagFile);  
	fwrite(endFlags, sizeof(char), nEndFlags, flagFile);     
    	fclose(flagFile);    	
}


//implementation of RFIFiltering methods ends


/*******************************************************************
CLASS: AdvancedAnalysis
*This is the class that does all the final analysis on the data.
*It has a routine to dedisperses the data and then collapses 
*the frequency channels creating a Dedispersed time series (fullDM). 
*It also performs folding of the data by using defualt fixed period
*or by using a polyco file (not yet implemented).
*******************************************************************/
class AdvancedAnalysis
{
	public:
		//Static variables:
		static Information 	info;			//contains all input parameters
		static int*		delayTable;		//An array that stores the delay of each frequency channel
		static int 		maxDelay;		//The maximum delay encountered
		static double		initPhasePolyco;	//Initial phase for polyco folding
		//The following three variables points to different arrays(variables), one for each polarization mode.
		static float** 		foldedProfile;		//Pointers to arrays containing the folded profiles
		static int**		countProfile;		//keeps track of number of data points in each bin	
		
		static double*		curPosMsStatic;		//Time upto which time series have been folded.
		static double		initLagMs;		//initial lag before folding is started.
		//The following three variables points to different arrays(variables), one for each polarization mode and are used to store unfiltered data.
		static float** 		foldedProfileUnfiltered;		//Pointers to arrays containing the folded profiles
		static int**		countProfileUnfiltered;			//keeps track of number of data points in each bin	

				
		//Variables:
		long int	blockIndex;		//Index of current block(window) being processed.
		int		polarIndex;		//Index of polarization to process. Decides foldedProfile, countProfile & curPosMsStatic indexes.
		double		curPosMs;		//Time upto which time series have been folded for the CURRENT polarization.
		int		polycoRowIndex;		//Row index of polycoTable for polyco based folding
		float*		rawData;		//2-D time frequency data to process
		long int 	length;			//length of time series to process
		float*		fullDM;			//Array that stores the dedispersed time series
		int*		count;			//Number of data points in each bin of fullDM array
		char*		dedispFlags;
		float*		excess;			//The excess from the previous block (details given later)
		int*		countExcess;		//Number of data points in each bin of excess array
		float*		curFoldedProfile;	//Mean Folded profile of the current polarization, upto the current window (used for plotting)
		char		hasEnoughDedispersedData; //Used to tell the folding routine when to start profile calculation.
		int		foldingStartIndex;	  ///Used to comunicate the start index of first block for which dedispersed data is available across all channels.
		//Maximum and minimum of the profile array and the fullDM array. Used for plotting.
		float 		maxProfile;		
		float 		minProfile;
		float		maxFullDM;
		float		minFullDM;
		
		//Following are the unfiltered counterpart of the above variables:

		float*		fullDMUnfiltered;			//Array that stores the dedispersed time series
		float*		excessUnfiltered;			//The excess from the previous block (details given later)
		int*		countUnfiltered;			//Number of data points in each bin of fullDM array
		int*		countExcessUnfiltered;		//Number of data points in each bin of excess array
		float*		curFoldedProfileUnfiltered;	//Mean Folded profile of the current polarization, upto the current window (used for plotting)
		float 		maxProfileUnfiltered;		
		float 		minProfileUnfiltered;
		float		maxFullDMUnfiltered;
		float		minFullDMUnfiltered;

		//Functions:
		AdvancedAnalysis(Information info_);	//Constructor for first intialization
		AdvancedAnalysis(long int blockIndex_,int polarIndex_,float* rawData_,long int length_); //constructor
		~AdvancedAnalysis();	//Destructor
		void calculateDelayTable();	//Calculates the delay table, a table containing shifts (in number of samples) of each channel.
		void calculateFullDM(char* timeFlag,char *freqFlag); //Calculates the dedispersed time series
		void calculateFullDM(short int* filteredRawData); //Calculates the dedispersed time series for replaced by median DM.
		void mergeExcess(float* excess_,int* countExcess_,float* excessUnfiltered_,int* countExcessUnfiltered_);
		void normalizeFullDM();
		void calculateProfile();	//Calculates the folded profile
		void writeProfile(const char* filename,const char* filenameUnfiltered);	//Writes out the folded profile
		void writeFullDM(const char* filename,const char* filenameUnfiltered);		//Writes out the dedispersed time series		
		void writeFullDMCount(const char*  filename);	//Writes out the number of samples in each dedispersed time series bin

		private:
		double calculateFixedPeriodPhase();		//Calculates phase of current sample for folding (based on a given fixed period)
		double calculatePolycoPhase();			//Calculates phase of current sample for folding (based on a polyCo file)
	
	
};

//implementation of advancedAnalysis methods begins

//Declaration of static variables.
Information AdvancedAnalysis::info;
int* AdvancedAnalysis::delayTable;
float** AdvancedAnalysis::foldedProfile;
int** AdvancedAnalysis::countProfile;
float** AdvancedAnalysis::foldedProfileUnfiltered;
int** AdvancedAnalysis::countProfileUnfiltered;
double* AdvancedAnalysis::curPosMsStatic;
int AdvancedAnalysis::maxDelay;
double AdvancedAnalysis::initLagMs=0.0;
double AdvancedAnalysis::initPhasePolyco=-1;
/*******************************************************************
*CONSTRUCTOR: AdvancedAnalysis::AdvancedAnalysis(Information _info)
*Information _info: All input parameters are contained in this object
*Computation of delay table and 
*initialization of static variables is done in this constructor
*******************************************************************/
AdvancedAnalysis::AdvancedAnalysis(Information info_)
{
	info=info_;
	delayTable=new int[info.noOfChannels];
	calculateDelayTable();
	foldedProfile=new float*[info.noOfPol];
	countProfile=new int*[info.noOfPol];
	curPosMsStatic=new double[info.noOfPol];
	
	foldedProfileUnfiltered=new float*[info.noOfPol];
	countProfileUnfiltered=new int*[info.noOfPol];
	//Initializing folded profiles and current positions, one for each polarization.
	for(int k=0;k<info.noOfPol;k++)
	{
		foldedProfile[k]=new float[info.periodInSamples];
		countProfile[k]=new int[info.periodInSamples];	
		curPosMsStatic[k]=0.0;
		
		foldedProfileUnfiltered[k]=new float[info.periodInSamples];
		countProfileUnfiltered[k]=new int[info.periodInSamples];	
		for(int i=0;i<info.periodInSamples;i++)
		{
			foldedProfile[k][i]=0;
			countProfile[k][i]=0;
			
			foldedProfileUnfiltered[k][i]=0;
			countProfileUnfiltered[k][i]=0;
		}
	}
	excess=NULL;
	countExcess=NULL;
	fullDM=NULL;
	count=NULL;
	curFoldedProfile=NULL;

	excessUnfiltered=NULL;
	countExcessUnfiltered=NULL;
	fullDMUnfiltered=NULL;
	countUnfiltered=NULL;
	curFoldedProfileUnfiltered=NULL;

	dedispFlags=NULL;
}
/*******************************************************************
*General comments about calculation of dedispersed time series:
*
*Dedispersion is done with respect to the lowest frequency.
*The lowest frequency is most delayed, hence positive delays must be
*added to all other channels to align them.
*
*While doing so the shifted position of some channels of 
*the time samples whose index is greater than length-maxDelay 
*will cross the last index of the fullDM array. These are then stored in
*the excess array and added with the dedispersed series of the
*next block. 
*******************************************************************/

/*******************************************************************
*CONSTRUCTOR: AdvancedAnalysis:AdvancedAnalysis(long int blockIndex_,int polarIndex_,unsigned short int* rawData_,long int length_,float* excess_,int* countExcess_)
*long int blockIndex_		:Index of current block(window) to process
*int polarIndex_		:Index of current polarization to process
*float* rawData_		:2-D raw data.
*long int length_		:length of time sample to dedisperse
*float* excess_			:excess from the previous block
*int* countExcess_		:number of data points in each bin of excess_ array
*******************************************************************/
AdvancedAnalysis::AdvancedAnalysis(long int blockIndex_,int polarIndex_,float* rawData_,long int length_)
{
	length=length_;	
	rawData=rawData_;
	blockIndex=blockIndex_;
	polarIndex=polarIndex_;	
	fullDM=new float[length+maxDelay];
	excess=new float[maxDelay];
	count=new int[length+maxDelay];
	countExcess=new int[maxDelay];
	curFoldedProfile=new float[info.periodInSamples];
	dedispFlags=new char[length];

	fullDMUnfiltered=new float[length+maxDelay];
	excessUnfiltered=new float[maxDelay];
	countUnfiltered=new int[length+maxDelay];
	countExcessUnfiltered=new int[maxDelay];
	curFoldedProfileUnfiltered=new float[info.periodInSamples];
	hasEnoughDedispersedData=1;
	foldingStartIndex=0;

	float* ptrFullDM=fullDM;
	float* ptrExcess=excess;
	int* ptrCount=count;
	int* ptrCountExcess=countExcess;
	char* ptrDedispFlags=dedispFlags;

	float* ptrFullDMUnfiltered=fullDMUnfiltered;
	float* ptrExcessUnfiltered=excessUnfiltered;
	int* ptrCountUnfiltered=countUnfiltered;
	int* ptrCountExcessUnfiltered=countExcessUnfiltered;
	
	//intializing fullDM and count
	for(int i=0;i<length+maxDelay;i++,ptrFullDM++,ptrCount++,ptrFullDMUnfiltered++,ptrCountUnfiltered++)
		*ptrFullDM=*ptrCount=*ptrFullDMUnfiltered=*ptrCountUnfiltered=0;
	
	//intializing dedispersion flag
	for(int i=0;i<length;i++,ptrDedispFlags++)
		*ptrDedispFlags=0;
	
	//excess_==NULL implies that this is the first block to get dedispersed	

	
	//copying of excess of last block to fullDM of this block
	/*
	memcpy(fullDM,excess_,maxDelay*sizeof(float));
	memcpy(count,countExcess_,maxDelay*sizeof(int));

	memcpy(fullDMUnfiltered,excessUnfiltered_,maxDelay*sizeof(float));
	memcpy(countUnfiltered,countExcessUnfiltered_,maxDelay*sizeof(int));
	*/
}
/*******************************************************************
*DESTRUCTOR: AdvancedAnalysis::~AdvancedAnalysis()
*frees up used memory.
*******************************************************************/
AdvancedAnalysis::~AdvancedAnalysis()
{
	delete[] curFoldedProfile;	
	delete[] fullDM;
	delete[] count;
	delete[] excess;
	delete[] countExcess;
	delete[] curFoldedProfileUnfiltered;	
	delete[] fullDMUnfiltered;
	delete[] countUnfiltered;
	delete[] excessUnfiltered;
	delete[] countExcessUnfiltered;
	delete[] dedispFlags;
	

}
/*******************************************************************
*FUNCTION: void AdvancedAnalysis::calculateDelayTable()
*Calculates the amount of shift of each channel with respect to the
*lowest frequency. This shift corrects for the smearing of pulse
*that occurs when the signal travels through interstellar plasma.
*Note that this function is called only once and the table calculated
*is stored in a static array delayTable.
*******************************************************************/
void AdvancedAnalysis::calculateDelayTable()
{
	int j;	
	float lowestFrequency=info.lowestFrequency;
	float KDM=4.148808*pow(10,3)*info.dispersionMeasure;
	float freqInterval=(info.bandwidth)/(float)(info.noOfChannels);
	lowestFrequency+=freqInterval/2.0;	
	
	for(int i=0;i<info.noOfChannels;i++)
	{
		if(info.sidebandFlag==1)	//if sidebandFlag is one(zero) then the first(last) channel is of the lowest frequency 
			j=i;
		else
			j=info.noOfChannels-i-1;
		if(info.refFrequency==0)
			delayTable[i]=(int)(((1.0/pow(lowestFrequency,2))-(1.0/pow(lowestFrequency+j*freqInterval,2)))*KDM/info.samplingInterval+0.5);
		else
			delayTable[i]=(int)((-(1.0/pow(lowestFrequency+j*freqInterval,2))+(1.0/pow(lowestFrequency+info.bandwidth,2)))*KDM/info.samplingInterval+0.5);
			
	}
	if(info.refFrequency==0)
	{
		if(info.sidebandFlag==1)
			maxDelay=delayTable[info.noOfChannels-1];
		else
			maxDelay=delayTable[0];	
	}
	else
	{
		if(info.sidebandFlag==0)
			maxDelay=-delayTable[info.noOfChannels-1];
		else
			maxDelay=-delayTable[0];
		for(int i=0;i<info.noOfChannels;i++)
			delayTable[i]+=maxDelay;
	}	
}
/*******************************************************************
*FUNCTION: AdvancedAnalysis::calculateFullDM(char* timeFlags,char* freqFlags)
*char* timeFlags	:Time samples marked 1 are ignored (or clipped).
*char* freqFlags	:Channels marked 1 are ignored (or clipped).
*Calculates the dedispersed time series.
*If time or channel or both filtering are turned off the corresponding
*arrays have all 0.
*******************************************************************/
void AdvancedAnalysis::calculateFullDM(char* timeFlags,char* freqFlags)
{
	
	float* ptrRawData=rawData;
	char* ptrTimeFlags=timeFlags;
	char* ptrFreqFlags;
	long int pos;

	int startChannel=info.startChannel;
	int stopChannel=info.stopChannel;
	int totalChan=info.noOfChannels;
	int endExclude=info.noOfChannels-stopChannel;

	for(long int i=0;i<length;i++,ptrTimeFlags++)
	{
		ptrRawData+=startChannel;
		ptrFreqFlags=freqFlags;	
			for(int j=startChannel;j<stopChannel;j++,ptrRawData++,ptrFreqFlags++)
			{

				pos=i+delayTable[j];	//shift to correct for dispersion.
				if(!(*ptrFreqFlags)&!(*ptrTimeFlags))
				{
					fullDM[pos]+=(*ptrRawData);
					count[pos]++;
				}
				else
				{
					fullDMUnfiltered[pos]+=(*ptrRawData);
					countUnfiltered[pos]++;
				}
			}
		
		ptrRawData+=endExclude;
	}
	float* ptrFullDM=fullDM;
	float* ptrFullDMUnfiltered=fullDMUnfiltered;
	int* ptrCount=count;
	int* ptrCountUnfiltered=countUnfiltered;
	

	for(int i=0;i<length+maxDelay;i++,ptrFullDM++,ptrFullDMUnfiltered++,ptrCount++,ptrCountUnfiltered++)
	{

		(*ptrFullDMUnfiltered)+=(*ptrFullDM);
		(*ptrCountUnfiltered)+=(*ptrCount);		
	}        

}
/*******************************************************************
*FUNCTION: AdvancedAnalysis::calculateFullDM(unsigned short int* filteredRawData)
*short int* filteredRawData - filtered raw data
*Calculates the dedispersed time series using the filtered raw data.
*******************************************************************/
void AdvancedAnalysis::calculateFullDM(short int* filteredRawData)
{
	

	
	long int pos;

	int startChannel=info.startChannel;
	int stopChannel=info.stopChannel;
	int totalChan=info.noOfChannels;
	int endExclude=info.noOfChannels-stopChannel;
	float* ptrRawData=rawData;
	short int* ptrFilteredRawData=filteredRawData;
	for(long int i=0;i<length;i++)
	{
			ptrFilteredRawData+=startChannel;	
			ptrRawData+=startChannel;	
			for(int j=startChannel;j<stopChannel;j++,ptrRawData++,ptrFilteredRawData++)
			{

				pos=i+delayTable[j];	//shift to correct for dispersion.
				
				fullDM[pos]+=(*ptrFilteredRawData);
				count[pos]++;
				
				
				fullDMUnfiltered[pos]+=(*ptrRawData)*info.meanval;
				countUnfiltered[pos]++;
			}
			ptrRawData+=endExclude;
			ptrFilteredRawData+=endExclude;
			
	}
}
void AdvancedAnalysis::mergeExcess(float* excess_,int* countExcess_,float* excessUnfiltered_,int* countExcessUnfiltered_)
{
	float* ptrFullDM=fullDM;
	float* ptrFullDMUnfiltered=fullDMUnfiltered;
	int* ptrCount=count;
	int* ptrCountUnfiltered=countUnfiltered;
	float* ptrExcess=excess_;
	float* ptrExcessUnfiltered=excessUnfiltered_;
	int* ptrCountExcess=countExcess_;
	int* ptrCountExcessUnfiltered=countExcessUnfiltered_;
	if(excess_)
	{
		for(int i=0;i<maxDelay;i++,ptrFullDM++,ptrFullDMUnfiltered++,ptrExcess++,ptrExcessUnfiltered++,ptrCount++,ptrCountUnfiltered++,ptrCountExcess++,ptrCountExcessUnfiltered++)
		{
			(*ptrFullDM)+=(*ptrExcess);
			(*ptrFullDMUnfiltered)+=(*ptrExcessUnfiltered);
			(*ptrCount)+=(*ptrCountExcess);
			(*ptrCountUnfiltered)+=(*ptrCountExcessUnfiltered);		
		}
	}
	memcpy(excess,&fullDM[length],maxDelay*sizeof(float));
	memcpy(excessUnfiltered,&fullDMUnfiltered[length],maxDelay*sizeof(float));
	memcpy(countExcess,&count[length],maxDelay*sizeof(int));	
	memcpy(countExcessUnfiltered,&countUnfiltered[length],maxDelay*sizeof(int));
}
void AdvancedAnalysis::normalizeFullDM()
{
	float* ptrFullDM;
	float* ptrFullDMUnfiltered;
	int* ptrCount;
	int* ptrCountUnfiltered;
	int nChan= info.stopChannel-info.startChannel;
	double samplingIntervalMs=info.samplingInterval*1000.0;
		/*******************************************************************
		*The following is the logic for folding to start. Folding is started
		*only when the pulse ariving on all channels is guaranteed, i.e
		*when maxDelay number of samples have elapsed. If the window-length
		*is less than maxDelay then maxDelay/length number of windows must
		*be skipped.
		*******************************************************************/
		if(length<maxDelay && ((blockIndex+1)*length<maxDelay))
		{	
			hasEnoughDedispersedData=0;	
			initLagMs+=length*samplingIntervalMs;		
		}
		/*******************************************************************
		*The if clause determines if this is the first block for which
		*folding will be performed. In case window(block) length is greater
		*than maxDelay then it suffices to check if blockIndex==0.
		*if length is lesser than maxDelay then the logic checks if it is
		*the first block to process.
		*******************************************************************/
		else if(blockIndex==0 || (length<maxDelay && int((blockIndex+1)*length/maxDelay)==1 && (blockIndex+1)*length%maxDelay<length))
		{		
			/*******************************************************************
			*maxDelay%length number of samples in the first block to process is
			*skipped to ensure that atleast maxDelay number of time samples have
			*been recorded since the start of observation.
							*******************************************************************/			
			foldingStartIndex=maxDelay%length;	
			initLagMs+=samplingIntervalMs*foldingStartIndex;
		}
	
	/*******************************************************************
	*The dedispersed time series is averaged over the number of samples 
	*added in each bin. Minimum and maximum of the series is found.
	*These are used while plotting the series.
	*******************************************************************/
	ptrFullDM=fullDM;
	ptrFullDMUnfiltered=fullDMUnfiltered;
	ptrCount=count;
	ptrCountUnfiltered=countUnfiltered;
	minFullDM=minFullDMUnfiltered=33554434;
	maxFullDM=maxFullDMUnfiltered=0;
	char* ptrDedispFlags=dedispFlags;
	
	for(long int i=0;i<length;i++,ptrFullDM++,ptrCount++,ptrDedispFlags++,ptrFullDMUnfiltered++,ptrCountUnfiltered++)
	{	


		if(*ptrCount==0)
		{
			*ptrDedispFlags=1;
			*ptrFullDM=0;
			continue;
		}
		else
		{
			(*ptrFullDM)/=(*ptrCount);		
			if((*ptrCount)<=0.4*nChan)
				*ptrDedispFlags=1;
			else
			{
							
				if(*ptrFullDM>maxFullDM)
					maxFullDM=*ptrFullDM;
				if(*ptrFullDM<minFullDM)
					minFullDM=*ptrFullDM;
			}
		}

		if(*ptrCountUnfiltered==0)
		{
			*ptrFullDMUnfiltered=0;
		}
		else
		{
			(*ptrFullDMUnfiltered)/=(*ptrCountUnfiltered);
			if(*ptrFullDMUnfiltered>maxFullDMUnfiltered)
				maxFullDMUnfiltered=*ptrFullDMUnfiltered;
			if(*ptrFullDMUnfiltered<minFullDMUnfiltered)
				minFullDMUnfiltered=*ptrFullDMUnfiltered;
		}

		
		
	}
	
}
/*******************************************************************
*FUNCTION: double AdvancedAnalysis::calculatePolycoPhase()
*returns double phase: folding phase for current time sample
*between 0 and 1.
*computes the phase of the current time sample using a polyco table.
*******************************************************************/
double AdvancedAnalysis::calculatePolycoPhase()
{
	
	//Check the time of the current sample wrt polyco start time (dt)	
	double dt = (initLagMs+curPosMs)/60000.0+ (double)((info.MJDObs - info.polycoTable[polycoRowIndex*(3+info.nCoeffPolyco)+0]) * 1440);
	//Logic to check if the time difference has crossed the span of current set.
	
	while(dt > info.spanPolyco/2.0)
  	{
    		dt -= info.spanPolyco;
    		polycoRowIndex += 1;
  	}
  	//Find the folding phase for the current time
  	double* polycoSet=&(info.polycoTable[polycoRowIndex*(3+info.nCoeffPolyco)]);
  	double phase = (polycoSet[1] + (dt*60*polycoSet[2])); 	//Ref phase + 0th order frequency
  	for(int i=0;i<info.nCoeffPolyco;i++)  	
    		phase += polycoSet[3+i] * pow(dt,i);
  	
  	
  	double F_Temp;

	//The following statement ensures that the first sample of the time series 
	//goes to the first profile bin.
  	if(initPhasePolyco == -1)  	
  		initPhasePolyco = modf( (modf(phase , &F_Temp)+ 1.0) , &F_Temp);
	//Calculates the fractional phase w.r.t the intial phase
  	phase -= initPhasePolyco;
  	double frac_phase = modf((modf(phase , &F_Temp)+ 1.0) , &F_Temp) ;  	
  	return frac_phase;
}
/*******************************************************************
*FUNCTION: double AdvancedAnalysis::calculateFixedPeriodPhase()
*returns double phase: folding phase for current time sample
*between 0 and 1.
*******************************************************************/
double AdvancedAnalysis::calculateFixedPeriodPhase()
{
	double periodInMs=info.periodInMs;
	int noOfPeriods=(int)floor(curPosMs/periodInMs);
	return (curPosMs/periodInMs)-noOfPeriods;
}
/*******************************************************************
*FUNCTION: void AdvancedAnalysis::calculateProfile()
*Folds the dedispersed time series to get the pulsar profile.
*******************************************************************/
void AdvancedAnalysis::calculateProfile()
{
	//if hasEnoughDedispersedData=0 then folding must not begin, see comments in calculateFullDM() function.
	if(hasEnoughDedispersedData==0)
		return;
	int periodInSamples=info.periodInSamples;
	double offset=info.profileOffset;
	double samplingIntervalMs=info.samplingInterval*1000.0;
	double phase;
	char* ptrDedispFlags=&dedispFlags[foldingStartIndex];	
	float* ptrFullDM=&fullDM[foldingStartIndex];
	float* ptrFullDMUnfiltered=&fullDMUnfiltered[foldingStartIndex];
	int noOfPeriods,index;
	//finds the appropiate static variables for given polarization (polarIndex)
	
	float* thisFoldedProfile,*thisFoldedProfileUnfiltered;
	int* thisCountProfile,*thisCountProfileUnfiltered;	
	

	thisFoldedProfile=foldedProfile[polarIndex];
	thisCountProfile=countProfile[polarIndex];
	curPosMs=curPosMsStatic[polarIndex];
	if(curPosMs==0.0)
		cout<<endl<<"Started folding after a lag of "<<initLagMs<<" ms"<<endl;
	thisFoldedProfileUnfiltered=foldedProfileUnfiltered[polarIndex];
	thisCountProfileUnfiltered=countProfileUnfiltered[polarIndex];


	if(!info.doFixedPeriodFolding)
		polycoRowIndex= info.polycoRowIndex; 	

	for(long int i=foldingStartIndex;i<length;i++,ptrFullDM++,ptrFullDMUnfiltered++,ptrDedispFlags++,curPosMs+=samplingIntervalMs)
	{	
		
		if(info.doFixedPeriodFolding)	
			phase=calculateFixedPeriodPhase()+offset;
		else
			phase=calculatePolycoPhase()+offset;
		if(*ptrDedispFlags)
			continue;
		if(phase>=1)
			phase=phase-1;	
		//logic to convert phase to sample index with given offset.
		index=(int)(phase*periodInSamples+0.5);	
		if(index==periodInSamples)
			index=0;
		thisFoldedProfile[index]+=(*ptrFullDM);
		thisFoldedProfileUnfiltered[index]+=(*ptrFullDMUnfiltered);
		thisCountProfileUnfiltered[index]++;
		thisCountProfile[index]++;
		
		
		
	}

	curPosMsStatic[polarIndex]=curPosMs;
	//Calculates the folded profile generate till the current block. This is used to plot.
	minProfile=maxProfile=thisFoldedProfile[(int)(offset*periodInSamples)+1]/thisCountProfile[(int)(offset*periodInSamples)+1]; //The slightly non-intuitive index is given here to ensure that countprofile is not zero, which can be the case if the first element was used to initialize the minimum. Such a situation can arise in the intial blocks with a non-zero phase offset.
	minProfileUnfiltered=maxProfileUnfiltered=thisFoldedProfileUnfiltered[(int)(offset*periodInSamples)+1]/thisCountProfile[(int)(offset*periodInSamples)+1]; 
	float* ptrFoldedProfile=thisFoldedProfile;
	float* ptrCurFoldedProfile=curFoldedProfile;
	int* ptrCountProfile=thisCountProfile;
	float* ptrFoldedProfileUnfiltered=thisFoldedProfileUnfiltered;
	float* ptrCurFoldedProfileUnfiltered=curFoldedProfileUnfiltered;
	int* ptrCountProfileUnfiltered=thisCountProfileUnfiltered;
	for(int i=0;i<periodInSamples;i++,ptrFoldedProfile++,ptrCurFoldedProfile++,ptrCountProfile++,ptrFoldedProfileUnfiltered++,ptrCurFoldedProfileUnfiltered++,ptrCountProfileUnfiltered++)
	{
		if((*ptrCountProfile)!=0)
		{		
			*ptrCurFoldedProfile=(float)((*ptrFoldedProfile)/(*ptrCountProfile));
			if(*ptrCurFoldedProfile>maxProfile)
				maxProfile=*ptrCurFoldedProfile;
			if(*ptrCurFoldedProfile<minProfile)
				minProfile=*ptrCurFoldedProfile;
		}
		else
			*ptrCurFoldedProfile=0;
		if((*ptrCountProfileUnfiltered)!=0)
		{		
			*ptrCurFoldedProfileUnfiltered=(float)((*ptrFoldedProfileUnfiltered)/(*ptrCountProfileUnfiltered));
			if(*ptrCurFoldedProfileUnfiltered>maxProfileUnfiltered)
				maxProfileUnfiltered=*ptrCurFoldedProfileUnfiltered;
			if(*ptrCurFoldedProfileUnfiltered<minProfileUnfiltered)
				minProfileUnfiltered=*ptrCurFoldedProfileUnfiltered;
		}
		else
			*ptrCurFoldedProfileUnfiltered=0;
	}
}
/*******************************************************************
*FUNCTION: void AdvancedAnalysis::writeFullDM(const char*  filename,const char* filenameUnfiltered)
*const char*  fileName: Filename to write filtered fullDM series to.
*const char*  fileNameUnfiltered: Filename to write unfiltered fullDM series to.
*Writes out the dedispersed time series to a files
*******************************************************************/
void AdvancedAnalysis::writeFullDM(const char*  filename,const char* filenameUnfiltered)
{
	FILE* fullDMFile;    
    fullDMFile = fopen(filename, "ab");
    fwrite(&fullDM[foldingStartIndex], sizeof(float), length-foldingStartIndex, fullDMFile);    
    fclose(fullDMFile); 
    
    fullDMFile = fopen(filenameUnfiltered, "ab");
    fwrite(&fullDMUnfiltered[foldingStartIndex], sizeof(float), length-foldingStartIndex, fullDMFile);    
    fclose(fullDMFile); 
}

/*******************************************************************
*FUNCTION: void AdvancedAnalysis::writeFullDMCount(const char*  filename)
*const char*  fileName: Filename to write to.
*Writes out the bin count of dedispersed time series to a file
*******************************************************************/
void AdvancedAnalysis::writeFullDMCount(const char*  filename)
{
	FILE* fullDMCountFile;    
    fullDMCountFile = fopen(filename, "ab");
    fwrite(&count[foldingStartIndex], sizeof(int), length-foldingStartIndex, fullDMCountFile);    
    fclose(fullDMCountFile); 
}
/*******************************************************************
*FUNCTION: void AdvancedAnalysis::writeProfile(const char*  filename,const char* filenameUnfiltered)
*const char*  fileName: Filename to write filtered profile to.
*const char*  fileNameUnfiltered: Filename to write unfiltered profile to.
*Writes out the folded profiles to files.
*******************************************************************/
void AdvancedAnalysis::writeProfile(const char*  filename,const char* filenameUnfiltered)
{
	
	ofstream profileFile,profileFileUnfiltered;
	profileFile.open(filename);
	profileFile<<"#phase\tvalue"<<endl;
	for(int i=0;i<info.periodInSamples;i++)
	{
		profileFile<<i/float(info.periodInSamples)<<"\t"<<setprecision(20)<<" "<<curFoldedProfile[i]<<endl;
	}


	profileFileUnfiltered.open(filenameUnfiltered);
	profileFileUnfiltered<<"#phase\tvalue"<<endl;
	for(int i=0;i<info.periodInSamples;i++)
	{
		profileFileUnfiltered<<i/float(info.periodInSamples)<<"\t"<<setprecision(20)<<" "<<curFoldedProfileUnfiltered[i]<<endl;
	}
	
}

//implementation of AdvancedAnalysis methods ends

class Plot
{
	public:
		static float*		profileAxis;
		static float* 		DMAxis;
		static float* 		timeAxis;
		static float*		bandshapeAxis;
		static Information	info;	
		static int		totalBlocks;
		Plot(Information info_,int totalBlocks_);
		void plotProfile(float *profile,float maxProfile,float minProfile,int k);
		void plotProfileUnfiltered(float *profile,float maxProfile,float minProfile);
		void plotFullDM(float *fullDM,float flMin,float flMax,int length,int k);
		void plotZeroDM(float *zeroDM,float zrMax,float zrMin,int length,int k);
		void plotBandshape(float *bandshape,float maxBandshape,float minBandshape,int k);
		void plotWaterfall(float* rawData,long int blockLength,float maxRawData,float minRawData);
		void plotFullDMCount(int* count,long int blockLength);
		void plotOtherBandshape(float *meanToRmsBandshape,float maxMeanToRmsBandshape,float minMeanToRmsBandshape,int k);
		void plotFullDMUnweighted(float* fullDM,int *count,long int length);
		void plotHistogram(float* histogram,float* histogramAxis,float histogramSize,float histMax);
		void plotChanFlags(char* chanFlag,int length);
		void plotTimeFlags(char* chanFlag,int length);
		void plotSmoothFlags(float* sFlags,long int blockLength);
		void plotDedispFlags(char* dedispFlags,int length);	
		void plotPolarLegend();

		void plotZeroDMSingleChannel(float *zeroDM,float zrMax,float zrMin,int length);
		void plotTimeFlagsSingleChannel(char* timeFlag,int length);
		void plotProfileSingleChannel(float *profile,float maxProfile,float minProfile);
		void plotProfileUnfilteredSingleChannel(float *profile,float maxProfile,float minProfile);	
		
		void plotAll(BasicAnalysis** basicAnalysis,AdvancedAnalysis** advancedAnalysis,RFIFiltering** rFIFilteringTime,RFIFiltering** rFIFilteringChan,int index);
		void plotAllNonPolar(BasicAnalysis* basicAnalysis,AdvancedAnalysis* advancedAnalysis,RFIFiltering* rFIFilteringTime,RFIFiltering* rFIFilteringChan,int index);
		void plotAllPolar(BasicAnalysis** basicAnalysis,AdvancedAnalysis** advancedAnalysis,int index);
		void plotAllSingleChannel(BasicAnalysis* basicAnalysis,AdvancedAnalysis* advancedAnalysis,RFIFiltering* rFIFilteringTime,RFIFiltering* rFIFilteringChan,int index);
		void plotBlockIndex(int index);
		void plotTitle();
		
		void foldingStartMessage();
};
//Implementation of Plot methods begins
float*		Plot::profileAxis;
float* 		Plot::DMAxis;
float* 		Plot::timeAxis;
float*		Plot::bandshapeAxis;
Information	Plot::info;
int		Plot::totalBlocks;

Plot::Plot(Information info_,int totalBlocks_)
{
	info=info_;
	totalBlocks=totalBlocks_;
	profileAxis=new float[info.periodInSamples];
	for(int i=0;i<info.periodInSamples;i++)
		profileAxis[i]=i;
	DMAxis=new float[info.blockSizeSamples+1];
	for(long int i=0;i<info.blockSizeSamples+1;i++)
		DMAxis[i]=i;
	timeAxis=new float[info.blockSizeSamples+1];
	for(long int i=0;i<info.blockSizeSamples+1;i++)
		timeAxis[i]=i*info.samplingInterval*1000;
	bandshapeAxis=new float[info.stopChannel-info.startChannel];
	for(long int i=info.startChannel;i<info.stopChannel;i++)
		bandshapeAxis[i-info.startChannel]=i;
	cpgbeg(0, "?", 1, 1);
	cpgpap (0.0,0.618); //10.0
	cpgsch(0.85);
	if(info.doManualMode)
		cpgask(1);
	else
		cpgask(0);
}


void Plot::plotProfile(float *profile,float maxProfile,float minProfile,int k)
{ 
	cpgsci(1);
	if(!info.doPolarMode || (info.doPolarMode && info.polarChanToDisplay!=-1) )
		cpgsvp(0.68,0.98,0.07,0.22);		
	else
	{
		switch(k)
		{
			case 0:
				cpgsvp(0.65,0.98,0.07,0.17);	
				break;
			case 2:
				cpgsvp(0.65,0.98,0.18,0.28);	
				break;	
			case 1:
				cpgsvp(0.65,0.88,0.86,0.96);
				break;
			case 3:
				cpgsvp(0.65,0.88,0.75,0.85);
				break;	
		}	
	}

	cpgswin(0,info.periodInSamples,minProfile-(0.1)*(maxProfile-minProfile),maxProfile+(0.1)*(maxProfile-minProfile));
 
	if(!info.doPolarMode || (info.doPolarMode && info.polarChanToDisplay!=-1))
	{		
		cpgbox("BNST",0.0,0,"BCNSTV",0.0,0);
		cpgtext(info.periodInSamples/3.7,minProfile-(maxProfile-minProfile)*0.6,"Filtered Profile (bin index)");
		cpgsci(k);
	}
	else
	{
		switch(k)
		{
			case 0:
				cpgbox("BNST",0.0,0,"BCNVST",0.0,2);
				cpgtext(info.periodInSamples/2.7,minProfile-(maxProfile-minProfile)*0.75,"Profile");
				break;
			case 2:		
				cpgbox("BST",0.0,0,"BCNVST",0.0,2);
				break;
			case 1:
				cpgbox("BST",0.0,0,"BCNVST",0.0,2);
				cpgtext(info.periodInSamples/2.7,minProfile+(maxProfile-minProfile)*1.2,"Profile");
				break;
			case 3:
				cpgbox("BNST",0.0,0,"BCNVST",0.0,2);				
				break;
		}
		cpgsci(7-k);	
	}
	
	cpgline(info.periodInSamples,profileAxis,profile);
}

void Plot::plotProfileUnfiltered(float *profile,float maxProfile,float minProfile)
{ 
	cpgsci(1);	
	cpgsvp(0.68,0.98,0.80,0.95);		
	cpgswin(0,info.periodInSamples,minProfile-(0.1)*(maxProfile-minProfile),maxProfile+(0.1)*(maxProfile-minProfile));
 	cpgbox("BNST",0.0,0,"BCNSTV",0.0,0);
	cpgtext(info.periodInSamples/4.0,maxProfile+(maxProfile-minProfile)*0.3,"Unfiltered Profile (bin index)");
	cpgline(info.periodInSamples,profileAxis,profile);
}

void Plot::plotFullDM(float *fullDM,float flMax,float flMin,int length,int k)
{
  	cpgsci(1); 

	
	if(!info.doPolarMode || (info.doPolarMode && info.polarChanToDisplay!=-1))
	{
		cpgsvp(0.07,0.60,0.80,0.95);  
	}
	else
	{		
		if(k==0 || k==2)
			cpgsvp(0.07,0.55,0.75,0.85);  
		
		else
			cpgsvp(0.07,0.55,0.86,0.96); 		
	}
	cpgswin(0,info.blockSizeSamples+1, flMin-(0.1)*(flMax-flMin),flMax+(0.3)*(flMax-flMin));
	if(!info.doPolarMode || (info.doPolarMode && info.polarChanToDisplay!=-1))
	{
		cpgbox("BNST",0.0,0,"BCNSTV",0.0,0);
		cpgtext((info.blockSizeSamples+1)/6.0,flMin-(flMax-flMin)*0.57,"Dedispersed time series (Time sample index)");
		cpgsci(k);		
	}
	else
	{
		
		if(k==0 || k==2)
			cpgbox("BNST",0.0,0,"BCNSTV",0.0,0);				
		else
		{
			cpgbox("BST",0.0,0,"BCNSTV",0.0,0);
			cpgtext(info.blockSizeSamples/3.0,flMin+(flMax-flMin)*1.2,"Dedispersed time series");			
		}
		cpgsci(7-k);
	}
	cpgline(length,DMAxis,fullDM);
}

void Plot::plotOtherBandshape(float *bandshape,float maxBandshape,float minBandshape,int k)
{
  	cpgsci(1);  	
   	cpgsvp(0.88,0.98,0.27,0.72);
   	if(info.sidebandFlag)
  		cpgswin(minBandshape-0.1*(maxBandshape-minBandshape),maxBandshape+0.1*(maxBandshape-minBandshape),info.startChannel,info.stopChannel);
 	 else
		cpgswin(minBandshape-0.1*(maxBandshape-minBandshape),maxBandshape+0.1*(maxBandshape-minBandshape),info.stopChannel,info.startChannel);
  	cpgbox("BNCT",0.0,0,"CST",0.0,0);
  	cpgline(info.stopChannel-info.startChannel,&bandshape[info.startChannel],bandshapeAxis);
}

void Plot::plotZeroDM(float *zeroDM,float zrMax,float zrMin,int length,int k)
{
  	cpgsci(1);
	if(!info.doPolarMode || (info.doPolarMode && info.polarChanToDisplay!=-1))
  		cpgsvp(0.07,0.60,0.07,0.19);
	else
	{
		if(k==0 || k==2)
			cpgsvp(0.07,0.55,0.07,0.17);
		else
			cpgsvp(0.07,0.55,0.18,0.28);
			
	}
  	cpgswin(0,info.blockSizeSec*1000.0,zrMin-0.1*(zrMax-zrMin),zrMax+0.1*(zrMax-zrMin));
	if(!info.doPolarMode || (info.doPolarMode && info.polarChanToDisplay!=-1))
	{
  		cpgbox("BNST",0.0,0,"BCNSTV",0.0,0);
  		cpgmtxt("B",2.5,0.5,0.5,"zero DM time series (Time in ms)");
  		cpgsci(k);
	}
	else
	{
		if(k==0 || k==2)
		{
			cpgbox("BNST",0.0,0,"BCNSTV",0.0,0);
			cpgtext(info.blockSizeSamples/3.0,zrMin-(zrMax-zrMin)*0.8,"zero DM time series (Time in ms)");	
		}
		else		
			cpgbox("BST",0.0,0,"BCNSTV",0.0,0);  			
		
		cpgsci(7-k);
	}
  	cpgline(length,timeAxis,zeroDM);
}

void Plot::plotBandshape(float *bandshape,float maxBandshape,float minBandshape,int k)
{
	cpgsci(1);
	if(!info.doPolarMode || (info.doPolarMode && info.polarChanToDisplay!=-1))
  		cpgsvp(0.68,0.88,0.27,0.72);
	else
	{
		if(k==0 || k==2)
			cpgsvp(0.6,0.78,0.34,0.70);
		else
			cpgsvp(0.80,0.98,0.34,0.70);
	}
  	if(info.sidebandFlag)
  		cpgswin(minBandshape-0.1*(maxBandshape-minBandshape),maxBandshape+0.1*(maxBandshape-minBandshape),info.startChannel,info.stopChannel);
  	else
		cpgswin(minBandshape-0.1*(maxBandshape-minBandshape),maxBandshape+0.1*(maxBandshape-minBandshape),info.stopChannel,info.startChannel);
	if(!info.doPolarMode || (info.doPolarMode && info.polarChanToDisplay!=-1))
	{
		cpgbox("BMCST",0.0,0,"BNST",0.0,0);
  		cpglab("", "Channel number", "");
  		if(info.sidebandFlag==0)
  			cpgtext((maxBandshape-minBandshape)/2.0,info.stopChannel+0.1*(info.stopChannel-info.startChannel),"Bandshape");
  		else
  			cpgtext((maxBandshape-minBandshape)/2.0,info.startChannel-0.1*(info.stopChannel-info.startChannel),"Bandshape");
  		
		cpgsci(k);
	}
	else
	{
		if(k==0 || k==2)
		{
			cpgbox("BNCST",0.0,0,"BNST",0.0,0);
			cpgptxt(minBandshape+(maxBandshape-minBandshape)*1.1,info.startChannel+(info.stopChannel-info.startChannel)*0.35,90,0,"Bandshape");				
		}
		else
		{
			cpgbox("BNCST",0.0,0,"BST",0.0,0);  
					
		}
		
		cpgsci(7-k);
	}  
  	
  	cpgline(info.stopChannel-info.startChannel,&bandshape[info.startChannel],bandshapeAxis);
}

void Plot::plotPolarLegend()
{
	cpgsvp(0.90,0.98,0.72,0.96);	
	cpgswin(0,100,0,100);
	
	cpgsci(7);
	float timeX[2]={0,25};
	float timeY[2]={25,25};	
	cpgtext(30,timeY[0]-2,"Self A");
	cpgline(2,timeX,timeY);		

	cpgsci(5);
	timeY[0]=50;
	timeY[1]=timeY[0];	
	cpgtext(30,timeY[0]-2,"Self B");
	cpgline(2,timeX,timeY);	

	cpgsci(6);
	timeY[0]=75;
	timeY[1]=timeY[0];
	cpgtext(30,timeY[0]-2,"Cross A");
	cpgline(2,timeX,timeY);	

	cpgsci(4);
	timeY[0]=100;
	timeY[1]=timeY[0];	
	cpgtext(30,timeY[0]-2,"Cross B");
	cpgline(2,timeX,timeY);		
	
}
void Plot::plotWaterfall(float* rawData,long int blockLength,float maxRawData,float minRawData)
{

	/*float RL[]={-0.5, 0.0, 0.17, 0.33,  0.5, 0.67, 0.83, 1.0, 1.7};
	float RR[]={ 0.0, 0.0,  0.0,  0.0,  0.6,  1.0,  1.0, 1.0, 1.0};
	float RG[]={ 0.0, 0.0,  0.0,  1.0,  1.0,  1.0,  0.6, 0.0, 1.0};
	float RB[]={ 0.0, 0.3,  0.8,  1.0,  0.3,  0.0,  0.0, 0.0, 1.0};

	float contra = 1,bright = 0.5;
	cpgctab(RL, RR, RG, RB, 9, contra, bright);*/
	cpgsfs(1);
	if(!info.doPolarMode || (info.doPolarMode && info.polarChanToDisplay!=-1))
		cpgsvp(0.07,0.60,0.27,0.72);
	else
		cpgsvp(0.07,0.55,0.34,0.70);
	cpgswin(0, info.blockSizeSec*1000.0, info.lowestFrequency+info.startChannel*info.bandwidth/(float)info.noOfChannels,info.lowestFrequency+info.stopChannel*info.bandwidth/(float)info.noOfChannels);
	if(!info.sidebandFlag)	
		cpgswin(0, info.blockSizeSec*1000.0, info.lowestFrequency+(info.noOfChannels-info.stopChannel)*info.bandwidth/(float)info.noOfChannels,info.lowestFrequency+(info.noOfChannels-info.startChannel)*info.bandwidth/(float)info.noOfChannels);
	
  
	cpgbox("BCINST",0.0,0,"BCINST",0.0,0);
	cpglab("", "Frequency (Mhz)", "");
	float tr[]={0,0, info.samplingInterval*1000.0,info.lowestFrequency,info.bandwidth/(float)info.noOfChannels,0};
	if(!info.sidebandFlag)    
	{	
		tr[3]=info.lowestFrequency+info.bandwidth;
		tr[4]*=-1;
	}
 	cpgimag(rawData,info.noOfChannels,blockLength,info.startChannel+1,info.stopChannel,1,blockLength,minRawData,maxRawData,tr);
 
}
void Plot::plotFullDMCount(int* count,long int blockLength)
{

	/*float RL[]={-0.5, 0.0, 0.17, 0.33,  0.5, 0.67, 0.83, 1.0, 1.7};
	float RR[]={ 0.0, 0.0,  0.0,  0.0,  0.6,  1.0,  1.0, 1.0, 1.0};
	float RG[]={ 0.0, 0.0,  0.0,  1.0,  1.0,  1.0,  0.6, 0.0, 1.0};
	float RB[]={ 0.0, 0.3,  0.8,  1.0,  0.3,  0.0,  0.0, 0.0, 1.0};

	float contra = 1,bright = 0.5;
	cpgctab(RL, RR, RG, RB, 9, contra, bright);*/
	cpgsfs(1);
	cpgsvp(0.07,0.60,0.93,0.945);	
	cpgswin( 0,info.blockSizeSamples+1,1,2);
	
  
	float tr[]={0,0, 1,0,1,0};
	float* countfl=new float[blockLength];
	for(int i=0;i<blockLength;i++)
		countfl[i]=count[i];
	
 	cpgimag(countfl,1,blockLength,1,1,1,blockLength,0,info.noOfChannels,tr);
	delete[] countfl;
 
}
void Plot::plotHistogram(float* histogram,float* histogramAxis,float histogramSize,float histMax)
{
	float histMin=0;
  	cpgsvp(0.68,0.98,0.77,0.92);
  	cpgswin(-8,+15,histMin-(0.1)*(histMax-histMin),histMax+(0.1)*(histMax-histMin));
  	cpgbox("BNST",0.0,0,"BCNSTV",0.0,0);
  	cpgline(histogramSize,histogramAxis,histogram);
}

void Plot::plotChanFlags(char* chanFlag,int length)
{
 	cpgsvp(0.87,0.8825,0.27,0.72);
        if(info.sidebandFlag)  
        	cpgswin(0,1,0,info.stopChannel-info.startChannel);
	else
		cpgswin(0,1,info.stopChannel-info.startChannel,0);
        cpgrect(0,1,0,info.stopChannel-info.startChannel);
	float timeY[2];
	float timeX[2]={0,1};	
        for(int i=0;i<length;i++)
        {
          	if(chanFlag[i]==1)
          	{
            		timeY[0]=i;
            		timeY[1]=timeY[0];
            		cpgsci(0);
            		cpgline(2,timeX,timeY);
            		cpgsci(1);
          	}
        }
       
}

void Plot::plotTimeFlags(char* timeFlag,int length)
{
	if(info.smoothFlagWindowLength>0)
 		cpgsvp(0.07,0.60,0.20,0.21);
	else
		cpgsvp(0.07,0.60,0.20,0.22);
        cpgswin(0,info.blockSizeSamples+1,0, 1);
        cpgrect(0,length,0,1);
	float timeX[2];
	float timeY[2]={0,1};
        for(long int i=0;i<length;i++)
        {
          	if(timeFlag[i]==1)
          	{
            		timeX[0]=i;
            		timeX[1]=timeX[0];
            		cpgsci(0);
            		cpgline(2,timeX,timeY);
            		cpgsci(1);
          	}
        }
}

void Plot::plotSmoothFlags(float* sFlags,long int blockLength)
{

	/*float RL[]={-0.5, 0.0, 0.17, 0.33,  0.5, 0.67, 0.83, 1.0, 1.7};
	float RR[]={ 0.0, 0.0,  0.0,  0.0,  0.6,  1.0,  1.0, 1.0, 1.0};
	float RG[]={ 0.0, 0.0,  0.0,  1.0,  1.0,  1.0,  0.6, 0.0, 1.0};
	float RB[]={ 0.0, 0.3,  0.8,  1.0,  0.3,  0.0,  0.0, 0.0, 1.0};

	float contra = 1,bright = 0.5;
	cpgctab(RL, RR, RG, RB, 9, contra, bright);*/
	cpgsfs(1);
	cpgsvp(0.07,0.60,0.215,0.24);	
	cpgswin( 0,info.blockSizeSamples+1,1,2);
	
  
	float tr[]={0,0, 1,0,1,0};
	
 	cpgimag(sFlags,1,blockLength,1,1,1,blockLength,0,1,tr);
 
}
void Plot::plotDedispFlags(char* dedispFlags,int length)
{
 	cpgsvp(0.07,0.60,0.95,0.954);
        cpgswin(0,info.blockSizeSamples+1,0, 1);
        cpgrect(0,length,0,1);
	float timeX[2];
	float timeY[2]={0,1};
        for(long int i=0;i<length;i++)
        {
          	if(dedispFlags[i]==1)
          	{
            		timeX[0]=i;
            		timeX[1]=timeX[0];
            		cpgsci(0);
            		cpgline(2,timeX,timeY);
            		cpgsci(1);
          	}
        }
}

void Plot::plotBlockIndex(int index)
{
	cpgsci(1);
	cpgsvp(0.0,0.40,0.90,1.00);
	cpgswin(0,100,0,100);
	ostringstream text1;
	if(info.doReadFromFile)
		text1<<"Block: "<<info.startBlockIndex+index+1<<"/"<<totalBlocks<<" | Time: "<<setiosflags(ios::fixed) << setprecision(1)<<(info.startBlockIndex+index)*info.blockSizeSec<<"/"<<setiosflags(ios::fixed) << setprecision(1)<<(totalBlocks-1)*info.blockSizeSec<<" sec";
	else
		text1<<"Block: "<<index+1<<" | Time: "<<setiosflags(ios::fixed) << setprecision(1)<<(index+1)*info.blockSizeSec<<" sec";

	cpgtext(0,80,text1.str().c_str());

}

void Plot::plotTitle()
{
	cpgsci(8);
	cpgsvp(0.40,0.65,0.90,1.00);
	cpgswin(0,100,0,100);
	ostringstream text1;
	text1<<"GMRT Pulsar Tool ver 4.2.1";
	cpgtext(0,80,text1.str().c_str());
	cpgsci(1);

}

void Plot::foldingStartMessage()
{
	ostringstream text1;
	cpgsvp(0.68,0.98,0.80,0.95);		
	cpgswin(0,100,0,100);	
	text1<<"Profile folding will start once sufficient time";
	cpgtext(0,80,text1.str().c_str());

	ostringstream text2;
	text2<<"has elapsed so that all channels are";
	cpgtext(0,65,text2.str().c_str());

	ostringstream text3;
	text3<<"available to each dedispersed series bin.";
	cpgtext(0,50,text3.str().c_str());
	

}

//Single Channel mode methods
void Plot::plotZeroDMSingleChannel(float *zeroDM,float zrMax,float zrMin,int length)
{
  	cpgsci(1);
	cpgsvp(0.05,0.95,0.55,0.9);
  	cpgswin(0,info.blockSizeSec*1000.0,zrMin-0.1*(zrMax-zrMin),zrMax+0.1*(zrMax-zrMin));
  	cpgbox("BNST",0.0,0,"BCNSTV",0.0,0);
  	cpgmtxt("B",2.5,0.5,0.5,"Time series (Time in ms)");
  	cpgline(length,timeAxis,zeroDM);
}

void Plot::plotTimeFlagsSingleChannel(char* timeFlag,int length)
{
	cpgsvp(0.05,0.95,0.9,0.93);
        cpgswin(0,info.blockSizeSamples+1,0, 1);
        cpgrect(0,length,0,1);
	float timeX[2];
	float timeY[2]={0,1};
        for(long int i=0;i<length;i++)
        {
          	if(timeFlag[i]==1)
          	{
            		timeX[0]=i;
            		timeX[1]=timeX[0];
            		cpgsci(0);
            		cpgline(2,timeX,timeY);
            		cpgsci(1);
          	}
        }
}

void Plot::plotProfileSingleChannel(float *profile,float maxProfile,float minProfile)
{ 
	cpgsci(3);	
	cpgsvp(0.05,0.45,0.1,0.45);		
	cpgswin(0,info.periodInSamples,minProfile-(0.1)*(maxProfile-minProfile),maxProfile+(0.1)*(maxProfile-minProfile));
 	cpgbox("BCNST",0.0,0,"BCNSTV",0.0,0);
	cpgtext(info.periodInSamples/3.7,minProfile-(maxProfile-minProfile)*0.3,"Filtered Profile (bin index)");
	cpgline(info.periodInSamples,profileAxis,profile);
}


void Plot::plotProfileUnfilteredSingleChannel(float *profile,float maxProfile,float minProfile)
{ 
	cpgsci(1);	
	cpgsvp(0.5,0.95,0.1,0.45);		
	cpgswin(0,info.periodInSamples,minProfile-(0.1)*(maxProfile-minProfile),maxProfile+(0.1)*(maxProfile-minProfile));
 	cpgbox("BCNST",0.0,0,"BCNSTV",0.0,0);
	cpgtext(info.periodInSamples/3.7,minProfile-(maxProfile-minProfile)*0.3,"Unfiltered Profile (bin index)");
	cpgline(info.periodInSamples,profileAxis,profile);
}



void Plot::plotAllNonPolar(BasicAnalysis* basicAnalysis,AdvancedAnalysis* advancedAnalysis,RFIFiltering* rFIFilteringTime,RFIFiltering* rFIFilteringChan,int index)
{
	cpgpage();	
	if(advancedAnalysis->hasEnoughDedispersedData!=0)
	{		
		plotProfile(advancedAnalysis->curFoldedProfile,advancedAnalysis->maxProfile,advancedAnalysis->minProfile,3);
		cpgsci(1);
		plotProfileUnfiltered(advancedAnalysis->curFoldedProfileUnfiltered,advancedAnalysis->maxProfileUnfiltered,advancedAnalysis->minProfileUnfiltered);
	}	
	else
	{
		foldingStartMessage();
	}
	plotFullDM(advancedAnalysis->fullDMUnfiltered,advancedAnalysis->maxFullDMUnfiltered,advancedAnalysis->minFullDM,advancedAnalysis->length,1);
	plotFullDM(advancedAnalysis->fullDM,advancedAnalysis->maxFullDMUnfiltered,advancedAnalysis->minFullDM,advancedAnalysis->length,3);
	plotZeroDM(basicAnalysis->zeroDMUnfiltered,basicAnalysis->maxZeroDM,basicAnalysis->minZeroDM,basicAnalysis->blockLength,1);
	plotZeroDM(basicAnalysis->zeroDM,basicAnalysis->maxZeroDM,basicAnalysis->minZeroDM,basicAnalysis->blockLength,3);
	cpgsci(1);
	plotTimeFlags(rFIFilteringTime->flags,rFIFilteringTime->inputSize);
	if(info.smoothFlagWindowLength>0)
		plotSmoothFlags(rFIFilteringTime->sFlags,rFIFilteringTime->inputSize);
	plotDedispFlags(advancedAnalysis->dedispFlags,advancedAnalysis->length);
	plotFullDMCount(advancedAnalysis->count,advancedAnalysis->length);
	plotBandshape(basicAnalysis->bandshape,basicAnalysis->maxBandshape,basicAnalysis->minBandshape,1);
	
	
	if((int)info.bandshapeToUse==3 || (int)info.bandshapeToUse==4)
		plotOtherBandshape(basicAnalysis->meanToRmsBandshape,basicAnalysis->maxMeanToRmsBandshape,basicAnalysis->minMeanToRmsBandshape,1);
	else if((int)info.bandshapeToUse==2)
		plotOtherBandshape(basicAnalysis->normalizedBandshape,1.5,0.5,1);
	plotChanFlags(rFIFilteringChan->flags,rFIFilteringChan->inputSize);


	if((int)info.bandshapeToUse==2 || info.doUseNormalizedData)
		plotBandshape(basicAnalysis->smoothBandshape,basicAnalysis->maxBandshape,basicAnalysis->minBandshape,3);
		cpgsci(1);	
	
	plotWaterfall(basicAnalysis->rawData,basicAnalysis->blockLength,(basicAnalysis->maxZeroDM),(basicAnalysis->minZeroDM));
	/*if(info.timeFlagAlgo==1 && info.doTimeFlag)
		plotHistogram(rFIFilteringTime->histogram,rFIFilteringTime->histogramAxis,rFIFilteringTime->histogramSize,rFIFilteringTime->histogramMax);
	*/
	plotTitle();
	plotBlockIndex(index);
}

void Plot::plotAllPolar(BasicAnalysis** basicAnalysis,AdvancedAnalysis** advancedAnalysis,int index)
{
	cpgpage();	
	float minFullDM,maxFullDM,minProfile,maxProfile,maxRangeProfile,minBandshape,maxBandshape,maxZeroDM,minZeroDM;

	minFullDM=advancedAnalysis[0]->minFullDM < advancedAnalysis[2]->minFullDM?advancedAnalysis[0]->minFullDM:advancedAnalysis[2]->minFullDM;
	maxFullDM=advancedAnalysis[0]->maxFullDM > advancedAnalysis[2]->maxFullDM?advancedAnalysis[0]->maxFullDM:advancedAnalysis[2]->maxFullDM;
	
	minBandshape=basicAnalysis[0]->minBandshape < basicAnalysis[2]->minBandshape?basicAnalysis[0]->minBandshape:basicAnalysis[2]->minBandshape;
	maxBandshape=basicAnalysis[0]->maxBandshape > basicAnalysis[2]->maxBandshape?basicAnalysis[0]->maxBandshape:basicAnalysis[2]->maxBandshape;

	minZeroDM=basicAnalysis[0]->minZeroDM < basicAnalysis[2]->minZeroDM?basicAnalysis[0]->minZeroDM:basicAnalysis[2]->minZeroDM;
	maxZeroDM=basicAnalysis[0]->maxZeroDM > basicAnalysis[2]->maxZeroDM?basicAnalysis[0]->maxZeroDM:basicAnalysis[2]->maxZeroDM;

	for(int i=0;i<info.noOfPol;i+=2)
	{
		if(advancedAnalysis[i]->hasEnoughDedispersedData!=0)
		{			
			plotProfile(advancedAnalysis[i]->curFoldedProfile,advancedAnalysis[i]->maxProfile,advancedAnalysis[i]->minProfile,i);
		}	
		plotFullDM(advancedAnalysis[i]->fullDM,maxFullDM,minFullDM,advancedAnalysis[i]->length,i);
		plotZeroDM(basicAnalysis[i]->zeroDM,maxZeroDM,minZeroDM,basicAnalysis[i]->blockLength,i);
		plotBandshape(basicAnalysis[i]->bandshape,maxBandshape,minBandshape,i);
		cpgsci(1);		
	}

	minFullDM=advancedAnalysis[1]->minFullDM < advancedAnalysis[3]->minFullDM?advancedAnalysis[1]->minFullDM:advancedAnalysis[3]->minFullDM;
	maxFullDM=advancedAnalysis[1]->maxFullDM > advancedAnalysis[3]->maxFullDM?advancedAnalysis[1]->maxFullDM:advancedAnalysis[3]->maxFullDM;
	
	
	minBandshape=basicAnalysis[1]->minBandshape < basicAnalysis[3]->minBandshape?basicAnalysis[1]->minBandshape:basicAnalysis[3]->minBandshape;
	maxBandshape=basicAnalysis[1]->maxBandshape > basicAnalysis[3]->maxBandshape?basicAnalysis[1]->maxBandshape:basicAnalysis[3]->maxBandshape;

	minZeroDM=basicAnalysis[1]->minZeroDM < basicAnalysis[3]->minZeroDM?basicAnalysis[1]->minZeroDM:basicAnalysis[3]->minZeroDM;
	maxZeroDM=basicAnalysis[1]->maxZeroDM > basicAnalysis[2]->maxZeroDM?basicAnalysis[1]->maxZeroDM:basicAnalysis[3]->maxZeroDM;

	for(int i=1;i<info.noOfPol;i+=2)
	{
		if(advancedAnalysis[i]->hasEnoughDedispersedData!=0)
		{			
			plotProfile(advancedAnalysis[i]->curFoldedProfile,advancedAnalysis[i]->maxProfile,advancedAnalysis[i]->minProfile,i);
		}	
		plotFullDM(advancedAnalysis[i]->fullDM,maxFullDM,minFullDM,advancedAnalysis[i]->length,i);
		plotZeroDM(basicAnalysis[i]->zeroDM,maxZeroDM,minZeroDM,basicAnalysis[i]->blockLength,i);
		plotBandshape(basicAnalysis[i]->bandshape,maxBandshape,minBandshape,i);
		cpgsci(1);		
	}

	plotWaterfall(basicAnalysis[0]->rawData,basicAnalysis[0]->blockLength,(basicAnalysis[0]->maxZeroDM),(basicAnalysis[0]->minZeroDM));
	plotTitle();
	plotBlockIndex(index);
	plotPolarLegend();
}
void Plot::plotAllSingleChannel(BasicAnalysis* basicAnalysis,AdvancedAnalysis* advancedAnalysis,RFIFiltering* rFIFilteringTime,RFIFiltering* rFIFilteringChan,int index)
{
	cpgpage();
	plotTitle();
	plotBlockIndex(index);	
	plotZeroDMSingleChannel(basicAnalysis->zeroDMUnfiltered,basicAnalysis->maxZeroDM,basicAnalysis->minZeroDM,basicAnalysis->blockLength);
	plotTimeFlagsSingleChannel(rFIFilteringTime->flags,rFIFilteringTime->inputSize);
	plotProfileSingleChannel(advancedAnalysis->curFoldedProfile,advancedAnalysis->maxProfile,advancedAnalysis->minProfile);
	cpgsci(1);
	plotProfileUnfilteredSingleChannel(advancedAnalysis->curFoldedProfileUnfiltered,advancedAnalysis->maxProfileUnfiltered,advancedAnalysis->minProfileUnfiltered);
}
void Plot::plotAll(BasicAnalysis** basicAnalysis,AdvancedAnalysis** advancedAnalysis,RFIFiltering** rFIFilteringTime,RFIFiltering** rFIFilteringChan,int index)
{
	if(info.noOfChannels==1)
	{
		plotAllSingleChannel(basicAnalysis[0],advancedAnalysis[0],rFIFilteringTime[0],rFIFilteringChan[0],index);
	}
	else if(!info.doPolarMode)
		plotAllNonPolar(basicAnalysis[0],advancedAnalysis[0],rFIFilteringTime[0],rFIFilteringChan[0],index);
	else if(info.doPolarMode && info.polarChanToDisplay!=-1)
		plotAllNonPolar(basicAnalysis[info.polarChanToDisplay],advancedAnalysis[info.polarChanToDisplay],rFIFilteringTime[info.polarChanToDisplay],rFIFilteringChan[info.polarChanToDisplay],index);
	else
		plotAllPolar(basicAnalysis,advancedAnalysis,index);
	plotBlockIndex(index);
	
		
}

//implementation of Plot methods ends
//Benchmark variables
double timeReadData=0.0;
double timeConvertToFloat=0.0;
double timeBandshape=0.0;
double timeNormalization=0.0;
double timeZeroDM=0.0;
double timeRFITimeStats=0.0;
double timeRFITimeFlags=0.0;
double timeRFITimeFlagsWrite=0.0;
double timeRFIChanStats=0.0;
double timeRFIChanFlag=0.0;
double timeRFIChanFlagsWrite=0.0;
double timeFullDMUnfilteredCalc=0.0;
double timeFullDMCalc=0.0;
double timeFullDMUnfilteredWrite=0.0;
double timeFullDMWrite=0.0;
double timeProfileCalc=0.0;
double timeProfileUnfilteredCalc=0.0;
double timePlot=0.0;
double timeThread1=0.0;
double timeThread2=0.0;
double timeThread3=0.0;
double timeThread4=0.0;
double timeThread5=0.0;
double timeThread6=0.0;
//End benchmark variables



char *startFlags;
char *endFlags;

static bool keepRunning = true;
class ThreadPacket
{
	public:
	int noOfPol;
	BasicAnalysis** 	basicAnalysis;
	AquireData*		aquireData;
	BasicAnalysis** 	basicAnalysisWrite;
	RFIFiltering** 		rFIFilteringTime;
	RFIFiltering** 		rFIFilteringChan;	
	AdvancedAnalysis** 	advancedAnalysis;
	AdvancedAnalysis** 	advancedAnalysisOld;
	ThreadPacket(int noOfPol_);
	~ThreadPacket();
	void copy(ThreadPacket* threadPacket);
	void copySelect(ThreadPacket* threadPacket);
	void freeMem();
};
ThreadPacket::ThreadPacket(int noOfPol_)
{
	noOfPol=noOfPol_;
	aquireData=NULL;
	basicAnalysis=new BasicAnalysis*[noOfPol];
	basicAnalysisWrite=new BasicAnalysis*[noOfPol];
	rFIFilteringChan=new RFIFiltering*[noOfPol];
	rFIFilteringTime=new RFIFiltering*[noOfPol];
	advancedAnalysis=new AdvancedAnalysis*[noOfPol];
	advancedAnalysisOld=new AdvancedAnalysis*[noOfPol];
	for(int k=0;k<noOfPol;k++)
	{
		basicAnalysis[k]=NULL;
		basicAnalysisWrite[k]=NULL;
		rFIFilteringTime[k]=NULL;
		rFIFilteringChan[k]=NULL;
		advancedAnalysis[k]=NULL;
		advancedAnalysisOld[k]=NULL;
	}
}
ThreadPacket::~ThreadPacket()
{
	if(aquireData!=NULL)
		delete aquireData;
	delete[] basicAnalysis;
	delete[] basicAnalysisWrite;
	delete[] advancedAnalysis;
	delete[] advancedAnalysisOld;
	delete[] rFIFilteringChan;
	delete[] rFIFilteringTime;
}
void ThreadPacket::copy(ThreadPacket* threadPacket)
{
	aquireData=threadPacket->aquireData;
	for(int k=0;k<noOfPol;k++)
	{
		basicAnalysis[k]=threadPacket->basicAnalysis[k];
		rFIFilteringTime[k]=threadPacket->rFIFilteringTime[k];
		rFIFilteringChan[k]=threadPacket->rFIFilteringChan[k];
		advancedAnalysis[k]=threadPacket->advancedAnalysis[k];
	}
}
void ThreadPacket::copySelect(ThreadPacket* threadPacket)
{
	for(int k=0;k<noOfPol;k++)
	{
		basicAnalysisWrite[k]=threadPacket->basicAnalysis[k];
		rFIFilteringTime[k]=threadPacket->rFIFilteringTime[k];
		rFIFilteringChan[k]=threadPacket->rFIFilteringChan[k];
		advancedAnalysis[k]=threadPacket->advancedAnalysis[k];
		advancedAnalysisOld[k]=threadPacket->advancedAnalysisOld[k];
	}
}
void ThreadPacket::freeMem()
{
	
		
	for(int i=0;i<noOfPol;i++)
	{

		if(basicAnalysisWrite[i]!=NULL)
			delete basicAnalysisWrite[i];
		if(advancedAnalysisOld[i]!=NULL)
			delete advancedAnalysisOld[i];
		if(rFIFilteringChan[i]!=NULL)
			delete rFIFilteringChan[i];
		if(rFIFilteringTime[i]!=NULL)
			delete rFIFilteringTime[i];

		basicAnalysisWrite[i]=NULL;		
		advancedAnalysisOld[i]=NULL;
		rFIFilteringChan[i]=NULL;
		rFIFilteringTime[i]=NULL;
	}

}

class Runtime
{
	public:
	Information info;
	int blockIndex;
	int totalBlocks;
	Plot* plot;
	char* blankTimeFlags;
	char* blankChanFlags;
	float* histogramInterval;
	char readDoneFlag;
	char readCompleteFlag;
	ThreadPacket** threadPacket;
	
	/**variables for inline mode of gptool**/
	Correlator* shmInterface;
	int nbuff;
	
	Runtime(Information info_,int nThreadMultiplicity_);
	~Runtime();
	void intializeFiles();
	void fillPipe();
	void loopThrough();
	void closePipe();
	void quickclosePipe();
	void action(int threadPacketIndex,int actionIndex);
	private:
	int nActions;
	int nThreadMultiplicity,nThreadMultiplicityTemp;
	char chanFirst;
	char hasReachedEof;
	
	void writeAll(ThreadPacket* threadPacket);
	void testStatistics(ThreadPacket* threadPacket);
	void displayBlockIndex(int blockIndex);
	void ioTasks(int threadPacketIndex);
	void plotTasks(int threadPacketIndex);
	void channelTasks(int threadPacketIndex);
	void timeTasks(int threadPacketIndex);
	void fullDMTask(int threadPacketIndex);
	void floatConversionTasks(int threadPacketIndex);
	void writeFlagStats(ThreadPacket* threadPacket);
	void copyToSHM(ThreadPacket* threadPacket);
};
Runtime::Runtime(Information info_,int nThreadMultiplicity_)
{
	nActions=5;
	nThreadMultiplicity=nThreadMultiplicity_;
	nThreadMultiplicityTemp=nThreadMultiplicity;
	info=info_;
	AquireData* aquireData=new AquireData(info);	
	if(!info.doReadFromFile)
	{
		aquireData->initializeSHM();
		if(info.isInline)
		{
			shmInterface=new Correlator(aquireData->dataHdr,aquireData->dataBuffer);
			shmInterface->initializeWriteSHM();
			nbuff=aquireData->nbuff;
		}
	}
	info.fillParams();
	if(info.isInline)
	{
		info.blockSizeSamples=(aquireData->info).blockSizeSamples;
		info.blockSizeSec=(aquireData->info).blockSizeSec;
	}
	AquireData::info=info;
	AquireData::curPos=long((info.startTime/info.samplingInterval))*info.noOfChannels*info.noOfPol* info.sampleSizeBytes;	
	AquireData::info.startTime=long(info.startTime/info.blockSizeSec)*info.blockSizeSec;
	info.display();
	threadPacket=new ThreadPacket*[nActions*nThreadMultiplicity];
	for(int i=0;i<nActions*nThreadMultiplicity;i++)
	{
		threadPacket[i]=new ThreadPacket(info.noOfPol);
	}

	histogramInterval=new float[info.noOfPol];
	int totalBlocksNoOff=0;
	if(info.doReadFromFile)
	{
		double totalTime=(AquireData::eof*info.samplingInterval)/(info.noOfChannels*info.sampleSizeBytes*info.noOfPol);		
		if(info.startTime>totalTime)
		{
			cout<<endl<<endl<<"File contains "<<totalTime<<" seconds of data. Please give a starting time less than that"<<endl;
			exit(0);
		}
		double extraOffset=info.startTime*1000.0/info.periodInMs;
		extraOffset=(extraOffset-floor(extraOffset));
		info.profileOffset+=extraOffset;
		if(info.profileOffset>=1.000)
			info.profileOffset-=1.00;
		totalBlocks=ceil((totalTime-info.startTime)/(info.blockSizeSec));
		totalBlocksNoOff=ceil(totalTime/(info.blockSizeSec));
	}
	else
		totalBlocks=totalBlocksNoOff=0;
	
	if(info.smoothFlagWindowLength>0)
		info.smoothFlagWindowLength=(int)(info.smoothFlagWindowLength/info.samplingInterval);
	info.concentrationThreshold=1.0-info.concentrationThreshold/100.0;
	if(!info.doFilteringOnly)
		plot=new Plot(info,totalBlocksNoOff);
	else
		plot=NULL;
	
	for(int k=0;k<info.noOfPol;k++)
	{
		if(!info.doFilteringOnly)
			threadPacket[(nActions-1)*nThreadMultiplicity]->advancedAnalysisOld[k]=new AdvancedAnalysis(info);
		threadPacket[nActions-1]->basicAnalysis[k]=new BasicAnalysis(info);	
	}

	blankTimeFlags=new char[info.blockSizeSamples+1];
	blankChanFlags=new char[info.stopChannel-info.startChannel];

	for(int i=0;i<info.blockSizeSamples+1;i++)
		blankTimeFlags[i]=0;
	
	for(int i=0;i<info.stopChannel-info.startChannel;i++)
		blankChanFlags[i]=0;
	
	blockIndex=0;
	chanFirst=0;
	if((info.doTimeFlag && info.doChanFlag && (info.flagOrder==1)) || info.doUseNormalizedData ||info.doChanFlag)
		chanFirst=1;
	//cout<<"Reached end of Runtime()"<<endl; //DEBUG
	//omp_set_num_threads(2+3*nThreadMultiplicity);
	
	//omp_set_dynamic(0);
}
Runtime::~Runtime()
{
	delete[] threadPacket;
	if(plot!=NULL)
		delete plot;
	delete[] blankTimeFlags;
	delete[] blankChanFlags;
	delete[] histogramInterval;
}
void Runtime::displayBlockIndex(int blockIndex)
{

	if(info.doReadFromFile)
			cout<<'\r'
	<<"Block:"<<(blockIndex)-nActions*nThreadMultiplicity+1<<" of "<<totalBlocks<<"                                  "<<std::flush;		
		else
			cout<<'\r'
	<<"Block:"<<(blockIndex)-nActions*nThreadMultiplicity+1<<"                                  "<<std::flush;

}
void Runtime::testStatistics(ThreadPacket* threadPacket) 
{
	for(int k=0;k<info.noOfPol;k++)
	{
		double rmsPreFlag,meanPreFlag,rmsPostFlag,meanPostFlag;
		float* ptrZeroDM;
		float* ptrZeroDMUnfiltered;
		char* ptrFlags;
		int count;
		int l=threadPacket->basicAnalysisWrite[k]->blockLength;
		ptrZeroDM=threadPacket->basicAnalysisWrite[k]->zeroDM;
		ptrZeroDMUnfiltered=threadPacket->basicAnalysisWrite[k]->zeroDMUnfiltered;
		ptrFlags=threadPacket->rFIFilteringTime[k]->flags;
			meanPreFlag=rmsPreFlag=meanPostFlag=rmsPostFlag=0.0;
			count=0;
		for(int i=0;i<l;i++,ptrZeroDM++,ptrZeroDMUnfiltered++,ptrFlags++)
		{
			meanPreFlag+=(*ptrZeroDMUnfiltered);
			rmsPreFlag+=((*ptrZeroDMUnfiltered)*(*ptrZeroDMUnfiltered));
			if(!(*ptrFlags))
			{
				meanPostFlag+=(*ptrZeroDM);
				rmsPostFlag+=(*ptrZeroDM)*(*ptrZeroDM);
				count++;
			}
		}
		meanPreFlag/=l;
		rmsPreFlag=sqrt((rmsPreFlag/l)-(meanPreFlag*meanPreFlag));
		meanPostFlag/=count;
		rmsPostFlag=sqrt((rmsPostFlag/count)-(meanPostFlag*meanPostFlag));
		ofstream statFile;
		if(info.doPolarMode)
		{
			ostringstream filename;		
			filename<<"stats"<<k+1<<".gpt";
			statFile.open(filename.str().c_str(),ios::app);
		}
		else
			statFile.open("stats.gpt",ios::app);
		statFile<<blockIndex-3<<"\t"<<threadPacket->rFIFilteringTime[k]->centralTendency<<"\t"<<meanPreFlag<<"\t"<<meanPostFlag<<"\t";
		statFile<<threadPacket->rFIFilteringTime[k]->rms<<"\t"<<rmsPreFlag<<"\t"<<rmsPostFlag<<"\t";	
		statFile<<(threadPacket->rFIFilteringTime[k]->centralTendency)/(threadPacket->rFIFilteringTime[k]->rms)<<"\t"<<meanPreFlag/rmsPreFlag<<"\t"<<meanPostFlag/rmsPostFlag<<endl;
		statFile.close();
	}
}
void Runtime::writeFlagStats(ThreadPacket* threadPacket)
{
	for(int k=0;k<info.noOfPol;k++)
	{
		ofstream statFile;
		if(info.doPolarMode)
		{
			ostringstream filename;		
			filename<<"flag_stats"<<k+1<<".gpt";
			statFile.open(filename.str().c_str(),ios::app);
		}
		else
			statFile.open("flag_stats.gpt",ios::app);

		char* ptrTimeFlags=threadPacket->rFIFilteringTime[k]->flags;
		char* ptrChanFlags=threadPacket->rFIFilteringChan[k]->flags;
		int l=threadPacket->basicAnalysisWrite[k]->blockLength;
		float timePercent;
		timePercent=0;
		for(int i=0;i<l;i++,ptrTimeFlags++)		
			timePercent+=*ptrTimeFlags;
		timePercent*=100.0/l;
		for(int i=0;i<info.startChannel;i++)
			statFile<<100.0<<" ";
		for(int i=info.startChannel;i<info.stopChannel;i++,ptrChanFlags++)	
		{
			if(!(*ptrChanFlags))	
				statFile<<timePercent<<" ";
			else
				statFile<<100.0<<" ";
		}
		for(int i=info.stopChannel;i<info.noOfChannels;i++)
			statFile<<100.0<<" ";
		statFile<<endl;
	}
}
void Runtime::intializeFiles()
{
	if(info.doWriteFiltered2D)
	{
		ostringstream filename;	
		filename<<info.outputfilepath<<".gpt";
		ofstream filtered2DFile;			
		filtered2DFile.open(filename.str().c_str(),ios::out | ios::trunc);
		if (filtered2DFile.fail())
		{
			cout<<endl<<"ERROR: Cannot create outputfile. Possible permission issues?"<<endl;
			exit(1);
		}
		filtered2DFile.close();
	}

	if(!info.doPolarMode)
	{
			
		if(info.doWriteChanFlags && info.doChanFlag)
		{
			ofstream chanflagfile;	
			chanflagfile.open("chanflag.gpt",ios::out | ios::trunc);
			chanflagfile.close();
		}
		if(info.doWriteTimeFlags && info.doTimeFlag)
		{
			ofstream timeflagfile;	
			timeflagfile.open("timeflag.gpt",ios::out | ios::trunc);
			timeflagfile.close();
		}
		if(info.doWriteFullDM && !info.doFilteringOnly)
		{			

			ofstream fullDMfile;	
			fullDMfile.open("fullDM_filtered.gpt",ios::out | ios::trunc);
			fullDMfile.close();
			
			ofstream fullDMUnfilteredfile;	
			fullDMUnfilteredfile.open("fullDM_unfiltered.gpt",ios::out | ios::trunc);
			fullDMUnfilteredfile.close();
			
			ofstream fullDMCountfile;	
			fullDMCountfile.open("fullDMCount.gpt",ios::out | ios::trunc);
			fullDMCountfile.close();
		}
		
		ofstream statFile;
		statFile.open("stats.gpt",ios::out | ios::trunc);
		statFile<<"#window_indx\tmean_pred\tmean_pre\tmean_post\trms_pred\trms_pre\trms_post\tm/r_pred\tm/r_pre\tm/r_post"<<endl;
		statFile.close();

		ofstream intensityFile;
		intensityFile.open("intensity_summary.gpt",ios::out | ios::trunc);
		intensityFile<<"#First element of each line denotes the number of time samples in the block, followed by intensity in each channel"<<endl;
		intensityFile.close();
	
		ofstream flagStatFile;
		flagStatFile.open("flag_stats.gpt",ios::out | ios::trunc);
		flagStatFile<<"#Each line represents a seperate block. For the particular block, the line contains the percentage of flagged data in each channel"<<endl;
		flagStatFile.close();
		
	}
	else
	{
		for(int k=0;k<info.noOfPol;k++)
		{
			ostringstream filename;
			if(info.doWriteChanFlags && info.doChanFlag)
			{
				filename<<"chanflag"<<k+1<<".gpt";
				ofstream chanflagfile;	
				chanflagfile.open(filename.str().c_str(),ios::out | ios::trunc);
				chanflagfile.close();
			}
			if(info.doWriteTimeFlags && info.doTimeFlag)
			{
				filename.str("");
				filename.clear();
				filename<<"timeflag"<<k+1<<".gpt";	
				ofstream timeflagfile;	
				timeflagfile.open(filename.str().c_str(),ios::out | ios::trunc);
				timeflagfile.close();
			}
			if(info.doWriteFullDM && !info.doFilteringOnly)
			{
				filename.str("");
				filename.clear();
				filename<<"fullDM_filtered"<<k+1<<".gpt";
				ofstream fullDMfile;	
				fullDMfile.open(filename.str().c_str(),ios::out | ios::trunc);
				fullDMfile.close();
				
				filename.str("");
				filename.clear();
				filename<<"fullDM_unfiltered"<<k+1<<".gpt";
				ofstream fullDMUnfilteredfile;	
				fullDMUnfilteredfile.open(filename.str().c_str(),ios::out | ios::trunc);
				fullDMUnfilteredfile.close();
				
				filename.str("");
				filename.clear();
				filename<<"fullDMCount"<<k+1<<".gpt";
				ofstream fullDMCountfile;	
				fullDMCountfile.open(filename.str().c_str(),ios::out | ios::trunc);
				fullDMCountfile.close();
			}
			
			filename.str("");
			filename.clear();
			filename<<"stats"<<k+1<<".gpt";
			ofstream statFile;
			statFile.open(filename.str().c_str(),ios::out | ios::trunc);
			statFile<<"#window_index\tmean_pred\tmean_pre\tmean_post\trms_pred\trms_pre\trms_post\tm/r_pred\tm/r_pre\tm/r_post"<<endl;
			statFile.close();

			filename.str("");
			filename.clear();
			filename<<"intensity_summary"<<k+1<<".gpt";
			ofstream intensityFile;
			intensityFile.open(filename.str().c_str(),ios::out | ios::trunc);
			intensityFile<<"#First element of each line denotes the number of time samples in the block, followed by intensity in each channel";
			intensityFile.close();

		}
	}
	
}

void Runtime::writeAll(ThreadPacket *threadPacket)
{

	
	if(!info.doPolarMode)
	{
		
		if(info.doWriteFullDM && !info.doFilteringOnly)
		{
			
			timeFullDMWrite-=omp_get_wtime(); //benchmark
			threadPacket->advancedAnalysis[0]->writeFullDM("fullDM_filtered.gpt","fullDM_unfiltered.gpt");
			threadPacket->advancedAnalysis[0]->writeFullDMCount("fullDMCount.gpt");
			timeFullDMWrite+=omp_get_wtime(); //benchmark			
		}
		if(info.doWriteFiltered2D)
		{
			if(info.sampleSizeBytes==1)
			{
				ostringstream filename;	
				filename.str("");
				filename.clear();
				filename<<info.outputfilepath<<".gpt";
				FILE* filteredRawDataFile;    
	       			filteredRawDataFile = fopen(filename.str().c_str(), "ab");
				short int *ptrFilteredData=threadPacket->basicAnalysisWrite[0]->filteredRawData;
				long int size=(threadPacket->basicAnalysisWrite[0]->blockLength)*info.noOfChannels;
				unsigned char* tmp=new unsigned char[size];
				unsigned char* ptrtmp=tmp;
				for(int i=0;i<size;i++,ptrtmp++)
				{
					*ptrtmp=(unsigned char)(*ptrFilteredData++);	                      		
				}
				fwrite(tmp, sizeof(unsigned char),size, filteredRawDataFile);    
				delete[] tmp;
		                fclose(filteredRawDataFile); 
			}
			else
			{
				ostringstream filename;	
				filename<<info.outputfilepath<<".gpt";
				threadPacket->basicAnalysisWrite[0]->writeFilteredRawData(filename.str().c_str());
			}
		}
		
		threadPacket->basicAnalysisWrite[0]->writeCurBandshape("intensity_summary.gpt");		
		timeRFITimeFlagsWrite-=omp_get_wtime(); //benchmark
		if(info.doWriteTimeFlags && info.doTimeFlag)
			threadPacket->rFIFilteringTime[0]->writeFlags("timeflag.gpt");
		timeRFITimeFlagsWrite+=omp_get_wtime(); //benchmark
	
		timeRFIChanFlagsWrite-=omp_get_wtime(); //benchmark
		if(info.doWriteChanFlags && info.doChanFlag)
			threadPacket->rFIFilteringChan[0]->writeFlags("chanflag.gpt",startFlags,info.startChannel,endFlags,info.noOfChannels-info.stopChannel);
		timeRFIChanFlagsWrite+=omp_get_wtime(); //benchmark	
		
		
	}
	else
	{
		if(info.doWriteFiltered2D)
		{
			ostringstream filename;	
			filename.str("");
			filename.clear();
			filename<<info.outputfilepath<<".gpt";
			FILE* filteredRawDataFile;    
       			filteredRawDataFile = fopen(filename.str().c_str(), "ab");
			short int **ptrFilteredData=new short int*[info.noOfPol];
			for(int k=0;k<info.noOfPol;k++)
			{
				ptrFilteredData[k]=threadPacket->basicAnalysisWrite[k]->filteredRawData;
			}
			if(info.sampleSizeBytes==1)
			{
				char tmp;
				for(int i=0;i<(threadPacket->basicAnalysisWrite[0]->blockLength)*info.noOfChannels;i++)
				{
					for(int k=0;k<info.noOfPol;k++)
					{
						tmp=(char)(*ptrFilteredData[k]++);
		                      		fwrite(&tmp, sizeof(char), 1, filteredRawDataFile);    
					}
				}
			}
			else
			{
				for(int i=0;i<(threadPacket->basicAnalysisWrite[0]->blockLength)*info.noOfChannels;i++)
				{
					for(int k=0;k<info.noOfPol;k++)
					{
		                      		fwrite(ptrFilteredData[k]++, sizeof(short int), 1, filteredRawDataFile);    
					}
				}
			}
                        fclose(filteredRawDataFile); 

		}

		for(int k=0;k<info.noOfPol;k++)
		{
			ostringstream filename;	
			ostringstream filenameUnfiltered;	
			timeFullDMWrite-=omp_get_wtime(); //benchmark
			if(info.doWriteFullDM && !info.doFilteringOnly)
			{
				filename<<"fullDM_filtered"<<k+1<<".gpt";				
				filenameUnfiltered<<"fullDM_unfiltered"<<k+1<<".gpt";	

				timeFullDMWrite-=omp_get_wtime(); //benchmark
				threadPacket->advancedAnalysis[k]->writeFullDM(filename.str().c_str(),filenameUnfiltered.str().c_str());
				timeFullDMWrite+=omp_get_wtime(); //benchmark	
				

				filename.str("");
				filename.clear();
				filename<<"fullDMCount"<<k+1<<".gpt";
				timeFullDMWrite-=omp_get_wtime(); //benchmark
				threadPacket->advancedAnalysis[k]->writeFullDMCount(filename.str().c_str());
				timeFullDMWrite+=omp_get_wtime(); //benchmark
				
				
				
			}
			
			/**
			filename.str("");
			filename.clear();
			filename<<"intensity_summary"<<k+1<<".gpt";
			threadPacket->basicAnalysis[0]->writeCurBandshape(filename.str().c_str());	
			**/
			timeFullDMWrite+=omp_get_wtime(); //benchmark
			
			filename.str("");
			filename.clear();
			filename<<"timeflag"<<k+1<<".gpt";	

			timeRFITimeFlagsWrite-=omp_get_wtime(); //benchmark
			if(info.doWriteTimeFlags && info.doTimeFlag)
				threadPacket->rFIFilteringTime[k]->writeFlags(filename.str().c_str());
			timeRFITimeFlagsWrite+=omp_get_wtime(); //benchmark
	
			filename.str("");
			filename.clear();
			filename<<"chanflag"<<k+1<<".gpt";


			timeRFIChanFlagsWrite-=omp_get_wtime(); //benchmark
			if(info.doWriteChanFlags && info.doChanFlag)
				threadPacket->rFIFilteringChan[k]->writeFlags(filename.str().c_str(),startFlags,info.startChannel,endFlags,info.noOfChannels-info.stopChannel);
			timeRFIChanFlagsWrite+=omp_get_wtime(); //benchmark	
		}
	}
	testStatistics(threadPacket);
	writeFlagStats(threadPacket);
}

double fillTime; //benchmark
void Runtime::fillPipe()
{	

	fillTime=omp_get_wtime(); //benchmark
	int index;
	readCompleteFlag=0;
	readDoneFlag=1;	
	
	for(int k=0;k<nActions;k++)
	{
		
		#pragma omp parallel for schedule(dynamic, 1)
		for(int j=1;j<k+1;j++)
				action(j*nThreadMultiplicity,j);
		
		while(!readCompleteFlag);
		for(int i=0;i<nThreadMultiplicity;i++)
		threadPacket[0+i]->copySelect(threadPacket[(nActions-1)*nThreadMultiplicity+i]);		//ready blocks for plot and io

		for(int i=(nActions-1)*nThreadMultiplicity-1;i>=0;i--)
			threadPacket[i+nThreadMultiplicity]->copy(threadPacket[i]);
		readCompleteFlag=0;
		readDoneFlag=1;	
		blockIndex+=nThreadMultiplicity;	
		
	}
	

	threadPacket[(nActions-1)*nThreadMultiplicity]->advancedAnalysisOld=threadPacket[nThreadMultiplicity-1]->advancedAnalysis;
	fillTime=(omp_get_wtime()-fillTime)/(nThreadMultiplicity*nActions); //benchmark
}
int numberOfThreadRuns=0;		//benchmark
void Runtime::loopThrough()
{
	double startTime,timeP,time0,time1,time2,time3,time4,time5,timeNet; //benchmark variables 
	omp_set_nested(1);
	ofstream benchmarkfile;
	benchmarkfile.open("benchmark_threadtime_indv.gpt",ios::out | ios::trunc);
	/*while(!hasReachedEof)
	{
		time0= omp_get_wtime();
		timeThread1 -= omp_get_wtime();
		action(0,0);
		timeThread1+=omp_get_wtime();	
		time0= omp_get_wtime()-time0;	
		benchmarkfile<<time0/nThreadMultiplicity<<endl;
		for(int i=0;i<nThreadMultiplicity;i++)
		{
			ThreadPacket* thisThreadPacket=threadPacket[i];		
			delete thisThreadPacket->aquireData;
			thisThreadPacket->aquireData=NULL;
		}
	}*/
	
			while(!hasReachedEof)
			{			
				numberOfThreadRuns+=nThreadMultiplicity;
				startTime=omp_get_wtime();
				timeNet=omp_get_wtime();


				if(!info.doFilteringOnly) //full mode of gptool
				{
					#pragma omp parallel sections 
					{
			
						//#pragma omp section
						//{
						//	timeP= omp_get_wtime();
							
						//	timeP= omp_get_wtime()-timeP;	
						//}
						#pragma omp section
						{
							time1= omp_get_wtime();
							timeThread1 -= omp_get_wtime();
							action(1*nThreadMultiplicity,1);
							action(0,-1);								
							timeThread1+=omp_get_wtime();	
							time1= omp_get_wtime()-time1;			
						}
						#pragma omp section
						{
							time2= omp_get_wtime();
							timeThread2 -= omp_get_wtime();
							action(2*nThreadMultiplicity,2);				
							timeThread2+=omp_get_wtime();	
							time2= omp_get_wtime()-time2;		
						}
						#pragma omp section
						{
							time3= omp_get_wtime();
							timeThread3-=omp_get_wtime();
							action(3*nThreadMultiplicity,3);	
							timeThread3+=omp_get_wtime();	
							time3= omp_get_wtime()-time3;	
						}
						#pragma omp section
						{
							time4= omp_get_wtime();
							timeThread4-=omp_get_wtime();
							action(4*nThreadMultiplicity,4);
							timeThread4+=omp_get_wtime();	
							time4= omp_get_wtime()-time4;	
						}
					
				
					}
				}
				else
				{
					#pragma omp parallel sections //filtering only mode of gptool
					{
						#pragma omp section
						{
							time1= omp_get_wtime();
							timeThread1 -= omp_get_wtime();
							action(1*nThreadMultiplicity,1);
							timeThread1+=omp_get_wtime();	
							time1= omp_get_wtime()-time1;			
						}
						#pragma omp section
						{
							time2= omp_get_wtime();
							timeThread2 -= omp_get_wtime();
							action(2*nThreadMultiplicity,2);				
							timeThread2+=omp_get_wtime();	
							time2= omp_get_wtime()-time2;		
						}
						#pragma omp section
						{
							time3= omp_get_wtime();
							timeThread3-=omp_get_wtime();
							action(3*nThreadMultiplicity,3);	
							timeThread3+=omp_get_wtime();	
							time3= omp_get_wtime()-time3;	
						}
				
					}
				}
				while(!readCompleteFlag);
				for(int i=0;i<nThreadMultiplicity;i++)
					threadPacket[i]->freeMem();
				
				for(int i=0;i<nThreadMultiplicity;i++)
					threadPacket[0+i]->copySelect(threadPacket[(nActions-1)*nThreadMultiplicity+i]);		//ready blocks for plot and io

				for(int i=(nActions-1)*nThreadMultiplicity-1;i>=0;i--)
					threadPacket[i+nThreadMultiplicity]->copy(threadPacket[i]);

				threadPacket[(nActions-1)*nThreadMultiplicity]->advancedAnalysisOld=threadPacket[nThreadMultiplicity-1]->advancedAnalysis;
				
				readCompleteFlag=0;
				readDoneFlag=1;	
				if(!keepRunning)
				{			
					cout<<endl<<"Terminating program.."<<endl;
					break;
				}

				if(info.doWindowDelay && info.doReadFromFile) 
					while(omp_get_wtime()-startTime<info.blockSizeSec);
				timeNet=omp_get_wtime()-timeNet;
				benchmarkfile<<blockIndex<<" "<<timeP/nThreadMultiplicity<<" "<<time0/nThreadMultiplicity<<" "<<time1/nThreadMultiplicity<<" "<<time2/nThreadMultiplicity<<" "<<time3/nThreadMultiplicity<<" "<<time4/nThreadMultiplicity<<endl;

				blockIndex+=nThreadMultiplicity;
			}
		
		
		
		 
	
	benchmarkfile.close();
		
	
	
	
}


void Runtime::quickclosePipe()
{
	cout<<endl<<"Closing pipe.."<<endl;
	for(int k=0;k<nThreadMultiplicityTemp;k++)
	{
		displayBlockIndex(blockIndex+k);
		writeAll(threadPacket[k]);
		if(!info.doFilteringOnly)
		{
			timePlot-=omp_get_wtime();		
			plot->plotAll(threadPacket[k]->basicAnalysisWrite,threadPacket[k]->advancedAnalysis,threadPacket[k]->rFIFilteringTime,threadPacket[k]->rFIFilteringChan,blockIndex+k-nActions*nThreadMultiplicity);
			timePlot+=omp_get_wtime();
		}
	}
	if(info.doPolarMode)
	{	
		for(int k=0;k<info.noOfPol;k++)
		{
			//writing final bits
			ostringstream filename;
			ostringstream filenameUnfiltered;
			filename<<"bandshape"<<k+1<<".gpt";
			threadPacket[nThreadMultiplicityTemp-1]->basicAnalysisWrite[k]->writeBandshape(filename.str().c_str());
			filename.str("");
			filename.clear();
			
			if(!info.doFilteringOnly)
			{
				filename<<"profile_filtered"<<k+1<<".gpt";
				filenameUnfiltered<<"profile_unfiltered"<<k+1<<".gpt";
				threadPacket[nThreadMultiplicityTemp-1]->advancedAnalysis[k]->writeProfile(filename.str().c_str(),filenameUnfiltered.str().c_str());
			}
	
		}
	}
	else
	{
		
		threadPacket[nThreadMultiplicityTemp-1]->basicAnalysisWrite[0]->writeBandshape("bandshape.gpt");
		if(!info.doFilteringOnly)
			threadPacket[nThreadMultiplicityTemp-1]->advancedAnalysis[0]->writeProfile("profile_filtered.gpt","profile_unfiltered.gpt");		
	}
	//for(int j=0;j<nThreadMultiplicity;j++)
	//	threadPacket[j]->freeMem();
	for(int j=0;j<nThreadMultiplicity;j++)
	{
		for(int i=0;i<info.noOfPol;i++)
		{
			delete threadPacket[2*nThreadMultiplicity+j]->basicAnalysis[i];
			delete threadPacket[3*nThreadMultiplicity+j]->basicAnalysis[i];
			delete threadPacket[4*nThreadMultiplicity+j]->basicAnalysis[i];

			if(!threadPacket[3*nThreadMultiplicity+j]->rFIFilteringChan[i])
				delete threadPacket[3*nThreadMultiplicity+j]->rFIFilteringChan[i];
			if(!threadPacket[4*nThreadMultiplicity+j]->rFIFilteringChan[i])
				delete threadPacket[4*nThreadMultiplicity+j]->rFIFilteringChan[i];

			if(!threadPacket[4*nThreadMultiplicity+j]->rFIFilteringTime[i])
				delete threadPacket[4*nThreadMultiplicity+j]->rFIFilteringTime[i];
		
			
		}

	}
	//	for(int i=0;i<info.noOfPol;i++)
	//	{
	//		delete threadPacket[4*nThreadMultiplicity]->advancedAnalysisOld[i];
	//	}
	cout<<endl;
}


void Runtime::closePipe()
{
	for(int k=0;k<nThreadMultiplicityTemp;k++)
	{
		displayBlockIndex(blockIndex+k);
		writeAll(threadPacket[k]);
		if(!info.doFilteringOnly)
		{
			timePlot-=omp_get_wtime();		
			plot->plotAll(threadPacket[k]->basicAnalysisWrite,threadPacket[k]->advancedAnalysis,threadPacket[k]->rFIFilteringTime,threadPacket[k]->rFIFilteringChan,blockIndex+k-nActions*nThreadMultiplicity);
			timePlot+=omp_get_wtime();
		}
	}
	int temp=nThreadMultiplicity;
	int nPlots=nThreadMultiplicity;
	blockIndex+=nThreadMultiplicity;
	for(int i=1;i<nActions;i++)
	{
		nThreadMultiplicity=nThreadMultiplicityTemp;
		action(i*temp,i);
		nThreadMultiplicity=temp;
		for(int j=i+1;j<nActions;j++)
		{
			action(j*nThreadMultiplicity,j);
		}

		if(i!=nActions-1)	//The last packets is retained in memory to print the final profile from.
		{
			for(int j=0;j<nThreadMultiplicity;j++)
				threadPacket[j]->freeMem();
		}

		//Packet transfers between different operations:

		for(int j=0;j<nThreadMultiplicity;j++)
			threadPacket[0+j]->copySelect(threadPacket[(nActions-1)*nThreadMultiplicity+j]);		//ready blocks for plot and io

		for(int j=(nActions-1)*nThreadMultiplicity-1;j>=0;j--)
			threadPacket[j+nThreadMultiplicity]->copy(threadPacket[j]);

		threadPacket[(nActions-1)*nThreadMultiplicity]->advancedAnalysisOld=threadPacket[nThreadMultiplicity-1]->advancedAnalysis;

		//Plotting ready packets:
		if(i==nActions-1)	//last chunck may not contain as many blocks
			nPlots=nThreadMultiplicityTemp;
		for(int k=0;k<nPlots;k++)
		{
			displayBlockIndex(blockIndex+k);
			writeAll(threadPacket[k]);
			if(!info.doFilteringOnly)
			{
				timePlot-=omp_get_wtime();	
				plot->plotAll(threadPacket[k]->basicAnalysisWrite,threadPacket[k]->advancedAnalysis,threadPacket[k]->rFIFilteringTime,threadPacket[k]->rFIFilteringChan,blockIndex+k-nActions*nThreadMultiplicity);
				timePlot+=omp_get_wtime();
			}
		}
		blockIndex+=nThreadMultiplicity;
	}

	if(info.doPolarMode)
	{	
		for(int k=0;k<info.noOfPol;k++)
		{
			//writing final bits
			ostringstream filename;
			ostringstream filenameUnfiltered;
			filename<<"bandshape"<<k+1<<".gpt";
			threadPacket[nThreadMultiplicityTemp-1]->basicAnalysisWrite[k]->writeBandshape(filename.str().c_str());
			filename.str("");
			filename.clear();
			
			if(!info.doFilteringOnly)
			{
				filename<<"profile_filtered"<<k+1<<".gpt";
				filenameUnfiltered<<"profile_unfiltered"<<k+1<<".gpt";
				threadPacket[nThreadMultiplicityTemp-1]->advancedAnalysis[k]->writeProfile(filename.str().c_str(),filenameUnfiltered.str().c_str());
			}
	
		}
	}
	else
	{
		
		threadPacket[nThreadMultiplicityTemp-1]->basicAnalysisWrite[0]->writeBandshape("bandshape.gpt");
		if(!info.doFilteringOnly)
			threadPacket[nThreadMultiplicityTemp-1]->advancedAnalysis[0]->writeProfile("profile_filtered.gpt","profile_unfiltered.gpt");		
	}
	
	for(int j=0;j<nThreadMultiplicityTemp;j++) //free'ing last packets
		threadPacket[j]->freeMem();
	cout<<endl;
}
void Runtime::action(int threadPacketIndex,int actionIndex)
{
	switch(actionIndex)
	{
		case -1:
			plotTasks(threadPacketIndex);
			break;
		case 0:
			ioTasks(threadPacketIndex);
			break;
		case 1:
			floatConversionTasks(threadPacketIndex);
			break;
		case 2:
			if(chanFirst)
				channelTasks(threadPacketIndex);
			else
				timeTasks(threadPacketIndex);
			break;
		case 3:
			if(chanFirst)
				timeTasks(threadPacketIndex);
			else
				channelTasks(threadPacketIndex);
			break;
		case 4:
			if(!info.doFilteringOnly)			
				fullDMTask(threadPacketIndex);
			break;
			
	}
}

/*******************************************************************
*FUNCTION: ioTasks(int threadPacketIndex)
*This function performs the tasks on thread one, i.e Reading data,
*writing data and plotting
*For optimization considerations refer to document titled
*"Multithreading Considerations for gptool"
*******************************************************************/
void Runtime::ioTasks(int threadPacketIndex)
{
	
	double readTime,floatTime,restTime;
	char killFlag=0;
	while(!killFlag)
	{
		ofstream benchmarkfile;
		benchmarkfile.open("benchmark_readtime.gpt",ios::app);
		//cout<<"i/o thread id:"<<sched_getcpu()<<endl;
		while(!readDoneFlag);
		readDoneFlag=0;
		
		for(int i=0;i<nThreadMultiplicity;i++)
		{
			ThreadPacket* thisThreadPacket=threadPacket[threadPacketIndex+i];
			
			thisThreadPacket->aquireData=new AquireData(blockIndex+i);	

			readTime=omp_get_wtime(); //benchmark
			timeReadData-=omp_get_wtime(); //benchmark
			thisThreadPacket->aquireData->readData();
			readTime=omp_get_wtime()-readTime; //benchmark
			benchmarkfile<<readTime<<endl;
	
		
			hasReachedEof=thisThreadPacket->aquireData->hasReachedEof;	
		
			
			
			if(hasReachedEof)
			{
				nThreadMultiplicityTemp=i+1;
				killFlag=1;
				break;
			}
		}
		for(int i=0;i<nThreadMultiplicity;i++)
		{
			ThreadPacket* thisThreadPacket=threadPacket[threadPacketIndex+i];
			if(blockIndex>=nThreadMultiplicity*nActions)
			{
				displayBlockIndex(blockIndex+i);
				writeAll(thisThreadPacket);	
			
			}
		}
		readCompleteFlag=1;
		if(!keepRunning)
			killFlag=1;
	}
	
	

}

void Runtime::floatConversionTasks(int threadPacketIndex)
{
	
	double readTime,floatTime,restTime;
	//cout<<"i/o thread id:"<<sched_getcpu()<<endl;
	ofstream benchmarkfile;
	benchmarkfile.open("benchmark_threadtime_indv.gpt",ios::app);

	#pragma omp parallel for schedule(dynamic, 1)
	for(int i=0;i<nThreadMultiplicity;i++)
	{
		ThreadPacket* thisThreadPacket=threadPacket[threadPacketIndex+i];
		
 		timeConvertToFloat-=omp_get_wtime(); //benchmark
		thisThreadPacket->aquireData->splitRawData();		
		timeConvertToFloat+=omp_get_wtime(); //benchmark

		for(int k=0;k<info.noOfPol;k++)
		{
			thisThreadPacket->basicAnalysis[k]=new BasicAnalysis(thisThreadPacket->aquireData->splittedRawData[k],k,thisThreadPacket->aquireData->blockLength);
		}
		thisThreadPacket->basicAnalysis[0]->headerInfo=thisThreadPacket->aquireData->headerInfo;
		delete thisThreadPacket->aquireData;
		thisThreadPacket->aquireData=NULL;


	}
	

}

void Runtime::plotTasks(int threadPacketIndex)
{
	int i=nThreadMultiplicity-1;
	ThreadPacket* thisThreadPacket=threadPacket[threadPacketIndex+i];	
		
	if(!info.doFilteringOnly)
	{
		timePlot-=omp_get_wtime();			
		plot->plotAll(thisThreadPacket->basicAnalysisWrite,thisThreadPacket->advancedAnalysis,thisThreadPacket->rFIFilteringTime,thisThreadPacket->rFIFilteringChan,blockIndex+i-nActions*nThreadMultiplicity);
		timePlot+=omp_get_wtime();
				
	}
}
/*******************************************************************
*FUNCTION: void channelTasks(int threadPacketIndex)
*This function performs the tasks on either thread two or three
*depending on if channel filtering is performed before or after time
*filtering.
*The tasks are :Calculating bandshapes, finding outliers in bandshape
*and normalizing data if enabled by user.
*For optimization considerations refer to document titled
*"Multithreading Considerations for gptool"
*******************************************************************/
void Runtime::channelTasks(int threadPacketIndex)
{
	#pragma omp parallel for ordered schedule(dynamic, 1)
	for(int t=0;t<nThreadMultiplicity;t++)
	{
		//cout<<"channel thread id:"<<sched_getcpu()<<endl;
		BasicAnalysis **basicAnalysis=threadPacket[threadPacketIndex+t]->basicAnalysis;
		RFIFiltering **rFIFilteringChan=threadPacket[threadPacketIndex+t]->rFIFilteringChan;
		RFIFiltering **rFIFilteringTime=threadPacket[threadPacketIndex+t]->rFIFilteringTime;
		for(int i=0;i<info.noOfPol;i++)
		{
			switch((int)info.bandshapeToUse)
			{
				case 1:
					
					rFIFilteringChan[i]=new RFIFiltering(&(basicAnalysis[i]->bandshape[info.startChannel]),info.stopChannel-info.startChannel);					
					rFIFilteringChan[i]->inputMax=basicAnalysis[i]->maxBandshape;
					rFIFilteringChan[i]->inputMin=basicAnalysis[i]->minBandshape;
					
					break;
				case 2:

					rFIFilteringChan[i]=new RFIFiltering(&(basicAnalysis[i]->normalizedBandshape[info.startChannel]),info.stopChannel-info.startChannel);
					rFIFilteringChan[i]->inputMax=basicAnalysis[i]->maxNormalizedBandshape;
					rFIFilteringChan[i]->inputMin=basicAnalysis[i]->minNormalizedBandshape;

					break;
				case 3:
					rFIFilteringChan[i]=new RFIFiltering(&(basicAnalysis[i]->meanToRmsBandshape[info.startChannel]),info.stopChannel-info.startChannel);	
					rFIFilteringChan[i]->inputMax=basicAnalysis[i]->maxMeanToRmsBandshape;
					rFIFilteringChan[i]->inputMin=basicAnalysis[i]->minMeanToRmsBandshape;
					break;
				case 4:
					rFIFilteringChan[i]=new RFIFiltering(&(basicAnalysis[i]->meanToRmsBandshape[info.startChannel]),info.stopChannel-info.startChannel);	
					rFIFilteringChan[i]->inputMax=basicAnalysis[i]->maxMeanToRmsBandshape;
					rFIFilteringChan[i]->inputMin=basicAnalysis[i]->minMeanToRmsBandshape;
					break;
				default:
					rFIFilteringChan[i]=new RFIFiltering(&(basicAnalysis[i]->bandshape[info.startChannel]),info.stopChannel-info.startChannel);
					rFIFilteringChan[i]->inputMax=basicAnalysis[i]->maxBandshape;
					rFIFilteringChan[i]->inputMin=basicAnalysis[i]->minBandshape;
					break;
			}
			
			if(info.doTimeFlag && info.doChanFlag && (info.flagOrder==2))
			{
				timeBandshape-=omp_get_wtime(); //benchmark
				basicAnalysis[i]->computeBandshape(rFIFilteringTime[i]->flags);			
				timeBandshape+=omp_get_wtime(); //benchmark
			}
			else
			{	
				timeBandshape-=omp_get_wtime(); //benchmark
				basicAnalysis[i]->computeBandshape();			
				timeBandshape+=omp_get_wtime(); //benchmark
			}
			timeBandshape-=omp_get_wtime(); //benchmark
			#pragma omp ordered
			basicAnalysis[i]->calculateCumulativeBandshapes();			
			timeBandshape+=omp_get_wtime(); //benchmark
			if(info.doChanFlag)
			{
				timeRFIChanStats-=omp_get_wtime(); //benchmark
				rFIFilteringChan[i]->cutoffToRms=info.chanCutOffToRMS;
				rFIFilteringChan[i]->computeStatistics(info.chanFlagAlgo);
				timeRFIChanStats+=omp_get_wtime(); //benchmark
			
				timeRFIChanFlag-=omp_get_wtime(); //benchmark
				rFIFilteringChan[i]->flagData();
				if((int)info.bandshapeToUse==4)
				{
					rFIFilteringChan[i]->input=&(basicAnalysis[i]->normalizedBandshape[info.startChannel]);
					rFIFilteringChan[i]->computeStatistics(info.chanFlagAlgo);
					rFIFilteringChan[i]->flagData();	
				}
				rFIFilteringChan[i]->generateManualFlags(info.nBadChanBlocks,info.badChanBlocks,info.startChannel);
				timeRFIChanFlag+=omp_get_wtime(); //benchmark	
			}
			else
			{
				rFIFilteringChan[i]->generateBlankFlags();
				rFIFilteringChan[i]->generateManualFlags(info.nBadChanBlocks,info.badChanBlocks,info.startChannel);
			}
		
		}
		if(info.doPolarMode)
		{ //transfer RR OR LL flags to all
			char* ptrChanFlag1=rFIFilteringChan[0]->flags;
			char* ptrChanFlag2=rFIFilteringChan[2]->flags;
			for(int i=0;i<info.stopChannel-info.startChannel;i++,ptrChanFlag1++,ptrChanFlag2++)
			{
				*ptrChanFlag1=(*ptrChanFlag1 | *ptrChanFlag2);			
			}
			for(int i=0;i<info.noOfPol;i++)
			{
				ptrChanFlag1=rFIFilteringChan[0]->flags;
				ptrChanFlag2=rFIFilteringChan[i]->flags;
				for(int j=0;j<info.stopChannel-info.startChannel;j++,ptrChanFlag1++,ptrChanFlag2++)
				{
					*ptrChanFlag2=*ptrChanFlag1;			
				}
			}
		}
	}
	

}
/*******************************************************************
*FUNCTION: void TimeTasks(int threadPacketIndex)
*This function performs the tasks on either thread two or three
*depending on if time filtering is performed before or after time
*filtering.
*The tasks are :Calculating zeroDM & finding outliers in zeroDM.
*For optimization considerations refer to document titled
*"Multithreading Considerations for gptool"
*******************************************************************/
void Runtime::timeTasks(int threadPacketIndex)
{
	float* histogramIntervalTemp=new float[info.noOfPol];
	#pragma omp parallel for schedule(dynamic, 1)
	for(int t=0;t<nThreadMultiplicity;t++)
	{
		//cout<<"time thread id:"<<sched_getcpu()<<endl;
		BasicAnalysis **basicAnalysis=threadPacket[threadPacketIndex+t]->basicAnalysis;
		RFIFiltering **rFIFilteringChan=threadPacket[threadPacketIndex+t]->rFIFilteringChan;
		RFIFiltering **rFIFilteringTime=threadPacket[threadPacketIndex+t]->rFIFilteringTime;
		
		for(int i=0;i<info.noOfPol;i++)
		{
			timeNormalization-=omp_get_wtime(); //benchmark
			if(info.doUseNormalizedData)
				basicAnalysis[i]->normalizeData();
			timeNormalization+=omp_get_wtime(); //benchmark
			rFIFilteringTime[i]=new RFIFiltering(basicAnalysis[i]->zeroDM,basicAnalysis[i]->blockLength);
			if(info.doChanFlag || (info.doTimeFlag && info.doChanFlag && (info.flagOrder==1)))
			{
				timeZeroDM-=omp_get_wtime(); //benchmark
				basicAnalysis[i]->computeZeroDM(rFIFilteringChan[i]->flags);			
				timeZeroDM+=omp_get_wtime(); //benchmark
			}
			else
			{		
				timeZeroDM-=omp_get_wtime(); //benchmark
				basicAnalysis[i]->computeZeroDM(blankChanFlags);			
				timeZeroDM+=omp_get_wtime(); //benchmark
			}
		
		
			if(info.doTimeFlag)
			{
				timeRFITimeStats-=omp_get_wtime(); //benchmark
				rFIFilteringTime[i]->cutoffToRms=info.timeCutOffToRMS;
				rFIFilteringTime[i]->inputMax=basicAnalysis[i]->maxZeroDM;
				rFIFilteringTime[i]->inputMin=basicAnalysis[i]->minZeroDM;
	
		 		rFIFilteringTime[i]->histogramInterval=histogramInterval[i];
				if(blockIndex==3*nThreadMultiplicity)
					rFIFilteringTime[i]->histogramInterval=(basicAnalysis[i]->maxZeroDM-basicAnalysis[i]->minZeroDM)/(pow(basicAnalysis[i]->blockLength,1/3.0));
				rFIFilteringTime[i]->computeStatistics(info.timeFlagAlgo);
				timeRFITimeStats+=omp_get_wtime(); //benchmark
			
			
				timeRFITimeFlags-=omp_get_wtime(); //benchmark
				if(info.doMultiPointFilter)
					rFIFilteringTime[i]->multiPointFlagData(info.cutoff);
				else
					rFIFilteringTime[i]->flagData();
				if(info.doZeroDMSub==1)				
					basicAnalysis[i]->subtractZeroDM(rFIFilteringChan[i]->flags,rFIFilteringTime[i]->centralTendency);

				if(info.smoothFlagWindowLength>0)
					rFIFilteringTime[i]->smoothFlags((int)info.smoothFlagWindowLength,info.concentrationThreshold);	
				
				if(!info.doReplaceByMean && info.doWriteFiltered2D)
					basicAnalysis[i]->getFilteredRawData(rFIFilteringTime[i]->flags,rFIFilteringChan[i]->flags,0);
				if(info.doReplaceByMean==1)
					basicAnalysis[i]->getFilteredRawData(rFIFilteringTime[i]->flags,rFIFilteringChan[i]->flags,rFIFilteringTime[i]->centralTendency);
				else if(info.doReplaceByMean==2)
					basicAnalysis[i]->getFilteredRawDataSmoothBshape(rFIFilteringTime[i]->flags,rFIFilteringChan[i]->flags);
				timeRFITimeFlags+=omp_get_wtime(); //benchmark
				
			
			}
			else
			{
				rFIFilteringTime[i]->generateBlankFlags();
				if(info.doZeroDMSub==1)				
					basicAnalysis[i]->subtractZeroDM(rFIFilteringChan[i]->flags,1);
			}
			
		}
		if(t==nThreadMultiplicity-1)
			for(int i=0;i<info.noOfPol;i++)
				histogramIntervalTemp[i]=rFIFilteringTime[i]->histogramInterval;
		if(info.doPolarMode)
		{ //transfer RR OR LL flags to all
			char* ptrTimeFlag1=rFIFilteringTime[0]->flags;
			char* ptrTimeFlag2=rFIFilteringTime[2]->flags;
			for(int i=0;i<basicAnalysis[0]->blockLength;i++,ptrTimeFlag1++,ptrTimeFlag2++)
			{
				*ptrTimeFlag1=(*ptrTimeFlag1 | *ptrTimeFlag2);			
			}
			for(int i=0;i<info.noOfPol;i++)
			{
				ptrTimeFlag1=rFIFilteringTime[0]->flags;
				ptrTimeFlag2=rFIFilteringTime[i]->flags;
				for(int j=0;j<basicAnalysis[0]->blockLength;j++,ptrTimeFlag1++,ptrTimeFlag2++)
				{
					*ptrTimeFlag2=*ptrTimeFlag1;			
				}
			}
		}
	}
	
	for(int i=0;i<info.noOfPol;i++)
		histogramInterval[i]=histogramIntervalTemp[i];
	delete[] histogramIntervalTemp;
	
}
/*******************************************************************
*FUNCTION: void thread4Tasks(int threadPacketIndex)
*This function performs the tasks on thread four.
*The tasks are :Calculating dedispersed time series and folding it
*to get the profile.
*For optimization considerations refer to document titled
*"Multithreading Considerations for gptool"
*******************************************************************/
void Runtime::fullDMTask(int threadPacketIndex)
{
	float timeCalc=0.0;
	float timeMerge=0.0;
	float timeNormalize=0.0;
	omp_set_nested(1);
	#pragma omp parallel for ordered schedule(dynamic, 1) 
	for(int t=0;t<nThreadMultiplicity;t++)
	{
		//cout<<"dedisp thread id:"<<sched_getcpu()<<endl;
		BasicAnalysis **basicAnalysis=threadPacket[threadPacketIndex+t]->basicAnalysis;
		RFIFiltering **rFIFilteringChan=threadPacket[threadPacketIndex+t]->rFIFilteringChan;
		RFIFiltering **rFIFilteringTime=threadPacket[threadPacketIndex+t]->rFIFilteringTime;
		AdvancedAnalysis **advancedAnalysis=threadPacket[threadPacketIndex+t]->advancedAnalysis;
		AdvancedAnalysis **advancedAnalysisOld;
		if(t==0)
			advancedAnalysisOld=threadPacket[threadPacketIndex]->advancedAnalysisOld;

		else
			advancedAnalysisOld=threadPacket[threadPacketIndex+t-1]->advancedAnalysis;
		for(int k=0;k<info.noOfPol;k++)
		{
			advancedAnalysis[k]=new AdvancedAnalysis(blockIndex+t-(nActions-1)*nThreadMultiplicity,k,basicAnalysis[k]->rawData,basicAnalysis[k]->blockLength);
			timeFullDMCalc-=omp_get_wtime(); //benchmark
			if(info.doReplaceByMean)
				advancedAnalysis[k]->calculateFullDM(basicAnalysis[k]->filteredRawData);
			else
				advancedAnalysis[k]->calculateFullDM(rFIFilteringTime[k]->flags,rFIFilteringChan[k]->flags);
			timeFullDMCalc+=omp_get_wtime(); //benchmark
		}
		timeFullDMCalc-=omp_get_wtime(); //benchmark
		
		#pragma omp ordered
		for(int k=0;k<info.noOfPol;k++)
			advancedAnalysis[k]->mergeExcess(advancedAnalysisOld[k]->excess,advancedAnalysisOld[k]->countExcess,advancedAnalysisOld[k]->excessUnfiltered,advancedAnalysisOld[k]->countExcessUnfiltered);
		timeFullDMCalc+=omp_get_wtime(); //benchmar
		
		for(int k=0;k<info.noOfPol;k++)
		{
			
			timeFullDMCalc-=omp_get_wtime(); //benchmark
			advancedAnalysis[k]->normalizeFullDM();
			timeFullDMCalc+=omp_get_wtime(); //benchmark
			
			timeProfileCalc-=omp_get_wtime(); //benchmark
			advancedAnalysis[k]->calculateProfile();	
			timeProfileCalc+=omp_get_wtime(); //benchmark
		}		
	}
	
}



void intHandler(int) {
    	keepRunning = false;
}


int main(int argc, char *argv[])
{
	double totalTime=omp_get_wtime(); //benchmark	
	
	Information info;		
	info.startTime=0.0;
	info.doFilteringOnly=0;	
	info.doUseTempo2=0;
	info.doZeroDMSub=0;
	info.doRunFilteredMode=0;
	info.psrcatdbPath=NULL;
	info.isInline=0;
	info.shmID=1;
	int arg = 1;
	int nThreadMultiplicity=1;
	info.meanval=8*1024;
  	if(argc <= 1)
  	{
    		cout<<"Not enough arguments."<<endl;
		info.displayNoOptionsHelp();
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
          				info.filepath = argv[arg+1];
          				info.doReadFromFile = 1;
          				arg+=2;
        			}
        			break;
        			case 'o':
        			{          
          				info.outputfilepath = argv[arg+1];
          				arg+=2;
        			}
        			break;
        			case 'r':
        			{          
					cout<<"Reading from shm"<<endl;	
					info.doReadFromFile=0;
          				arg+=1;
        			}
        			break;
        			case 's':
        			{       
					if(string(argv[arg]) == "-shmID")
					{
						info.shmID=info.stringToDouble(argv[arg+1]);				
          					arg+=2;
					}
					else
					{   
						if(info.doReadFromFile)
						{
							info.startTime=info.stringToDouble(argv[arg+1]);
		  					if(info.startTime<0)
		  					{
		  						cout<<"Start time cannot be negetive!"<<endl;
		  						exit(0);
		  					}
		  				}
					}
          				
          				arg+=2;
        			}
				break;
				case 't':
        			{          
					if(string(argv[arg]) == "-tempo2")
					{
						info.doUseTempo2=1;
						arg+=1;
					}
					else
					{
						nThreadMultiplicity=info.stringToDouble(argv[arg+1]);				
          					arg+=2;
					}
        			}
				break;
				case 'z':
        			{          
					if(string(argv[arg]) == "-zsub")
					{
						info.doZeroDMSub=1;
						arg+=1;
					}
        			}
				break;
				case 'i':
        			{          
					if(string(argv[arg]) == "-inline")
					{
						info.doReadFromFile=0;
						//info.doRunFilteredMode=1;
						info.isInline=1;
						arg+=1;
					}
        			}
				break;
				case 'g':
        			{          
					if(string(argv[arg]) == "-gfilt")
					{
						info.doRunFilteredMode=1;
						arg+=1;
					}
        			}
        			break; 
				case 'm':
        			{          
					if(string(argv[arg]) == "-m")
					{
						info.meanval=int(info.stringToDouble(argv[arg+1]));
						arg+=2;
					}
        			}
        			break;
				default:
        			{    
						    
					if(string(argv[arg]) == "-nodedisp")
        					info.doFilteringOnly=1;	
							
          				arg+=1;
        			}
        			break;     
      
    			}
    		}
    		else
    		{
      			cout<<"Invalid option."<<endl;
			info.displayNoOptionsHelp();
      			exit(1);
    		}
  	}

	if(!info.checkGptoolInputFileVersion())
	{
		cout<<"Old version of gptool.in file found.\n Replacing by new version formatting.\n Old version copied to gptool.in.oldver"<<endl;
		cout<<"Note this conversion will fail if the oldversion is not ver 1.5"<<endl;
		info.reformatGptoolInputFile();
	}
	info.readGptoolInputFile();

	
	

	startFlags=new char[info.startChannel];
	endFlags=new char[info.noOfChannels-info.stopChannel];
	for(int i=0;i<info.startChannel;i++)
		startFlags[i]=1;
	for(int i=0;i<info.noOfChannels-info.stopChannel;i++)
		endFlags[i]=1;
	Runtime* runtime=new Runtime(info,nThreadMultiplicity);
	runtime->intializeFiles();
	
		
		
	
	//code to capture cltr+c termination
	struct sigaction act;
    	act.sa_handler = intHandler;
    	sigaction(SIGINT, &act, NULL);
	
    	
    	#pragma omp parallel sections 
	{
			
				
		#pragma omp section
		{
			
			runtime->action(0,0);
					
		}
		#pragma omp section
		{
			runtime->fillPipe();
			double loopTime=omp_get_wtime(); //benchmark
			runtime->loopThrough();
			loopTime=omp_get_wtime()-loopTime;
			if(!keepRunning)
				runtime->quickclosePipe();
			else
				runtime->closePipe();
		}
	}

	char *gptoolPath = getenv("GPTOOL_PATH");
	cout<<gptoolPath<<endl;
	if(gptoolPath==NULL)
		cout<<"Cannot plot summary pdf. Please set GPTOOL_PATH to path of plotgptoolsummary.py"<<endl;
	else
	{
		cout<<"plotting summmary pdf..."<<endl;	
		stringstream plotcmd;
		plotcmd<<"python2 "<<gptoolPath<<"/plotgptoolsummary.py";	
		system(plotcmd.str().c_str());
	}
	//writing benchmark files
	int i=runtime->blockIndex;
	ofstream benchmarkfile;
	benchmarkfile.open("benchmark.gpt",ios::app);	
	benchmarkfile<<info.samplingInterval<<",";
	benchmarkfile<<info.blockSizeSamples<<",";	
	benchmarkfile<<(timeReadData+timeWaitTime)/(float)(i)<<",";
	benchmarkfile<<(timeConvertToFloat)/(float)(i)<<",";
	benchmarkfile<<-timeWaitTime/(float)(i)<<",";
	benchmarkfile<<timeBandshape/(float)(i)<<",";
	benchmarkfile<<timeZeroDM/(float)(i)<<",";
	benchmarkfile<<timeNormalization/(float)(i)<<",";
	benchmarkfile<<timeRFITimeStats/(float)(i)<<",";
	benchmarkfile<<timeRFITimeFlags/(float)(i)<<",";
	benchmarkfile<<timeRFITimeFlagsWrite/(float)(i)<<",";
	benchmarkfile<<timeRFIChanStats/(float)(i)<<",";
	benchmarkfile<<timeRFIChanFlag/(float)(i)<<",";
	benchmarkfile<<timeRFIChanFlagsWrite/(float)(i)<<",";
	benchmarkfile<<timeFullDMCalc/(float)(i)<<",";
	benchmarkfile<<timeFullDMWrite/(float)(i)<<",";
	benchmarkfile<<timeProfileCalc/(float)(i)<<",";
	benchmarkfile<<timeFullDMUnfilteredCalc/(float)(i)<<",";
	benchmarkfile<<timeFullDMUnfilteredWrite/(float)(i)<<",";
	benchmarkfile<<timeProfileUnfilteredCalc/(float)(i)<<",";
	benchmarkfile<<timePlot/(float)(i)<<endl;
	benchmarkfile.close();
	
	totalTime=omp_get_wtime()-totalTime;
	
	benchmarkfile.open("benchmark_threadtime.gpt",ios::app);	
	benchmarkfile<<nThreadMultiplicity<<","<<runtime->info.blockSizeSamples<<","<<i<<","<<timeThread1/(float)(numberOfThreadRuns)<<","<<timeThread2/(float)(numberOfThreadRuns)<<","<<timeThread3/(float)(numberOfThreadRuns)<<","<<timeThread4/(float)(numberOfThreadRuns)<<","<<timeWaitTime<<","<<fillTime<<","<<totalTime<<endl;
	benchmarkfile.close();
	
	benchmarkfile.open("benchmark_fillTime.gpt",ios::app);
	benchmarkfile<<nThreadMultiplicity<<","<<fillTime<<endl;	
	benchmarkfile.close();
	delete runtime;
	
	exit(0);
	
}



