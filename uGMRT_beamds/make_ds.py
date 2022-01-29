from optparse import OptionParser
import numpy as np,pandas as pd,seaborn as sns,matplotlib.pyplot as plt,os,glob
import uGMRT_beamutils
from astropy.time import Time
from candidate import SigprocFile

'''
This script is written by Devojyoti Kansabanik, NCRA-TIFR, May 2019
'''


datadir=os.path.dirname(uGMRT_beamutils.__file__)

def convert_16to8_bit(rawfil,outfil):
	'''
	Function to convert RAW 16-bit file into 8-bit file
	Parameters
	----------
	rawfil : str
		Name of the RAW 16-bit file
	outfil : str
		Output 8-bit file name
	Returns
	-------
	str
		Name of the 8-bit file
	'''
	readPA=datadir+'/readPA/read_PAbeam.polar.freqint.16_to_8'
	print (readPA+' '+rawfil+' '+outfil+' '+str(duration)+' '+str(nchan)+' '+timeres+' 1 8'+'\n')
	os.system(readPA+' '+rawfil+' '+outfil+' '+str(duration)+' '+str(nchan)+' '+timeres+' 1 8')
	return outfil	

def make_hdr(raw_file,timestampfile,working_dir='',beammode='PA',nchan=4096,start_freq=500,sideband='LSB',bandwidth=200,nbit=8,timeres=81.92,sourcename='',sourcera='',sourcedec=''):
	'''
	Function to make header file
	Parameters
	----------
	raw_file : str
		RAW beamformer file name (Full Path)
	timestampfile : str
		Time stamp file name (full path)
	beammode : str
		Beamformer mode ('IA' or 'PA')
	nchan : int
		Number of channels
	start_freq : float
		Start channel frequency in MHz
	sideband : str
		Sideband used in observation (LSB : Lower side band, USB : Upper side band)
	bandwidth : float
		Bandwidth of observation in MHz
	nbit : int
		Number of bits per sample (8 or 16)
	timeres : float
		Time resolution of the observation in microsecond
	sourcename : str
		Source name (optional)
	sourcera : str 
		Source RA in hh:mm:ss format (optional)
	sourcedec : str
		Source DEC in dd:mm:ss format (optional)
	Returns
	-------
	str
		Linked data file path
	str
		Header file path
	'''
	if working_dir=='':
		working_dir=os.getcwd()
	elif os.path.isdir(working_dir)==False:
		os.makedirs(working_dir)
	if os.path.exists(raw_file)==False:
		print ('RAW data file does not exist.\n')
		return 1
	if os.path.exists(timestampfile)==False:
		print ('Timestamp file does not exist.\n')
		return 1
	timfile=open(timestampfile,'r')
	try:
		time_list=timfile.readlines()[0].split(' ')[:7]
		date=Time('-'.join(time_list[:3]))
		mjddate=date.mjd
		utc_time=':'.join(time_list[3:5])+':'+str(float(time_list[5])+float(time_list[6]))
		timfile.close()
	except:
		try:
			lines=timfile.readlines()
			date= lines[2].split('Date: ')[-1].split('\n')[0].split(':').reverse()
			date=Time('-'.join(date))
			mjddate=date.mjd
			utc_time=lines[1].split('IST Time: ')[-1].split('\n')[0]
			timfile.close()
		except:
			print ('Wrong timestamp file format.\n')
			timfile.close()
			return
	if os.path.exists(working_dir+'/'+os.path.basename(raw_file)+'.gmrt_dat'):
		if os.path.islink(working_dir+'/'+os.path.basename(raw_file)+'.gmrt_dat'):
			os.unlink(working_dir+'/'+os.path.basename(raw_file)+'.gmrt_dat')
		else:
			os.system('rm -rf '+working_dir+'/'+os.path.basename(raw_file)+'.gmrt_dat')
	os.system('ln -s '+raw_file+' '+working_dir+'/'+os.path.basename(raw_file)+'.gmrt_dat')
	if os.path.exists(working_dir+'/'+os.path.basename(raw_file)+'.gmrt_hdr'):
		os.system('rm -rf '+working_dir+'/'+os.path.basename(raw_file)+'.gmrt_hdr')
	os.system('cp -r '+datadir+'/template.gmrt.hdr '+working_dir+'/'+os.path.basename(raw_file)+'.gmrt_hdr')
	if sideband=='LSB':
		lowest_freq=int(start_freq-bandwidth)
	else:
		lowest_freq=start_freq
	freq_res=bandwidth/nchan
	if sideband=='LSB':
		freq_res=freq_res*-1
	timfil=open(timestampfile)
	headfil=open(working_dir+'/'+os.path.basename(raw_file)+'.gmrt_hdr','r+')
	lines=headfil.readlines()
	for i in range(len(lines)):
		if 'Array Mode' in lines[i]:
			lines[i]='Array Mode	: '+str(beammode)+'\n'
		if 'Num Channels' in lines[i]:
			lines[i]='Num Channels    : '+str(nchan)+'\n'
		if 'Channel width' in lines[i]:
			lines[i]='Channel width   : '+str(freq_res)+'\n'
		if 'Sampling Time' in lines[i]:
			lines[i]='Sampling Time   : '+str(timeres)+'\n'
		if 'Num bits/sample' in lines[i]:
			lines[i]='Num bits/sample : '+str(nbit)+'\n'	
		if sourcename!='':
			if 'Source' in lines[i]:
				lines[i]='Source          : '+str(sourcename)+'\n'
		if sourcera!='' and sourcedec!='':
			if 'Coordinates     : ' in lines[i]:
				lines[i]='Coordinates     : '+str(sourcera)+', '+str(sourcedec)+'\n'
		if 'MJD' in lines[i]:
			lines[i]='MJD             : '+str(mjddate)+'\n'
		if 'UTC' in lines[i]:
			lines[i]='UTC             : '+str(utc_time)+'\n'
	headfil.close()
	os.system('rm -rf '+working_dir+'/'+os.path.basename(raw_file)+'.gmrt_hdr')
	headfil=open(working_dir+'/'+os.path.basename(raw_file)+'.gmrt_hdr','a+')
	headfil.writelines(lines)
	headfil.close()
	return os.path.basename(raw_file)+'.gmrt_dat',os.path.basename(raw_file)+'.gmrt_hdr'

