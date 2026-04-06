#pragma once

// ============================================================================
// PerlinNoise — Deterministic 2D Perlin noise.
// Uses Ken Perlin's improved noise algorithm with a seed-shuffled permutation
// table.  Output range: approximately [-1, 1] for a single octave.
// Thread-safe (stateless after construction).
// ============================================================================

class PerlinNoise
{
public:
    // Construct with a seed that shuffles the permutation table.
    explicit PerlinNoise(int _seed);

    // Single-octave 2D noise.  Returns value in approximately [-1, 1].
    float Noise(float _x, float _y) const;

    // Multi-octave (fractal Brownian motion) 2D noise.
    // Returns value in approximately [-1, 1] (normalized by amplitude sum).
    float FBM(float _x, float _y, int _octaves, float _persistence) const;

private:
    int m_perm[512]; // Doubled permutation table for wrapping

    static float Fade(float _t);
    static float Lerp(float _t, float _a, float _b);
    static float Grad(int _hash, float _x, float _y);
};
