from setuptools import setup,find_packages
import os,sys,shutil,subprocess,glob,pip,sysconfig
from distutils.sysconfig import get_python_lib
from pkg_resources import resource_filename

with open("README.rst", "r") as fh:
    long_description = fh.read()

setup(
    name='uGMRT_beamds',
    version='1.1.1',
    packages=find_packages(),
    package_data={'uGMRT_beamutils':['*','gptool_ver4.2.1/*','sigproc_install/bin/*','readPA/*']},
    author='Devojyoti Kansabanik',
    author_email='dkansabanik@ncra.tifr.res.in',
	description="A python package to make and plot dynamic spectrum from uGMRT beamformer observations",
	long_description=long_description,
	long_description_content_type='text/markdown',
	url="https://github.com/devojyoti96/uGMRT_beamds",
    install_requires=["numpy==1.19.0", "scipy==1.6.2","astropy==4.3",'pandas','seaborn','pysigproc'],
	python_requires='>=3',
	entry_points={
        'console_scripts': ['make_uGMRT_ds = uGMRT_beamtool.make_ds:main','plot_uGMRT_ds = uGMRT_beamtool.plot_uGMRT_ds:main']}
)
