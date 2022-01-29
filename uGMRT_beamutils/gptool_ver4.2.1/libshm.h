#include "gmrt_newcorr.h"

class Correlator
{	
	public:
		void initializeReadSHM();
		int initializeWriteSHM();
		void writeToSHM(unsigned short int* rawData);
		void readFromSHM(unsigned short int* rawData);
		void writeToSHM(unsigned short int* rawData,char* header);
		void copyHeaderInfo();		
		Correlator(int _nchan,float _sampling);
		Correlator(DasHdrType *_dataHdrRead ,DataBufType *_dataBufferRead);
	private:
		static DasHdrType*	dataHdrWrite;
		static DataBufType*	dataBufferWrite;
		static DataTabType*	dataTabWrite;
		static int		recNumWrite;	

		static DasHdrType*	dataHdrRead;
		static DataBufType*	dataBufferRead;
		static DataTabType*	dataTabRead;
		static int		recNumRead;	
		static long int		currentReadBlock;
		int			DataOff;
		char			debug;
		int			nchan;
		float			sampling;
};
