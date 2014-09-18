#include "complext.h"
#include "cuSolverUtils.h"
#define MAX_THREADS_PER_BLOCK 512

using namespace Gadgetron;
template <class T> __global__ static void filter_kernel(T* x, T* g, int elements){
	const int idx = blockIdx.y*gridDim.x*blockDim.x + blockIdx.x*blockDim.x + threadIdx.x;
	if (idx < elements){
		if ( x[idx] <= T(0) && g[idx] > 0) g[idx]=T(0);
	}
}

template <class REAL> __global__ static void filter_kernel(complext<REAL>* x, complext<REAL>* g, int elements){
	const int idx = blockIdx.y*gridDim.x*blockDim.x + blockIdx.x*blockDim.x + threadIdx.x;
	if (idx < elements){
		if ( real(x[idx]) <= REAL(0) && real(g[idx]) > 0) g[idx].vec[0] = REAL(0);
		g[idx].vec[1]=REAL(0);
	}
}

template <class T> void EXPORTGPUSOLVERS Gadgetron::solver_non_negativity_filter(cuNDArray<T>* x , cuNDArray<T>* g)
{
	int elements = g->get_number_of_elements();

	int threadsPerBlock = std::min(elements,MAX_THREADS_PER_BLOCK);
	dim3 dimBlock( threadsPerBlock);
	int totalBlocksPerGrid = std::max(1,elements/MAX_THREADS_PER_BLOCK);
	dim3 dimGrid(totalBlocksPerGrid);

	filter_kernel<typename realType<T>::Type><<<dimGrid,dimBlock>>>(x->get_data_ptr(),g->get_data_ptr(),elements);
}


template void EXPORTGPUSOLVERS Gadgetron::solver_non_negativity_filter<float>(cuNDArray<float>*, cuNDArray<float>*);
template void EXPORTGPUSOLVERS Gadgetron::solver_non_negativity_filter<double>(cuNDArray<double>*, cuNDArray<double>*);
template void EXPORTGPUSOLVERS Gadgetron::solver_non_negativity_filter<float_complext>(cuNDArray<float_complext>*, cuNDArray<float_complext>*);
template void EXPORTGPUSOLVERS Gadgetron::solver_non_negativity_filter<double_complext>(cuNDArray<double_complext>*, cuNDArray<double_complext>*);

