/*
  blending_aarch64.cpp - Image blending functions with ARM64/aarch64 support
  Demonstrates how to adapt image blending operations for both architectures
*/

#ifdef __aarch64__
  #include <arm_neon.h>
  #include "sse2neon.h"
#else
  #include <emmintrin.h>
#endif

#include <stdint.h>
#include <stdlib.h>

// Structure for image data
struct Image {
    uint8_t* data;
    int width;
    int height;
    int channels;
};

// Blend two images with a weight mask
void blend_images_weighted(uint8_t* output, const uint8_t* img1, const uint8_t* img2, 
                          const uint8_t* weights, size_t pixel_count) {
    size_t i = 0;
    
    // Process 16 pixels at a time
    for (; i + 15 < pixel_count; i += 16) {
        // Load pixels
        __m128i v_img1 = _mm_loadu_si128((const __m128i*)&img1[i]);
        __m128i v_img2 = _mm_loadu_si128((const __m128i*)&img2[i]);
        __m128i v_weights = _mm_loadu_si128((const __m128i*)&weights[i]);
        
        // Expand to 16-bit for calculation (process in two halves)
        __m128i zeros = _mm_setzero_si128();
        
        // Low 8 pixels
        __m128i img1_lo = _mm_unpacklo_epi8(v_img1, zeros);
        __m128i img2_lo = _mm_unpacklo_epi8(v_img2, zeros);
        __m128i weights_lo = _mm_unpacklo_epi8(v_weights, zeros);
        __m128i inv_weights_lo = _mm_sub_epi16(_mm_set1_epi16(255), weights_lo);
        
        // High 8 pixels
        __m128i img1_hi = _mm_unpackhi_epi8(v_img1, zeros);
        __m128i img2_hi = _mm_unpackhi_epi8(v_img2, zeros);
        __m128i weights_hi = _mm_unpackhi_epi8(v_weights, zeros);
        __m128i inv_weights_hi = _mm_sub_epi16(_mm_set1_epi16(255), weights_hi);
        
        // Blend: output = (img1 * weight + img2 * (255-weight) + 128) / 255
        __m128i blend_lo = _mm_add_epi16(
            _mm_mullo_epi16(img1_lo, weights_lo),
            _mm_mullo_epi16(img2_lo, inv_weights_lo)
        );
        blend_lo = _mm_add_epi16(blend_lo, _mm_set1_epi16(128));
        blend_lo = _mm_srli_epi16(_mm_mulhi_epu16(blend_lo, _mm_set1_epi16(0x8081)), 7);
        
        __m128i blend_hi = _mm_add_epi16(
            _mm_mullo_epi16(img1_hi, weights_hi),
            _mm_mullo_epi16(img2_hi, inv_weights_hi)
        );
        blend_hi = _mm_add_epi16(blend_hi, _mm_set1_epi16(128));
        blend_hi = _mm_srli_epi16(_mm_mulhi_epu16(blend_hi, _mm_set1_epi16(0x8081)), 7);
        
        // Pack back to 8-bit
        __m128i result = _mm_packus_epi16(blend_lo, blend_hi);
        _mm_storeu_si128((__m128i*)&output[i], result);
    }
    
    // Handle remaining pixels
    for (; i < pixel_count; i++) {
        int w = weights[i];
        output[i] = (img1[i] * w + img2[i] * (255 - w) + 128) / 255;
    }
}

// Multiband blending - blend different frequency bands with different weights
void multiband_blend_channel(float* output, const float* band1, const float* band2,
                            const float* weights, size_t pixel_count) {
    size_t i = 0;
    
    // Process 4 floats at a time
    for (; i + 3 < pixel_count; i += 4) {
        __m128 v_band1 = _mm_loadu_ps(&band1[i]);
        __m128 v_band2 = _mm_loadu_ps(&band2[i]);
        __m128 v_weights = _mm_loadu_ps(&weights[i]);
        __m128 v_inv_weights = _mm_sub_ps(_mm_set1_ps(1.0f), v_weights);
        
        // Blend: output = band1 * weight + band2 * (1 - weight)
        __m128 v_result = _mm_add_ps(
            _mm_mul_ps(v_band1, v_weights),
            _mm_mul_ps(v_band2, v_inv_weights)
        );
        
        _mm_storeu_ps(&output[i], v_result);
    }
    
    // Handle remaining pixels
    for (; i < pixel_count; i++) {
        float w = weights[i];
        output[i] = band1[i] * w + band2[i] * (1.0f - w);
    }
}

// Laplacian pyramid blending step
void laplacian_blend(float* output, const float* lap1, const float* lap2,
                    const float* mask, int width, int height) {
    size_t total_pixels = width * height;
    size_t i = 0;
    
    // Process 4 pixels at a time
    for (; i + 3 < total_pixels; i += 4) {
        __m128 v_lap1 = _mm_loadu_ps(&lap1[i]);
        __m128 v_lap2 = _mm_loadu_ps(&lap2[i]);
        __m128 v_mask = _mm_loadu_ps(&mask[i]);
        
        // Blend using mask
        __m128 v_result = _mm_add_ps(
            _mm_mul_ps(v_lap1, v_mask),
            _mm_mul_ps(v_lap2, _mm_sub_ps(_mm_set1_ps(1.0f), v_mask))
        );
        
        _mm_storeu_ps(&output[i], v_result);
    }
    
    // Handle remaining pixels
    for (; i < total_pixels; i++) {
        output[i] = lap1[i] * mask[i] + lap2[i] * (1.0f - mask[i]);
    }
}

