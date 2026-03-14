// Shared per-frame constant buffer layout.
// Must match C++ OpenGLD3D::PerFrameConstants struct exactly (d3d12_backend.h).

cbuffer PerFrameConstants : register(b0)
{
    row_major float4x4 WorldMatrix;
    row_major float4x4 ProjectionMatrix;

    // Lighting (8 lights max)
    struct LightData
    {
        float4 Position;    // .w = 0 directional, 1 point
        float4 Direction;   // for directional lights (pre-transformed)
        float4 Diffuse;
        float4 Specular;
        float4 Ambient;
        uint Enabled;
        float3 _pad;
    };
    LightData Lights[8];

    // Material
    float4 MatAmbient;
    float4 MatDiffuse;
    float4 MatSpecular;
    float4 MatEmissive;
    float MatShininess;
    float3 _matPad;

    // Global state
    float4 GlobalAmbient;
    uint LightingEnabled;
    uint FogEnabled;
    uint TexturingEnabled0;
    uint TexturingEnabled1;

    // Fog
    float4 FogColor;
    float FogStart;
    float FogEnd;
    float FogDensity;
    uint FogMode; // 0 = linear

    // Texture env
    uint TexEnvMode0;
    uint TexEnvMode1;

    // Color material
    uint ColorMaterialEnabled;
    uint ColorMaterialMode; // 0 = DIFFUSE, 1 = AMBIENT_AND_DIFFUSE

    // Alpha test
    uint AlphaTestEnabled;
    uint AlphaTestFunc; // 0 = LEQUAL, 1 = GREATER
    float AlphaTestRef;

    // Misc
    uint NormalizeNormals;
    uint FlatShading;
    float PointSize;
    float _miscPad;
};
