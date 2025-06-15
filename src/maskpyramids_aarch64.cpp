/*
  maskpyramids_aarch64.cpp - Mask pyramid operations with ARM64/aarch64 support
  Used for multi-resolution blending in panorama stitching
*/

#ifdef __aarch64__
  #include <arm_neon.h>
  #include "sse2neon.h"
#else
  #include <emmintrin.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

// Gaussian kernel for pyramid construction (5x5 approximation of Gaussian)
static const float gaussian_kernel_5[5] = { 0.0625f, 0.25f, 0.375f, 0.25f, 0.0625f };

// Downsample image by factor of 2 (with Gaussian filtering)
void downsample_2x(float* dst, const float* src, int src_width, int src_height) {
    int dst_width = (src_width + 1) / 2;
    int dst_height = (src_height + 1) / 2;
    
    // Process each destination pixel
    for (int y = 0; y < dst_height; y++) {
        for (int x = 0; x < dst_width; x++) {
            __m128 sum = _mm_setzero_ps();
            float weight_sum = 0.0f;
            
            // Apply 5x5 Gaussian kernel
            for (int ky = -2; ky <= 2; ky++) {
                for (int kx = -2; kx <= 2; kx++) {
                    int src_y = y * 2 + ky;
                    int src_x = x * 2 + kx;
                    
                    // Check bounds
                    if (src_y >= 0 && src_y < src_height && src_x >= 0 && src_x < src_width) {
                        float weight = gaussian_kernel_5[kx + 2] * gaussian_kernel_5[ky + 2];
                        sum = _mm_add_ps(sum, _mm_set1_ps(src[src_y * src_width + src_x] * weight));
                        weight_sum += weight;
                    }
                }
            }
            
            // Normalize and store
            float result[4];
            _mm_storeu_ps(result, sum);
            dst[y * dst_width + x] = (result[0] + result[1] + result[2] + result[3]) / weight_sum;
        }
    }
}

// Upsample image by factor of 2 (bilinear interpolation)
void upsample_2x(float* dst, const float* src, int src_width, int src_height) {
    int dst_width = src_width * 2;
    int dst_height = src_height * 2;
    
    // Process 4 destination pixels at a time when possible
    for (int y = 0; y < src_height; y++) {
        int dst_y = y * 2;
        
        for (int x = 0; x < src_width - 1; x++) {
            int dst_x = x * 2;
            
            // Get source values
            float tl = src[y * src_width + x];        // top-left
            float tr = src[y * src_width + x + 1];    // top-right
            float bl = (y < src_height - 1) ? src[(y + 1) * src_width + x] : tl;      // bottom-left
            float br = (y < src_height - 1) ? src[(y + 1) * src_width + x + 1] : tr;  // bottom-right
            
            // Compute interpolated values
            __m128 v_tl = _mm_set1_ps(tl);
            __m128 v_tr = _mm_set1_ps(tr);
            __m128 v_bl = _mm_set1_ps(bl);
            __m128 v_br = _mm_set1_ps(br);
            
            // Weights for bilinear interpolation
            __m128 w_x = _mm_set_ps(0.5f, 1.0f, 0.5f, 0.0f);  // x weights
            __m128 w_y_top = _mm_set1_ps(1.0f);
            __m128 w_y_bot = _mm_set1_ps(0.5f);
            
            // Top row interpolation
            __m128 top = _mm_add_ps(
                _mm_mul_ps(v_tl, _mm_sub_ps(_mm_set1_ps(1.0f), w_x)),
                _mm_mul_ps(v_tr, w_x)
            );
            
            // Bottom row interpolation
            __m128 bot = _mm_add_ps(
                _mm_mul_ps(v_bl, _mm_sub_ps(_mm_set1_ps(1.0f), w_x)),
                _mm_mul_ps(v_br, w_x)
            );
            
            // Store results
            float top_vals[4], bot_vals[4];
            _mm_storeu_ps(top_vals, top);
            _mm_storeu_ps(bot_vals, bot);
            
            // Write to destination
            dst[dst_y * dst_width + dst_x] = top_vals[0];         // tl
            dst[dst_y * dst_width + dst_x + 1] = top_vals[2];     // tr
            if (dst_y + 1 < dst_height) {
                dst[(dst_y + 1) * dst_width + dst_x] = bot_vals[0];     // bl
                dst[(dst_y + 1) * dst_width + dst_x + 1] = bot_vals[2]; // br
            }
        }
        
        // Handle right edge
        int x = src_width - 1;
        int dst_x = x * 2;
        float val = src[y * src_width + x];
        dst[dst_y * dst_width + dst_x] = val;
        if (dst_x + 1 < dst_width) {
            dst[dst_y * dst_width + dst_x + 1] = val;
        }
        if (dst_y + 1 < dst_height) {
            dst[(dst_y + 1) * dst_width + dst_x] = val;
            if (dst_x + 1 < dst_width) {
                dst[(dst_y + 1) * dst_width + dst_x + 1] = val;
            }
        }
    }
}

