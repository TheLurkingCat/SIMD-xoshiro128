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
  constexpr XoShiRo128Jump() : jump{} {
    constexpr uint32_t JUMP[4] = {0x8764000b, 0xf542d2d3, 0x6fa035c3, 0x77f2db5b};
    for (int i = 0; i < 4; ++i)
      for (int b = 0; b < 32; ++b) jump[i][b] = static_cast<bool>(JUMP[i] & 1U << b);
  }
  constexpr bool check(int i, int j) const { return jump[i][j]; }
  bool jump[4][32];
};

void jump(uint32_t* __restrict out0, uint32_t* __restrict out1, uint32_t* __restrict out2, uint32_t* __restrict out3) {
  uint32_t s0 = *(out0 - 1);
  uint32_t s1 = *(out1 - 1);
  uint32_t s2 = *(out2 - 1);
  uint32_t s3 = *(out3 - 1);

  uint32_t _s0 = 0;
  uint32_t _s1 = 0;
  uint32_t _s2 = 0;
  uint32_t _s3 = 0;
  constexpr XoShiRo128Jump JumpTable;
  for (int i = 0; i < 4; ++i)
    for (int b = 0; b < 32; ++b) {
      if (JumpTable.check(i, b)) {
        _s0 ^= s0;
        _s1 ^= s1;
        _s2 ^= s2;
        _s3 ^= s3;
      }
      uint32_t temp = s1 << 9;
      s2 ^= s0;
      s3 ^= s1;
      s1 ^= s2;
      s0 ^= s3;
      s2 ^= temp;
      s3 = (s3 << 11) | (s3 >> 21);
    }

  *out0 = _s0;
  *out1 = _s1;
  *out2 = _s2;
  *out3 = _s3;
}
}  // namespace

XoShiRo128::XoShiRo128(uint64_t seed) noexcept {
  // SplitMix64
  SplitMix64 SM64(seed);

  alignas(32) uint32_t S0[8];
  alignas(32) uint32_t S1[8];
  alignas(32) uint32_t S2[8];
  alignas(32) uint32_t S3[8];

  S0[0] = SM64.generate();
  S1[0] = SM64.generate();
  S2[0] = SM64.generate();
  S3[0] = SM64.generate();

  jump(S0 + 1, S1 + 1, S2 + 1, S3 + 1);
  jump(S0 + 2, S1 + 2, S2 + 2, S3 + 2);
  jump(S0 + 3, S1 + 3, S2 + 3, S3 + 3);
  jump(S0 + 4, S1 + 4, S2 + 4, S3 + 4);
  jump(S0 + 5, S1 + 5, S2 + 5, S3 + 5);
  jump(S0 + 6, S1 + 6, S2 + 6, S3 + 6);
  jump(S0 + 7, S1 + 7, S2 + 7, S3 + 7);

  s[0] = _mm256_load_si256((const __m256i*)S0);
  s[1] = _mm256_load_si256((const __m256i*)S1);
  s[2] = _mm256_load_si256((const __m256i*)S2);
  s[3] = _mm256_load_si256((const __m256i*)S3);
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