// Gaussian blur for pyramid construction (simplified 1D)
void gaussian_blur_1d(float* output, const float* input, int length, const float* kernel, int kernel_size) {
    int half_kernel = kernel_size / 2;
    
    for (int i = 0; i < length; i++) {
        __m128 sum = _mm_setzero_ps();
        
        // Apply kernel
        int j = 0;
        for (; j + 3 < kernel_size; j += 4) {
            int idx = i - half_kernel + j;
            if (idx >= 0 && idx + 3 < length) {
                __m128 v_input = _mm_loadu_ps(&input[idx]);
                __m128 v_kernel = _mm_loadu_ps(&kernel[j]);
                sum = _mm_add_ps(sum, _mm_mul_ps(v_input, v_kernel));
            } else {
                // Handle boundary - process one by one
                for (int k = 0; k < 4; k++) {
                    int idx_k = i - half_kernel + j + k;
                    if (idx_k >= 0 && idx_k < length) {
                        float val = input[idx_k] * kernel[j + k];
                        sum = _mm_add_ps(sum, _mm_set_ps(k == 3 ? val : 0, 
                                                         k == 2 ? val : 0,
                                                         k == 1 ? val : 0,
                                                         k == 0 ? val : 0));
                    }
                }
            }
        }
        
        // Handle remaining kernel elements
        for (; j < kernel_size; j++) {
            int idx = i - half_kernel + j;
            if (idx >= 0 && idx < length) {
                float val = input[idx] * kernel[j];
                sum = _mm_add_ps(sum, _mm_set1_ps(val));
            }
        }
        
        // Sum up the components
        float result[4];
        _mm_storeu_ps(result, sum);
        output[i] = result[0] + result[1] + result[2] + result[3];
    }
}

// Feather blending (gradual transition)
void feather_blend(uint8_t* output, const uint8_t* img1, const uint8_t* img2,
                  int width, int height, int feather_width) {
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x += 16) {
            int remaining = width - x;
            int process_count = (remaining >= 16) ? 16 : remaining;
            
            // Calculate blend weights based on position
            uint8_t weights[16];
            for (int i = 0; i < process_count; i++) {
                int pos = x + i;
                float weight;
                
                if (pos < feather_width) {
                    weight = (float)pos / feather_width;
                } else if (pos > width - feather_width) {
                    weight = (float)(width - pos) / feather_width;
                } else {
                    weight = 1.0f;
                }
                
                weights[i] = (uint8_t)(weight * 255);
            }
            
            // Blend using calculated weights
            if (process_count == 16) {
                __m128i v_img1 = _mm_loadu_si128((const __m128i*)&img1[y * width + x]);
                __m128i v_img2 = _mm_loadu_si128((const __m128i*)&img2[y * width + x]);
                __m128i v_weights = _mm_loadu_si128((const __m128i*)weights);
                
                // Blend calculation
                __m128i zeros = _mm_setzero_si128();
                __m128i ones = _mm_set1_epi16(1);
                __m128i v255 = _mm_set1_epi16(255);
                
                // Process low 8 bytes
                __m128i img1_lo = _mm_unpacklo_epi8(v_img1, zeros);
                __m128i img2_lo = _mm_unpacklo_epi8(v_img2, zeros);
                __m128i w_lo = _mm_unpacklo_epi8(v_weights, zeros);
                __m128i inv_w_lo = _mm_sub_epi16(v255, w_lo);
                
                __m128i blend_lo = _mm_add_epi16(
                    _mm_mullo_epi16(img1_lo, w_lo),
                    _mm_mullo_epi16(img2_lo, inv_w_lo)
                );
                blend_lo = _mm_add_epi16(blend_lo, _mm_set1_epi16(128));
                blend_lo = _mm_srli_epi16(blend_lo, 8);
                
                // Process high 8 bytes
                __m128i img1_hi = _mm_unpackhi_epi8(v_img1, zeros);
                __m128i img2_hi = _mm_unpackhi_epi8(v_img2, zeros);
                __m128i w_hi = _mm_unpackhi_epi8(v_weights, zeros);
                __m128i inv_w_hi = _mm_sub_epi16(v255, w_hi);
                
                __m128i blend_hi = _mm_add_epi16(
                    _mm_mullo_epi16(img1_hi, w_hi),
                    _mm_mullo_epi16(img2_hi, inv_w_hi)
                );
                blend_hi = _mm_add_epi16(blend_hi, _mm_set1_epi16(128));
                blend_hi = _mm_srli_epi16(blend_hi, 8);
                
                // Pack and store
                __m128i result = _mm_packus_epi16(blend_lo, blend_hi);
                _mm_storeu_si128((__m128i*)&output[y * width + x], result);
            } else {
                // Handle remaining pixels
                for (int i = 0; i < process_count; i++) {
                    int idx = y * width + x + i;
                    int w = weights[i];
                    output[idx] = (img1[idx] * w + img2[idx] * (255 - w) + 128) / 255;
                }
            }
        }
    }
}

// Initialize blending module
void init_blending() {
    #ifdef __aarch64__
    printf("Blending module initialized for ARM64/NEON\n");
    #else
    printf("Blending module initialized for x86/SSE2\n");
    #endif
}
