#include <cstdio>

/*
	diameter: 2*Radius + 1

	0 0 0 0 0 0 0 0
	x x x x x x x x
	x x x x x x x x
	x x x x x x x x
	x x x x x x x x
	x x x x x x x x
	x x x x x x x x
	0 0 0 0 0 0 0 0 

	x - data
	0 - redundant halo
*/
template<int radius, int diameter>
__global__ void dFdY_kernel(const float * input, float * output, int nx, int ny)
{
	/// sm_20: 13 registers used by this kernel

	const int gtidx = blockDim.x * blockIdx.x + threadIdx.x;

	int outputIndex = gtidx + radius * nx;
	int inputIndex  = outputIndex - radius * nx;

	// declare local register buffer
	float buffer[diameter];

	// Fill the buffer up to start computations
	#pragma unroll
	for (int i = 1; i < diameter; ++i)
	{
		buffer[i] = input[inputIndex];
		inputIndex += nx;
	}

	/// Move front towards y (vertical) direction
	for (int y = 0; y < ny; ++y)
	{
		// update register values
		#pragma unroll
		for (int i = 0; i < diameter - 1; ++i)
		{
			buffer[i] = buffer[i + 1];
		}
		buffer[diameter - 1] = input[inputIndex];

		// compute (df/dy)(x,y) := [f(x,y+h) - f(x,y+h)]/2*h
		float derivative = 0.5f * (buffer[2] - buffer[0]);

		// write output
		output[outputIndex] = derivative;

		outputIndex += nx;
		inputIndex += nx;
	}
}


void TestPartialDerivative_dFdY()
{
	const int nx       = 2048;
	const int ny       = 2048;

	const int paddedny = (1 + ny + 1);

	const int nelem  = nx * paddedny;
	const int nbytes = nelem * sizeof(float);


	float* fh; cudaMallocHost((void**)&fh, nbytes);
	float* dh; cudaMallocHost((void**)&dh, nbytes);


	/// Fill input array: f(x,y) := (x-A)^2 + (y-B)^2

	/// Fill first halo row
	int a = -1;
	for (int x = 0; x < nx; ++x)
	{
		fh[x + (a + 1) * nx] = (float)((x - nx * 0.5f)*(x - nx * 0.5f) + (a - ny * 0.5f)*(a - ny * 0.5f));
	}
	/// Fill working data (function values)
	for (int y = 0; y < ny; ++y)
	{
		for (int x = 0; x < nx; ++x)
		{
			fh[x + (y + 1) * nx] = (float)((x - nx * 0.5f)*(x - nx * 0.5f) + (y - ny * 0.5f)*(y - ny * 0.5f));
		}
	}
	/// Fill last halo row
	a = ny;
	for (int x = 0; x < nx; ++x)
	{
		fh[x + (a + 1) * nx] = (float)((x - nx * 0.5f)*(x - nx * 0.5f) + (a - ny * 0.5f)*(a - ny * 0.5f));
	}

	/// Fill output array with zeros
	for (int y = 0; y < paddedny; ++y)
	{
		for (int x = 0; x < nx; ++x)
		{
			dh[x + y * nx] = 0.0f;
		}
	}

	float* fd; cudaMalloc((void**)&fd, nbytes);
	float* dd; cudaMalloc((void**)&dd, nbytes);

	cudaMemcpy(fd, fh, nbytes, cudaMemcpyHostToDevice);
	cudaMemcpy(dd, dh, nbytes, cudaMemcpyHostToDevice);

	/// Initialize timer
	cudaEvent_t start;
	cudaEvent_t stop;
	cudaEventCreate(&start);
	cudaEventCreate(&stop);

	int nthread = 128;
	int nblock  = nx/nthread;

	/// Record time befor kernel launch
	cudaEventRecord(start, 0);

	const int radius = 1;
	dFdY_kernel<radius, 2*radius + 1><<<nblock, nthread, (nthread + 2 * radius) * sizeof(float)>>>(fd, dd, nx, ny);

	/// Record time after simulation
	cudaEventRecord(stop, 0);
	cudaEventSynchronize(stop);

	cudaMemcpy(dh, dd, nbytes, cudaMemcpyDeviceToHost);


	/// Calculate kernel time
	float time_ms; cudaEventElapsedTime(&time_ms, start, stop);
	printf("df/dy kernel time: %f ms\n", time_ms);

	/// Release timer
	cudaEventDestroy(start);
	cudaEventDestroy(stop);

	/// Free device memory
	cudaFree(fd);
	cudaFree(dd);

	/// write result to file
	FILE* file = fopen("resultDfDy.txt","w");

	for (int y = 0; y < paddedny; ++y)
	{
		for (int x = 0; x < nx; ++x)
		{
			fprintf(file,"%d %d %f \n", x, y, dh[x + y * nx]);
		}
		fprintf(file,"\n");
	}

	fclose(file);

	/// Free host memory
	cudaFreeHost(fh);
	cudaFreeHost(dh);
}


/*
    offset: 32 elements = 128B

	0 x x x x x x x x 0 
	0 x x x x x x x x 0 
	0 x x x x x x x x 0 
	0 x x x x x x x x 0 
	0 x x x x x x x x 0
	0 x x x x x x x x 0
	0 x x x x x x x x 0
	0 x x x x x x x x 0
	0 x x x x x x x x 0 

	x - data
	0 - redundant halo

	where 0 symbol means 128B offset (32*4B)
*/


