/** \file   gtPlusISMRMRDReconWorker2DTGRAPPA.h
    \brief  Implement the 2DT GRAPPA reconstruction
    \author Hui Xue
*/

#pragma once

#include "ismrmrd.h"
#include "GadgetronTimer.h"
#include "gtPlusISMRMRDReconUtil.h"
#include "gtPlusISMRMRDReconCoilMapEstimation.h"
#include "gtPlusISMRMRDReconWorker2DT.h"
#include "gtPlusGRAPPA.h"

namespace Gadgetron { namespace gtPlus {

template <typename T> 
class gtPlusReconWorker2DTGRAPPA : public gtPlusReconWorker2DT<T>
{
public:

    typedef gtPlusReconWorker2DT<T> BaseClass;
    typedef typename BaseClass::value_type value_type;

    gtPlusReconWorker2DTGRAPPA() : BaseClass() {}
    virtual ~gtPlusReconWorker2DTGRAPPA() {}

    virtual bool performCalibPrep(const hoNDArray<T>& ref_src, const hoNDArray<T>& ref_dst, gtPlusReconWorkOrder2DT<T>* workOrder2DT);
    virtual bool performCalibImpl(const hoNDArray<T>& ref_src, const hoNDArray<T>& ref_dst, gtPlusReconWorkOrder2DT<T>* workOrder2DT, size_t n, size_t usedS);

    virtual bool performUnwrapping(gtPlusReconWorkOrder2DT<T>* workOrder2DT, const hoNDArray<T>& data);

    using BaseClass::gt_timer1_;
    using BaseClass::gt_timer2_;
    using BaseClass::gt_timer3_;
    using BaseClass::performTiming_;
    using BaseClass::verbose_;
    using BaseClass::gt_exporter_;
    using BaseClass::debugFolder_;
    using BaseClass::gtPlus_util_;
    using BaseClass::gtPlus_mem_manager_;

    using BaseClass::buffer2DT_;
    using BaseClass::buffer2DT_unwrapping_;
    using BaseClass::buffer2DT_partial_fourier_;
    using BaseClass::buffer2DT_partial_fourier_kspaceIter_;
    using BaseClass::ref_src_;
    using BaseClass::ref_dst_;
    using BaseClass::data_dst_;
    using BaseClass::ref_coil_map_dst_;
    using BaseClass::startE1_;
    using BaseClass::endE1_;

