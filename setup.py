from setuptools import setup,find_packages
import os,sys,shutil,subprocess,glob,pip,sysconfig
from distutils.sysconfig import get_python_lib
from pkg_resources import resource_filename


setup(
    name='uGMRT_beamtool',
    version='1.0.0',
    packages=find_packages(),
    package_data={'uGMRT_beamutils':['*','gptool_ver4.2.1/*','sigproc_install/bin/*','readPA/*']},
    author='Devojyoti Kansabanik',
    author_email='dkansabanik@ncra.tifr.res.in',
    description='uGMRT beamformer data analysis tool',
    install_requires=["numpy==1.19.0", "scipy==1.6.2","astropy==4.3","julian","psutil","cmake",'pandas','seaborn','pysigproc'],
	python_requires='>=3',
	entry_points={
        'console_scripts': ['make_uGMRT_ds = uGMRT_beamds.make_ds:main','plot_uGMRT_ds = uGMRT_beamds.plot_uGMRT_ds:main']}
)
