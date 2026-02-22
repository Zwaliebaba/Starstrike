// Uber vertex shader — fixed-function emulation for OpenGL-on-D3D12

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

struct VSInput
{
    float3 Position : POSITION;
    float3 Normal   : NORMAL;
    float4 Color    : COLOR;
    float2 TexCoord0: TEXCOORD0;
    float2 TexCoord1: TEXCOORD1;
};

struct VSOutput
{
    float4 Position  : SV_POSITION;
    float4 Color     : COLOR0;
    float2 TexCoord0 : TEXCOORD0;
    float2 TexCoord1 : TEXCOORD1;
    float  FogDist   : TEXCOORD2;
    float3 WorldPos  : TEXCOORD3;
};

VSOutput main(VSInput input)
{
    VSOutput output;

    // Transform position: WorldMatrix acts as combined modelview
    float4 worldPos = mul(float4(input.Position, 1.0f), WorldMatrix);
    output.Position = mul(worldPos, ProjectionMatrix);
    output.WorldPos = worldPos.xyz;

    // Pass through texture coordinates
    output.TexCoord0 = input.TexCoord0;
    output.TexCoord1 = input.TexCoord1;

    // Fog distance (eye-space Z magnitude)
    output.FogDist = length(worldPos.xyz);

    // Vertex color passthrough or lighting
    float4 vertexColor = input.Color;

    if (LightingEnabled)
    {
        float3 normal = input.Normal;
        if (NormalizeNormals)
            normal = normalize(normal);

        // Transform normal by world matrix (no non-uniform scale assumed, or use inverse transpose)
        float3 worldNormal = normalize(mul(normal, (float3x3) WorldMatrix));

        // Material colors — potentially overridden by vertex color
        float4 matAmbient = MatAmbient;
        float4 matDiffuse = MatDiffuse;

        if (ColorMaterialEnabled)
        {
            if (ColorMaterialMode == 1) // AMBIENT_AND_DIFFUSE
            {
                matAmbient = vertexColor;
                matDiffuse = vertexColor;
            }
            else // DIFFUSE only
            {
                matDiffuse = vertexColor;
            }
        }

        // Accumulate lighting
        float3 ambient = GlobalAmbient.rgb * matAmbient.rgb;
        float3 diffuseAccum = float3(0, 0, 0);
        float3 specularAccum = float3(0, 0, 0);

        for (uint i = 0; i < 8; i++)
        {
            if (!Lights[i].Enabled)
                continue;

            float3 lightDir;
            float attenuation = 1.0f;

            if (Lights[i].Position.w == 0.0f) // directional
            {
                // Position stores the direction (pre-transformed by modelview at glLightfv time)
                lightDir = normalize(-Lights[i].Position.xyz);
            }
            else // point light
            {
                float3 toLight = Lights[i].Position.xyz - worldPos.xyz;
                float dist = length(toLight);
                lightDir = toLight / max(dist, 0.0001f);
                // Simple attenuation (constant=1, matches D3D9 default)
            }

            // Per-light ambient
            ambient += Lights[i].Ambient.rgb * matAmbient.rgb;

            // Diffuse
            float NdotL = max(dot(worldNormal, lightDir), 0.0f);
            diffuseAccum += Lights[i].Diffuse.rgb * NdotL * attenuation;

            // Specular
            if (NdotL > 0.0f && MatShininess > 0.0f)
            {
                // Non-local viewer (matches D3DRS_LOCALVIEWER = FALSE)
                float3 viewDir = float3(0, 0, 1);
                float3 halfVec = normalize(lightDir + viewDir);
                float NdotH = max(dot(worldNormal, halfVec), 0.0f);
                specularAccum += Lights[i].Specular.rgb * pow(NdotH, MatShininess) * attenuation;
            }
        }

        float3 finalColor = ambient + diffuseAccum * matDiffuse.rgb + specularAccum * MatSpecular.rgb;
        output.Color = float4(saturate(finalColor), matDiffuse.a);
    }
    else
    {
        output.Color = vertexColor;
    }

    return output;
}
