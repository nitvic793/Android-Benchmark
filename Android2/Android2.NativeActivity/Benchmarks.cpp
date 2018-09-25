#include "Benchmarks.h"
#include <malloc.h>
#include <typeinfo>
#include <algorithm>
#include <ctime>
#include <chrono>
#include <random>


#ifndef L1_SIZE
#define L1_SIZE 24 // 24 KB
#endif

#ifndef L2_SIZE
#define L2_SIZE 2400 // 2400 KB - 2.4 MB. Fits within 4MB of L2 cache
#endif

#ifndef MAIN_SIZE
#define MAIN_SIZE 24000 // 24000 KB - 24 MB. Large enough to hit the main memory
#endif



/* simple class for a pseudo-random generator producing
uniformely distributed integers */
class UniformIntDistribution {
public:
	UniformIntDistribution() : engine(std::random_device()()) {}
	/* return number in the range of [0..upper_limit) */
	unsigned int draw(unsigned int upper_limit) {
		
		return std::uniform_int_distribution<unsigned int>
			(0, upper_limit - 1)(engine);
	}
private:
	std::mt19937 engine;
};

static unsigned int log2(unsigned int val) {
	unsigned int count = 0;
	while (val >>= 1) {
		++count;
	}
	return count;
}

/* reverse the given number of bits within val */
static unsigned int bit_reverse(unsigned int val, unsigned int bits) {
	unsigned int result = 0;
	while (bits > 0) {
		result = (result << 1) | (val & 1);
		val >>= 1;
		--bits;
	}
	return result;
}

/* generate a bit-reversal permutation; see https://oeis.org/A030109 */
static void gen_bit_reversal_permutation(unsigned int* seq,
	unsigned int bits, unsigned int count) {
	/* generate a bit-reversal permutation for integers from 0 to (2^bits)-1 */
	unsigned int maxval = 1 << bits;
	for (unsigned int val = 0; val < maxval; ++val) {
		seq[val] = bit_reverse(val, bits);
	}
	/* cut sequence short if count is not a power of 2, i.e. count < 2^bits */
	unsigned int current = maxval;
	unsigned int index = 0;
	while (current > count) {
		while (seq[index] < count) ++index;
		--current; seq[index] = seq[current];
	}
}

/* create a cyclic pointer chain where the individual locations
are stride bytes apart */
void** create_linear_chain(std::size_t size, std::size_t stride) {
	std::size_t len = size / sizeof(void*);
	void** memory = new void*[len];

	unsigned int runs = stride / sizeof(void*);
	unsigned int bits = log2(runs);
	if ((1 << bits) != runs) ++bits;
	unsigned int* offset = new unsigned int[1 << bits];
	gen_bit_reversal_permutation(offset, bits, runs);

	/* generate the actual pointer chain */
	unsigned int run = 0;
	void** last = NULL;
	for (unsigned int run = 0; run < runs; ++run) {
		char* next = (char*)memory + offset[run] * sizeof(void*);
		if (last) {
			*last = (void*)next;
		}
		last = (void**)next;
		for (;;) {
			char* next = (char*)last + stride;
			if (next >= (char*)memory + size) break;
			*last = (void*)next; last = (void**)next;
		}
	}
	*last = (void*)memory; // close the cycle 
	return memory;
}

void** create_random_chain(std::size_t size) {
	std::size_t len = size / sizeof(void*);
	void** memory = new void*[len];

	UniformIntDistribution uniform;

	// shuffle indices
	size_t* indices = new std::size_t[len];
	for (std::size_t i = 0; i < len; ++i) {
		indices[i] = i;
	}
	for (std::size_t i = 0; i < len - 1; ++i) {
		std::size_t j = i + uniform.draw(len - i);
		if (i != j) {
			std::swap(indices[i], indices[j]);
		}
	}
	// fill memory with pointer references
	for (std::size_t i = 1; i < len; ++i) {
		memory[indices[i - 1]] = (void*)&memory[indices[i]];
	}
	memory[indices[len - 1]] = (void*)&memory[indices[0]];
	delete[] indices;
	return memory;
}

int64_t chase_pointers(void** memory, std::size_t count) {
	auto start = std::chrono::high_resolution_clock::now();
	void** p = (void**)memory;
	while (count-- > 0) {
		p = (void**)*p;
	}
	auto finish = std::chrono::high_resolution_clock::now();
	return std::chrono::duration_cast<std::chrono::nanoseconds>(finish - start).count();
	//return 0;
}

double Benchmark(unsigned int sizeInKB) {
	size_t byteSize = sizeInKB * 1024; //24 KB
	size_t stride = 4;// sizeof(void*);
	std::size_t count = (std::size_t) 1 << 30;
	void **memory = create_random_chain(byteSize);
	auto timeTaken = chase_pointers(memory, count);
	auto avgtimeTaken = (double)timeTaken / (double)count;
	return avgtimeTaken;
}

double BenchmarkL1ToCPU()
{
	return Benchmark(L1_SIZE);
}

double BenchmarkL2ToCPU()
{
	return Benchmark(L2_SIZE);
}

double BenchmarkMainMemToCPU()
{
	return Benchmark(MAIN_SIZE);
}