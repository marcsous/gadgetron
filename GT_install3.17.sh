echo "This script will set up Gadgetron and related libraries in the home folder."

# set up the working directory to be the home folder
GT_WORKING_DIR=~/gadgetron
GT_INSTALL_DIR=${GT_WORKING_DIR}/local

# the branch to pull, can be changed
BRANCHNAME=master

# Build type
BUILD_TYPE=Release
# for debug, BUILD_TYPE=Debug

# Generator
GENERATOR="Eclipse CDT4 - Unix Makefiles"

# Disable Cuda
CUDA=false

# Check Matlab path is set
if [ -z ${MATLAB_ROOT} ]
then
    echo Error! MATLAB_ROOT not set.
    exit 1
fi
echo Using MATLAB_ROOT=${MATLAB_ROOT}

# --------------------------------------------------------------------------------------------
# pull source from git hub

ISMRMRD_REPO=https://github.com/ismrmrd/ismrmrd.git
ISMRMRD_BRANCH=${BRANCHNAME}

GT_REPO=https://github.com/marcsous/gadgetron
GT_BRANCH=Gadgetron3.17_stable 

GT_CONVERTER_REPO=https://github.com/ismrmrd/siemens_to_ismrmrd.git
GT_CONVERTER_BRANCH=${BRANCHNAME}

ISMRMRD_PYTHON_API_REPO=https://github.com/ismrmrd/ismrmrd-python.git
ISMRMRD_PYTHON_API_BRANCH=${BRANCHNAME}

ISMRMRD_PYTHON_TOOLS_REPO=https://github.com/ismrmrd/ismrmrd-python-tools.git
ISMRMRD_PYTHON_TOOLS_BRANCH=${BRANCHNAME}

# ----------------------------------------------------------------------------------------------------------
# clean the old installation
# ----------------------------------------------------------------------------------------------------------
echo Deleting ${GT_WORKING_DIR}
read -p "Are you sure (type Y to proceed)? " -n 1 -r
echo    # (optional) move to a new line
if [[ ! $REPLY =~ ^[Yy]$ ]]
then
    exit 1
fi

rm -R -f ${GT_WORKING_DIR}/local
mkdir ${GT_WORKING_DIR}/local
mkdir ${GT_WORKING_DIR}/mrprogs

GADGETRON_HOME=${GT_WORKING_DIR}/local
ISMRMRD_HOME=${GT_WORKING_DIR}/local
CMAKE_PREFIX_PATH=${GT_WORKING_DIR}/local/lib/cmake/ISMRMRD

# ----------------------------------------------------------------------------------------------------------
#ISMRMRD PYTHON API, require sudo to install
# ----------------------------------------------------------------------------------------------------------
cd ${GT_WORKING_DIR}/mrprogs
git clone ${ISMRMRD_PYTHON_API_REPO}
cd ismrmrd-python
python setup.py install

# ----------------------------------------------------------------------------------------------------------
#ISMRMRD PYTHON TOOLS, require sudo to install
# ----------------------------------------------------------------------------------------------------------
cd ${GT_WORKING_DIR}/mrprogs
git clone ${ISMRMRD_PYTHON_TOOLS_REPO}
cd ismrmrd-python-tools
python setup.py install

# ----------------------------------------------------------------------------------------------------------
# ismrmrd
# ----------------------------------------------------------------------------------------------------------
rm -R -f ${GT_WORKING_DIR}/mrprogs/ismrmrd
cd ${GT_WORKING_DIR}/mrprogs
git clone ${ISMRMRD_REPO} ${GT_WORKING_DIR}/mrprogs/ismrmrd
cd ${GT_WORKING_DIR}/mrprogs/ismrmrd
git checkout -b ${ISMRMRD_BRANCH} origin/${ISMRMRD_BRANCH}

rm -R -f ${GT_WORKING_DIR}/mrprogs/build_ismrmrd_${BUILD_TYPE}
cd ${GT_WORKING_DIR}/mrprogs
mkdir build_ismrmrd_${BUILD_TYPE}
cd ${GT_WORKING_DIR}/mrprogs/build_ismrmrd_${BUILD_TYPE}

cmake -G "${GENERATOR}" -DCMAKE_INSTALL_PREFIX=${GT_INSTALL_DIR} -DCMAKE_BUILD_TYPE=${BUILD_TYPE} -DJava_JAVAC_EXECUTABLE= -DJava_JAVA_EXECUTABLE= -DJAVA_INCLUDE_PATH=  ../ismrmrd

make -j $(nproc)
make install

# ----------------------------------------------------------------------------------------------------------
# gadgetron
# ----------------------------------------------------------------------------------------------------------
rm -R -f ${GT_WORKING_DIR}/mrprogs/gadgetron
cd ${GT_WORKING_DIR}/mrprogs
git clone  ${GT_REPO} ${GT_WORKING_DIR}/mrprogs/gadgetron
cd ${GT_WORKING_DIR}/mrprogs/gadgetron
git checkout -b ${GT_BRANCH} origin/${GT_BRANCH}

rm -R -f ${GT_WORKING_DIR}/mrprogs/build_gadgetron_${BUILD_TYPE}
cd ${GT_WORKING_DIR}/mrprogs
mkdir build_gadgetron_${BUILD_TYPE}
cd ${GT_WORKING_DIR}/mrprogs/build_gadgetron_${BUILD_TYPE}

cmake -DUSE_CUDA=${CUDA} -G "${GENERATOR}" -DCMAKE_INSTALL_PREFIX=${GT_INSTALL_DIR} -DCMAKE_BUILD_TYPE=${BUILD_TYPE} -DCMAKE_PREFIX_PATH=${GT_WORKING_DIR}/local/lib/cmake/ISMRMRD ../gadgetron

make -j $(nproc)
make install
    
# ----------------------------------------------------------------------------------------------------------
# siemens_to_ismrmrd
# ----------------------------------------------------------------------------------------------------------
rm -R -f ${GT_WORKING_DIR}/mrprogs/siemens_to_ismrmrd
cd ${GT_WORKING_DIR}/mrprogs
git clone  ${GT_CONVERTER_REPO}  ${GT_WORKING_DIR}/mrprogs/siemens_to_ismrmrd
cd ${GT_WORKING_DIR}/mrprogs/siemens_to_ismrmrd
git checkout -b ${GT_CONVERTER_BRANCH} origin/${GT_CONVERTER_BRANCH}

rm -R -f ${GT_WORKING_DIR}/mrprogs/build_siemens_to_ismrmrd_${BUILD_TYPE}
cd ${GT_WORKING_DIR}/mrprogs
mkdir build_siemens_to_ismrmrd_${BUILD_TYPE}
cd ${GT_WORKING_DIR}/mrprogs/build_siemens_to_ismrmrd_${BUILD_TYPE}

cmake -DCMAKE_INSTALL_PREFIX=${GT_INSTALL_DIR} -DCMAKE_BUILD_TYPE=${BUILD_TYPE} -DCMAKE_PREFIX_PATH=${GT_WORKING_DIR}/local/lib/cmake/ISMRMRD ../siemens_to_ismrmrd

make -j $(nproc)
make install

# ----------------------------------------------------------------------------------------------------------
# make gadgetron ready
# ----------------------------------------------------------------------------------------------------------
cp -f ${GT_INSTALL_DIR}/share/gadgetron/config/gadgetron.xml.example ${GT_INSTALL_DIR}/share/gadgetron/config/gadgetron.xml

