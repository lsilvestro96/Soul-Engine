#include "RayEngine.cuh"

struct is_marked
{
	__host__ __device__
		bool operator()(const Ray& x)
	{
		return (!x.active);
	}
};
// Template structure to pass to kernel
template <typename T>
struct KernelArray : public Managed
{
public:
	T*  _array;
	int _size;

public:
	__device__ int Size() const{
		return _size;
	}

	__device__ T& operator[](int& i) {
		return _array[i];
	}
	__device__ T& operator[](uint& i) {
		return _array[i];
	}
	__host__ KernelArray() {
		_array = NULL;
		_size = 0;

	}

	// constructor allows for implicit conversion

	__host__ KernelArray(thrust::device_vector<T>& dVec) {
		_array = thrust::raw_pointer_cast(&dVec[0]);
		_size = (int)dVec.size();
	}
	__host__ KernelArray(T* arr, int size) {
		_array = arr;
		_size = size;
	}
	__host__ ~KernelArray(){

	}

};

KernelArray<RayJob*> jobL;
Ray* deviceRays=NULL;
uint raySeedGl = 0;
uint numRays = 0;
const uint rayDepth = 3;

inline CUDA_FUNCTION uint WangHash(uint a) {
	a = (a ^ 61) ^ (a >> 16);
	a = a + (a << 3);
	a = a ^ (a >> 4);
	a = a * 0x27d4eb2d;
	a = a ^ (a >> 15);
	return a;
}

inline __device__ int GetCurrentJob(KernelArray<RayJob*>& jobs, const uint& index, const uint& startIndex){

	int i = 0;
	for (;
		i < jobs.Size() && !(index < startIndex + jobs[i]->GetRayAmount()*jobs[i]->GetSampleAmount());
		++i){

	}
	return i;
	//while (job->nextRay != NULL && !(index < startIndex + job->rayAmount*job->samples)){
	//	startIndex += job->rayAmount*job->samples;
	//	job = job->nextRay;
	//}

}
inline __device__ int GetCurrentJobNoSample(KernelArray<RayJob*>& jobs, const uint& index, const uint& startIndex){

	int i = 0;
	for (;
		i < jobs.Size() && !(index < startIndex + jobs[i]->GetRayAmount());
		++i){

	}
	return i;
	//while (job->nextRay != NULL && !(index < startIndex + job->rayAmount*job->samples)){
	//	startIndex += job->rayAmount*job->samples;
	//	job = job->nextRay;
	//}

}

__global__ void EngineResultClear(const uint n, KernelArray<RayJob*> jobs){


	uint index = getGlobalIdx_1D_1D();

	if (index >= n){
		return;
	}

	uint startIndex = 0;

	int cur = GetCurrentJobNoSample(jobs, index, startIndex);

	((glm::vec4*)jobs[cur]->GetResultPointer(0))[(index - startIndex)] = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
}

__global__ void EngineExecute(const uint n, KernelArray<RayJob*> job, KernelArray<Ray> rays, const uint raySeed, const Scene* scene){

	uint index = getGlobalIdx_1D_1D();

	if (index >= n){
		return;
	}

	uint startIndex = 0;
	int cur = GetCurrentJob(job, index, startIndex);


	//thrust::default_random_engine rng(randHash(raySeed) * randHash(index));
	//thrust::uniform_real_distribution<float> uniformDistribution(0.0f, 1.0f);


	curandState randState;
	curand_init(raySeed + index, 0, 0, &randState);


	//float prob = uniformDistribution(rng);
	float prob = curand_uniform(&randState);

	

	uint localIndex = (index - startIndex) / job[cur]->GetSampleAmount();
	uint rayAmount = job[cur]->GetRayAmount();

	Ray ray;
	ray.active = true;
	ray.storage = glm::vec4(1.0f);
	ray.resultOffset = localIndex;
	job[cur]->GetCamera()->SetupRay(localIndex, ray, randState);


	//calculate something
	glm::vec3 col = scene->IntersectColour(ray, randState) / ((float)job[cur]->GetSampleAmount());

	rays[index] = ray;

	glm::vec4* pt = &((glm::vec4*)job[cur]->GetResultPointer(0))[localIndex];

	atomicAdd(&(pt->x), col.x);

	atomicAdd(&(pt->y), col.y);

	atomicAdd(&(pt->z), col.z);

}


__global__ void EngineExecuteSample(const uint n, KernelArray<RayJob*> job, KernelArray<Ray> rays, const uint raySeed, const Scene* scene){

	uint index = getGlobalIdx_1D_1D();

	uint startIndex = 0;
	int cur = GetCurrentJob(job, index, startIndex);


	//thrust::default_random_engine rng(randHash(raySeed) * randHash(index));
	//thrust::uniform_real_distribution<float> uniformDistribution(0.0f, 1.0f);


	curandState randState;
	curand_init(raySeed + index, 0, 0, &randState);


	//float prob = uniformDistribution(rng);
	float prob = curand_uniform(&randState);
	if (index >= n){
		return;
	}


	uint rayAmount = job[cur]->GetRayAmount();

	Ray ray = rays[index];
	uint localIndex = ray.resultOffset;


	//calculate something
	glm::vec3 col = scene->IntersectColour(ray, randState) / ((float)job[cur]->GetSampleAmount());

	rays[index] = ray;

	glm::vec4* pt = &((glm::vec4*)job[cur]->GetResultPointer(0))[localIndex];

	atomicAdd(&(pt->x), col.x);

	atomicAdd(&(pt->y), col.y);

	atomicAdd(&(pt->z), col.z);
}


