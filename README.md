GADGETRON IMAGE RECONSTRUCTION FRAMEWORK
========================================

1. Forked from 3.17_stable to resurrect the Matlab Acquisition Gadget

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
3. Execute ```GT_install3.17.sh``` to install in /home/user/gadgetron instead of /usr (no root required)

4. Add to .bashrc
```
export PATH=$PATH:/home/user/gadgetron/local/bin
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/home/user/gadgetron/local/lib
alias gadgetron="export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$MATLAB_ROOT/bin/glnxa64;gadgetron"
```

5. Add to matlab ```~/Documents/MATLAB/startup.m```
```
addpath('/home/user/gadgetron/local/share/ismrmrd/matlab/');
addpath('/home/user/gadgetron/local/share/gadgetron/matlab/');
```