    gtPlusGRAPPA<T> grappa_;
};

template <typename T> 
bool gtPlusReconWorker2DTGRAPPA<T>::
performCalibPrep(const hoNDArray<T>& ref_src, const hoNDArray<T>& ref_dst, gtPlusReconWorkOrder2DT<T>* workOrder2DT)
{
    try
    {
        size_t RO = workOrder2DT->data_.get_size(0);
        size_t E1 = workOrder2DT->data_.get_size(1);
        size_t N = workOrder2DT->data_.get_size(3);
        size_t S = workOrder2DT->data_.get_size(4);

        size_t srcCHA = ref_src.get_size(2);

        size_t refRO = ref_dst.get_size(0);
        size_t refE1 = ref_dst.get_size(1);
        size_t refN = ref_dst.get_size(3);
        size_t dstCHA = ref_dst.get_size(2);

        std::vector<int> kE1, oE1;
        bool fitItself = true;
        GADGET_CHECK_RETURN_FALSE(grappa_.kerPattern(kE1, oE1, (int)workOrder2DT->acceFactorE1_, workOrder2DT->grappa_kSize_E1_, fitItself));

        size_t kRO = workOrder2DT->grappa_kSize_RO_;
        size_t kNE1 = workOrder2DT->grappa_kSize_E1_;
        size_t oNE1 = oE1.size();

        workOrder2DT->kernel_->create(kRO, kNE1, srcCHA, dstCHA, oNE1, refN, S);
        workOrder2DT->kernelIm_->create(RO, E1, srcCHA, dstCHA, refN, S);
        workOrder2DT->unmixingCoeffIm_->create(RO, E1, srcCHA, refN, S);
        workOrder2DT->gfactor_.create(RO, E1, refN, S);

        if ( workOrder2DT->wrap_around_map_needed_ )
        {
            workOrder2DT->wrap_around_map_.create(RO, E1, 2, refN, S);
        }
    }
    catch(...)
    {
        GADGET_ERROR_MSG("Errors in gtPlusReconWorker2DTGRAPPA<T>::performCalibPrep(...) ... ");
        return false;
    }

    return true;
}

template <typename T> 
bool gtPlusReconWorker2DTGRAPPA<T>::
performCalibImpl(const hoNDArray<T>& ref_src, const hoNDArray<T>& ref_dst, gtPlusReconWorkOrder2DT<T>* workOrder2DT, size_t n, size_t usedS)
{
    try
    {
        size_t RO = workOrder2DT->data_.get_size(0);
        size_t E1 = workOrder2DT->data_.get_size(1);
        size_t N = workOrder2DT->data_.get_size(3);
        size_t S = workOrder2DT->data_.get_size(4);

        size_t srcCHA = ref_src.get_size(2);

        size_t refRO = ref_dst.get_size(0);
        size_t refE1 = ref_dst.get_size(1);
        size_t refN = ref_dst.get_size(3);
        size_t dstCHA = ref_dst.get_size(2);

        gtPlusGRAPPA<T> grappa_local;

        std::vector<int> kE1, oE1;
        bool fitItself = true;
        GADGET_CHECK_RETURN_FALSE(grappa_local.kerPattern(kE1, oE1, (size_t)workOrder2DT->acceFactorE1_, workOrder2DT->grappa_kSize_E1_, fitItself));

        size_t kRO = workOrder2DT->grappa_kSize_RO_;
        size_t kNE1 = workOrder2DT->grappa_kSize_E1_;
        size_t oNE1 = oE1.size();

        ho3DArray<T> acsSrc(refRO, refE1, srcCHA, const_cast<T*>(ref_src.begin()+n*refRO*refE1*srcCHA+usedS*refRO*refE1*srcCHA*refN));
        ho3DArray<T> acsDst(refRO, refE1, dstCHA, const_cast<T*>(ref_dst.begin()+n*refRO*refE1*dstCHA+usedS*refRO*refE1*dstCHA*refN));

        std::ostringstream ostr;
        ostr << "_n_" << n << "s_" << usedS;
        std::string suffix = ostr.str();

        std::string filename = "acsSrc";
        GADGET_EXPORT_ARRAY_COMPLEX(debugFolder_, gt_exporter_, acsSrc, filename+suffix);

        filename = "acsDst";
        GADGET_EXPORT_ARRAY_COMPLEX(debugFolder_, gt_exporter_, acsDst, filename+suffix);

        grappa_local.calib_use_gpu_  = workOrder2DT->grappa_use_gpu_;

        ho5DArray<T> ker(kRO, kNE1, srcCHA, dstCHA, oNE1, workOrder2DT->kernel_->begin()+n*kRO*kNE1*srcCHA*dstCHA*oNE1+usedS*kRO*kNE1*srcCHA*dstCHA*oNE1*refN);
        GADGET_CHECK_RETURN_FALSE(grappa_local.calib(acsSrc, acsDst, workOrder2DT->grappa_reg_lamda_, kRO, kE1, oE1, ker));

        filename = "ker";
        GADGET_EXPORT_ARRAY_COMPLEX(debugFolder_, gt_exporter_, ker, filename+suffix);

        hoNDArray<T> kIm(RO, E1, srcCHA, dstCHA, workOrder2DT->kernelIm_->begin()+n*RO*E1*srcCHA*dstCHA+usedS*RO*E1*srcCHA*dstCHA*refN);
        GADGET_CHECK_RETURN_FALSE(grappa_local.imageDomainKernel(ker, kRO, kE1, oE1, RO, E1, kIm));

        filename = "kIm";
        GADGET_EXPORT_ARRAY_COMPLEX(debugFolder_, gt_exporter_, kIm, filename+suffix);

        hoNDArray<T> coilMap(RO, E1, dstCHA, workOrder2DT->coilMap_->begin()+n*RO*E1*dstCHA+usedS*RO*E1*dstCHA*refN);
        hoNDArray<T> unmixC(RO, E1, srcCHA, workOrder2DT->unmixingCoeffIm_->begin()+n*RO*E1*srcCHA+usedS*RO*E1*srcCHA*refN);
        hoNDArray<T> gFactor(RO, E1, workOrder2DT->gfactor_.begin()+n*RO*E1+usedS*RO*E1*refN);

        GADGET_CHECK_RETURN_FALSE(this->unmixCoeff(kIm, coilMap, unmixC, gFactor));

        GADGET_CHECK_RETURN_FALSE(Gadgetron::scal( (value_type)(1.0/workOrder2DT->acceFactorE1_), gFactor));

        filename = "unmixC";
        GADGET_EXPORT_ARRAY_COMPLEX(debugFolder_, gt_exporter_, unmixC, filename+suffix);

        filename = "gFactor";
        GADGET_EXPORT_ARRAY_COMPLEX(debugFolder_, gt_exporter_, gFactor, filename+suffix);

        if ( workOrder2DT->wrap_around_map_needed_ )
        {
            hoNDArray<T> wrapAroundMap(RO, E1, 2, workOrder2DT->wrap_around_map_.begin()+n*RO*E1*2+usedS*RO*E1*2*refN);

            gtPlusISMRMRDReconCoilMapEstimation<T> coil_map_util;

            hoNDArray<T> coilMap(RO, E1, acsDst.get_size(2));
            hoNDArray<value_type> eigD(RO, E1, 2);

            value_type thres = workOrder2DT->spirit_reg_lamda_;

            GADGET_CHECK_RETURN_FALSE(coil_map_util.coilMap2DSPIRIT(acsDst, coilMap, eigD, workOrder2DT->spirit_kSize_RO_, workOrder2DT->spirit_kSize_E1_, thres));
            GADGET_CHECK_RETURN_FALSE(wrapAroundMap.copyFrom(eigD));

            filename = "wrapAroundMap";
            GADGET_EXPORT_ARRAY(debugFolder_, gt_exporter_, eigD, filename+suffix);
        }
    }
    catch(...)
    {
        GADGET_ERROR_MSG("Errors in gtPlusReconWorker2DTGRAPPA<T>::performCalibImpl(...) ... ");
        return false;
    }

    return true;
}

template <typename T> 
bool gtPlusReconWorker2DTGRAPPA<T>::
performUnwrapping(gtPlusReconWorkOrder2DT<T>* workOrder2DT, const hoNDArray<T>& data_dst)
{
    try
    {
        int n;

        size_t RO = workOrder2DT->data_.get_size(0);
        size_t E1 = workOrder2DT->data_.get_size(1);
        size_t N = workOrder2DT->data_.get_size(3);
        size_t S = workOrder2DT->data_.get_size(4);

        size_t srcCHA = workOrder2DT->kernelIm_->get_size(2);
        size_t dstCHA = workOrder2DT->kernelIm_->get_size(3);

        size_t refN = workOrder2DT->kernelIm_->get_size(4);

        workOrder2DT->complexIm_.create(RO, E1, N, S);

        if ( workOrder2DT->downstream_coil_compression_ )
        {
            Gadgetron::hoNDFFT<typename realType<T>::Type>::instance()->ifft2c(workOrder2DT->data_, buffer2DT_);
        }
        else
        {
            Gadgetron::hoNDFFT<typename realType<T>::Type>::instance()->ifft2c(data_dst, buffer2DT_);
        }

        double effectiveAcceFactor = workOrder2DT->acceFactorE1_;
        if ( workOrder2DT->start_E1_>0 && workOrder2DT->end_E1_>0 )
        {
            size_t num = workOrder2DT->end_E1_ - workOrder2DT->start_E1_ + 1;
            size_t res = (size_t)( num % (size_t)(std::ceil(workOrder2DT->acceFactorE1_)) );
            double N = std::floor( (double)(num-res)/(double)workOrder2DT->acceFactorE1_);
            effectiveAcceFactor = (double)num/N;
        }
        else
        {
            size_t num = E1;
            size_t res = (size_t)( num % (size_t)(std::ceil(workOrder2DT->acceFactorE1_)) );
            double N = std::floor( (double)(num-res)/(double)workOrder2DT->acceFactorE1_);
            effectiveAcceFactor = (double)num/N;
        }

        typename realType<T>::Type fftCompensationRatio = (typename realType<T>::Type)(1.0/std::sqrt(effectiveAcceFactor));
        Gadgetron::scal( fftCompensationRatio, buffer2DT_);

        // if the image data is scaled and ref lines are going to be filled back to the data, 
        // the reference lines should be scaled too
        if ( workOrder2DT->CalibMode_ == ISMRMRD_embedded )
        {
            if ( workOrder2DT->embedded_ref_fillback_ )
            {
                Gadgetron::scal( fftCompensationRatio, workOrder2DT->ref_);
            }
        }

        GADGET_EXPORT_ARRAY_COMPLEX(debugFolder_, gt_exporter_, buffer2DT_, "buffer2DT_");

        bool recon_kspace = false;

        if ( workOrder2DT->CalibMode_ == ISMRMRD_embedded )
        {
            if ( workOrder2DT->embedded_fullres_coilmap_ || workOrder2DT->embedded_ref_fillback_ )
            {
                recon_kspace = true;
            }
        }

        if ( workOrder2DT->CalibMode_ == ISMRMRD_separate )
        {
            if ( workOrder2DT->separate_fullres_coilmap_ )
            {
                recon_kspace = true;
            }
        }

        if ( workOrder2DT->recon_kspace_needed_ )
        {
            recon_kspace = true;
        }

        // if kspace is actually needed
        if ( recon_kspace )
        {
            workOrder2DT->fullkspace_ = data_dst;

            buffer2DT_unwrapping_.create(RO, E1, srcCHA, dstCHA);

            size_t usedS;
            for ( usedS=0; usedS<S; usedS++ )
            {
                if ( (refN<N) || (refN==1) )
                {
                    hoNDArray<T> kIm(RO, E1, srcCHA, dstCHA, workOrder2DT->kernelIm_->begin()+usedS*RO*E1*srcCHA*dstCHA*refN);
                    hoNDArray<T> aliasedIm(RO, E1, srcCHA, N, buffer2DT_.begin()+usedS*RO*E1*srcCHA*N);
                    hoNDArray<T> unwarppedIm(RO, E1, dstCHA, N, workOrder2DT->fullkspace_.begin()+usedS*RO*E1*dstCHA*N);

                    this->applyImageDomainKernelImage(aliasedIm, kIm, buffer2DT_unwrapping_, unwarppedIm);

                    if ( !debugFolder_.empty() )
                    {
                        {
                            std::ostringstream ostr;
                            ostr << "kIm_" << usedS;
                            GADGET_EXPORT_ARRAY_COMPLEX(debugFolder_, gt_exporter_, kIm, ostr.str());
                        }

                        {
                            std::ostringstream ostr;
                            ostr << "aliasedIm_" << usedS;
                            GADGET_EXPORT_ARRAY_COMPLEX(debugFolder_, gt_exporter_, aliasedIm, ostr.str());
                        }

                        std::ostringstream ostr;
                        ostr << "unwarppedIm_" << usedS;
                        GADGET_EXPORT_ARRAY_COMPLEX(debugFolder_, gt_exporter_, unwarppedIm, ostr.str());
                    }
                }
                else
                {
                    #pragma omp parallel private(n)
                    {
                        hoNDArray<T> complexIm(RO, E1, dstCHA);

                        #pragma omp for
                        for ( n=0; n<(int)N; n++ )
                        {
                            hoNDArray<T> kIm(RO, E1, srcCHA, dstCHA, workOrder2DT->kernelIm_->begin()+n*RO*E1*srcCHA*dstCHA+usedS*RO*E1*srcCHA*dstCHA*refN);

                            GADGET_EXPORT_ARRAY_COMPLEX(debugFolder_, gt_exporter_, kIm, "kIm_n");

                            T* pIm2D = buffer2DT_.begin()+n*RO*E1*srcCHA+usedS*RO*E1*srcCHA*N;
                            hoNDArray<T> aliasedIm(RO, E1, srcCHA, pIm2D);

                            GADGET_EXPORT_ARRAY_COMPLEX(debugFolder_, gt_exporter_, aliasedIm, "aliasedIm_n");

                            this->applyImageDomainKernelImage(aliasedIm, kIm, complexIm);
                            GADGET_EXPORT_ARRAY_COMPLEX(debugFolder_, gt_exporter_, complexIm, "complexIm_n");

                            memcpy(workOrder2DT->fullkspace_.begin()+n*RO*E1*dstCHA+usedS*RO*E1*dstCHA*N, complexIm.begin(), sizeof(T)*RO*E1*dstCHA);
                        }
                    }
                }

                hoNDArray<T> unwarppedIm(RO, E1, dstCHA, N, workOrder2DT->fullkspace_.begin()+usedS*RO*E1*dstCHA*N);
                hoNDArray<T> combined(RO, E1, N, workOrder2DT->complexIm_.begin()+usedS*RO*E1*N);

                if ( refN == N )
                {
                    hoNDArray<T> coilMap(RO, E1, dstCHA, refN, workOrder2DT->coilMap_->begin()+usedS*RO*E1*dstCHA*refN);
                    gtPlusISMRMRDReconUtilComplex<T>().coilCombine(unwarppedIm, coilMap, combined);
                }
                else
                {
                    hoNDArray<T> coilMap(RO, E1, dstCHA, workOrder2DT->coilMap_->begin()+usedS*RO*E1*dstCHA*refN);
                    gtPlusISMRMRDReconUtilComplex<T>().coilCombine(unwarppedIm, coilMap, combined);
                }

                if ( !debugFolder_.empty() )
                {
                    std::ostringstream ostr;
                    ostr << "combined_" << usedS;
                    GADGET_EXPORT_ARRAY_COMPLEX(debugFolder_, gt_exporter_, combined, ostr.str());
                }
            }

            Gadgetron::hoNDFFT<typename realType<T>::Type>::instance()->fft2c(workOrder2DT->fullkspace_);

            if ( !debugFolder_.empty() )
            {
                std::ostringstream ostr;
                ostr << "fullkspace_" << usedS;
                GADGET_EXPORT_ARRAY_COMPLEX(debugFolder_, gt_exporter_, workOrder2DT->fullkspace_, ostr.str());
            }
        }
        else
        {
            size_t usedS;
            for ( usedS=0; usedS<S; usedS++ )
            {
                if ( (refN<N) || (refN==1) )
                {
                    hoNDArray<T> unmixCoeff(RO, E1, srcCHA, workOrder2DT->unmixingCoeffIm_->begin()+usedS*RO*E1*srcCHA*refN);
                    hoNDArray<T> aliasedIm(RO, E1, srcCHA, N, buffer2DT_.begin()+usedS*RO*E1*srcCHA*N);
                    hoNDArray<T> unwarppedIm(RO, E1, 1, N, workOrder2DT->complexIm_.begin()+usedS*RO*E1*N);

                    this->applyUnmixCoeffImage(aliasedIm, unmixCoeff, unwarppedIm);

                    GADGET_EXPORT_ARRAY_COMPLEX(debugFolder_, gt_exporter_, unwarppedIm, "unwarppedIm");
                }
                else
                {
                    // #pragma omp parallel for private(n)
                    for ( n=0; n<(int)N; n++ )
                    {
                        hoNDArray<T> unmixCoeff(RO, E1, srcCHA, workOrder2DT->unmixingCoeffIm_->begin()+n*RO*E1*srcCHA+usedS*RO*E1*srcCHA*refN);
                        hoNDArray<T> aliasedIm(RO, E1, srcCHA, buffer2DT_.begin()+n*RO*E1*srcCHA+usedS*RO*E1*srcCHA*N);
                        hoNDArray<T> unwarppedIm(RO, E1, 1, workOrder2DT->complexIm_.begin()+n*RO*E1+usedS*RO*E1*N);

                        this->applyUnmixCoeffImage(aliasedIm, unmixCoeff, unwarppedIm);

                        GADGET_EXPORT_ARRAY_COMPLEX(debugFolder_, gt_exporter_, unwarppedIm, "unwarppedIm");
                    }
                }
            }

            workOrder2DT->fullkspace_.create(RO, E1, 1, N, S);
            memcpy(workOrder2DT->fullkspace_.begin(), workOrder2DT->complexIm_.begin(), workOrder2DT->complexIm_.get_number_of_bytes());
            Gadgetron::hoNDFFT<typename realType<T>::Type>::instance()->fft2c(workOrder2DT->fullkspace_);
        }
    }
    catch(...)
    {
        GADGET_ERROR_MSG("Errors in gtPlusReconWorker2DTGRAPPA<T>::performUnwrapping(gtPlusReconWorkOrder2DT<T>* workOrder2DT, const hoNDArray<T>& data) ... ");
        return false;
    }

    return true;
}

}}
