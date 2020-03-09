GADGETRON IMAGE RECONSTRUCTION FRAMEWORK
========================================

1. Forked from 3.17_stable to resurrect the Matlab Acquisition Gadget

2. Need to install the following dependencies:
```
sudo apt-get install libhdf5-serial-dev cmake git-core \
libboost-all-dev build-essential libfftw3-dev h5utils \
hdf5-tools python-dev python-numpy liblapack-dev libxml2-dev \
libxslt-dev libarmadillo-dev libace-dev python-h5py \
python-matplotlib python-libxml2 gcc-multilib python-psutil \
libgtest-dev libplplot-dev csh libatlas-base-dev
```
3. Execute GT_install3.17.sh to install without root

4. Add to .bashrc
```
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/home/user/gadgetron/local/lib
alias gadgetron="/home/user/gadgetron/local/bin/gadgetron -p 9001"
```
