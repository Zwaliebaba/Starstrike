#include "pch.h"
#include "PerlinNoise.h"
#include <cmath>

// Ken Perlin's reference permutation table (0..255 shuffled).
static constexpr int s_defaultPerm[256] = {
    151,160,137, 91, 90, 15,131, 13,201, 95, 96, 53,194,233,  7,225,
    140, 36,103, 30, 69,142,  8, 99, 37,240, 21, 10, 23,190,  6,148,
    247,120,234, 75,  0, 26,197, 62, 94,252,219,203,117, 35, 11, 32,
     57,177, 33, 88,237,149, 56, 87,174, 20,125,136,171,168, 68,175,
     74,165, 71,134,139, 48, 27,166, 77,146,158,231, 83,111,229,122,
     60,211,133,230,220,105, 92, 41, 55, 46,245, 40,244,102,143, 54,
     65, 25, 63,161,  1,216, 80, 73,209, 76,132,187,208, 89, 18,169,
    200,196,135,130,116,188,159, 86,164,100,109,198,173,186,  3, 64,
     52,217,226,250,124,123,  5,202, 38,147,118,126,255, 82, 85,212,
    207,206, 59,227, 47, 16, 58, 17,182,189, 28, 42,223,183,170,213,
    119,248,152,  2, 44,154,163, 70,221,153,101,155,167, 43,172,  9,
    129, 22, 39,253, 19, 98,108,110, 79,113,224,232,178,185,112,104,
    218,246, 97,228,251, 34,242,193,238,210,144, 12,191,179,162,241,
     81, 51,145,235,249, 14,239,107, 49,192,214, 31,181,199,106,157,
    184, 84,204,176,115,121, 50, 45,127,  4,150,254,138,236,205, 93,
    222,114, 67, 29, 24, 72,243,141,128,195, 78, 66,215, 61,156,180
};


// ============================================================================
// Construction — seed-based permutation shuffle
// ============================================================================

PerlinNoise::PerlinNoise(int _seed)
{
    // Copy the reference table.
    int p[256];
    for (int i = 0; i < 256; ++i)
        p[i] = s_defaultPerm[i];

    // Fisher-Yates shuffle seeded by a simple LCG.
    unsigned int s = static_cast<unsigned int>(_seed);
    for (int i = 255; i > 0; --i)
    {
        s = s * 1664525u + 1013904223u; // Numerical Recipes LCG
        int j = static_cast<int>((s >> 16) % static_cast<unsigned int>(i + 1));
        int tmp = p[i];
        p[i] = p[j];
        p[j] = tmp;
    }

    // Double the table to avoid index wrapping.
    for (int i = 0; i < 256; ++i)
    {
        m_perm[i]       = p[i];
        m_perm[i + 256] = p[i];
    }
}


// ============================================================================
// Noise helpers
// ============================================================================

float PerlinNoise::Fade(float _t)
{
    // 6t^5 - 15t^4 + 10t^3
    return _t * _t * _t * (_t * (_t * 6.0f - 15.0f) + 10.0f);
}

float PerlinNoise::Lerp(float _t, float _a, float _b)
{
    return _a + _t * (_b - _a);
}

float PerlinNoise::Grad(int _hash, float _x, float _y)
{
    // Use low 2 bits to select one of 4 gradient directions.
    int h = _hash & 3;
    switch (h)
    {
    case 0: return  _x + _y;
    case 1: return -_x + _y;
    case 2: return  _x - _y;
    case 3: return -_x - _y;
    }
    return 0.0f; // unreachable
}


// ============================================================================
// Single-octave 2D Perlin noise
// ============================================================================

float PerlinNoise::Noise(float _x, float _y) const
{
    // Integer cell coordinates.
    int xi = static_cast<int>(std::floor(_x)) & 255;
    int yi = static_cast<int>(std::floor(_y)) & 255;

    // Fractional position within cell.
    float xf = _x - std::floor(_x);
    float yf = _y - std::floor(_y);

    // Fade curves.
    float u = Fade(xf);
    float v = Fade(yf);

    // Hash the four corners.
    int aa = m_perm[m_perm[xi]     + yi];
    int ab = m_perm[m_perm[xi]     + yi + 1];
    int ba = m_perm[m_perm[xi + 1] + yi];
    int bb = m_perm[m_perm[xi + 1] + yi + 1];

    // Bilinear interpolation of gradients.
    float x1 = Lerp(u, Grad(aa, xf, yf),       Grad(ba, xf - 1.0f, yf));
    float x2 = Lerp(u, Grad(ab, xf, yf - 1.0f), Grad(bb, xf - 1.0f, yf - 1.0f));

    return Lerp(v, x1, x2);
}


// ============================================================================
// Multi-octave fractal Brownian motion
// ============================================================================

float PerlinNoise::FBM(float _x, float _y, int _octaves, float _persistence) const
{
    float total     = 0.0f;
    float frequency = 1.0f;
    float amplitude = 1.0f;
    float maxValue  = 0.0f; // For normalization

    for (int i = 0; i < _octaves; ++i)
    {
        total    += Noise(_x * frequency, _y * frequency) * amplitude;
        maxValue += amplitude;
        amplitude *= _persistence;
        frequency *= 2.0f;
    }

    return total / maxValue;
}
