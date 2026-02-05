# Graphics and Shader Development Guide

## Overview

This guide covers the DirectX 12 graphics pipeline in Starstrike, shader development practices, and rendering techniques.

## DirectX 12 Rendering Pipeline

### Frame Rendering Flow

```
┌───────────────────────────────────────────────────────────┐
│ 1. Begin Frame                                             │
│    - Acquire backbuffer                                    │
│    - Reset command allocators                              │
└─────────────────────┬─────────────────────────────────────┘
                      │
┌─────────────────────▼─────────────────────────────────────┐
│ 2. Update Stage                                            │
│    - Upload dynamic data (constant buffers)                │
│    - Update per-frame resources                            │
└─────────────────────┬─────────────────────────────────────┘
                      │
┌─────────────────────▼─────────────────────────────────────┐
│ 3. Culling & Sorting                                       │
│    - Frustum culling                                       │
│    - Sort by material/depth                                │
└─────────────────────┬─────────────────────────────────────┘
                      │
┌─────────────────────▼─────────────────────────────────────┐
│ 4. Render Passes                                           │
│    - Shadow pass                                           │
│    - Opaque geometry pass                                  │
│    - Skybox                                                │
│    - Transparent geometry                                  │
│    - Post-processing                                       │
└─────────────────────┬─────────────────────────────────────┘
                      │
┌─────────────────────▼─────────────────────────────────────┐
│ 5. Present                                                 │
│    - Transition backbuffer                                 │
│    - Present to screen                                     │
└───────────────────────────────────────────────────────────┘
```

### Resource Barriers

DirectX 12 requires explicit resource state transitions:

```cpp
// Example: Transition render target for rendering
D3D12_RESOURCE_BARRIER barrier = {};
barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
barrier.Transition.pResource = renderTarget;
barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

commandList->ResourceBarrier(1, &barrier);
```

**Common States:**
- `D3D12_RESOURCE_STATE_RENDER_TARGET`: Rendering to texture
- `D3D12_RESOURCE_STATE_DEPTH_WRITE`: Writing to depth buffer
- `D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE`: Reading in pixel shader
- `D3D12_RESOURCE_STATE_PRESENT`: Presenting to screen
- `D3D12_RESOURCE_STATE_COPY_SOURCE/DEST`: Copy operations

## Shader Model 6.0+

Starstrike uses DirectX Shader Model 6.0 or higher, compiled with DXC (DirectX Shader Compiler).

### Shader Stages

#### Vertex Shader

```hlsl
// Vertex shader input (from vertex buffer)
struct VSInput {
    float3 position : POSITION;
    float3 normal : NORMAL;
    float2 texCoord : TEXCOORD;
    float3 tangent : TANGENT;
};

// Vertex shader output (to pixel shader)
struct VSOutput {
    float4 position : SV_POSITION;  // Required: clip-space position
    float3 worldPos : WORLD_POSITION;
    float3 normal : NORMAL;
    float2 texCoord : TEXCOORD;
    float3 tangent : TANGENT;
};

// Constant buffer for per-object data
cbuffer ObjectConstants : register(b0) {
    float4x4 worldMatrix;
    float4x4 worldViewProjMatrix;
    float4x4 normalMatrix;
};

VSOutput VSMain(VSInput input) {
    VSOutput output;
    
    // Transform to clip space
    output.position = mul(float4(input.position, 1.0), worldViewProjMatrix);
    
    // Transform to world space
    output.worldPos = mul(float4(input.position, 1.0), worldMatrix).xyz;
    output.normal = normalize(mul(input.normal, (float3x3)normalMatrix));
    output.tangent = normalize(mul(input.tangent, (float3x3)normalMatrix));
    
    // Pass through texture coordinates
    output.texCoord = input.texCoord;
    
    return output;
}
```

#### Pixel Shader

