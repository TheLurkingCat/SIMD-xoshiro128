#include "xoshiro128.h"

namespace {
struct SplitMix64 {
  explicit SplitMix64(uint64_t seed) : state(seed) {}
  uint64_t generate() {
    uint64_t result = (state += 0x9E3779B97f4A7C15);
    result = (result ^ (result >> 30)) * 0xBF58476D1CE4E5B9;
    result = (result ^ (result >> 27)) * 0x94D049BB133111EB;
    return result ^ (result >> 31);
  }
  uint64_t state;
};

struct XoShiRo128Jump {
  constexpr XoShiRo128Jump() : table{} {
    constexpr uint32_t JUMP[4] = {0x8764000b, 0xf542d2d3, 0x6fa035c3, 0x77f2db5b};
    for (int i = 0; i < 4; ++i)
      for (int b = 0; b < 32; ++b) table[32 * i + b] = static_cast<bool>(JUMP[i] & 1U << b);
  }
  constexpr bool check(int i) const { return table[i]; }
  bool table[128];
};

void jump(uint32_t* __restrict s0, uint32_t* __restrict s1, uint32_t* __restrict s2, uint32_t* __restrict s3) {
  alignas(32) uint32_t temp[8];
  __m256i mask2 = _mm256_set_epi32(0, 0xFFFFFFFF, 0, 0, 0, 0xFFFFFFFF, 0, 0);
  __m256i mask3 = _mm256_set_epi32(0, 0, 0xFFFFFFFF, 0, 0, 0, 0xFFFFFFFF, 0);
  __m256i preState = _mm256_set_epi32(s0[0], s1[0], s2[0], s3[0], s0[1], s1[1], s2[1], s3[1]);
  __m256i curState = _mm256_setzero_si256();

  constexpr XoShiRo128Jump JumpTable;
  for (int iter = 0; iter < 3; ++iter) {
    for (int b = 0; b < 128; ++b) {
      if (JumpTable.check(b)) {
        curState = _mm256_xor_si256(curState, preState);
      }
      __m256i shiftTemp_t1 = _mm256_slli_epi32(preState, 9);
      __m256i shiftTemp = _mm256_and_si256(shiftTemp_t1, mask2);
      __m256i shuffleTemp = _mm256_shuffle_epi32(preState, _MM_SHUFFLE(0, 1, 3, 2));
      preState = _mm256_xor_si256(preState, shuffleTemp);
      preState = _mm256_xor_si256(preState, shiftTemp);

      __m256i shiftTemp2_t = _mm256_or_si256(_mm256_slli_epi32(preState, 11), _mm256_srli_epi32(preState, 21));
      __m256i shiftTemp2 = _mm256_and_si256(_mm256_xor_si256(shiftTemp2_t, preState), mask3);
      preState = _mm256_xor_si256(preState, shiftTemp2);
    }
    _mm256_store_si256(reinterpret_cast<__m256i*>(temp), curState);
    preState = curState;
    curState = _mm256_setzero_si256();
    s0[iter * 2 + 2] = temp[0];
    s1[iter * 2 + 2] = temp[1];
    s2[iter * 2 + 2] = temp[2];
    s3[iter * 2 + 2] = temp[3];
    s0[iter * 2 + 3] = temp[4];
    s1[iter * 2 + 3] = temp[5];
    s2[iter * 2 + 3] = temp[6];
    s3[iter * 2 + 3] = temp[7];
  }
}
}  // namespace

XoShiRo128::XoShiRo128(uint64_t seed) noexcept {
  alignas(32) uint32_t s0[8];
  alignas(32) uint32_t s1[8];
  alignas(32) uint32_t s2[8];
  alignas(32) uint32_t s3[8];
  // SplitMix64
  SplitMix64 SM64(seed);
  uint64_t temp[4] = {SM64.generate(), SM64.generate(), SM64.generate(), SM64.generate()};
  s0[0] = temp[0];
  s0[1] = temp[2];
  s1[0] = temp[0] >> 32;
  s1[1] = temp[2] >> 32;
  s2[0] = temp[1];
  s2[1] = temp[3];
  s3[0] = temp[1] >> 32;
  s3[1] = temp[3] >> 32;

  jump(s0, s1, s2, s3);

  s[0] = _mm256_load_si256((const __m256i*)s0);
  s[1] = _mm256_load_si256((const __m256i*)s1);
  s[2] = _mm256_load_si256((const __m256i*)s2);
  s[3] = _mm256_load_si256((const __m256i*)s3);
}

void XoShiRo128::next() {
  __m256i temp = _mm256_slli_epi32(s[1], 9);
  s[2] = _mm256_xor_si256(s[2], s[0]);
  s[3] = _mm256_xor_si256(s[3], s[1]);
  s[1] = _mm256_xor_si256(s[1], s[2]);
  s[0] = _mm256_xor_si256(s[0], s[3]);
  s[2] = _mm256_xor_si256(s[2], temp);
  s[3] = rotl(s[3], 11);
}