// Build Gaussian pyramid
void build_gaussian_pyramid(PyramidLevel* pyramid, const float* image, 
                          int width, int height, int levels) {
    // Level 0 is the original image
    pyramid[0].width = width;
    pyramid[0].height = height;
    pyramid[0].stride = align_up(width, SIMD_ALIGN / sizeof(float));
    pyramid[0].data = (float*)aligned_malloc(pyramid[0].stride * height * sizeof(float), SIMD_ALIGN);
    
    // Copy with aligned stride
    for (int y = 0; y < height; y++) {
        memcpy(&pyramid[0].data[y * pyramid[0].stride], 
               &image[y * width], 
               width * sizeof(float));
    }
    
    // Build remaining levels
    for (int l = 1; l < levels; l++) {
        pyramid[l].width = (pyramid[l-1].width + 1) / 2;
        pyramid[l].height = (pyramid[l-1].height + 1) / 2;
        pyramid[l].stride = align_up(pyramid[l].width, SIMD_ALIGN / sizeof(float));
        pyramid[l].data = (float*)aligned_malloc(pyramid[l].stride * pyramid[l].height * sizeof(float), SIMD_ALIGN);
        
        downsample_2x(pyramid[l].data, pyramid[l-1].data, 
                     pyramid[l-1].width, pyramid[l-1].height);
    }
}

// Build Laplacian pyramid from Gaussian pyramid
void build_laplacian_pyramid(PyramidLevel* laplacian, PyramidLevel* gaussian, int levels) {
    // Temporary buffer for upsampled image
    float* temp = NULL;
    int max_size = 0;
    
    // Find maximum size needed
    for (int l = 0; l < levels - 1; l++) {
        int size = gaussian[l].stride * gaussian[l].height;
        if (size > max_size) {
            max_size = size;
        }
    }
    temp = (float*)aligned_malloc(max_size * sizeof(float), SIMD_ALIGN);
    
    // Build Laplacian levels (all except the last)
    for (int l = 0; l < levels - 1; l++) {
        // Allocate Laplacian level
        laplacian[l].width = gaussian[l].width;
        laplacian[l].height = gaussian[l].height;
        laplacian[l].stride = gaussian[l].stride;
        laplacian[l].data = (float*)aligned_malloc(
            laplacian[l].stride * laplacian[l].height * sizeof(float), SIMD_ALIGN);
        
        // Upsample the next level
        upsample_2x(temp, gaussian[l+1].data, gaussian[l+1].width, gaussian[l+1].height);
        
        // Compute difference (Laplacian = Gaussian - upsampled(next level))
        int width = laplacian[l].width;
        int height = laplacian[l].height;
        int stride = laplacian[l].stride;
        
        for (int y = 0; y < height; y++) {
            int x = 0;
            
            // Process 4 pixels at a time
            for (; x + 3 < width; x += 4) {
                __m128 g = _mm_loadu_ps(&gaussian[l].data[y * stride + x]);
                __m128 u = _mm_loadu_ps(&temp[y * width + x]);  // Note: temp uses width, not stride
                __m128 diff = _mm_sub_ps(g, u);
                _mm_storeu_ps(&laplacian[l].data[y * stride + x], diff);
            }
            
            // Handle remaining pixels
            for (; x < width; x++) {
                laplacian[l].data[y * stride + x] = 
                    gaussian[l].data[y * stride + x] - temp[y * width + x];
            }
        }
    }
    
    // Last level of Laplacian is same as last level of Gaussian
    int l = levels - 1;
    laplacian[l].width = gaussian[l].width;
    laplacian[l].height = gaussian[l].height;
    laplacian[l].stride = gaussian[l].stride;
    laplacian[l].data = (float*)aligned_malloc(
        laplacian[l].stride * laplacian[l].height * sizeof(float), SIMD_ALIGN);
    
    memcpy(laplacian[l].data, gaussian[l].data, 
           laplacian[l].stride * laplacian[l].height * sizeof(float));
    
    aligned_free(temp);
}