```hlsl
// Pixel shader input (from vertex shader)
struct PSInput {
    float4 position : SV_POSITION;
    float3 worldPos : WORLD_POSITION;
    float3 normal : NORMAL;
    float2 texCoord : TEXCOORD;
    float3 tangent : TANGENT;
};

// Pixel shader output
struct PSOutput {
    float4 color : SV_TARGET;  // Output to render target
};

// Constant buffer for per-frame data
cbuffer FrameConstants : register(b1) {
    float3 cameraPosition;
    float time;
    float3 lightDirection;
    float _padding;
    float4 lightColor;
};

// Textures and samplers
Texture2D albedoTexture : register(t0);
Texture2D normalTexture : register(t1);
Texture2D metallicRoughnessTexture : register(t2);

SamplerState linearSampler : register(s0);

PSOutput PSMain(PSInput input) {
    PSOutput output;
    
    // Sample textures
    float4 albedo = albedoTexture.Sample(linearSampler, input.texCoord);
    float3 normal = normalTexture.Sample(linearSampler, input.texCoord).xyz;
    float2 metallicRoughness = metallicRoughnessTexture.Sample(linearSampler, input.texCoord).bg;
    
    // Convert normal from [0,1] to [-1,1]
    normal = normal * 2.0 - 1.0;
    
    // Transform normal to world space (TBN matrix)
    float3 N = normalize(input.normal);
    float3 T = normalize(input.tangent);
    float3 B = cross(N, T);
    float3x3 TBN = float3x3(T, B, N);
    normal = normalize(mul(normal, TBN));
    
    // Simple lighting calculation (placeholder)
    float3 L = normalize(-lightDirection);
    float NdotL = max(dot(normal, L), 0.0);
    
    float3 finalColor = albedo.rgb * lightColor.rgb * NdotL;
    
    output.color = float4(finalColor, albedo.a);
    return output;
}
```

#### Compute Shader

```hlsl
// Compute shader for particle simulation
struct Particle {
    float3 position;
    float3 velocity;
    float lifetime;
    float size;
};

// Structured buffers
StructuredBuffer<Particle> particlesIn : register(t0);
RWStructuredBuffer<Particle> particlesOut : register(u0);

cbuffer ComputeConstants : register(b0) {
    float deltaTime;
    float3 gravity;
};

[numthreads(256, 1, 1)]
void CSMain(uint3 dispatchThreadID : SV_DispatchThreadID) {
    uint index = dispatchThreadID.x;
    
    // Load particle
    Particle p = particlesIn[index];
    
    // Update physics
    p.velocity += gravity * deltaTime;
    p.position += p.velocity * deltaTime;
    p.lifetime -= deltaTime;
    
    // Write back
    particlesOut[index] = p;
}
```

## Shader Compilation

### DXC Compiler

Shaders are compiled using DXC (DirectX Shader Compiler):

```bash
# Compile vertex shader
dxc -T vs_6_0 -E VSMain shader.hlsl -Fo shader_vs.bin -Zi -Od

# Compile pixel shader
dxc -T ps_6_0 -E PSMain shader.hlsl -Fo shader_ps.bin -Zi -Od

# Compile compute shader
dxc -T cs_6_0 -E CSMain compute.hlsl -Fo compute_cs.bin -Zi -Od
```

**Compiler Flags:**
- `-T <target>`: Shader target (vs_6_0, ps_6_0, cs_6_0, etc.)
- `-E <entry>`: Entry point function name
- `-Fo <output>`: Output binary file
- `-Zi`: Enable debug info
- `-Od`: Disable optimizations (debug)
- `-O3`: Maximum optimization (release)
- `-Qstrip_reflect`: Strip reflection data (smaller binaries)

### Build Integration

Shaders are automatically compiled during build:

```cmake
# CMake shader compilation
set(SHADER_FILES
    shaders/basic.hlsl
    shaders/pbr.hlsl
    shaders/skybox.hlsl
)

foreach(SHADER ${SHADER_FILES})
    # Compile vertex shader
    add_custom_command(
        OUTPUT ${CMAKE_BINARY_DIR}/shaders/${SHADER}_vs.bin
        COMMAND dxc -T vs_6_0 -E VSMain ${SHADER} -Fo ${CMAKE_BINARY_DIR}/shaders/${SHADER}_vs.bin
        DEPENDS ${SHADER}
    )
    
    # Compile pixel shader
    add_custom_command(
        OUTPUT ${CMAKE_BINARY_DIR}/shaders/${SHADER}_ps.bin
        COMMAND dxc -T ps_6_0 -E PSMain ${SHADER} -Fo ${CMAKE_BINARY_DIR}/shaders/${SHADER}_ps.bin
        DEPENDS ${SHADER}
    )
endforeach()
```

## Root Signature

Root signatures define the layout of resources accessible to shaders:

```cpp
// C++ code to create root signature
CD3DX12_ROOT_PARAMETER rootParams[3];

// CBV for per-object constants
rootParams[0].InitAsConstantBufferView(0); // b0

// CBV for per-frame constants  
rootParams[1].InitAsConstantBufferView(1); // b1

// Descriptor table for textures
CD3DX12_DESCRIPTOR_RANGE texTable[1];
texTable[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 3, 0); // t0-t2
rootParams[2].InitAsDescriptorTable(1, texTable);

// Static samplers
CD3DX12_STATIC_SAMPLER_DESC samplers[1];
samplers[0].Init(0, D3D12_FILTER_MIN_MAG_MIP_LINEAR); // s0

CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc;
rootSigDesc.Init(3, rootParams, 1, samplers,
    D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
```