def run_gptool(raw_file,working_dir='',beammode='PA',nchan=4096,start_freq=500,sideband='LSB',bandwidth=200,nbit=8,timeres=81.92,freqsigma=5,timesigma=5,num_round=1):
	'''
	Function to make header file
	Parameters
	----------
	raw_file : str
		RAW beamformer file name (Full Path)
	beammode : str
		Beamformer mode ('IA' or 'PA')
	nchan : int
		Number of channels
	start_freq : float
		Start channel frequency in MHz
	sideband : str
		Sideband used in observation (LSB : Lower side band, USB : Upper side band)
	bandwidth : float
		Bandwidth of observation in MHz
	nbit : int
		Number of bits per sample (8 or 16)
	timeres : float
		Time resolution of the observation in microsecond
	freqsigma : float
		Sigma value for flagging along frequency axis
	timesigma : float
		Sigma value for time axis flagging
	num_round : int
		Number of flagging round
	'''
	if os.path.exists(working_dir+'/gptool.in'):
		os.system('rm -rf '+working_dir+'/gptool.in')
	os.system('cp -r '+datadir+'/gptool.in '+working_dir+'/gptool.in')
	gptool_path=datadir+'/gptool_ver4.2.1/gptool'
	if os.path.exists(raw_file)==False:
		print ('RAW data file does not exist.\n')
		return 
	os.chdir(working_dir)
	if sideband=='LSB':
		lowest_freq=int(start_freq-bandwidth)
	else:
		lowest_freq=start_freq
	do_flag=True
	freq_smooth_window=int((20/4096.0)*nchan)
	for nround in range(num_round):
		gptool_input=open(working_dir+'/gptool.in','r+')
		lines=gptool_input.readlines()
		for i in range(len(lines)):
			if 'Beam mode' in lines[i]:
				lines[i]=beammode+'\t\t:Beam mode\n'
			if 'Sample size of data (in bytes, usually 2)' in lines[i]:
				lines[i]=str(8/int(nbit))+'\t\t:\tSample size of data (in bytes, usually 2)\n'
			if 'Frequency band (lowest value in Mhz)' in lines[i]:
				lines[i]=str(lowest_freq)+'\t\t:\tFrequency band (lowest value in Mhz)\n'
			if 'Bandwidth(in Mhz)' in lines[i]:
				lines[i]=str(bandwidth)+'\t\t:\tBandwidth(in Mhz)\n'
			if 'Sideband flag (-1-> decreasing +1-> increasing)' in lines[i]:
				if sideband=='LSB':
					lines[i]='-1\t\t:\tSideband flag (-1-> decreasing +1-> increasing)\n'
				elif sideband=='LSB':
					lines[i]='1\t\t:\tSideband flag (-1-> decreasing +1-> increasing)\n'
			if 'Number of channels' in lines[i] and 'Number of channels to flag at band beginning' not in lines[i] and 'Number of channels to flag at band end' not in lines[i]:
				lines[i]=str(nchan)+'\t\t:Number of channels\n'
			if 'Sampling Interval (in ms)' in lines[i]:
				lines[i]=str(timeres/1000.0)+'\t\t:\tSampling Interval (in ms)\n'		
			if 'Threshold for frequency flagging (in units of RMS deviation)' in lines[i]:
				lines[i]=str(freqsigma)+'\t\t:\tThreshold for frequency flagging (in units of RMS deviation)\n'
			if 'Threshold for time flagging (in units of RMS deviation)' in lines[i]:
				lines[i]=str(timesigma)+'\t\t:Threshold for time flagging (in units of RMS deviation)\n'
			if 'Smoothing window size for bandshape normalization (in number of channels)' in lines[i]:
				lines[i]=str(freq_smooth_window)+'\t\t:Smoothing window size for bandshape normalization (in number of channels)\n'
			if 'Normalization procedure (1-> cumulative smooth bandshape, 2-> externally supplied bandshape.dat)' in lines[i]:	
				if nround==1 and os.path.exists(working_dir+'/bandshape.dat'):
					print ('Performing second rounf using previous round cumalative bandshape.\n')
					do_flag=True
					lines[i]='2\t\t:\tNormalization procedure (1-> cumulative smooth bandshape, 2-> externally supplied bandshape.dat)\n'
				elif nround==1:
					do_flag=False
					lines[i]='1\t\t:\tNormalization procedure (1-> cumulative smooth bandshape, 2-> externally supplied bandshape.dat)\n'
				else:
					do_flag=True
			if num_round>1 and nround<num_round-1:
				if 'Number of channels to flag at band beginning' in lines[i]:
					lines[i]='0\t\t:\tNumber of channels to flag at band beginning\n'
				if 'Number of channels to flag at band end' in lines[i]:
					lines[i]='0\t\t:\tNumber of channels to flag at band end\n'
			elif nround==num_round-1 or num_round==1:
				if 'Number of channels to flag at band beginning' in lines[i]:
					lines[i]='50\t\t:\tNumber of channels to flag at band beginning\n'
				if 'Number of channels to flag at band end' in lines[i]:
					lines[i]='50\t\t:\tNumber of channels to flag at band end\n'
		gptool_input.seek(0)
		gptool_input.writelines(lines)
		gptool_input.close()
		outputfile=working_dir+'/'+os.path.basename(raw_file)+'.gpt'
		gptool_cmd_args=[gptool_path,'-m 64','-nodedisp','-o '+working_dir,'-f '+raw_file]
		if do_flag:
			print ('Flagging round : '+str(nround)+'\n######################\n')
			print (' '.join(gptool_cmd_args)+'\n')
			os.system(' '.join(gptool_cmd_args))
			if nround==1 and num_round>1:
				os.system('cp -r '+working_dir+'/bandshape.gpt '+working_dir+'/bandshape.dat')
	print ('Flagging done.\n')
	return outputfile