__host__ void ClearResults(std::vector<RayJob*>& jobs){
	CudaCheck(cudaDeviceSynchronize());
	if (jobs.size() > 0){

		uint n = 0;
		for (int i = 0; i < jobs.size(); ++i){
			n += jobs[i]->GetRayAmount();
		}

		if (n != 0){

			thrust::device_vector<RayJob*> deviceJobList(jobs);

			uint blockSize = 64;
			uint gridSize = (n + blockSize - 1) / blockSize;


			//execute engine
			jobL = KernelArray<RayJob*>(deviceJobList);

			cudaEvent_t start, stop;
			float time;
			cudaEventCreate(&start);
			cudaEventCreate(&stop);
			cudaEventRecord(start, 0);

			EngineResultClear << <gridSize, blockSize >> >(n, jobL);

			cudaEventRecord(stop, 0);
			cudaEventSynchronize(stop);
			cudaEventElapsedTime(&time, start, stop);
			cudaEventDestroy(start);
			cudaEventDestroy(stop);

			std::cout << "RayClear Execution: " << time << "ms" << std::endl;
		}
		CudaCheck(cudaDeviceSynchronize());
	}
}
__host__ void ProcessJobs(std::vector<RayJob*>& jobs, const Scene* scene){
	CudaCheck(cudaDeviceSynchronize());
	if (jobs.size() > 0){

		uint n = 0;
		uint samplesMax = 0;
		for (int i = 0; i < jobs.size(); ++i){
			n += jobs[i]->GetRayAmount()* jobs[i]->GetSampleAmount();
			if (jobs[i]->GetSampleAmount()>samplesMax){
				samplesMax = jobs[i]->GetSampleAmount();
			}
		}

		if (n != 0){
			if (n > numRays){
				//deviceRays.resize(n);
				if (deviceRays != NULL){
					CudaCheck(cudaFree(&deviceRays));
				}
				CudaCheck(cudaMallocManaged((void**)&deviceRays, n*sizeof(Ray)));
				numRays = n;
			}

			thrust::device_vector<RayJob*> deviceJobList(jobs);

			


			//execute engine
			jobL = KernelArray<RayJob*>(deviceJobList);

			KernelArray<Ray> raysL = KernelArray<Ray>(deviceRays, numRays);

			thrust::device_ptr<Ray> rayPtr = thrust::device_pointer_cast(deviceRays);


			cudaEvent_t start, stop;
			float time;
			cudaEventCreate(&start);
			cudaEventCreate(&stop);
			cudaEventRecord(start, 0);


			uint blockSize = 64;
			uint gridSize = (n + blockSize - 1) / blockSize;

			EngineExecute << <gridSize, blockSize >> >(n, jobL, raysL, WangHash(++raySeedGl), scene);
			CudaCheck(cudaDeviceSynchronize());
			uint numActive = numRays;
			for (uint i = 0; i < rayDepth; i++){
				CudaCheck(cudaDeviceSynchronize());

				size_t mem_tot_0 = 0;
				size_t mem_free_0 = 0;
				cudaMemGetInfo(&mem_free_0, &mem_tot_0);
				std::cout << "GPU Memory left: " << mem_free_0/1000000000.0f << std::endl;
				thrust::device_ptr<Ray> newEnd = thrust::remove_if(rayPtr, rayPtr + numActive, is_marked());
				CudaCheck(cudaDeviceSynchronize());

				numActive = newEnd.get() - rayPtr.get();

				blockSize = 64;
				gridSize = (numActive + blockSize - 1) / blockSize;

				EngineExecuteSample << <gridSize, blockSize >> >(numActive, jobL, raysL, WangHash(++raySeedGl), scene);
				CudaCheck(cudaDeviceSynchronize());
			}
			CudaCheck(cudaDeviceSynchronize());

			cudaEventRecord(stop, 0);
			cudaEventSynchronize(stop);
			cudaEventElapsedTime(&time, start, stop);
			cudaEventDestroy(start);
			cudaEventDestroy(stop);

			std::cout << "RayEngine Execution: " << time << "ms" << std::endl;

			CudaCheck(cudaDeviceSynchronize());
		}
	}

}

__host__ void Cleanup(){

	// empty the vector
	//deviceRays.clear();

	// deallocate any capacity which may currently be associated with vec
	//deviceRays.shrink_to_fit();
}

//__host__ void ClearResults(std::vector<RayJob*>& jobs){
//	internals::ClearResults(jobs);
//}
//__host__ void ProcessJobs(std::vector<RayJob*>& jobs, const Scene* scene){
//	internals::ProcessJobs(jobs, scene);
//}
//
//__host__ void Cleanup(){
//	internals::Cleanup();
//}