## Pipeline State Objects (PSO)

PSOs encapsulate all pipeline state:

```cpp
D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};

// Shaders
psoDesc.VS = { vertexShaderBytecode, vertexShaderSize };
psoDesc.PS = { pixelShaderBytecode, pixelShaderSize };

// Root signature
psoDesc.pRootSignature = rootSignature;

// Input layout
D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
    { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
};
psoDesc.InputLayout = { inputLayout, _countof(inputLayout) };

// Rasterizer state
psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;

// Blend state
psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);

// Depth stencil state
psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
psoDesc.DepthStencilState.DepthEnable = TRUE;
psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;

// Render target formats
psoDesc.NumRenderTargets = 1;
psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
psoDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

// Multisampling
psoDesc.SampleMask = UINT_MAX;
psoDesc.SampleDesc.Count = 1;

// Topology
psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pso));
```

## Rendering Techniques

### Physically Based Rendering (PBR)

PBR shader using Cook-Torrance BRDF:

```hlsl
float3 CalculatePBR(float3 albedo, float metallic, float roughness,
                    float3 N, float3 V, float3 L, float3 F0) {
    float3 H = normalize(V + L);
    
    // Normal Distribution Function (GGX/Trowbridge-Reitz)
    float NdotH = max(dot(N, H), 0.0);
    float alpha = roughness * roughness;
    float alphaSq = alpha * alpha;
    float denom = (NdotH * NdotH) * (alphaSq - 1.0) + 1.0;
    float D = alphaSq / (PI * denom * denom);
    
    // Geometry Function (Smith's method)
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float k = (roughness + 1.0) * (roughness + 1.0) / 8.0;
    float G1_V = NdotV / (NdotV * (1.0 - k) + k);
    float G1_L = NdotL / (NdotL * (1.0 - k) + k);
    float G = G1_V * G1_L;
    
    // Fresnel (Schlick's approximation)
    float VdotH = max(dot(V, H), 0.0);
    float3 F = F0 + (1.0 - F0) * pow(1.0 - VdotH, 5.0);
    
    // Cook-Torrance BRDF
    float3 specular = (D * G * F) / (4.0 * NdotV * NdotL + 0.001);
    
    // Diffuse (Lambertian)
    float3 kD = (1.0 - F) * (1.0 - metallic);
    float3 diffuse = kD * albedo / PI;
    
    return (diffuse + specular) * NdotL;
}
```

### Shadow Mapping

```hlsl
// Shadow map sampling (PCF - Percentage Closer Filtering)
float SampleShadowMap(Texture2D shadowMap, SamplerComparisonState shadowSampler,
                       float4 shadowPos, float2 texelSize) {
    // Perspective divide
    float3 projCoords = shadowPos.xyz / shadowPos.w;
    
    // Transform to [0,1] range
    projCoords.xy = projCoords.xy * 0.5 + 0.5;
    projCoords.y = 1.0 - projCoords.y;
    
    // PCF sampling (3x3 kernel)
    float shadow = 0.0;
    for (int x = -1; x <= 1; ++x) {
        for (int y = -1; y <= 1; ++y) {
            float2 offset = float2(x, y) * texelSize;
            shadow += shadowMap.SampleCmpLevelZero(shadowSampler,
                projCoords.xy + offset, projCoords.z);
        }
    }
    return shadow / 9.0;
}
```

### Skybox Rendering

```hlsl
// Skybox vertex shader
struct SkyboxVSOutput {
    float4 position : SV_POSITION;
    float3 texCoord : TEXCOORD;
};

SkyboxVSOutput SkyboxVS(float3 position : POSITION) {
    SkyboxVSOutput output;
    
    // Remove translation from view matrix
    float4x4 viewNoTranslation = viewMatrix;
    viewNoTranslation[3] = float4(0, 0, 0, 1);
    
    // Transform to clip space (z = w for far plane)
    output.position = mul(float4(position, 1.0), mul(viewNoTranslation, projMatrix));
    output.position.z = output.position.w;
    
    output.texCoord = position;
    return output;
}

// Skybox pixel shader
float4 SkyboxPS(SkyboxVSOutput input) : SV_TARGET {
    return skyboxCubemap.Sample(linearSampler, input.texCoord);
}
```