def make_filterbank(rawfil):
	'''
	Function to make filterbank file
	Parameters
	----------
	rawfil : str
		Soft linked RAW file name with .gmrt_dat extension (.gmrt_hdr header file should be present)
	Returns
	-------
	str
		Filterbank file name
	'''
	filterbank_path=datadir+'/sigproc_install/bin/filterbank'
	filterbank_file='.'.join(rawfil.split('.')[:-1])+'.fil'
	print ('Converting to filterbak format....\n')
	print (filterbank_path+' '+rawfil+' -o '+filterbank_file+'\n')
	os.system(filterbank_path+' '+rawfil+' -o '+filterbank_file)
	return filterbank_file

def read_and_save_filterbank(filterbank_file,output_file=''):
	'''
	Function to read and save filterbank data as pandas Dataframe
	Parameters
	----------
	filterbank_file : str
		Name of the filterbank file
	output_file : str
		Name of the output file
	Returns
	-------
	str
		Name of the output file to save the data
	'''
	fil=SigprocFile(fp=filterbank_file)
	tstart=fil.tstart
	tend=fil.tend
	tsamp=fil.tsamp
	ntot=int((tend-tstart)*24*3600/(tsamp))-1
	data=fil.get_data(0,ntot)[:,0,:]
	timelist=np.arange(fil.tstart,fil.tend,fil.tsamp/(24*3600))[:data.shape[0]]
	freqlist=np.arange(fil.fch1,fil.fch1+(fil.foff*fil.nchans),fil.foff)[:data.shape[1]]
	zeropos=np.where(data==0)
	data[zeropos]=np.nan
	pdataframe=pd.DataFrame(data.T,columns=timelist,index=np.round(freqlist,2))	
	if output_file=='':
		output_file=filterbank_file+'.pd'
	pdataframe.to_pickle(output_file)
	return output_file