template<int radius, int offset>
__global__ void dFdX_kernel(const float * input, float * output, int nx, int ny)
{
	/// sm_20: 14 registers used by this kernel

	extern __shared__ float smem[];

	const int gtidx     = blockDim.x * blockIdx.x + threadIdx.x;
	const int ltidx     = threadIdx.x;
	const int blockdimx = blockDim.x;
	const int rowsize   = offset + nx + offset;
	const int tx        = ltidx + radius;

	/// Move front towards y (vertical) direction
	for (int y = 0; y < ny; ++y)
	{
		// calculate global input index
		const int inputIndex = gtidx + offset + y * rowsize;

		__syncthreads();

		// load "halo" left && right
		if (ltidx < radius)
		{
			smem[ltidx] = input[inputIndex - radius];
			smem[ltidx + blockdimx + radius] = input[blockdimx + inputIndex];
		}

		// load "internal" data
		smem[tx] = input[inputIndex];

		__syncthreads();
		
		// compute (df/dx)(x,y) := [f(x+h,y) - f(x-h,y)]/2*h
		float derivative = 0.5f * (smem[tx + 1] - smem[tx - 1]);

		// write output
		output[inputIndex] = derivative;
	}
}


void TestPartialDerivative_dFdX()
{
	const int nx       = 2048;
	const int ny       = 2048;

	const int pad32    = 32;
	const int paddednx = (pad32 + nx + pad32);

	const int nelem  = paddednx * ny;
	const int nbytes = nelem * sizeof(float);


	float* fh; cudaMallocHost((void**)&fh, nbytes);
	float* dh; cudaMallocHost((void**)&dh, nbytes);
	memset(fh, 0, nbytes);
	memset(dh, 0, nbytes);

	/// Fill input array: f(x,y) := (x-A)^2 + (y-B)^2
	for (int y = 0; y < ny; ++y)
	{
		/// Fill first 32 elements in the row
		for (int x = 0; x < pad32; ++x)
		{
			fh[x + y * paddednx] = 0.0f;
		}
		int a = pad32 - 1;
		fh[a + y * paddednx] = (float)((a - paddednx * 0.5f)*(a - paddednx * 0.5f) + (y - ny * 0.5f)*(y - ny * 0.5f));
		/// Fill working data (function values)
		for (int x = pad32; x < pad32 + nx; ++x)
		{
			fh[x + y * paddednx] = (float)((x - paddednx * 0.5f)*(x - paddednx * 0.5f) + (y - ny * 0.5f)*(y - ny * 0.5f));
		}
		/// Fill last 32 elements in the row
		for (int x = pad32 + nx; x < pad32 + nx + pad32; ++x)
		{
			fh[x + y * paddednx] = 0.0f;
		}
		a = pad32 + nx;
		fh[a + y * paddednx] = (float)((a - paddednx * 0.5f)*(a - paddednx * 0.5f) + (y - ny * 0.5f)*(y - ny * 0.5f));
	}

	/// Fill output array with zeros
	for (int y = 0; y < ny; ++y)
	{
		for (int x = 0; x < paddednx; ++x)
		{
			dh[x + y * paddednx] = 0.0f;
		}
	}

	float* fd; cudaMalloc((void**)&fd, nbytes);
	float* dd; cudaMalloc((void**)&dd, nbytes);

	cudaMemcpy(fd, fh, nbytes, cudaMemcpyHostToDevice);
	cudaMemcpy(dd, dh, nbytes, cudaMemcpyHostToDevice);

	/// Initialize timer
	cudaEvent_t start;
	cudaEvent_t stop;
	cudaEventCreate(&start);
	cudaEventCreate(&stop);


	int nthread = 128;
	int nblock  = nx/nthread;

	/// Record time befor kernel launch
	cudaEventRecord(start, 0);

	const int radius = 1;
	dFdX_kernel<radius, pad32><<<nblock, nthread, (nthread + 2*radius) * sizeof(float)>>>(fd, dd, nx, ny);

	/// Record time after simulation
	cudaEventRecord(stop, 0);
	cudaEventSynchronize(stop);

	cudaMemcpy(dh, dd, nbytes, cudaMemcpyDeviceToHost);

	/// Calculate kernel time
	float time_ms; cudaEventElapsedTime(&time_ms, start, stop);
	printf("df/dx kernel time: %f ms\n", time_ms);

	/// Release timer
	cudaEventDestroy(start);
	cudaEventDestroy(stop);

	/// Free device memory
	cudaFree(fd);
	cudaFree(dd);

	/// write result to file
	FILE* file = fopen("resultDfDx.txt","w");

	for (int y = 0; y < ny; ++y)
	{
		for (int x = pad32; x < pad32 + nx; ++x)
		{
			fprintf(file,"%d %d %f \n", x, y, dh[x + y * paddednx]);
		}
		fprintf(file,"\n");
	}

	fclose(file);

	/// Free host memory
	cudaFreeHost(fh);
	cudaFreeHost(dh);
}


int main(int argc, char** argv)
{
	TestPartialDerivative_dFdY();
	TestPartialDerivative_dFdX();

	return 0;
}