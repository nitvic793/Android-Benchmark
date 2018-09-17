#include "Benchmarks.h"
#include <malloc.h>

void BenchmarkL1ToCPU()
{
	int iterations = 8;
	void* mem = malloc(sizeof(int32_t) * 8); // 32 KB
	void* tmpPtr = mem;
	for (int i = 0; i < iterations; ++i) {
		tmpPtr = *(void**)tmpPtr; // Pointer chasing
	}
}

void BenchmarkL2ToCPU()
{
}

void BenchmarkMainMemToCPU()
{
}
