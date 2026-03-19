// Shared constant buffer layouts.
// Must match C++ OpenGLD3D::SceneConstants / DrawConstants structs exactly

// Lighting struct (used by DrawConstants)
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

// --- Scene constants — uploaded once per frame (b0) ---
cbuffer SceneConstants : register(b0)
{
    float4 GlobalAmbient;
    float4 FogColor;
    float  FogStart;
    float  FogEnd;
    float  FogDensity;
    uint   FogMode;       // 0 = linear
    float  FadeAlpha;     // 0 = fully visible, 1 = fully black
    uint3  _scenePad;
};

// --- Per-draw constants — uploaded per draw call (b1) ---
cbuffer DrawConstants : register(b1)
{
    row_major float4x4 WorldMatrix;
    row_major float4x4 ProjectionMatrix;

    LightData Lights[8];

    // Material
    float4 MatAmbient;
    float4 MatDiffuse;
    float4 MatSpecular;
    float4 MatEmissive;
    float  MatShininess;
    float3 _matPad;

    uint LightingEnabled;
    uint FogEnabled;
    uint TexturingEnabled0;
    uint TexturingEnabled1;

    // Texture env
    uint TexEnvMode0;
    uint TexEnvMode1;

    // Color material
    uint ColorMaterialEnabled;
    uint ColorMaterialMode; // 0 = DIFFUSE, 1 = AMBIENT_AND_DIFFUSE

    // Alpha test
    uint  AlphaTestEnabled;
    uint  AlphaTestFunc; // 0 = LEQUAL, 1 = GREATER
    float AlphaTestRef;

    // Misc
    uint  NormalizeNormals;
    uint  FlatShading;
    float PointSize;
    float _drawPad;
};
