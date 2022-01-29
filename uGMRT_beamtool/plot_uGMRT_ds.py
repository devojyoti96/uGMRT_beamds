from optparse import OptionParser
import numpy as np,pandas as pd,seaborn as sns,matplotlib.pyplot as plt,os,glob

'''
This script is written by Devojyoti Kansabanik, NCRA-TIFR, May 2019
'''

def plot_ds(datafile,timerange='',freqrange='',outfile='',show_plot=False):
	'''
	Function to plot uGMRT dynamic spectrum
	(NB : If the data array has more than 4096x4096 size, it will plot 4096x4096 plots separately)
	Parameters
	----------
	datafile : str
		Panda Dataframe file
	timerange : str
		Timerange to plot in MJD (startime,endtime)
	freqrange : str
		Frequency range in MHz (startfreq,endfreq)
	outfile : str
		Name of the output plot file (with extension)
	show_plot : bool
		Display iinteractive plot
	'''
	df=pd.read_pickle(datafile)
	timestamps=np.array(df.columns)
	freqs=np.array(df.index)
	if timerange!='':
		starttime=float(timerange.split(',')[0])
		endtime=float(timerange.split(',')[-1])
	else:
		starttime=timestamps[0]
		endtime=timestamps[-1]
	if freqrange!='':
		startfreq=float(freqrange.split(',')[0])
		endfreq=float(freqrange.split(',')[-1])
	else:
		startfreq=freqs[0]
		endfreq=freqs[-1]
	if freqrange!='':
		freq0=np.argmin(abs(freqs-startfreq))
		freq1=np.argmin(abs(freqs-endfreq))
		if freq0==freq1:
			print ('Provide correct frequency range.')
			freqrange=''
	if timerange!='':
		time0=np.argmin(abs(timestamps-starttime))
		time1=np.argmin(abs(timestamps-endtime))
		if time1==0 or time0==time1:
			print ('Provide a correct time range.\n')
			timerange=''
	if timerange!='' and freqrange=='':
		df_sliced=df.iloc[:,time0:time1]
	elif timerange=='' and freqrange!='':
		df_sliced=df.iloc[freq0:freq1,:]
	elif freqrange!='' and timerange!='':
		df_sliced=df.iloc[freq0:freq1,time0:time1]	
	else:
		df_sliced=df
	shape=df_sliced.shape
	if shape[0]==0 or shape[1]==0:
		print ('Either or frequency slice is zero. Provide correct time or frequnecy slice.\n')
		return 
	if outfile=='':
		outfile='uGMRT_DS.png'
	if shape[0]>4096 or shape[1]>4096:
		print('Data array size is : '+str(shape)+' , which is more than 4096 either in time or frequency. It may long time to plot and may face memory issue. Plot small chunks.\n')
		print ('Time range (MJD) : '+str(starttime)+'~'+str(endtime)+'\n')
		print ('Frequency range (MHz) : '+str(startfreq)+'~'+str(endfreq)+'\n')
		cont=input('Do you still want to continue? Y/N\n')
		if cont=='y' or cont=='Y':
			plt.style.use('tableau-colorblind10')
			fig,ax=plt.subplots(figsize=(12,8))	
			s=sns.heatmap(df_sliced,cbar_kws={'label': 'Uncalibrated flux density'},linewidths=0.0,rasterized=True)
			s.figure.axes[-1].yaxis.label.set_size(15)
			s.figure.axes[-1].tick_params(labelsize=15)
			sns.despine(top=False,right=False,bottom=False,left=False)
			times=df_sliced.columns
			time_indices=[]
			time_ticks=[]
			skip_time=int(len(times)/20)
			for i in range(len(times)):
				if i==0 or (i/skip_time-i//skip_time)==0:
					time_indices.append(i)
					time_ticks.append(times[i])
			plt.xticks(time_indices,time_ticks,rotation=30,ha='right',fontsize=10)
			plt.yticks(fontsize=10)
			fig.tight_layout(rect=[0, 0.03, 1, 0.95])	
			plt.xlabel('Timestamps (MJD)',fontsize=15)
			plt.ylabel('Frequency (MHz)',fontsize=15)
			plt.savefig(outfile)
			if show_plot:
				plt.show()
		else:
			return
	else:
		plt.style.use('tableau-colorblind10')
		fig,ax=plt.subplots(figsize=(12,8))
		s=sns.heatmap(df_sliced,cbar_kws={'label': 'Uncalibrated flux density'},linewidths=0.0,rasterized=True)
		s.figure.axes[-1].yaxis.label.set_size(15)
		s.figure.axes[-1].tick_params(labelsize=15)
		sns.despine(top=False,right=False,bottom=False,left=False)
		times=df_sliced.columns
		time_indices=[]
		time_ticks=[]
		skip_time=int(len(times)/20)
		for i in range(len(times)):
			if i==0 or (i/skip_time-i//skip_time)==0:
				time_indices.append(i)
				time_ticks.append(times[i])
		plt.xticks(time_indices,time_ticks,rotation=30,ha='right',fontsize=10)
		plt.yticks(fontsize=10)
		fig.tight_layout(rect=[0, 0.03, 1, 0.95])	
		plt.xlabel('Timestamps (MJD)',fontsize=15)
		plt.ylabel('Frequency (MHz)',fontsize=15)
		plt.savefig(outfile)
		if show_plot:
			plt.show()
	return outfile

def main():
	usage= 'Plot dynamic spectrum from uGMRT beamformer pandas Dataframe file'
	parser = OptionParser(usage=usage)
	parser.add_option('--datafile',dest="datafile",default=None,help="Name of uGMRT beam data pandas dataframe file",metavar="File path")
	parser.add_option('--timerange',dest="timerange",default=None,help="Timerange in MJD (starttime,endtime)",metavar="Comma separated string")
	parser.add_option('--freqrange',dest='freqrange',default=None,help='Frequency range in MHz (startfreq,endfreq)',metavar='Comma separate string')
	parser.add_option('--outfile',dest="outfile",default=None,help="Output plot file name",metavar="File path")
	parser.add_option('--show_plot',dest="show_plot",default=False,help="Show dynamic spectrum",metavar="Boolean")
	(options, args) = parser.parse_args()

	if options.datafile==None or os.path.exists(options.datafile)==False:
		print ('Data file does not exist.\n')
		return 1
	if options.timerange==None:
		timerange=''
	else:
		timerange=options.timerange
	if options.freqrange==None:
		freqrange=''
	else:
		freqrange=options.freqrange 
	if options.outfile==None:
		outfile=''
	else:
		outfile=options.outfile
	show_plot=eval(str(options.show_plot))
	final_plot=plot_ds(options.datafile,timerange=timerange,freqrange=freqrange,outfile=outfile,show_plot=show_plot)
	if final_plot!=None:
		print ('Final plot is saved at : '+final_plot+'\n')
	return 0

if __name__=='__main__':
	msg=main()
	os._exit(msg)


