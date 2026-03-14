#pragma once

#include <vector>

struct TreeVertex
{
    float x, y, z;
    float u, v;
};

struct TreeMeshData
{
    std::vector<TreeVertex> vertices;
    void Clear() { vertices.clear(); }
    void Reserve(size_t n) { vertices.reserve(n); }
};
