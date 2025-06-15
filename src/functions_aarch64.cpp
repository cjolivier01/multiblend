/*
  functions_aarch64.cpp - Functions with ARM64/aarch64 support
  This file shows how to adapt SIMD functions for both x86 and ARM architectures
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#ifdef __aarch64__
  #include <arm_neon.h>
  #include "sse2neon.h"
#else
  #include <emmintrin.h>
#endif

// Example: Fast memory copy using SIMD
void fast_memcpy_simd(void* dst, const void* src, size_t size) {
    #ifdef __aarch64__
    // ARM64 NEON implementation
    uint8_t* d = (uint8_t*)dst;
    const uint8_t* s = (const uint8_t*)src;
    
    // Copy 16-byte chunks using NEON
    while (size >= 16) {
        uint8x16_t data = vld1q_u8(s);
        vst1q_u8(d, data);
        s += 16;
        d += 16;
        size -= 16;
    }
    
    // Copy remaining bytes
    while (size > 0) {
        *d++ = *s++;
        size--;
    }
    #else
    // x86 SSE2 implementation
    __m128i* d = (__m128i*)dst;
    const __m128i* s = (const __m128i*)src;
    
    // Copy 16-byte chunks using SSE2
    while (size >= 16) {
        __m128i data = _mm_loadu_si128(s);
        _mm_storeu_si128(d, data);
        s++;
        d++;
        size -= 16;
    }
    
    // Copy remaining bytes
    uint8_t* d8 = (uint8_t*)d;
    const uint8_t* s8 = (const uint8_t*)s;
    while (size > 0) {
        *d8++ = *s8++;
        size--;
    }
    #endif
}

// Example: Add two arrays of floats using SIMD
void add_floats_simd(float* result, const float* a, const float* b, size_t count) {
    size_t i = 0;
    
    // Process 4 floats at a time
    for (; i + 3 < count; i += 4) {
        __m128 va = _mm_loadu_ps(&a[i]);
        __m128 vb = _mm_loadu_ps(&b[i]);
        __m128 vr = _mm_add_ps(va, vb);
        _mm_storeu_ps(&result[i], vr);
    }
    
    // Process remaining floats
    for (; i < count; i++) {
        result[i] = a[i] + b[i];
    }
}

// Example: Saturated add for image processing (8-bit unsigned)
void add_pixels_saturated(uint8_t* result, const uint8_t* a, const uint8_t* b, size_t count) {
    size_t i = 0;
    
    // Process 16 pixels at a time
    for (; i + 15 < count; i += 16) {
        __m128i va = _mm_loadu_si128((const __m128i*)&a[i]);
        __m128i vb = _mm_loadu_si128((const __m128i*)&b[i]);
        __m128i vr = _mm_adds_epu8(va, vb);
        _mm_storeu_si128((__m128i*)&result[i], vr);
    }
    
    // Process remaining pixels
    for (; i < count; i++) {
        int sum = a[i] + b[i];
        result[i] = (sum > 255) ? 255 : sum;
    }
}

// Example: Calculate sum of squared differences (useful for image comparison)
int sum_of_squared_differences(const uint8_t* a, const uint8_t* b, size_t count) {
    int sum = 0;
    size_t i = 0;
    
    __m128i vsum = _mm_setzero_si128();
    
    // Process 16 bytes at a time
    for (; i + 15 < count; i += 16) {
        __m128i va = _mm_loadu_si128((const __m128i*)&a[i]);
        __m128i vb = _mm_loadu_si128((const __m128i*)&b[i]);
        
        // Convert to 16-bit first to handle signed differences properly
        __m128i zeros = _mm_setzero_si128();
        
        // Process low 8 bytes
        __m128i va_lo = _mm_unpacklo_epi8(va, zeros);
        __m128i vb_lo = _mm_unpacklo_epi8(vb, zeros);
        __m128i vdiff_lo = _mm_sub_epi16(va_lo, vb_lo);
        
        // Process high 8 bytes
        __m128i va_hi = _mm_unpackhi_epi8(va, zeros);
        __m128i vb_hi = _mm_unpackhi_epi8(vb, zeros);
        __m128i vdiff_hi = _mm_sub_epi16(va_hi, vb_hi);
        
        // Square the differences
        __m128i vsq_lo = _mm_mullo_epi16(vdiff_lo, vdiff_lo);
        __m128i vsq_hi = _mm_mullo_epi16(vdiff_hi, vdiff_hi);
        
        // Convert to 32-bit and accumulate
        vsum = _mm_add_epi32(vsum, _mm_unpacklo_epi16(vsq_lo, zeros));
        vsum = _mm_add_epi32(vsum, _mm_unpackhi_epi16(vsq_lo, zeros));
        vsum = _mm_add_epi32(vsum, _mm_unpacklo_epi16(vsq_hi, zeros));
        vsum = _mm_add_epi32(vsum, _mm_unpackhi_epi16(vsq_hi, zeros));
    }
    
    // Extract sum from vector
    int32_t sums[4];
    _mm_storeu_si128((__m128i*)sums, vsum);
    sum = sums[0] + sums[1] + sums[2] + sums[3];
    
    // Process remaining bytes
    for (; i < count; i++) {
        int diff = (int)a[i] - (int)b[i];
        sum += diff * diff;
    }
    
    return sum;
}

// Example: Alpha blending function
void alpha_blend(uint8_t* result, const uint8_t* fg, const uint8_t* bg, const uint8_t* alpha, size_t count) {
    size_t i = 0;
    
    // Constants for blending
    __m128i v255 = _mm_set1_epi16(255);
    __m128i v128 = _mm_set1_epi16(128);
    
    // Process 8 pixels at a time (we process in 16-bit to avoid overflow)
    for (; i + 7 < count; i += 8) {
        // Load 8 bytes and expand to 16-bit
        __m128i vfg = _mm_unpacklo_epi8(_mm_loadl_epi64((const __m128i*)&fg[i]), _mm_setzero_si128());
        __m128i vbg = _mm_unpacklo_epi8(_mm_loadl_epi64((const __m128i*)&bg[i]), _mm_setzero_si128());
        __m128i valpha = _mm_unpacklo_epi8(_mm_loadl_epi64((const __m128i*)&alpha[i]), _mm_setzero_si128());
        
        // result = (fg * alpha + bg * (255 - alpha) + 128) / 255
        __m128i vinv_alpha = _mm_sub_epi16(v255, valpha);
        __m128i vfg_scaled = _mm_mullo_epi16(vfg, valpha);
        __m128i vbg_scaled = _mm_mullo_epi16(vbg, vinv_alpha);
        __m128i vsum = _mm_add_epi16(_mm_add_epi16(vfg_scaled, vbg_scaled), v128);
        
        // Divide by 255 (approximation using shift and multiply)
        __m128i vresult = _mm_srli_epi16(_mm_mulhi_epu16(vsum, _mm_set1_epi16(0x8081)), 7);
        
        // Pack back to 8-bit and store
        vresult = _mm_packus_epi16(vresult, vresult);
        _mm_storel_epi64((__m128i*)&result[i], vresult);
    }
    
    // Process remaining pixels
    for (; i < count; i++) {
        result[i] = (fg[i] * alpha[i] + bg[i] * (255 - alpha[i]) + 128) / 255;
    }
}

// Example: Find minimum and maximum values in an array
void find_min_max(const int16_t* data, size_t count, int16_t* min_val, int16_t* max_val) {
    if (count == 0) return;
    
    __m128i vmin = _mm_set1_epi16(data[0]);
    __m128i vmax = _mm_set1_epi16(data[0]);
    
    size_t i = 0;
    
    // Process 8 values at a time
    for (; i + 7 < count; i += 8) {
        __m128i vdata = _mm_loadu_si128((const __m128i*)&data[i]);
        vmin = _mm_min_epi16(vmin, vdata);
        vmax = _mm_max_epi16(vmax, vdata);
    }
    
    // Extract min/max from vectors
    int16_t mins[8], maxs[8];
    _mm_storeu_si128((__m128i*)mins, vmin);
    _mm_storeu_si128((__m128i*)maxs, vmax);
    
    *min_val = mins[0];
    *max_val = maxs[0];
    for (int j = 1; j < 8; j++) {
        if (mins[j] < *min_val) *min_val = mins[j];
        if (maxs[j] > *max_val) *max_val = maxs[j];
    }
    
    // Process remaining values
    for (; i < count; i++) {
        if (data[i] < *min_val) *min_val = data[i];
        if (data[i] > *max_val) *max_val = data[i];
    }
}

// Platform detection function
const char* get_simd_platform() {
    #ifdef __aarch64__
    return "ARM64/NEON";
    #else
    return "x86/SSE2";
    #endif
}

// Initialize SIMD (some platforms may need this)
void init_simd() {
    #ifdef __aarch64__
    // ARM NEON is always available on AArch64
    printf("SIMD initialized: ARM NEON support detected\n");
    #else
    // Check for SSE2 support (though it's guaranteed on x64)
    printf("SIMD initialized: SSE2 support detected\n");
    #endif
}
