#pragma once
#include <immintrin.h>
#include <cstdint>

// Use to select XoShiRo128+ / XoShiRo128++ / XoShiRo128**
// XoShiRo128+ version is faster, but XoShiRo128** is better
// + version have linear artifacts in the low bits
// Use slower XoShiRo128** will be better.
enum class GenType { PLUS, PLUSPLUS, STARSTAR };
// My implement of XOR - Shift - Rotate with 128bit state using SIMD
// https://en.wikipedia.org/wiki/Xorshift
class XoShiRo128 {
 public:
  // Simple use 4 32bits seed, can use up to 128 bit initial state
  explicit XoShiRo128(uint64_t seed) noexcept;
  // Selectable generation of 8 32bits int numbers
  template <GenType T>
  __m256i generate() {
    __m256i result;
    // Compile time branch (replace) for performance
    if constexpr (T == GenType::PLUS)
      // Plus
      result = _mm256_add_epi32(s[0], s[3]);
    else if constexpr (T == GenType::PLUSPLUS)
      // Plus - Rotate - Plus
      result = _mm256_add_epi32(s[0], rotl(_mm256_add_epi32(s[0], s[3]), 7));
    else if constexpr (T == GenType::STARSTAR)
      // Multiply - Rotate - Multiply
      result = _mm256_mullo_epi32(rotl(_mm256_mullo_epi32(s[1], _mm256_set1_epi32(5)), 7), _mm256_set1_epi32(9));
    else
      // Unknown: set all 0s.
      result = _mm256_setzero_si256();
    next();
    return result;
  }

  template <GenType T>
  __m256 generateFloat() {
    __m256i result = generate<T>();
    const __m256 baseFloat = _mm256_set1_ps(0x1.0p-24F);
    result = _mm256_srli_epi32(result, 8);
    const __m256 resultFloat = _mm256_cvtepi32_ps(result);
    return _mm256_mul_ps(resultFloat, baseFloat);
  }

 private:
  // The xor shift function
  void next();
  // The rotate function
  __m256i rotl(__m256i x, int32_t k) { return _mm256_or_si256(_mm256_slli_epi32(x, k), _mm256_srli_epi32(x, 32 - k)); }
  __m256i s[4];
};
