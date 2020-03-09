#pragma once

#include "gadgetron_matlab_export.h"
#include "Gadget.h"
//#include "gadgetron_paths.h"
#include "hoNDArray.h"
#include "ismrmrd/ismrmrd.h"
#include "log.h"
#include "engine.h"     // Matlab Engine header

#include "ace/Synch.h"  // For the MatlabCommandServer
#include "ace/SOCK_Connector.h"
#include "ace/INET_Addr.h"


#include <stdio.h>
#include <stdlib.h>
#include <complex>
#include <boost/lexical_cast.hpp>
#include "gadgetron_home.h"

// TODO:
//Make the port option work so that we can have multiple matlabs running, each with its own command server.
//Create a debug option to use evalstring and get back the matlab output on every function call.
//Finish the image stuff
//Is there a better way to kill the command server?
//Test on windows


// MB buffer size to capture matlab output
#define MATBUFSIZE 1000000 // no. chars


namespace Gadgetron
{

template <class T> class MatlabGadget :
    public Gadget2<T, hoNDArray< std::complex<float> > >
{
public:
    MatlabGadget(): Gadget2<T, hoNDArray< std::complex<float> > >()
    {
        // Open the Matlab Engine on the current host
        GDEBUG("Starting MATLAB engine\n");
        if (!(engine_ = engOpen("matlab -nosplash -nodesktop -nodisplay")))
        {
            // TODO: error checking!
            GDEBUG("Can't start MATLAB engine - is csh installed?\n");
        }
        else
        {
            // Add ISMRMRD Java bindings jar to Matlab's path
            // TODO: this should be in user's Matlab path NOT HERE

            // Prepare buffer for collecting Matlab's output
		    char *matlab_text_buffer = (char*)calloc(MATBUFSIZE, sizeof(char));
			if (matlab_text_buffer)
			{
				engOutputBuffer(engine_, matlab_text_buffer, MATBUFSIZE);
				GDEBUG("Initialized matlab_text_buffer (size %i chars).\n",MATBUFSIZE);
			}
			else
			{
				GDEBUG("Can't initialize matlab_text_buffer - there will be no output from MATLAB.\n");
			}

            // Add the necessary paths to the matlab environment
            // Java matlab command server
            std::string gadgetron_matlab_path = get_gadgetron_home().string() + "/share/gadgetron/matlab";
            std::string java_add_path_cmd = std::string("javaaddpath('") + gadgetron_matlab_path + std::string("');");
            std::string add_path_cmd = std::string("addpath('") + gadgetron_matlab_path + std::string("');");
            // Gadgetron matlab scripts
            engEvalString(engine_, java_add_path_cmd.c_str());
            engEvalString(engine_, add_path_cmd.c_str());
            // ISMRMRD matlab library
            engEvalString(engine_, "addpath(fullfile(getenv('ISMRMRD_HOME'), '/share/ismrmrd/matlab'));");

            GDEBUG("%s", matlab_text_buffer);
			if (matlab_text_buffer) free(matlab_text_buffer);

        }
    }

    ~MatlabGadget()
    {
        // Close the Matlab engine
        GDEBUG("Closing down Matlab\n");
        engClose(engine_);

		// these free the cell members recursively 
		mxDestroyArray(buffer_hdr_);
		mxDestroyArray(buffer_data_);
    }

protected:
    GADGET_PROPERTY(debug_mode, bool, "Debug mode", false);
    GADGET_PROPERTY(matlab_path, std::string, "Path to Matlab code", "");
    GADGET_PROPERTY(matlab_classname, std::string, "Name of Matlab gadget class", "");
    GADGET_PROPERTY(matlab_port, int, "Port on which to run Matlab command server", 3000);
	GADGET_PROPERTY(matlab_buffer_time_ms, int, "Time to buffer before calling Matlab", 100);

    int process_config(ACE_Message_Block* mb)
    {
        std::string cmd;

        debug_mode_     = debug_mode.value();
        path_           = matlab_path.value();
        classname_      = matlab_classname.value();
        if (classname_.empty())
        {
            GERROR("Missing Matlab Gadget classname in config!");
            return GADGET_FAIL;
        }
		buffer_time_ms_ = matlab_buffer_time_ms.value();
      	if (buffer_time_ms_ < 1)
        {
            GERROR("Impossible value for Matlab Gadget buffer time!");
            return GADGET_FAIL;
        }
        command_server_port_ = matlab_port.value();

        GDEBUG("MATLAB Class Name : %s\n", classname_.c_str());
		GDEBUG("MATLAB buffer_time_ms = %i\n", buffer_time_ms_);

        // add user specified path for this gadget
        if (!path_.empty())
        {
            cmd = "addpath('" + path_ + "');";
            send_matlab_command(cmd);
        }

        // Put the XML Header into the matlab workspace
        std::string xmlConfig = std::string(mb->rd_ptr());
        mxArray *xmlstring = mxCreateString(xmlConfig.c_str());
        engPutVariable(engine_, "xmlstring", xmlstring);

        // Instantiate the Matlab gadget object from the user specified class
        // Call matlab gadget's init method with the XML Header
        // and the user defined config method
        cmd = "matgadget = " + classname_ + "();";
        cmd += "matgadget.init(xmlstring); matgadget.config();";
        if (send_matlab_command(cmd) != GADGET_OK)
        {
            GDEBUG("Failed to send matlab command.\n");
            return GADGET_FAIL;
        }
        mxDestroyArray(xmlstring);

		if (setup_buffer() != GADGET_OK)
			return GADGET_FAIL;

		counter_ = 0;
		start_time_ = 0; // initialize with 0 to trigger the first flush right away

        return GADGET_OK;
    }

    int send_matlab_command(std::string& command)
    {
        // Prepare buffer for collecting Matlab's output
	    char *matlab_text_buffer = (char*)calloc(MATBUFSIZE, sizeof(char));
		if (matlab_text_buffer)
		{
			engOutputBuffer(engine_, matlab_text_buffer, MATBUFSIZE);
		}
		else
		{
			GDEBUG("Can't initialize matlab_text_buffer - no output from MATLAB.\n");
		}
        engEvalString(engine_, command.c_str());

        if (strlen(matlab_text_buffer))
        {
            matlab_text_buffer[2] = '\n'; // insert a newline after the >> prompt
            GDEBUG("MATLAB output %s", matlab_text_buffer);
        }

		if (matlab_text_buffer) free(matlab_text_buffer);

        return GADGET_OK;
    }

    std::string path_;
    std::string classname_;
    int command_server_port_;
    int debug_mode_;

    Engine *engine_;

    // Buffer to reduce Matlab engine calls. Prefer cell arrays because they can store
    // different size ADCs, which may be useful e.g. for navigators.
    //
	const size_t max_buffer_size_ = 1000; // maximum no. ADCs to hold before flushing (not crucial)
	int buffer_time_ms_; 
	clock_t start_time_; 
	size_t counter_;
	mxArray *buffer_hdr_;
	mxArray *buffer_data_;

	int setup_buffer()
	{
		// allocate large buffer of empty cells
		buffer_hdr_ = mxCreateCellMatrix(max_buffer_size_, 1);
		if(buffer_hdr_ == NULL) {
			GDEBUG("Couldn't allocate buffer_hdr_\n");
			return GADGET_FAIL;
		}
		buffer_data_ = mxCreateCellMatrix(max_buffer_size_, 1);
		if(buffer_hdr_ == NULL) {
			GDEBUG("Couldn't allocate buffer_data_\n");
			return GADGET_FAIL;
		}

		// set no. elements to 0
		mxSetM(buffer_hdr_,0);
		mxSetM(buffer_data_,0);
		GDEBUG("Matlab buffer initialized (%u elements)\n",max_buffer_size_);

		return GADGET_OK;
	}

	int add_to_buffer(ISMRMRD::AcquisitionHeader *acq, std::complex<float> *raw_data)
	{
		// Checks
		int M = mxGetM(buffer_hdr_);
		if (mxGetM(buffer_data_) != M) {
			GDEBUG("Size error: buffer_hdr_ (%i), buffer_data_ (%i)\n", M, mxGetM(buffer_data_));
			return GADGET_FAIL;
		}

		// === Header ===

		// Remove existing cell (unfortunately mxArray doesn't support realloc for cells) 
		mxDestroyArray(mxGetCell(buffer_hdr_, M));

		// Create new entry
		mxArray *acq_hdr_bytes = mxCreateNumericMatrix(sizeof(ISMRMRD::AcquisitionHeader), 1, mxUINT8_CLASS, mxREAL);
		if(acq_hdr_bytes == NULL) {
			GDEBUG("Couldn't allocate acq_hdr_bytes\n");
			return GADGET_FAIL;
		}
		memcpy(mxGetData(acq_hdr_bytes), acq, sizeof(ISMRMRD::AcquisitionHeader));

		// Update cell array
		mxSetCell(buffer_hdr_, M, acq_hdr_bytes);
		mxSetM(buffer_hdr_, M+1);


		// === Data ===

		// Remove existing cell (unfortunately mxArray doesn't support realloc for cells) 
		mxDestroyArray(mxGetCell(buffer_data_, M));

		// Create new entry
		mxArray *acq_data = mxCreateNumericMatrix(acq->number_of_samples, acq->active_channels, mxSINGLE_CLASS, mxCOMPLEX);
		if(acq_data == NULL) {
			GDEBUG("Couldn't allocate acq_data\n");
			return GADGET_FAIL;
		}

#ifdef MX_HAS_INTERLEAVED_COMPLEX
		mxComplexSingle* data = mxGetComplexSingles(acq_data);
		for(size_t i = 0; i<acq->number_of_samples*acq->active_channels; i++)
		{
			data[i].real = raw_data[i].real();
			data[i].imag = raw_data[i].imag();
		}
#else
		float *real = (float *)mxGetData(acq_data);
		float *imag = (float *)mxGetImagData(acq_data);
		for(size_t i = 0; i<acq->number_of_samples*acq->active_channels; i++)
		{
			real[i] = raw_data[i].real();
			imag[i] = raw_data[i].imag();
		}
#endif

		// Update cell array
		mxSetCell(buffer_data_, M, acq_data);
		mxSetM(buffer_data_, M+1);

		return GADGET_OK;
	}

	long flush_buffer(ISMRMRD::AcquisitionHeader *acq)
	{
		clock_t current_time = clock();
    	float time_since_last_flush_ms = 1e3*(current_time - start_time_)/float(CLOCKS_PER_SEC);

		// decide whether to flush
        bool do_flush = time_since_last_flush_ms >= buffer_time_ms_ ||
                        mxGetM(buffer_hdr_) >= max_buffer_size_     ||
                        start_time_ == 0                            ||
                        ismrmrd_is_flag_set(acq->flags, ISMRMRD::ISMRMRD_ACQ_LAST_IN_MEASUREMENT);

  		// flush or not?
		if (do_flush)
		{
			if(engPutVariable(engine_, "hdr_bytes", buffer_hdr_)) {
				GDEBUG("Failed to engPutVariable hdr_bytes\n");
				return GADGET_FAIL;
			}
    		if(engPutVariable(engine_, "data", buffer_data_)) {
				GDEBUG("Failed to engPutVariable data\n");
				return GADGET_FAIL;
			}

			counter_ += mxGetM(buffer_hdr_);
			GDEBUG("Flush: time (%.1f ms), count (%i), total (%u)\n", time_since_last_flush_ms,  mxGetM(buffer_hdr_), counter_);

			mxSetM(buffer_hdr_,0);
			mxSetM(buffer_data_,0);
			start_time_ = current_time;

			// need a return value that is not OK or FAIL to indicate a flush 
			return(+33783315524); // my phone number - text me if you know a better way
		}

		// no problem encountered, but we didn't flush
		return GADGET_OK;
	}

};


class EXPORTGADGETSMATLAB AcquisitionMatlabGadget :
    public MatlabGadget<ISMRMRD::AcquisitionHeader>
{
public:
    GADGET_DECLARE(AcquisitionMatlabGadget);
    int process(GadgetContainerMessage<ISMRMRD::AcquisitionHeader>* m1,
                GadgetContainerMessage< hoNDArray< std::complex<float> > >* m2);

};

class EXPORTGADGETSMATLAB ImageMatlabGadget :
    public MatlabGadget<ISMRMRD::ImageHeader>
{
public:
    GADGET_DECLARE(ImageMatlabGadget);
    int process(GadgetContainerMessage<ISMRMRD::ImageHeader>* m1,
                GadgetContainerMessage< hoNDArray< std::complex<float> > >* m2);

};
}
