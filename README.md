GADGETRON IMAGE RECONSTRUCTION FRAMEWORK
========================================

1. Forked from ```3.17_stable``` to resurrect the Matlab Acquisition Gadget

2. Need to install the following dependencies (Ubuntu 18.04):
```
sudo apt-get install libhdf5-serial-dev cmake git-core \
    libboost-all-dev build-essential libfftw3-dev h5utils \
    hdf5-tools python-dev python-numpy liblapack-dev libxml2-dev \
    libxslt-dev libarmadillo-dev libace-dev python-h5py \
    python-matplotlib python-libxml2 gcc-multilib python-psutil \
    libgtest-dev libplplot-dev csh libatlas-base-dev qt4-default \
    git-core wget make cmake gcc-multilib libgtest-dev libboost-all-dev \
    libarmadillo-dev libopenblas-dev libfftw3-dev liblapack-dev liblapacke-dev \
    libxml2-dev libxslt-dev libpugixml-dev libhdf5-dev libplplot-dev libdcmtk-dev \
    python3-dev python3-pip python3-h5py python3-scipy python3-pyxb
```
3. Add to ```~/.bashrc``` to set up paths for ```~/gadgetron3.17``` (no root required)
<pre>
## gadgetron - use port 9001 instead of default 9002 for main branch 
export GADGETRON_HOME=~/gadgetron3.17/local
export ISMRMRD_HOME=$GADGETRON_HOME
export PATH=$PATH:$GADGETRON_HOME/bin
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$GADGETRON_HOME/lib

<b>## path to matlab - EDIT THIS LINE AS NEEDED</b>
<b>export MATLAB_ROOT=/usr/local/MATLAB/R2020a</b>

## tricky - need to adjust the path just for the process running gadgetron
alias gadgetron="PATH=$MATLAB_ROOT/bin:$PATH;LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$MATLAB_ROOT/bin/glnxa64 gadgetron -p 9001"
</pre>

4. Execute ```source ~/.bashrc``` to make paths active

5. Copy [```GT_install3.17.sh```](https://github.com/marcsous/gadgetron-matlab/blob/Gadgetron3.17_stable/GT_install3.17.sh), chmod +x and execute to install in ```~/gadgetron3.17``` (no root required)

6. Add to ```~/Documents/MATLAB/startup.m``` to set up paths in matlab. It would be nice to read ISMRMRD_HOME and GADGETRON_HOME using getenv() but it doesn't work for me
```
addpath('~/gadgetron3.17/recon/');
addpath('~/gadgetron3.17/local/share/ismrmrd/matlab/');
addpath('~/gadgetron3.17/local/share/gadgetron/matlab/');
```

7. Execute ```gadgetron``` and (hopefully) see the following:
```
03-10 11:45:50.868 INFO [main.cpp:101] Starting Gadgetron (version 3.17.0)
03-10 11:45:50.868 INFO [main.cpp:193] Starting ReST interface on port 9080
03-10 11:45:50.878 INFO [main.cpp:205] Starting cloudBus: localhost:8002
03-10 11:45:50.879 INFO [main.cpp:253] Configuring services, Running on port 9002
```
8. Go to [gadgetron-example](https://github.com/marcsous/gadgetron-example) to do an example
