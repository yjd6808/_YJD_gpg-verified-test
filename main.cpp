
#pragma comment(lib, "Shlwapi.lib")

#include <benchmark/benchmark.h>
#include <iostream>


using namespace std;

constexpr int bit_length(int n)
{
	int len = 0;

	while (n > 0) {
		n /= 2;
		len++;
	}

	return len;
}

// 64 byte cache line
// 16-way associated cache
// 8MB L3 cache size

constexpr int cache_line_size = 64;
constexpr int l3_cache_size = 8 * 1024 * 1024; // 8MB

// 세트당 크기 = 64 byte * 16-way
//			 = 1024 byte
constexpr int set_size = 1 << 10;

// 캐시 라인을 표현가능한 최대 bit 크기 (64 byte이므로 0b000000 ~ 0b111111 6개의 비트만 있으면 댐)
constexpr int offset_bit_size = bit_length(1 << 6) - 1;

// 세트당 크기가 1024 byte이고 L3 캐시 크기가 8MB이므로
// L3 캐시의 세트 갯수는 8192개이다.
// 8192는 13개의 bit로 모두 표현이가능하다.
constexpr int index_bit_size = bit_length(l3_cache_size / set_size) - 1;

// 32 bit 프로세스의 포인터크기는 32 bit
// 64 bit 프로세스의 포인터크기는 64 bit -> 근데 48bit만 실제로 사용한다.
// https://stackoverflow.com/questions/6716946/why-do-x86-64-systems-have-only-a-48-bit-virtual-address-space
#ifdef X64
using ptr_t = unsigned long long;
constexpr int valid_pointer_bit_size = 48;
constexpr int tag_bit_size = valid_pointer_bit_size - offset_bit_size - index_bit_size;
#else
using ptr_t = unsigned;
constexpr int valid_pointer_bit_size = 32;
constexpr int tag_bit_size = valid_pointer_bit_size - offset_bit_size - index_bit_size;
#endif 



void clear_cache()
{
	static char _[l3_cache_size]{}; // 8mb = 1024kb * 8;
	for (int i = 0; i < l3_cache_size; i++) {
		_[i] = rand();
	}
}

constexpr int extract_index_bit(const ptr_t ptr_val)
{
	constexpr ptr_t mask = 0x1fff; // 0b1111111111111 13 bit mask
	return (ptr_val >> offset_bit_size) & mask;
}

struct alignas(cache_line_size) cache_line_chunk {
	int values[16]{};
};

constexpr int temp_count = 8192 * 16;
cache_line_chunk temps[temp_count]{};


void BM_call(benchmark::State& state) {
	int val = state.range(0);
	clear_cache();
	for (auto _ : state) {
		for (auto& temp : temps) {
			temp.values[0] = val;
		}
	}
}

void BM_call2(benchmark::State& state) {
	int val = state.range(0);
	for (auto _ : state) {
		for (auto& temp : temps) {
			temp.values[0] = val;
		}
	}
}

BENCHMARK(BM_call)->Arg(1)->Iterations(100)->Iterations(200)->Iterations(300);
BENCHMARK(BM_call2)->Arg(2)->Iterations(100);

int main(int argc, char** argv) {
	::benchmark::Initialize(&argc, argv);                               
	::benchmark::RunSpecifiedBenchmarks();
	::benchmark::Shutdown();                                            
	return 0;                                                           
}


// mod