## Performance Optimization

### Instancing

Reduce draw calls by instancing identical meshes:

```hlsl
// Vertex shader with instancing
struct InstanceData {
    float4x4 worldMatrix;
    float4 color;
};

StructuredBuffer<InstanceData> instanceData : register(t0);

VSOutput VSMain(VSInput input, uint instanceID : SV_InstanceID) {
    VSOutput output;
    
    // Get instance-specific data
    InstanceData instance = instanceData[instanceID];
    
    // Transform vertex
    float4 worldPos = mul(float4(input.position, 1.0), instance.worldMatrix);
    output.position = mul(worldPos, viewProjMatrix);
    output.color = instance.color;
    
    return output;
}
```

### GPU Culling

Perform frustum culling on GPU:

```hlsl
// Compute shader for GPU culling
cbuffer CullingConstants : register(b0) {
    float4 frustumPlanes[6];
    float4x4 viewProjMatrix;
};

struct DrawCommand {
    uint indexCount;
    uint instanceCount;
    uint startIndex;
    int baseVertex;
    uint startInstance;
};

StructuredBuffer<float4> boundingSpheres : register(t0);
RWStructuredBuffer<DrawCommand> drawCommands : register(u0);
RWByteAddressBuffer commandCount : register(u1);

[numthreads(64, 1, 1)]
void CullCS(uint3 id : SV_DispatchThreadID) {
    uint objectIndex = id.x;
    
    // Load bounding sphere
    float4 sphere = boundingSpheres[objectIndex];
    float3 center = sphere.xyz;
    float radius = sphere.w;
    
    // Frustum culling
    bool visible = true;
    for (uint i = 0; i < 6; ++i) {
        float distance = dot(frustumPlanes[i].xyz, center) + frustumPlanes[i].w;
        if (distance < -radius) {
            visible = false;
            break;
        }
    }
    
    // Add to draw list if visible
    if (visible) {
        uint commandIndex;
        commandCount.InterlockedAdd(0, 1, commandIndex);
        
        DrawCommand cmd;
        cmd.indexCount = meshInfo[objectIndex].indexCount;
        cmd.instanceCount = 1;
        cmd.startIndex = meshInfo[objectIndex].startIndex;
        cmd.baseVertex = 0;
        cmd.startInstance = objectIndex;
        
        drawCommands[commandIndex] = cmd;
    }
}
```

## Debugging Shaders

### Visual Studio Graphics Debugger

1. Run application with Graphics Diagnostics enabled
2. Capture frame (Alt+F5)
3. Analyze draw calls, resources, and pipeline state
4. Debug pixel/vertex shaders

### RenderDoc

1. Launch application through RenderDoc
2. Press F12 to capture frame
3. Inspect draw calls, textures, buffers
4. Visualize shader inputs/outputs

### PIX

1. Open PIX for Windows
2. Select Starstrike.exe
3. Start GPU capture
4. Analyze GPU timeline and resource usage

### Debug Output

```hlsl
// Debug visualization in pixel shader
float4 DebugPS(PSInput input) : SV_TARGET {
    // Visualize normals
    return float4(input.normal * 0.5 + 0.5, 1.0);
    
    // Visualize texture coordinates
    // return float4(input.texCoord, 0.0, 1.0);
    
    // Visualize depth
    // return float4(input.position.z.xxx, 1.0);
}
```

## Best Practices

1. **Minimize Resource Barriers**: Batch barriers when possible
2. **Use PSO Caching**: Create PSOs at load time, not runtime
3. **Descriptor Heap Management**: Use ring buffers for dynamic descriptors
4. **Command List Reuse**: Reuse command allocators across frames
5. **Avoid Partial Updates**: Upload entire constant buffers
6. **Profile Early**: Use PIX/RenderDoc to find bottlenecks
7. **Validate with Debug Layer**: Enable D3D12 debug layer during development

## Shader Resources

- [HLSL Reference](https://docs.microsoft.com/en-us/windows/win32/direct3dhlsl/dx-graphics-hlsl)
- [DirectX 12 Graphics](https://docs.microsoft.com/en-us/windows/win32/direct3d12/directx-12-graphics)
- [Learn OpenGL](https://learnopengl.com/) - Concepts apply to DX12
- [Real Shading in Unreal Engine 4](https://blog.selfshadow.com/publications/s2013-shading-course/karis/s2013_pbs_epic_notes_v2.pdf)

---

**Document Status**: Graphics development guide - will be updated with implementation details.