// Collapse Laplacian pyramid to reconstruct image
void collapse_pyramid(float* output, PyramidLevel* laplacian, int levels) {
    // Start with the coarsest level
    float* current = (float*)aligned_malloc(
        laplacian[levels-1].stride * laplacian[levels-1].height * sizeof(float), SIMD_ALIGN);
    memcpy(current, laplacian[levels-1].data, 
           laplacian[levels-1].stride * laplacian[levels-1].height * sizeof(float));
    
    // Work backwards through the pyramid
    for (int l = levels - 2; l >= 0; l--) {
        // Allocate expanded version
        float* expanded = (float*)aligned_malloc(
            laplacian[l].stride * laplacian[l].height * sizeof(float), SIMD_ALIGN);
        
        // Upsample current level
        upsample_2x(expanded, current, 
                   laplacian[l+1].width, laplacian[l+1].height);
        
        // Free old current
        aligned_free(current);
        
        // Add Laplacian level
        int width = laplacian[l].width;
        int height = laplacian[l].height;
        int stride = laplacian[l].stride;
        
        current = (float*)aligned_malloc(stride * height * sizeof(float), SIMD_ALIGN);
        
        for (int y = 0; y < height; y++) {
            int x = 0;
            
            // Process 4 pixels at a time
            for (; x + 3 < width; x += 4) {
                __m128 lap = _mm_loadu_ps(&laplacian[l].data[y * stride + x]);
                __m128 exp = _mm_loadu_ps(&expanded[y * stride + x]);
                __m128 sum = _mm_add_ps(lap, exp);
                _mm_storeu_ps(&current[y * stride + x], sum);
            }
            
            // Handle remaining pixels
            for (; x < width; x++) {
                current[y * stride + x] = 
                    laplacian[l].data[y * stride + x] + expanded[y * stride + x];
            }
        }
        
        aligned_free(expanded);
    }
    
    // Copy final result to output
    for (int y = 0; y < laplacian[0].height; y++) {
        memcpy(&output[y * laplacian[0].width], 
               &current[y * laplacian[0].stride], 
               laplacian[0].width * sizeof(float));
    }
    
    aligned_free(current);
}

// Create weight mask for blending (distance from image center)
void create_weight_mask(float* mask, int width, int height, float feather_radius) {
    float center_x = width / 2.0f;
    float center_y = height / 2.0f;
    float max_dist = sqrtf(center_x * center_x + center_y * center_y);
    
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x += 4) {
            __m128 vx = _mm_set_ps(x + 3.0f, x + 2.0f, x + 1.0f, x + 0.0f);
            __m128 vy = _mm_set1_ps((float)y);
            __m128 vcx = _mm_set1_ps(center_x);
            __m128 vcy = _mm_set1_ps(center_y);
            
            // Calculate distance from center
            __m128 dx = _mm_sub_ps(vx, vcx);
            __m128 dy = _mm_sub_ps(vy, vcy);
            __m128 dist_sq = _mm_add_ps(_mm_mul_ps(dx, dx), _mm_mul_ps(dy, dy));
            
            // Can't use _mm_sqrt_ps in SSE2, so store and compute individually
            float dist_sq_arr[4];
            _mm_storeu_ps(dist_sq_arr, dist_sq);
            
            for (int i = 0; i < 4 && x + i < width; i++) {
                float dist = sqrtf(dist_sq_arr[i]);
                float weight = 1.0f - (dist / max_dist);
                
                // Apply feathering
                if (feather_radius > 0) {
                    weight = powf(weight, 1.0f / feather_radius);
                }
                
                // Clamp to [0, 1]
                if (weight < 0.0f) weight = 0.0f;
                if (weight > 1.0f) weight = 1.0f;
                
                mask[y * width + x + i] = weight;
            }
        }
    }
}

// Free pyramid memory
void free_pyramid(PyramidLevel* pyramid, int levels) {
    for (int i = 0; i < levels; i++) {
        if (pyramid[i].data) {
            aligned_free(pyramid[i].data);
            pyramid[i].data = NULL;
        }
    }
}

// Initialize mask pyramids module
void init_mask_pyramids() {
    #ifdef __aarch64__
    printf("Mask pyramids module initialized for ARM64/NEON\n");
    #else
    printf("Mask pyramids module initialized for x86/SSE2\n");
    #endif
}
