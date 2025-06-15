/*
  globals_aarch64.cpp - Global definitions and structures with ARM64 support
*/

#ifndef GLOBALS_AARCH64_H
#define GLOBALS_AARCH64_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef __aarch64__
  #include <arm_neon.h>
  #include "sse2neon.h"
#else
  #include <emmintrin.h>
#endif

// Platform-specific alignment
#ifdef __aarch64__
  #define SIMD_ALIGN 16
  #define CACHE_LINE_SIZE 64
#else
  #define SIMD_ALIGN 16
  #define CACHE_LINE_SIZE 64
#endif

// Aligned memory allocation
inline void* aligned_malloc(size_t size, size_t alignment) {
    #ifdef __aarch64__
        void* ptr;
        if (posix_memalign(&ptr, alignment, size) != 0) {
            return NULL;
        }
        return ptr;
    #else
        return _mm_malloc(size, alignment);
    #endif
}

inline void aligned_free(void* ptr) {
    #ifdef __aarch64__
        free(ptr);
    #else
        _mm_free(ptr);
    #endif
}

// Image pyramid structure
struct PyramidLevel {
    float* data;
    int width;
    int height;
    int stride;  // Aligned stride for SIMD
};

struct ImagePyramid {
    PyramidLevel* levels;
    int num_levels;
    int channels;
};

// Image information structure
struct ImageInfo {
    char* filename;
    uint8_t* data;
    int width;
    int height;
    int channels;
    ImagePyramid* pyramid;
    float* weight_map;
    uint8_t* mask;
};

// Global image array
extern ImageInfo* g_images;
extern int g_num_images;

// Blending parameters
struct BlendParams {
    int pyramid_levels;
    float sigma;
    int cache_images;
    int use_gpu;
    int num_threads;
};

extern BlendParams g_blend_params;

// Timer utility
class my_timer {
private:
    #ifdef __aarch64__
        uint64_t start_time;
        uint64_t freq;
    #else
        uint64_t start_time;
    #endif
    
public:
    my_timer() {
        #ifdef __aarch64__
            // Get timer frequency
            asm volatile("mrs %0, cntfrq_el0" : "=r" (freq));
        #endif
    }
    
    void set() {
        #ifdef __aarch64__
            // ARM64 cycle counter
            asm volatile("mrs %0, cntvct_el0" : "=r" (start_time));
        #else
            // x86 timestamp counter
            unsigned int lo, hi;
            asm volatile("rdtsc" : "=a" (lo), "=d" (hi));
            start_time = ((uint64_t)hi << 32) | lo;
        #endif
    }
    
    double get() {
        uint64_t end_time;
        #ifdef __aarch64__
            asm volatile("mrs %0, cntvct_el0" : "=r" (end_time));
            // Convert to seconds using actual frequency
            return (double)(end_time - start_time) / (double)freq;
        #else
            unsigned int lo, hi;
            asm volatile("rdtsc" : "=a" (lo), "=d" (hi));
            end_time = ((uint64_t)hi << 32) | lo;
            // Convert to seconds (assuming 3GHz for simplicity, adjust as needed)
            return (double)(end_time - start_time) / 3000000000.0;
        #endif
    }
};

// SIMD constants
struct SIMDConstants {
    __m128i zero_si128;
    __m128i ones_epi8;
    __m128i ones_epi16;
    __m128i v255_epi16;
    __m128i v128_epi16;
    __m128 zero_ps;
    __m128 ones_ps;
    __m128 half_ps;
    
    void init() {
        zero_si128 = _mm_setzero_si128();
        ones_epi8 = _mm_set1_epi8(1);
        ones_epi16 = _mm_set1_epi16(1);
        v255_epi16 = _mm_set1_epi16(255);
        v128_epi16 = _mm_set1_epi16(128);
        zero_ps = _mm_setzero_ps();
        ones_ps = _mm_set1_ps(1.0f);
        half_ps = _mm_set1_ps(0.5f);
    }
};

extern SIMDConstants g_simd_constants;

// Utility functions
inline int align_up(int value, int alignment) {
    return (value + alignment - 1) & ~(alignment - 1);
}

inline bool is_aligned(const void* ptr, size_t alignment) {
    return ((uintptr_t)ptr & (alignment - 1)) == 0;
}

// Platform info
inline void print_platform_info() {
    #ifdef __aarch64__
        printf("Platform: ARM64/aarch64\n");
        printf("SIMD: ARM NEON\n");
        
        // Check for specific ARM features if available
        #ifdef __ARM_FEATURE_SVE
        printf("SVE: Supported\n");
        #endif
        
        #ifdef __ARM_FEATURE_SVE2
        printf("SVE2: Supported\n");
        #endif
    #else
        printf("Platform: x86/x86_64\n");
        printf("SIMD: SSE2");
        
        // Check for additional x86 features
        #ifdef __SSE3__
        printf(", SSE3");
        #endif
        #ifdef __SSSE3__
        printf(", SSSE3");
        #endif
        #ifdef __SSE4_1__
        printf(", SSE4.1");
        #endif
        #ifdef __SSE4_2__
        printf(", SSE4.2");
        #endif
        #ifdef __AVX__
        printf(", AVX");
        #endif
        #ifdef __AVX2__
        printf(", AVX2");
        #endif
        printf("\n");
    #endif
    
    printf("Alignment: %d bytes\n", SIMD_ALIGN);
    printf("Cache line: %d bytes\n", CACHE_LINE_SIZE);
}

// Error handling
#define CHECK_ALIGNED(ptr, align) \
    if (!is_aligned(ptr, align)) { \
        fprintf(stderr, "Error: Pointer %p not aligned to %zu bytes\n", ptr, (size_t)align); \
        exit(1); \
    }

#define CHECK_NULL(ptr) \
    if (ptr == NULL) { \
        fprintf(stderr, "Error: NULL pointer at %s:%d\n", __FILE__, __LINE__); \
        exit(1); \
    }

// Debug macros
#ifdef DEBUG
    #define DBG_PRINT(...) printf(__VA_ARGS__)
#else
    #define DBG_PRINT(...)
#endif

#endif // GLOBALS_AARCH64_H