def main():
	usage= 'Make dynamic spectrum from uGMRT beamformer data file'
	parser = OptionParser(usage=usage)
	parser.add_option('--rawfile',dest="rawfile",default=None,help="Name of raw uGMRT beam data file",metavar="File path")
	parser.add_option('--timestamp_file',dest="timestamp",default=None,help="Name of raw uGMRT beam data timestamp file",metavar="File path")
	parser.add_option('--workdir',dest='workdir',default=None,help='Name of the working directory',metavar='Directory path')
	parser.add_option('--verbose',dest="verbose",default=False,help="Verbose mode",metavar="Boolean")
	parser.add_option('--nbit',dest="nbit",default=16,help="Bit samples of the beam data recording",metavar="Integer")
	parser.add_option('--start_freq',dest="start_freq",default=None,help="Start frequency in MHz (LO frequency)",metavar="Float")
	parser.add_option('--bandwidth',dest="bandwidth",default=None,help="Bandwidth in MHz",metavar="Float")
	parser.add_option('--nchan',dest="nchan",default=None,help="Number of channels",metavar="Integer")
	parser.add_option('--sideband',dest="sideband",default='USB',help="Side band used ('LSB' for lower sideband, 'USB' for upper sideband)",metavar="String")
	parser.add_option('--beam_type',dest="beammode",default='IA',help="Beam type ('IA' for Incoherent array, 'PA' for Phased array)",metavar="String")
	parser.add_option('--timeres',dest="timeres",default=81.92,help="Time resolution in micro second",metavar="Float")
	parser.add_option('--flag_round',dest="nflag",default=1,help="GPtool flagging rounds",metavar="Integer")
	parser.add_option('--do_flag',dest="do_flag",default=True,help="Perform flagging using GPtool",metavar="Boolean")
	parser.add_option('--source_name',dest="source",default='',help="Source name (optional)",metavar="String")
	parser.add_option('--source_ra',dest="sourcera",default='',help="Source RA (optional)",metavar="String")
	parser.add_option('--source_dec',dest="sourcedec",default='',help="Source DEC (optional)",metavar="String")
	(options, args) = parser.parse_args()

	if options.rawfile==None or os.path.exists(options.rawfile)==False:
		print ('RAW uGMRT beam data file does not exist.\n')
		return 1
	else:
		if options.timestamp==None:
			print ('Please provide correct timestamp file.\n')
			return 1
		else:
			timestampfile=options.timestamp
	if options.workdir==None:
		workdir=os.getcwd()+'/temp'
		if os.path.isdir(workdir):
			os.system('rm -rf '+workdir)
		os.makedirs(workdir)
	elif os.path.isdir(options.workdir)==False:
		workdir=os.getcwd()+'/temp'
		if os.path.isdir(workdir):
			os.system('rm -rf '+workdir)
		os.makedirs(workdir)
	else:
		workdir=options.workdir

	if options.start_freq==None:
		print ('Please provide start frequency in MHz.\n')
		return 1
	elif float(options.start_freq)<100 or float(options.start_freq)>1500:
		print ('Start frequency is outside uGMRT band. Please provide correct start frequency in MHz.\n') 
		return 1	
		
	if options.bandwidth==None or float(options.bandwidth)>400:
		print ('Please provide correct bandwidth.\n')
		return 1

	if options.nchan==None:
		print ('Please provide correct number of channels.\n')
		return 1

	if options.sideband!='USB' and options.sideband!='LSB':
		print ('Sideband should be either \'LSB\' or \'USB\'\n')
		return 1

	verbose=eval(str(options.verbose))
	if options.rawfile[-1]=='/':
		options.rawfile=options.rawfile[:-1]

	cwd=os.getcwd()
	os.chdir(workdir)

	if int(options.nbit)==16:
		if verbose:
			print ('Converting 16 bit to 8 bit....\n')
		rawfile=convert_16to8_bit(options.rawfile,os.path.basename(options.rawfile)+'.8bit')
	else:
		rawfile=options.rawfile

	if eval(str(options.do_flag)):
		if verbose:
			print ('Perform GPTool flagging.....\n')
		rawfile=run_gptool(rawfile,working_dir=workdir,beammode=options.beammode,nchan=int(options.nchan),start_freq=float(options.start_freq),sideband=options.sideband,\
				bandwidth=float(options.bandwidth),nbit=8,timeres=float(options.timeres),freqsigma=5,timesigma=5,num_round=int(options.nflag))
	
	result=make_hdr(rawfile,timestampfile,working_dir=workdir,beammode=options.beammode,nchan=int(options.nchan),start_freq=float(options.start_freq),sideband=options.sideband,\
				bandwidth=float(options.bandwidth),nbit=8,timeres=float(options.timeres),sourcename=options.source,sourcera=options.sourcera,sourcedec=options.sourcedec)
	if type(result)==int and result==1:
		print ('Flagged raw file does not exist.\n')
		return 1
	linked_file,header_file=result
	filterbank_file=make_filterbank(linked_file)
	final_numpy_table=read_and_save_filterbank(filterbank_file,output_file='')
	return final_numpy_table

if __name__=='__main__':
	msg=main()
	print (msg)
	if msg==1:
		os._exit(msg)
	else:
		print ('Final data saved in : '+msg+'\n')
		os._exit(0)


