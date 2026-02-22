#include "pch.h"
#include "opengl_trace.h"

#include "opengl_directx_internals.h"
#include "opengl_directx_dlist_dev.h"
#include "opengl_directx_dlist.h"
#include "opengl_directx_matrix_stack.h"
#include "ogl_extensions.h"
#include "d3d12_backend.h"

#include <limits>
#include <vector>
#include <map>
#include <stack>
#include <algorithm>
#include <cmath>

using namespace OpenGLD3D;
using namespace DirectX;

#define FRAMES_PER_SECOND_COUNTER

// ============================================================================
// CPU-side GL state
// ============================================================================

namespace OpenGLD3D
{
    struct TextureState
    {
        GLuint target;
        GLuint lastTarget;
        GLenum envMode;
        GLenum lastEnvMode;
        GLenum rgbCombineMode;

        TextureState()
            : target(0), lastTarget(static_cast<GLuint>(-1)),
              envMode(GL_MODULATE), lastEnvMode(0), rgbCombineMode(0)
        {
        }
    };
}

// --- Misc ---
static bool s_ccwFrontFace = true;
static bool s_cullBackFace = true;

// --- Hints ---
static GLenum s_fogHint = GL_DONT_CARE;
static GLenum s_polygonSmoothHint = GL_DONT_CARE;

// --- Matrix ---
static GLenum s_matrixMode = GL_MODELVIEW;
static MatrixStack* s_pTargetMatrixStack = nullptr;
static MatrixStack s_modelViewMatrixStack;
static MatrixStack s_projectionMatrixStack;

// --- Colours ---
static UINT32 s_clearColor = 0xFF000000; // ARGB packed

// --- Primitives ---
static GLenum s_primitiveMode = GL_QUADS;

// --- Vertices ---
static CustomVertex* s_vertices = nullptr;
static CustomVertex* s_currentVertex = nullptr;
static unsigned s_allocatedVertices = 0;
static unsigned s_currentVertexNumber = 0;
static int s_quadVertexCount = 0;

// --- Textures ---
struct TextureResource
{
    Microsoft::WRL::ComPtr<ID3D12Resource> resource;
    UINT srvIndex = 0;
    UINT width = 0, height = 0;
    bool valid = false;
};
static std::vector<TextureResource> s_textureResources;

// --- Texture Samplers ---
constexpr int MAX_ACTIVE_TEXTURES = 2;
static unsigned s_activeTexture = 0;
static TextureState s_textureStates[MAX_ACTIVE_TEXTURES];
static TextureState* s_activeTextureState = &s_textureStates[0];

// --- Display lists ---
static std::map<unsigned, DisplayList*> s_displayLists;
static unsigned s_lastDisplayId = 0;
static DisplayListRecorder* s_pDisplayListRecorder = nullptr;

// --- Vertex arrays (client state) ---
struct VertexArrayConfig
{
    const void* pointer = nullptr;
    GLint size = 4;
    GLenum type = GL_FLOAT;
    GLsizei stride = 0;
};
static bool s_clientStateVertex = false;
static bool s_clientStateNormal = false;
static bool s_clientStateColor = false;
static bool s_clientStateTexCoord = false;
static VertexArrayConfig s_vertexArray;
static VertexArrayConfig s_normalArray;
static VertexArrayConfig s_colorArray;
static VertexArrayConfig s_texCoordArray;

// --- Current vertex attributes ---
class CurrentAttributes : public CustomVertex
{
public:
    CurrentAttributes();

    float alphaRef;

    bool texturingEnabled[MAX_ACTIVE_TEXTURES];
    bool backCullingEnabled;
    bool colorMaterialEnabled;
    GLenum colorMaterialMode;
};

static CurrentAttributes s_currentAttribs;

// --- Render state (CPU-side) ---
struct RenderState
{
    bool blendEnabled = false;
    bool depthTestEnabled = false;
    bool depthWriteEnabled = true;
    bool fogEnabled = false;
    bool lightingEnabled = false;
    bool lightsEnabled[8] = {};
    bool alphaTestEnabled = false;
    bool scissorTestEnabled = false;
    bool normalizeEnabled = false;
    bool lineSmoothEnabled = false;

    GLenum blendSrc = GL_ONE;
    GLenum blendDst = GL_ZERO;
    GLenum depthFunc = GL_LEQUAL;
    GLenum alphaFunc = GL_LEQUAL;
    float alphaRef = 0.0f;

    GLenum fillMode = GL_FILL;
    GLenum shadeModel = GL_SMOOTH;

    // Fog
    float fogStart = 0.0f;
    float fogEnd = 1.0f;
    float fogDensity = 1.0f;
    float fogColor[4] = { 0, 0, 0, 0 };
    GLenum fogMode = GL_LINEAR;

    // Lights
    struct Light {
        float position[4] = { 0, 0, 1, 0 };
        float diffuse[4] = { 0, 0, 0, 1 };
        float specular[4] = { 0, 0, 0, 1 };
        float ambient[4] = { 0, 0, 0, 1 };
        bool isDirectional = true;
    } lights[8];

    // Material
    float matAmbient[4] = { 0.2f, 0.2f, 0.2f, 1.0f };
    float matDiffuse[4] = { 0.8f, 0.8f, 0.8f, 1.0f };
    float matSpecular[4] = { 0, 0, 0, 1 };
    float matEmissive[4] = { 0, 0, 0, 1 };
    float matShininess = 0.0f;

    float globalAmbient[4] = { 0.2f, 0.2f, 0.2f, 1.0f };

    // Point
    float pointSize = 1.0f;

    // Viewport
    int viewportX = 0, viewportY = 0, viewportW = 0, viewportH = 0;

    // Sampler state per texture unit
    struct SamplerConfig {
        GLenum minFilter = GL_NEAREST_MIPMAP_LINEAR;
        GLenum magFilter = GL_LINEAR;
        GLenum wrapS = GL_REPEAT;
        GLenum wrapT = GL_REPEAT;
    } samplers[MAX_ACTIVE_TEXTURES];
};
static RenderState s_renderState;

// --- Attribute Stack ---
struct SavedState
{
    CurrentAttributes attribs;
    RenderState renderState;
};
static std::stack<SavedState> s_attributeStack;

// ============================================================================
// CurrentAttributes
// ============================================================================

CurrentAttributes::CurrentAttributes()
{
    ZeroMemory(this, sizeof(*this));
    colorMaterialMode = GL_DIFFUSE;
    a8 = 255;
}

// ============================================================================
// Helpers
// ============================================================================

static D3D12_BLEND glBlendToD3D12(GLenum factor)
{
    switch (factor)
    {
    case GL_ZERO:                return D3D12_BLEND_ZERO;
    case GL_ONE:                 return D3D12_BLEND_ONE;
    case GL_SRC_COLOR:           return D3D12_BLEND_SRC_COLOR;
    case GL_ONE_MINUS_SRC_COLOR: return D3D12_BLEND_INV_SRC_COLOR;
    case GL_SRC_ALPHA:           return D3D12_BLEND_SRC_ALPHA;
    case GL_ONE_MINUS_SRC_ALPHA: return D3D12_BLEND_INV_SRC_ALPHA;
    default:                     return D3D12_BLEND_ONE;
    }
}

static D3D12_COMPARISON_FUNC glFuncToD3D12(GLenum func)
{
    switch (func)
    {
    case GL_LEQUAL:  return D3D12_COMPARISON_FUNC_LESS_EQUAL;
    case GL_GREATER: return D3D12_COMPARISON_FUNC_GREATER;
    default:         return D3D12_COMPARISON_FUNC_LESS_EQUAL;
    }
}

static UINT glTexEnvModeToShader(GLenum mode)
{
    switch (mode)
    {
    case GL_MODULATE: return 0;
    case GL_REPLACE:  return 1;
    case GL_DECAL:    return 2;
    default:          return 0; // default to modulate
    }
}

static UINT glAlphaFuncToShader(GLenum func)
{
    switch (func)
    {
    case GL_LEQUAL:  return 0;
    case GL_GREATER: return 1;
    default:         return 0;
    }
}

static PSOKey buildPSOKey(UINT8 topologyType)
{
    PSOKey key = {};

    // Cull mode: map GL front face + cull face
    if (!s_currentAttribs.backCullingEnabled)
        key.cullMode = 0; // NONE
    else
        key.cullMode = s_ccwFrontFace ? 1 : 2; // FRONT or BACK (D3D12 CW front face)

    key.fillMode = (s_renderState.fillMode == GL_LINE) ? 1 : 0;

    key.depthEnable = s_renderState.depthTestEnabled ? 1 : 0;
    key.depthWriteEnable = s_renderState.depthWriteEnabled ? 1 : 0;
    key.depthFunc = static_cast<UINT8>(glFuncToD3D12(s_renderState.depthFunc));

    key.blendEnable = s_renderState.blendEnabled ? 1 : 0;
    key.srcBlend = static_cast<UINT8>(glBlendToD3D12(s_renderState.blendSrc));
    key.dstBlend = static_cast<UINT8>(glBlendToD3D12(s_renderState.blendDst));

    key.topologyType = topologyType;
    memset(key._pad, 0, sizeof(key._pad));

    return key;
}

static void uploadAndBindConstants()
{
    PerFrameConstants cb = {};

    // Matrices
    XMStoreFloat4x4(&cb.WorldMatrix, s_modelViewMatrixStack.GetTopXM());
    XMStoreFloat4x4(&cb.ProjectionMatrix, s_projectionMatrixStack.GetTopXM());

    // Lights
    for (int i = 0; i < 8; i++)
    {
        cb.Lights[i].Enabled = s_renderState.lightsEnabled[i] ? 1 : 0;
        memcpy(&cb.Lights[i].Position, s_renderState.lights[i].position, sizeof(float) * 4);
        memcpy(&cb.Lights[i].Diffuse, s_renderState.lights[i].diffuse, sizeof(float) * 4);
        memcpy(&cb.Lights[i].Specular, s_renderState.lights[i].specular, sizeof(float) * 4);
        memcpy(&cb.Lights[i].Ambient, s_renderState.lights[i].ambient, sizeof(float) * 4);
    }

    // Material
    memcpy(&cb.MatAmbient, s_renderState.matAmbient, sizeof(float) * 4);
    memcpy(&cb.MatDiffuse, s_renderState.matDiffuse, sizeof(float) * 4);
    memcpy(&cb.MatSpecular, s_renderState.matSpecular, sizeof(float) * 4);
    memcpy(&cb.MatEmissive, s_renderState.matEmissive, sizeof(float) * 4);
    cb.MatShininess = s_renderState.matShininess;

    memcpy(&cb.GlobalAmbient, s_renderState.globalAmbient, sizeof(float) * 4);

    cb.LightingEnabled = s_renderState.lightingEnabled ? 1 : 0;
    cb.FogEnabled = s_renderState.fogEnabled ? 1 : 0;
    cb.TexturingEnabled0 = s_currentAttribs.texturingEnabled[0] ? 1 : 0;
    cb.TexturingEnabled1 = (MAX_ACTIVE_TEXTURES > 1 && s_currentAttribs.texturingEnabled[1]) ? 1 : 0;

    // Fog
    memcpy(&cb.FogColor, s_renderState.fogColor, sizeof(float) * 4);
    cb.FogStart = s_renderState.fogStart;
    cb.FogEnd = s_renderState.fogEnd;
    cb.FogDensity = s_renderState.fogDensity;
    cb.FogMode = 0; // linear

    // Tex env
    cb.TexEnvMode0 = glTexEnvModeToShader(s_textureStates[0].envMode);
    cb.TexEnvMode1 = (MAX_ACTIVE_TEXTURES > 1) ? glTexEnvModeToShader(s_textureStates[1].envMode) : 0;

    // Color material
    cb.ColorMaterialEnabled = s_currentAttribs.colorMaterialEnabled ? 1 : 0;
    cb.ColorMaterialMode = (s_currentAttribs.colorMaterialMode == GL_AMBIENT_AND_DIFFUSE) ? 1 : 0;

    // Alpha test
    cb.AlphaTestEnabled = s_renderState.alphaTestEnabled ? 1 : 0;
    cb.AlphaTestFunc = glAlphaFuncToShader(s_renderState.alphaFunc);
    cb.AlphaTestRef = s_renderState.alphaRef;

    cb.NormalizeNormals = s_renderState.normalizeEnabled ? 1 : 0;
    cb.FlatShading = (s_renderState.shadeModel == GL_FLAT) ? 1 : 0;
    cb.PointSize = s_renderState.pointSize;

    // Allocate a unique constant buffer region per draw call from the ring buffer.
    // A single fixed slot would be overwritten by later draws before the GPU reads it.
    auto alloc = g_backend.GetUploadBuffer().Allocate(sizeof(PerFrameConstants), 256);
    memcpy(alloc.cpuPtr, &cb, sizeof(PerFrameConstants));

    auto* cmdList = g_backend.GetCommandList();
    cmdList->SetGraphicsRootConstantBufferView(0, alloc.gpuAddr);
}

static UINT getSamplerIndex(unsigned texUnit)
{
    auto& s = s_renderState.samplers[texUnit];
    bool linear = (s.magFilter == GL_LINEAR || s.magFilter == GL_LINEAR_MIPMAP_LINEAR);
    bool wrap = (s.wrapS == GL_REPEAT);
    bool mipLinear = (s.minFilter == GL_LINEAR_MIPMAP_LINEAR);

    // Map to our static sampler indices
    if (linear && wrap && mipLinear) return 0;
    if (!linear && wrap) return 1;
    if (linear && !wrap && mipLinear) return 2;
    if (!linear && !wrap) return 3;
    if (linear && wrap && !mipLinear) return 4;
    if (linear && !wrap && !mipLinear) return 5;
    return 0; // default
}

static void bindTexturesAndSamplers()
{
    auto* cmdList = g_backend.GetCommandList();

    // Texture 0
    UINT srvIndex0 = g_backend.GetDefaultTextureSRVIndex();
    if (s_currentAttribs.texturingEnabled[0] &&
        s_textureStates[0].target < s_textureResources.size() &&
        s_textureResources[s_textureStates[0].target].valid)
    {
        srvIndex0 = s_textureResources[s_textureStates[0].target].srvIndex;
    }
    cmdList->SetGraphicsRootDescriptorTable(1, g_backend.GetSRVGPUHandle(srvIndex0));

    // Texture 1
    UINT srvIndex1 = g_backend.GetDefaultTextureSRVIndex();
    if (MAX_ACTIVE_TEXTURES > 1 && s_currentAttribs.texturingEnabled[1] &&
        s_textureStates[1].target < s_textureResources.size() &&
        s_textureResources[s_textureStates[1].target].valid)
    {
        srvIndex1 = s_textureResources[s_textureStates[1].target].srvIndex;
    }
    cmdList->SetGraphicsRootDescriptorTable(2, g_backend.GetSRVGPUHandle(srvIndex1));

    // Samplers — bind a pair starting at sampler index for unit 0
    UINT samplerIdx = getSamplerIndex(0);
    D3D12_GPU_DESCRIPTOR_HANDLE samplerHandle = g_backend.GetSamplerHeap()->GetGPUDescriptorHandleForHeapStart();
    samplerHandle.ptr += samplerIdx * g_backend.GetSamplerDescriptorSize();
    cmdList->SetGraphicsRootDescriptorTable(3, samplerHandle);
}

static UINT8 topologyTypeFromTopology(D3D_PRIMITIVE_TOPOLOGY topology)
{
    if (topology == D3D_PRIMITIVE_TOPOLOGY_POINTLIST)
        return 0;
    if (topology == D3D_PRIMITIVE_TOPOLOGY_LINELIST || topology == D3D_PRIMITIVE_TOPOLOGY_LINESTRIP)
        return 1;
    return 2; // triangle
}

void OpenGLD3D::PrepareDrawState(D3D_PRIMITIVE_TOPOLOGY topology)
{
    uploadAndBindConstants();
    bindTexturesAndSamplers();

    UINT8 topoType = topologyTypeFromTopology(topology);
    PSOKey key = buildPSOKey(topoType);
    auto* pso = g_backend.GetOrCreatePSO(key);

    auto* cmdList = g_backend.GetCommandList();
    cmdList->SetPipelineState(pso);
    cmdList->IASetPrimitiveTopology(topology);

    D3D12_VIEWPORT viewport = {};
    viewport.TopLeftX = static_cast<float>(s_renderState.viewportX);
    viewport.TopLeftY = static_cast<float>(s_renderState.viewportY);
    viewport.Width = static_cast<float>(s_renderState.viewportW);
    viewport.Height = static_cast<float>(s_renderState.viewportH);
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;
    cmdList->RSSetViewports(1, &viewport);

    D3D12_RECT scissor = { 0, 0, static_cast<LONG>(s_renderState.viewportW), static_cast<LONG>(s_renderState.viewportH) };
    cmdList->RSSetScissorRects(1, &scissor);
}

static void issueDrawCall(D3D_PRIMITIVE_TOPOLOGY topology, UINT vertexCount, const CustomVertex* vertices)
{
    if (vertexCount == 0) return;

    auto* cmdList = g_backend.GetCommandList();

    // Upload vertices to ring buffer
    UINT vbSize = vertexCount * sizeof(CustomVertex);
    auto alloc = g_backend.GetUploadBuffer().Allocate(vbSize, sizeof(float));

    memcpy(alloc.cpuPtr, vertices, vbSize);

    D3D12_VERTEX_BUFFER_VIEW vbView = {};
    vbView.BufferLocation = alloc.gpuAddr;
    vbView.SizeInBytes = vbSize;
    vbView.StrideInBytes = sizeof(CustomVertex);

    PrepareDrawState(topology);
    cmdList->IASetVertexBuffers(0, 1, &vbView);

    cmdList->DrawInstanced(vertexCount, 1, 0, 0);
}

// ============================================================================
// Initialisation
// ============================================================================

static void InitialiseData()
{
    s_modelViewMatrixStack = MatrixStack();
    s_projectionMatrixStack = MatrixStack();
    s_matrixMode = GL_MODELVIEW;
    s_pTargetMatrixStack = &s_modelViewMatrixStack;

    s_clearColor = 0xFF000000;

    if (s_vertices == nullptr)
    {
        s_allocatedVertices = 1024;
        s_vertices = new CustomVertex[s_allocatedVertices];
    }
    s_currentVertexNumber = 0;
    s_currentVertex = s_vertices;

    s_textureResources.clear();
    s_activeTexture = 0;
    s_activeTextureState = &s_textureStates[s_activeTexture];
    for (int i = 0; i < MAX_ACTIVE_TEXTURES; i++)
        s_textureStates[i] = TextureState();

    s_ccwFrontFace = true;
    s_cullBackFace = true;

    s_fogHint = GL_DONT_CARE;
    s_polygonSmoothHint = GL_DONT_CARE;

    s_currentAttribs = CurrentAttributes();
    s_renderState = RenderState();

    for (auto& entry : s_displayLists)
        delete entry.second;
    s_displayLists.clear();
    s_lastDisplayId = 1;
    s_pDisplayListRecorder = nullptr;
}

bool Direct3DInit(HWND _hWnd, bool _windowed, int _width, int _height, int _colourDepth, int _zDepth, bool _waitVRT)
{
    if (!g_backend.Init(_hWnd, _windowed, _width, _height, _colourDepth, _zDepth, _waitVRT))
        return false;

    InitialiseData();
    s_renderState.viewportW = _width;
    s_renderState.viewportH = _height;

    return true;
}

void Direct3DShutdown()
{
    g_backend.Shutdown();
}

void Direct3DSwapBuffers()
{
    g_backend.EndFrame();

#ifdef FRAMES_PER_SECOND_COUNTER
    static DWORD s_startTime = GetTickCount();
    static unsigned s_numFrames = 0;
    s_numFrames++;
    if (GetTickCount() - s_startTime > 5000)
    {
        s_numFrames = 0;
        s_startTime = GetTickCount();
    }
#endif
}

// ============================================================================
// Clear / Color
// ============================================================================

void glClearColor(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha)
{
    GL_TRACE_IMP(" glClearColor(%10.4g, %10.4g, %10.4g, %10.4g)", (double) red, (double) green, (double) blue, (double) alpha)
    // Store as float array for ClearRenderTargetView
    s_clearColor = (static_cast<UINT32>(alpha * 255) << 24) |
                   (static_cast<UINT32>(red * 255) << 16) |
                   (static_cast<UINT32>(green * 255) << 8) |
                   static_cast<UINT32>(blue * 255);
}

void glClear(GLbitfield mask)
{
    GL_TRACE_IMP(" glClear(%s)", glBitfieldToString(mask))

    auto* cmdList = g_backend.GetCommandList();

    if (mask & GL_COLOR_BUFFER_BIT)
    {
        float clearColor[4] = {
            ((s_clearColor >> 16) & 0xFF) / 255.0f,
            ((s_clearColor >> 8) & 0xFF) / 255.0f,
            (s_clearColor & 0xFF) / 255.0f,
            ((s_clearColor >> 24) & 0xFF) / 255.0f
        };

        cmdList->ClearRenderTargetView(g_backend.GetCurrentRTVHandle(), clearColor, 0, nullptr);
    }

    if (mask & GL_DEPTH_BUFFER_BIT)
    {
        cmdList->ClearDepthStencilView(
            g_backend.GetCurrentDSVHandle(),
            D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
    }

    DEBUG_ASSERT((mask & ~(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT)) == 0);
}

// ============================================================================
// Matrix operations
// ============================================================================

void glMatrixMode(GLenum mode)
{
    GL_TRACE_IMP(" glMatrixMode(%s)", glEnumToString(mode))
    DEBUG_ASSERT(mode == GL_MODELVIEW || mode == GL_PROJECTION);

    s_matrixMode = mode;
    switch (mode)
    {
    case GL_MODELVIEW:  s_pTargetMatrixStack = &s_modelViewMatrixStack; break;
    case GL_PROJECTION: s_pTargetMatrixStack = &s_projectionMatrixStack; break;
    }
}

void glLoadIdentity()
{
    GL_TRACE_IMP(" glLoadIdentity()")
    s_pTargetMatrixStack->LoadIdentity();
}

void gluOrtho2D(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top)
{
    GL_TRACE_IMP(" gluOrtho2D(%10.4g, %10.4g, %10.4g, %10.4g)", left, right, bottom, top)

    XMMATRIX m = XMMatrixOrthographicOffCenterRH(
        static_cast<float>(left), static_cast<float>(right),
        static_cast<float>(bottom), static_cast<float>(top),
        -1.0f, 1.0f);

    XMFLOAT4X4 mat;
    XMStoreFloat4x4(&mat, m);
    s_pTargetMatrixStack->Load(mat);
}

void gluLookAt(GLdouble eyex, GLdouble eyey, GLdouble eyez,
               GLdouble centerx, GLdouble centery, GLdouble centerz,
               GLdouble upx, GLdouble upy, GLdouble upz)
{
    GL_TRACE_IMP(" gluLookAt(%10.4g, %10.4g, %10.4g, %10.4g, %10.4g, %10.4g, %10.4g, %10.4g, %10.4g)",
                 eyex, eyey, eyez, centerx, centery, centerz, upx, upy, upz)

    XMVECTOR eye = XMVectorSet(static_cast<float>(eyex), static_cast<float>(eyey), static_cast<float>(eyez), 0);
    XMVECTOR at = XMVectorSet(static_cast<float>(centerx), static_cast<float>(centery), static_cast<float>(centerz), 0);
    XMVECTOR up = XMVectorSet(static_cast<float>(upx), static_cast<float>(upy), static_cast<float>(upz), 0);

    XMMATRIX m = XMMatrixLookAtRH(eye, at, up);
    XMFLOAT4X4 mat;
    XMStoreFloat4x4(&mat, m);
    s_pTargetMatrixStack->Load(mat);
}

void gluPerspective(GLdouble fovy, GLdouble aspect, GLdouble zNear, GLdouble zFar)
{
    GL_TRACE_IMP(" gluPerspective(%10.4g, %10.4g, %10.4g, %10.4g)", fovy, aspect, zNear, zFar)

    XMMATRIX m = XMMatrixPerspectiveFovRH(
        XMConvertToRadians(static_cast<float>(fovy)),
        static_cast<float>(aspect),
        static_cast<float>(zNear),
        static_cast<float>(zFar));

    XMFLOAT4X4 mat;
    XMStoreFloat4x4(&mat, m);
    s_pTargetMatrixStack->Load(mat);
}

void glPushMatrix()
{
    GL_TRACE_IMP(" glPushMatrix()");
    s_pTargetMatrixStack->Push();
}

void glPopMatrix()
{
    GL_TRACE_IMP(" glPopMatrix()");
    s_pTargetMatrixStack->Pop();
}

void glMultMatrixf(const GLfloat* m)
{
    GL_TRACE_IMP(" glMultMatrixf((const float *)%p)", m)

    XMFLOAT4X4 mat;
    // OpenGL column-major → our row-major (same as D3D9 code which copied directly)
    memcpy(&mat, m, sizeof(XMFLOAT4X4));
    s_pTargetMatrixStack->Multiply(mat);
}

void glLoadMatrixd(const GLdouble* _m)
{
    GL_TRACE_IMP(" glLoadMatrixd((const double *)%p)", _m)

    XMFLOAT4X4 mat;
    for (int i = 0; i < 16; i++)
        reinterpret_cast<float*>(&mat)[i] = static_cast<float>(_m[i]);
    s_pTargetMatrixStack->Load(mat);
}

void glTranslatef(GLfloat x, GLfloat y, GLfloat z)
{
    GL_TRACE_IMP(" glTranslatef(%10.4g, %10.4g, %10.4g)", (double) x, (double) y, (double) z)

    XMFLOAT4X4 mat;
    XMStoreFloat4x4(&mat, XMMatrixTranslation(x, y, z));
    s_pTargetMatrixStack->Multiply(mat);
}

void glScalef(GLfloat x, GLfloat y, GLfloat z)
{
    GL_TRACE_IMP(" glScalef(%10.4g, %10.4g, %10.4g)", (double) x, (double) y, (double) z)

    XMFLOAT4X4 mat;
    XMStoreFloat4x4(&mat, XMMatrixScaling(x, y, z));
    s_pTargetMatrixStack->Multiply(mat);
}

void glRotatef(GLfloat angle, GLfloat x, GLfloat y, GLfloat z)
{
    GL_TRACE_IMP(" glRotatef(%10.4g, %10.4g, %10.4g, %10.4g)", (double) angle, (double) x, (double) y, (double) z)

    XMVECTOR axis = XMVectorSet(x, y, z, 0);
    XMFLOAT4X4 mat;
    XMStoreFloat4x4(&mat, XMMatrixRotationAxis(axis, XMConvertToRadians(angle)));
    s_pTargetMatrixStack->Multiply(mat);
}

void SwapToViewMatrix() { /* No-op in D3D12 — matrices are handled via constant buffer */ }
void SwapToModelMatrix() { /* No-op in D3D12 */ }

// ============================================================================
// Vertex assembly
// ============================================================================

static void reallocateVertexList()
{
    unsigned allocatedVertices = s_allocatedVertices * 2;
    auto buffer = new CustomVertex[allocatedVertices];
    memcpy(buffer, s_vertices, sizeof(CustomVertex) * s_currentVertexNumber);
    delete[] s_vertices;
    s_vertices = buffer;
    s_allocatedVertices = allocatedVertices;
    s_currentVertex = s_vertices + s_currentVertexNumber;
}

void glBegin(GLenum mode)
{
    GL_TRACE_IMP(" glBegin(%s)", glEnumToString(mode))
    s_currentVertex = s_vertices;
    s_currentVertexNumber = 0;
    s_quadVertexCount = 0;
    s_primitiveMode = mode;
}

void glEnd()
{
    GL_TRACE_IMP(" glEnd()")

    if (s_currentVertexNumber == 0) return;

    // If recording a display list, record instead of drawing
    if (s_pDisplayListRecorder)
    {
        D3D_PRIMITIVE_TOPOLOGY topology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
        switch (s_primitiveMode)
        {
        case GL_POINTS:         topology = D3D_PRIMITIVE_TOPOLOGY_POINTLIST; break;
        case GL_LINES:          topology = D3D_PRIMITIVE_TOPOLOGY_LINELIST; break;
        case GL_LINE_LOOP:      topology = D3D_PRIMITIVE_TOPOLOGY_LINESTRIP; break;
        case GL_TRIANGLES:
        case GL_QUADS:          topology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST; break;
        case GL_TRIANGLE_STRIP:
        case GL_QUAD_STRIP:     topology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP; break;
        case GL_TRIANGLE_FAN:   topology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST; break;
        }
        s_pDisplayListRecorder->RecordDraw(topology, s_vertices, s_currentVertexNumber);
        return;
    }

    switch (s_primitiveMode)
    {
    case GL_POINTS:
        issueDrawCall(D3D_PRIMITIVE_TOPOLOGY_POINTLIST, s_currentVertexNumber, s_vertices);
        break;

    case GL_LINES:
        issueDrawCall(D3D_PRIMITIVE_TOPOLOGY_LINELIST, s_currentVertexNumber, s_vertices);
        break;

    case GL_LINE_LOOP:
        if (s_currentVertexNumber + 2 >= s_allocatedVertices)
            reallocateVertexList();
        s_vertices[s_currentVertexNumber] = s_vertices[0];
        issueDrawCall(D3D_PRIMITIVE_TOPOLOGY_LINESTRIP, s_currentVertexNumber + 1, s_vertices);
        break;

    case GL_TRIANGLES:
    case GL_QUADS: // already decomposed into triangles by glVertex3f_impl
        issueDrawCall(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST, s_currentVertexNumber, s_vertices);
        break;

    case GL_TRIANGLE_STRIP:
    case GL_QUAD_STRIP:
        issueDrawCall(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP, s_currentVertexNumber, s_vertices);
        break;

    case GL_TRIANGLE_FAN:
        // D3D12 has no triangle fan — decompose to triangle list
        {
            unsigned numTris = s_currentVertexNumber - 2;
            if (numTris == 0) break;
            unsigned newVertCount = numTris * 3;
            // Ensure space
            while (newVertCount >= s_allocatedVertices)
                reallocateVertexList();

            // Build triangle list in a temporary buffer
            auto* temp = new CustomVertex[newVertCount];
            for (unsigned i = 0; i < numTris; i++)
            {
                temp[i * 3 + 0] = s_vertices[0];
                temp[i * 3 + 1] = s_vertices[i + 1];
                temp[i * 3 + 2] = s_vertices[i + 2];
            }
            issueDrawCall(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST, newVertCount, temp);
            delete[] temp;
        }
        break;

    default:
        GL_TRACE("-glEnd() not implemented for %s", glEnumToString(s_primitiveMode));
        break;
    }
}

void glVertex3f_impl(GLfloat x, GLfloat y, GLfloat z)
{
    if (s_currentVertexNumber + 2 >= s_allocatedVertices)
        reallocateVertexList();

    *s_currentVertex = s_currentAttribs;
    s_currentVertex->x = x;
    s_currentVertex->y = y;
    s_currentVertex->z = z;

    s_currentVertex++;
    s_currentVertexNumber++;

    if (s_primitiveMode == GL_QUADS)
    {
        if (s_quadVertexCount == 2)
        {
            *s_currentVertex = s_vertices[s_currentVertexNumber - 3];
            s_currentVertex++;
            s_currentVertexNumber++;

            *s_currentVertex = s_vertices[s_currentVertexNumber - 2];
            s_currentVertex++;
            s_currentVertexNumber++;
        }

        s_quadVertexCount = (s_quadVertexCount + 1) & 3;
    }
}

void glColor4ub(GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha)
{
    s_currentAttribs.r8 = red;
    s_currentAttribs.g8 = green;
    s_currentAttribs.b8 = blue;
    s_currentAttribs.a8 = alpha;
}

void glNormal3f_impl(GLfloat nx, GLfloat ny, GLfloat nz)
{
    // Invert normals for RH coordinate system (same as D3D9 version)
    s_currentAttribs.nx = -nx;
    s_currentAttribs.ny = -ny;
    s_currentAttribs.nz = -nz;
}

void glTexCoord2f_impl(GLfloat u, GLfloat v)
{
    s_currentAttribs.u = u;
    s_currentAttribs.v = v;
}

// ============================================================================
// Enable / Disable
// ============================================================================

void glDisable(GLenum cap)
{
    GL_TRACE_IMP(" glDisable(%s)", glEnumToString(cap))

    switch (cap)
    {
    case GL_ALPHA_TEST:      s_renderState.alphaTestEnabled = false; break;
    case GL_BLEND:           s_renderState.blendEnabled = false; break;
    case GL_COLOR_MATERIAL:  s_currentAttribs.colorMaterialEnabled = false; break;
    case GL_CULL_FACE:       s_currentAttribs.backCullingEnabled = false; break;
    case GL_DEPTH_TEST:      s_renderState.depthTestEnabled = false; break;
    case GL_FOG:             s_renderState.fogEnabled = false; break;
    case GL_LIGHTING:        s_renderState.lightingEnabled = false; break;
    case GL_LIGHT0: case GL_LIGHT1: case GL_LIGHT2: case GL_LIGHT3:
    case GL_LIGHT4: case GL_LIGHT5: case GL_LIGHT6: case GL_LIGHT7:
        s_renderState.lightsEnabled[cap - GL_LIGHT0] = false; break;
    case GL_LINE_SMOOTH:     s_renderState.lineSmoothEnabled = false; break;
    case GL_NORMALIZE:       s_renderState.normalizeEnabled = false; break;
    case GL_SCISSOR_TEST:    s_renderState.scissorTestEnabled = false; break;
    case GL_POINT_SMOOTH:    break;
    case GL_TEXTURE_2D:      s_currentAttribs.texturingEnabled[s_activeTexture] = false; break;
    case GL_CLIP_PLANE0: case GL_CLIP_PLANE1: case GL_CLIP_PLANE2: break; // TODO
    default: GL_TRACE("-glDisable(%s) not implemented", glEnumToString(cap))
    }
}

void glEnable(GLenum cap)
{
    GL_TRACE_IMP(" glEnable(%s)", glEnumToString(cap))

    switch (cap)
    {
    case GL_ALPHA_TEST:      s_renderState.alphaTestEnabled = true; break;
    case GL_BLEND:           s_renderState.blendEnabled = true; break;
    case GL_COLOR_MATERIAL:  s_currentAttribs.colorMaterialEnabled = true; break;
    case GL_CULL_FACE:       s_currentAttribs.backCullingEnabled = true; break;
    case GL_DEPTH_TEST:      s_renderState.depthTestEnabled = true; break;
    case GL_FOG:             s_renderState.fogEnabled = true; break;
    case GL_LIGHTING:        s_renderState.lightingEnabled = true; break;
    case GL_LIGHT0: case GL_LIGHT1: case GL_LIGHT2: case GL_LIGHT3:
    case GL_LIGHT4: case GL_LIGHT5: case GL_LIGHT6: case GL_LIGHT7:
        s_renderState.lightsEnabled[cap - GL_LIGHT0] = true; break;
    case GL_LINE_SMOOTH:     s_renderState.lineSmoothEnabled = true; break;
    case GL_NORMALIZE:       s_renderState.normalizeEnabled = true; break;
    case GL_SCISSOR_TEST:    s_renderState.scissorTestEnabled = true; break;
    case GL_TEXTURE_2D:      s_currentAttribs.texturingEnabled[s_activeTexture] = true; break;
    case GL_CLIP_PLANE0: case GL_CLIP_PLANE1: case GL_CLIP_PLANE2: break; // TODO
    default: GL_TRACE("-glEnable(%s) not implemented", glEnumToString(cap))
    }
}

GLboolean glIsEnabled(GLenum cap)
{
    GL_TRACE_IMP(" glIsEnabled(%s)", glEnumToString(cap));
    switch (cap)
    {
    case GL_ALPHA_TEST:      return s_renderState.alphaTestEnabled;
    case GL_BLEND:           return s_renderState.blendEnabled;
    case GL_COLOR_MATERIAL:  return s_currentAttribs.colorMaterialEnabled;
    case GL_CULL_FACE:       return s_currentAttribs.backCullingEnabled;
    case GL_DEPTH_TEST:      return s_renderState.depthTestEnabled;
    case GL_FOG:             return s_renderState.fogEnabled;
    case GL_LIGHTING:        return s_renderState.lightingEnabled;
    case GL_LIGHT0: case GL_LIGHT1: case GL_LIGHT2: case GL_LIGHT3:
    case GL_LIGHT4: case GL_LIGHT5: case GL_LIGHT6: case GL_LIGHT7:
        return s_renderState.lightsEnabled[cap - GL_LIGHT0];
    case GL_LINE_SMOOTH:     return s_renderState.lineSmoothEnabled;
    case GL_NORMALIZE:       return s_renderState.normalizeEnabled;
    case GL_POINT_SMOOTH:    return FALSE;
    case GL_SCISSOR_TEST:    return s_renderState.scissorTestEnabled;
    case GL_TEXTURE_2D:      return s_currentAttribs.texturingEnabled[s_activeTexture];
    default: DEBUG_ASSERT(FALSE); return 0;
    }
}

// ============================================================================
// Blend, Depth, Alpha, Shade, Fill, Front Face
// ============================================================================

void glBlendFunc(GLenum sfactor, GLenum dfactor)
{
    GL_TRACE_IMP(" glBlendFunc(%s, %s)", glEnumToString(sfactor), glEnumToString(dfactor))
    s_renderState.blendSrc = sfactor;
    s_renderState.blendDst = dfactor;
}

void glDepthMask(GLboolean flag)
{
    GL_TRACE_IMP(" glDepthMask(%s)", glBooleanToString(flag))
    s_renderState.depthWriteEnabled = flag != 0;
}

void glDepthFunc(GLenum func)
{
    GL_TRACE_IMP(" glDepthFunc(%s)", glEnumToString(func))
    s_renderState.depthFunc = func;
}

void glAlphaFunc(GLenum func, GLclampf ref)
{
    GL_TRACE_IMP(" glAlphaFunc(%s, %10.4g)", glEnumToString(func), (double) ref)
    s_renderState.alphaFunc = func;
    float cref = (ref < 0) ? 0 : ((ref > 1) ? 1 : ref);
    s_renderState.alphaRef = cref;
    s_currentAttribs.alphaRef = cref;
}

void glShadeModel(GLenum mode)
{
    GL_TRACE_IMP(" glShadeModel(%s)", glEnumToString(mode))
    s_renderState.shadeModel = mode;
}

void glPolygonMode(GLenum face, GLenum mode)
{
    GL_TRACE_IMP(" glPolygonMode(%s, %s)", glEnumToString(face), glEnumToString(mode));
    s_renderState.fillMode = mode;
}

void glFrontFace(GLenum mode)
{
    GL_TRACE_IMP(" glFrontFace(%s)", glEnumToString(mode));
    s_ccwFrontFace = (mode == GL_CCW);
}

void glColorMaterial(GLenum face, GLenum mode)
{
    GL_TRACE_IMP(" glColorMaterial(%s, %s)", glEnumToString(face), glEnumToString(mode))
    s_currentAttribs.colorMaterialMode = mode;
}

// ============================================================================
// Lighting
// ============================================================================

static void transformByModelView(float* v, float w)
{
    XMVECTOR in = XMVectorSet(v[0], v[1], v[2], w);
    XMMATRIX mv = s_modelViewMatrixStack.GetTopXM();
    XMVECTOR out = XMVector4Transform(in, mv);
    XMFLOAT4 result;
    XMStoreFloat4(&result, out);
    v[0] = result.x;
    v[1] = result.y;
    v[2] = result.z;
}

void glLightfv(GLenum light, GLenum pname, const GLfloat* params)
{
    GL_TRACE_IMP(" glLightfv(%s, %s, (const float *)%p)", glEnumToString(light), glEnumToString(pname), params)

    DWORD idx = light - GL_LIGHT0;
    auto& l = s_renderState.lights[idx];

    switch (pname)
    {
    case GL_POSITION:
        l.position[0] = params[0];
        l.position[1] = params[1];
        l.position[2] = params[2];
        l.position[3] = params[3];
        l.isDirectional = (params[3] == 0.0f);
        transformByModelView(l.position, params[3]);
        break;
    case GL_DIFFUSE:  memcpy(l.diffuse, params, 4 * sizeof(float)); break;
    case GL_SPECULAR: memcpy(l.specular, params, 4 * sizeof(float)); break;
    case GL_AMBIENT:  memcpy(l.ambient, params, 4 * sizeof(float)); break;
    default: break;
    }
}

void glGetLightfv(GLenum light, GLenum pname, GLfloat* params)
{
    GL_TRACE_IMP(" glGetLightfv(%s, %s, (float *)%p)", glEnumToString(light), glEnumToString(pname), params);
    int idx = light - GL_LIGHT0;
    auto& l = s_renderState.lights[idx];
    switch (pname)
    {
    case GL_POSITION: memcpy(params, l.position, 4 * sizeof(float)); break;
    case GL_DIFFUSE:  memcpy(params, l.diffuse, 4 * sizeof(float)); break;
    case GL_SPECULAR: memcpy(params, l.specular, 4 * sizeof(float)); break;
    case GL_AMBIENT:  memcpy(params, l.ambient, 4 * sizeof(float)); break;
    default: DEBUG_ASSERT(false); break;
    }
}

void glLightModelfv(GLenum pname, const GLfloat* params)
{
    GL_TRACE_IMP(" glLightModelfv(%s, (const float *)%p)", glEnumToString(pname), params);
    switch (pname)
    {
    case GL_LIGHT_MODEL_AMBIENT:
        memcpy(s_renderState.globalAmbient, params, 4 * sizeof(float));
        break;
    default: DEBUG_ASSERT(FALSE);
    }
}

// ============================================================================
// Materials
// ============================================================================

void glMaterialfv(GLenum face, GLenum pname, const GLfloat* params)
{
    GL_TRACE_IMP(" glMaterialfv(%s, %s, (const float *)%p)", glEnumToString(face), glEnumToString(pname), params);
    switch (pname)
    {
    case GL_SPECULAR:           memcpy(s_renderState.matSpecular, params, 4 * sizeof(float)); break;
    case GL_DIFFUSE:            memcpy(s_renderState.matDiffuse, params, 4 * sizeof(float)); break;
    case GL_AMBIENT:            memcpy(s_renderState.matAmbient, params, 4 * sizeof(float)); break;
    case GL_AMBIENT_AND_DIFFUSE:
        memcpy(s_renderState.matAmbient, params, 4 * sizeof(float));
        memcpy(s_renderState.matDiffuse, params, 4 * sizeof(float));
        break;
    case GL_SHININESS:          s_renderState.matShininess = params[0]; break;
    default: DEBUG_ASSERT(FALSE); break;
    }
}

// ============================================================================
// Fog
// ============================================================================

void glFogf_impl(GLenum pname, GLfloat param)
{
    switch (pname)
    {
    case GL_FOG_DENSITY: s_renderState.fogDensity = param; break;
    case GL_FOG_END:     s_renderState.fogEnd = param; break;
    case GL_FOG_START:   s_renderState.fogStart = param; break;
    case GL_FOG_MODE:    break; // handled by glFogi
    default: DEBUG_ASSERT(FALSE); break;
    }
}

void glFogi_impl(GLenum pname, GLint param)
{
    switch (pname)
    {
    case GL_FOG_DENSITY: case GL_FOG_END: case GL_FOG_START:
        if (param > 0) glFogf_impl(pname, static_cast<float>(param) / INT_MAX);
        else glFogf_impl(pname, static_cast<float>(param) / INT_MIN);
        break;
    case GL_FOG_MODE:
        s_renderState.fogMode = static_cast<GLenum>(param);
        break;
    }
}

void glFogfv(GLenum pname, const GLfloat* params)
{
    GL_TRACE_IMP(" glFogfv(%s, (const float *)%p)", glEnumToString(pname), params)
    switch (pname)
    {
    case GL_FOG_COLOR:
        memcpy(s_renderState.fogColor, params, 4 * sizeof(float));
        break;
    default: DEBUG_ASSERT(FALSE); break;
    }
}

// ============================================================================
// Textures
// ============================================================================

void glGenTextures(GLsizei n, GLuint* textures)
{
    GL_TRACE_IMP(" glGenTextures(%d, (float *)%p)", n, textures)
    for (int i = 0; i < n; i++)
    {
        textures[i] = static_cast<GLuint>(s_textureResources.size());
        s_textureResources.push_back(TextureResource());
    }
}

void glBindTexture(GLenum target, GLuint texture)
{
    GL_TRACE_IMP(" glBindTexture(%s, %u)", glEnumToString(target), texture)
    if (target == GL_TEXTURE_2D)
        s_activeTextureState->target = texture;
}

int gluBuild2DMipmaps(GLenum target, GLint components, GLint width, GLint height, GLenum format, GLenum type, const void* data)
{
    GL_TRACE_IMP(" gluBuild2DMipmaps(%s, %d, %d, %d, %s, %s, %p)", glEnumToString(target), components, width, height, glEnumToString(format), glEnumToString(type), data)

    DEBUG_ASSERT(target == GL_TEXTURE_2D);
    DEBUG_ASSERT(format == GL_RGBA);
    DEBUG_ASSERT(type == GL_UNSIGNED_BYTE);

    GLuint texIdx = s_activeTextureState->target;
    if (texIdx >= s_textureResources.size()) return -1;

    auto& tex = s_textureResources[texIdx];
    auto* device = g_backend.GetDevice();
    auto* cmdList = g_backend.GetCommandList();

    // Release old resource
    tex.resource.Reset();

    // Create texture resource
    D3D12_HEAP_PROPERTIES heapProps = {};
    heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;

    D3D12_RESOURCE_DESC texDesc = {};
    texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    texDesc.Width = width;
    texDesc.Height = height;
    texDesc.DepthOrArraySize = 1;
    texDesc.MipLevels = 1; // No auto mip gen for now
    texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    texDesc.SampleDesc.Count = 1;

    HRESULT hr = device->CreateCommittedResource(
        &heapProps, D3D12_HEAP_FLAG_NONE, &texDesc,
        D3D12_RESOURCE_STATE_COPY_DEST, nullptr,
        IID_PPV_ARGS(&tex.resource));
    if (FAILED(hr)) return -1;

    // Upload via ring buffer
    UINT64 uploadSize = 0;
    D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint;
    device->GetCopyableFootprints(&texDesc, 0, 1, 0, &footprint, nullptr, nullptr, &uploadSize);

    auto alloc = g_backend.GetUploadBuffer().Allocate(static_cast<SIZE_T>(uploadSize), D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT);

    // Copy row by row (respecting row pitch alignment)
    const UINT8* srcData = static_cast<const UINT8*>(data);
    UINT8* destData = static_cast<UINT8*>(alloc.cpuPtr);
    UINT srcRowPitch = width * 4;
    UINT dstRowPitch = footprint.Footprint.RowPitch;
    for (int row = 0; row < height; row++)
    {
        memcpy(destData + row * dstRowPitch, srcData + row * srcRowPitch, srcRowPitch);
    }

    D3D12_TEXTURE_COPY_LOCATION srcLoc = {};
    srcLoc.pResource = g_backend.GetUploadBuffer().GetResource();
    srcLoc.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
    srcLoc.PlacedFootprint = footprint;
    srcLoc.PlacedFootprint.Offset = alloc.offset;

    D3D12_TEXTURE_COPY_LOCATION dstLoc = {};
    dstLoc.pResource = tex.resource.Get();
    dstLoc.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    dstLoc.SubresourceIndex = 0;

    cmdList->CopyTextureRegion(&dstLoc, 0, 0, 0, &srcLoc, nullptr);

    // Transition to shader resource
    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource = tex.resource.Get();
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    cmdList->ResourceBarrier(1, &barrier);

    // Create SRV
    if (!tex.valid)
    {
        D3D12_CPU_DESCRIPTOR_HANDLE srvHandle = g_backend.AllocateSRVSlot(tex.srvIndex);
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.Texture2D.MipLevels = 1;
        device->CreateShaderResourceView(tex.resource.Get(), &srvDesc, srvHandle);
    }
    else
    {
        // Update existing SRV
        D3D12_CPU_DESCRIPTOR_HANDLE srvHandle = g_backend.GetSRVCBVHeap()->GetCPUDescriptorHandleForHeapStart();
        srvHandle.ptr += tex.srvIndex * g_backend.GetSRVDescriptorSize();
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.Texture2D.MipLevels = 1;
        device->CreateShaderResourceView(tex.resource.Get(), &srvDesc, srvHandle);
    }

    tex.width = width;
    tex.height = height;
    tex.valid = true;

    return 0;
}

void glTexImage2D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid* pixels)
{
    GL_TRACE_IMP(" glTexImage2D(%s, %d, %d, %d, %d, %d, %s, %s, %p)", glEnumToString(target), level, internalformat, width, height, border, glEnumToString(format), glEnumToString(type), pixels)
    gluBuild2DMipmaps(target, 4, width, height, format, type, pixels);
}

void glTexParameteri(GLenum target, GLenum pname, GLint param)
{
    GL_TRACE_IMP(" glTexParameteri(%s, %s, %s)", glEnumToString(target), glEnumToString(pname), glEnumToString(param))
    auto& s = s_renderState.samplers[s_activeTexture];
    switch (pname)
    {
    case GL_TEXTURE_WRAP_S:     s.wrapS = param; break;
    case GL_TEXTURE_WRAP_T:     s.wrapT = param; break;
    case GL_TEXTURE_MAG_FILTER: s.magFilter = param; break;
    case GL_TEXTURE_MIN_FILTER: s.minFilter = param; break;
    }
}

void glTexParameterf(GLenum target, GLenum pname, GLfloat param)
{
    GL_TRACE_IMP(" glTexParameterf(%s, %s, %s)", glEnumToString(target), glEnumToString(pname), glEnumToString((GLenum)param));
    glTexParameteri(target, pname, static_cast<GLint>(param));
}

void glCopyTexImage2D(GLenum target, GLint level, GLenum internalFormat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border)
{
    GL_TRACE_IMP(" glCopyTexImage2D(%s, %d, %s, %d, %d, %d, %d, %d)", glEnumToString(target), level, glEnumToString(internalFormat), x, y, width, height, border);
    // TODO: implement back buffer copy for D3D12
}

// ============================================================================
// Texture env
// ============================================================================

void glTexEnvf(GLenum target, GLenum pname, GLfloat param)
{
    GL_TRACE_IMP(" glTexEnvf(%s, %s, %s)", glEnumToString(target), glEnumToString(pname), glEnumToString(param));
    if (target == GL_TEXTURE_ENV)
    {
        if (pname == GL_TEXTURE_ENV_MODE) { s_activeTextureState->envMode = static_cast<GLenum>(param); return; }
        if (pname == GL_COMBINE_RGB_EXT) { s_activeTextureState->rgbCombineMode = static_cast<GLenum>(param); return; }
    }
}

void glTexEnvi(GLenum target, GLenum pname, GLint param)
{
    GL_TRACE_IMP(" glTexEnvi(%s, %s, %s)", glEnumToString(target), glEnumToString(pname), glEnumToString(param));
    if (target == GL_TEXTURE_ENV && pname == GL_TEXTURE_ENV_MODE)
        s_activeTextureState->envMode = param;
}

void glTexEnviv(GLenum target, GLenum pname, const GLint* params)
{
    GL_TRACE_IMP(" glTexEnviv(%s, %s, (const int *)%p)", glEnumToString(target), glEnumToString(pname), params);
    // Minimal implementation
}

void glGetTexEnviv(GLenum target, GLenum pname, GLint* params)
{
    GL_TRACE_IMP(" glGetTexEnviv(%s, %s, (int *)%p)", glEnumToString(target), glEnumToString(pname), params);
    if (target == GL_TEXTURE_ENV && pname == GL_TEXTURE_ENV_MODE)
        params[0] = s_activeTextureState->envMode;
}

void glGetTexParameteriv(GLenum target, GLenum pname, GLint* params)
{
    GL_TRACE_IMP(" glGetTexParameteriv(%s, %s, (int *)%p)", glEnumToString(target), glEnumToString(pname), params);
    auto& s = s_renderState.samplers[s_activeTexture];
    switch (pname)
    {
    case GL_TEXTURE_MAG_FILTER: params[0] = s.magFilter; break;
    case GL_TEXTURE_MIN_FILTER: params[0] = s.minFilter; break;
    case GL_TEXTURE_WRAP_S:     params[0] = s.wrapS; break;
    case GL_TEXTURE_WRAP_T:     params[0] = s.wrapT; break;
    default: DEBUG_ASSERT(FALSE);
    }
}

// ============================================================================
// Multi-texture
// ============================================================================

void __stdcall OpenGLD3D::glActiveTextureD3D(int _target)
{
    GL_TRACE_IMP(" glActiveTextureD3D(%d)", _target);
    _target -= GL_TEXTURE0_ARB;
    DEBUG_ASSERT(_target >= 0 && _target < MAX_ACTIVE_TEXTURES);
    s_activeTexture = _target;
    s_activeTextureState = &s_textureStates[s_activeTexture];
}

// ============================================================================
// Vertex Arrays
// ============================================================================

void glEnableClientState(GLenum array)
{
    GL_TRACE_IMP(" glEnableClientState(%s)", glEnumToString(array))
    switch (array)
    {
    case GL_VERTEX_ARRAY:          s_clientStateVertex = true; break;
    case GL_NORMAL_ARRAY:          s_clientStateNormal = true; break;
    case GL_COLOR_ARRAY:           s_clientStateColor = true; break;
    case GL_TEXTURE_COORD_ARRAY:   s_clientStateTexCoord = true; break;
    }
}

void glDisableClientState(GLenum array)
{
    GL_TRACE_IMP(" glDisableClientState(%s)", glEnumToString(array))
    switch (array)
    {
    case GL_VERTEX_ARRAY:          s_clientStateVertex = false; break;
    case GL_NORMAL_ARRAY:          s_clientStateNormal = false; break;
    case GL_COLOR_ARRAY:           s_clientStateColor = false; break;
    case GL_TEXTURE_COORD_ARRAY:   s_clientStateTexCoord = false; break;
    }
}

void glVertexPointer(GLint size, GLenum type, GLsizei stride, const GLvoid* pointer)
{
    GL_TRACE_IMP(" glVertexPointer(%d, %s, %d, %p)", size, glEnumToString(type), stride, pointer)
    s_vertexArray.pointer = pointer;
    s_vertexArray.size = size;
    s_vertexArray.type = type;
    s_vertexArray.stride = stride;
}

void glNormalPointer(GLenum type, GLsizei stride, const GLvoid* pointer)
{
    GL_TRACE_IMP(" glNormalPointer(%s, %d, %p)", glEnumToString(type), stride, pointer)
    s_normalArray.pointer = pointer;
    s_normalArray.size = 3;
    s_normalArray.type = type;
    s_normalArray.stride = stride;
}

void glColorPointer(GLint size, GLenum type, GLsizei stride, const GLvoid* pointer)
{
    GL_TRACE_IMP(" glColorPointer(%d, %s, %d, %p)", size, glEnumToString(type), stride, pointer)
    s_colorArray.pointer = pointer;
    s_colorArray.size = size;
    s_colorArray.type = type;
    s_colorArray.stride = stride;
}

void glTexCoordPointer(GLint size, GLenum type, GLsizei stride, const GLvoid* pointer)
{
    GL_TRACE_IMP(" glTexCoordPointer(%d, %s, %d, %p)", size, glEnumToString(type), stride, pointer)
    s_texCoordArray.pointer = pointer;
    s_texCoordArray.size = size;
    s_texCoordArray.type = type;
    s_texCoordArray.stride = stride;
}

void glDrawArrays(GLenum mode, GLint first, GLsizei count)
{
    GL_TRACE_IMP(" glDrawArrays(%s, %d, %d)", glEnumToString(mode), first, count)

    if (count <= 0 || !s_clientStateVertex) return;

    // Allocate temporary CustomVertex array
    auto* verts = new CustomVertex[count];
    memset(verts, 0, sizeof(CustomVertex) * count);

    GLsizei vstride = s_vertexArray.stride;
    GLsizei nstride = s_normalArray.stride;
    GLsizei cstride = s_colorArray.stride;
    GLsizei tstride = s_texCoordArray.stride;

    for (GLsizei i = 0; i < count; ++i)
    {
        GLsizei idx = first + i;
        CustomVertex& v = verts[i];

        // Position (required)
        {
            auto src = reinterpret_cast<const char*>(s_vertexArray.pointer) + idx * vstride;
            auto f = reinterpret_cast<const float*>(src);
            v.x = f[0];
            v.y = f[1];
            v.z = (s_vertexArray.size >= 3) ? f[2] : 0.0f;
        }

        // Normal
        if (s_clientStateNormal)
        {
            auto src = reinterpret_cast<const char*>(s_normalArray.pointer) + idx * nstride;
            auto f = reinterpret_cast<const float*>(src);
            v.nx = f[0];
            v.ny = f[1];
            v.nz = f[2];
        }

        // Color (RGBA source → BGRA destination)
        if (s_clientStateColor)
        {
            auto src = reinterpret_cast<const char*>(s_colorArray.pointer) + idx * cstride;
            if (s_colorArray.type == GL_UNSIGNED_BYTE && s_colorArray.size == 4)
            {
                v.r8 = src[0];
                v.g8 = src[1];
                v.b8 = src[2];
                v.a8 = src[3];
            }
            else if (s_colorArray.type == GL_UNSIGNED_BYTE && s_colorArray.size == 3)
            {
                v.r8 = src[0];
                v.g8 = src[1];
                v.b8 = src[2];
                v.a8 = 255;
            }
        }
        else
        {
            // Use current color
            v.r8 = s_currentAttribs.r8;
            v.g8 = s_currentAttribs.g8;
            v.b8 = s_currentAttribs.b8;
            v.a8 = s_currentAttribs.a8;
        }

        // Texture coordinates
        if (s_clientStateTexCoord)
        {
            auto src = reinterpret_cast<const char*>(s_texCoordArray.pointer) + idx * tstride;
            auto f = reinterpret_cast<const float*>(src);
            v.u = f[0];
            v.v = (s_texCoordArray.size >= 2) ? f[1] : 0.0f;
        }
    }

    D3D_PRIMITIVE_TOPOLOGY topology;
    switch (mode)
    {
    case GL_POINTS:         topology = D3D_PRIMITIVE_TOPOLOGY_POINTLIST; break;
    case GL_LINES:          topology = D3D_PRIMITIVE_TOPOLOGY_LINELIST; break;
    case GL_LINE_LOOP:      topology = D3D_PRIMITIVE_TOPOLOGY_LINESTRIP; break;
    case GL_TRIANGLES:      topology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST; break;
    case GL_TRIANGLE_STRIP: topology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP; break;
    case GL_TRIANGLE_FAN:   topology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST; break;
    default:                topology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST; break;
    }

    issueDrawCall(topology, count, verts);
    delete[] verts;
}

void __stdcall OpenGLD3D::glMultiTexCoord2fD3D(int _target, float _x, float _y)
{
    _target -= GL_TEXTURE0_ARB;
    if (_target == 0) { s_currentAttribs.u = _x; s_currentAttribs.v = _y; }
    else { s_currentAttribs.u2 = _x; s_currentAttribs.v2 = _y; }
}

// ============================================================================
// Display lists
// ============================================================================

GLuint glGenLists(GLsizei range)
{
    GL_TRACE_IMP(" glGenLists(%d)", range);
    int result = s_lastDisplayId;
    for (unsigned i = 0; i < static_cast<unsigned>(range); i++)
        s_displayLists[s_lastDisplayId++] = nullptr;
    return result;
}

void glDeleteLists(GLuint list, GLsizei range)
{
    GL_TRACE_IMP(" glDeleteLists(%u, %d)", list, range);
    for (unsigned i = 0; i < static_cast<unsigned>(range); i++)
    {
        delete s_displayLists[list + i];
        s_displayLists.erase(list + i);
    }
}

void glNewList(GLuint list, GLenum mode)
{
    GL_TRACE_IMP(" glNewList(%u, %s)", list, glEnumToString(mode));
    s_pDisplayListRecorder = new DisplayListRecorder(list);
}

void glEndList()
{
    GL_TRACE_IMP(" glEndList()")
    DEBUG_ASSERT(s_pDisplayListRecorder != nullptr);
    if (!s_pDisplayListRecorder) return;

    s_displayLists[s_pDisplayListRecorder->GetId()] = s_pDisplayListRecorder->Compile();
    delete s_pDisplayListRecorder;
    s_pDisplayListRecorder = nullptr;
}

void glCallList(GLuint list)
{
    GL_TRACE_IMP(" glCallList(%u)", list);
    DisplayList* dl = s_displayLists[list];
    if (dl) dl->Draw();
}

// ============================================================================
// Viewport
// ============================================================================

void glViewport(GLint x, GLint y, GLsizei width, GLsizei height)
{
    GL_TRACE_IMP(" glViewport(%d, %d, %d, %d)", x, y, width, height)
    s_renderState.viewportX = x;
    s_renderState.viewportY = y;
    s_renderState.viewportW = width;
    s_renderState.viewportH = height;
}

// ============================================================================
// State queries
// ============================================================================

void glGetIntegerv(GLenum pname, GLint* params)
{
    GL_TRACE_IMP(" glGetIntegerv(%s, (int *)%p)", glEnumToString(pname), params);
    switch (pname)
    {
    case GL_ALPHA_TEST_FUNC:         params[0] = s_renderState.alphaFunc; break;
    case GL_BLEND_DST:               params[0] = s_renderState.blendDst; break;
    case GL_BLEND_SRC:               params[0] = s_renderState.blendSrc; break;
    case GL_DEPTH_FUNC:              params[0] = s_renderState.depthFunc; break;
    case GL_DEPTH_WRITEMASK:         params[0] = s_renderState.depthWriteEnabled ? 1 : 0; break;
    case GL_COLOR_MATERIAL_FACE:     params[0] = GL_FRONT_AND_BACK; break;
    case GL_COLOR_MATERIAL_PARAMETER: params[0] = s_currentAttribs.colorMaterialMode; break;
    case GL_FOG_HINT:                params[0] = s_fogHint; break;
    case GL_FOG_MODE:                params[0] = s_renderState.fogMode; break;
    case GL_FRONT_FACE:              params[0] = s_ccwFrontFace ? GL_CCW : GL_CW; break;
    case GL_MATRIX_MODE:             params[0] = s_matrixMode; break;
    case GL_POLYGON_MODE:            params[0] = params[1] = s_renderState.fillMode; break;
    case GL_POLYGON_SMOOTH_HINT:     params[0] = s_polygonSmoothHint; break;
    case GL_SHADE_MODEL:             params[0] = s_renderState.shadeModel; break;
    case GL_VIEWPORT:
        params[0] = s_renderState.viewportX;
        params[1] = s_renderState.viewportY;
        params[2] = s_renderState.viewportW;
        params[3] = s_renderState.viewportH;
        break;
    default: GL_TRACE("-glGetIntegerv(%s) not implemented", glEnumToString(pname));
    }
}

void glGetFloatv(GLenum pname, GLfloat* params)
{
    GL_TRACE_IMP(" glGetFloatv(%s, (float *)%p)", glEnumToString(pname), params);
    switch (pname)
    {
    case GL_ALPHA_TEST_REF:
        params[0] = s_currentAttribs.alphaRef;
        break;
    case GL_CURRENT_COLOR:
        params[0] = s_currentAttribs.r8 / 255.0f;
        params[1] = s_currentAttribs.g8 / 255.0f;
        params[2] = s_currentAttribs.b8 / 255.0f;
        params[3] = s_currentAttribs.a8 / 255.0f;
        break;
    case GL_FOG_COLOR:    memcpy(params, s_renderState.fogColor, 4 * sizeof(float)); break;
    case GL_FOG_DENSITY:  params[0] = s_renderState.fogDensity; break;
    case GL_FOG_END:      params[0] = s_renderState.fogEnd; break;
    case GL_LIGHT_MODEL_AMBIENT: memcpy(params, s_renderState.globalAmbient, 4 * sizeof(float)); break;
    default: GL_TRACE("-glGetFloatv(%s) not implemented", glEnumToString(pname)); break;
    }
}

void glGetDoublev(GLenum pname, GLdouble* params)
{
    GL_TRACE_IMP(" glGetDoublev(%s, (double *)%p)", glEnumToString(pname), params)
    switch (pname)
    {
    case GL_MODELVIEW_MATRIX:
        {
            const auto& m = s_modelViewMatrixStack.GetTop();
            for (int i = 0; i < 16; i++)
                params[i] = reinterpret_cast<const float*>(&m)[i];
        }
        break;
    case GL_PROJECTION_MATRIX:
        {
            const auto& m = s_projectionMatrixStack.GetTop();
            for (int i = 0; i < 16; i++)
                params[i] = reinterpret_cast<const float*>(&m)[i];
        }
        break;
    default: break;
    }
}

// ============================================================================
// gluProject / gluUnProject
// ============================================================================

int gluProject(GLdouble objx, GLdouble objy, GLdouble objz, const GLdouble modelMatrix[16], const GLdouble projMatrix[16], const GLint viewport[4], GLdouble* winx, GLdouble* winy, GLdouble* winz)
{
    XMFLOAT4X4 model4, proj4;
    for (int i = 0; i < 16; i++) { reinterpret_cast<float*>(&model4)[i] = static_cast<float>(modelMatrix[i]); reinterpret_cast<float*>(&proj4)[i] = static_cast<float>(projMatrix[i]); }

    XMVECTOR obj = XMVectorSet(static_cast<float>(objx), static_cast<float>(objy), static_cast<float>(objz), 1.0f);
    XMMATRIX model = XMLoadFloat4x4(&model4);
    XMMATRIX proj = XMLoadFloat4x4(&proj4);
    XMVECTOR win0 = XMVector4Transform(obj, model);
    XMVECTOR win = XMVector4Transform(win0, proj);

    XMFLOAT4 w; XMStoreFloat4(&w, win);
    if (w.w == 0.0f) return false;
    w.x /= w.w; w.y /= w.w; w.z /= w.w;
    w.x = w.x * 0.5f + 0.5f;
    w.y = w.y * 0.5f + 0.5f;
    w.z = w.z * 0.5f + 0.5f;
    *winx = w.x * viewport[2] + viewport[0];
    *winy = w.y * viewport[3] + viewport[1];
    *winz = w.z;
    return true;
}

int gluUnProject(GLdouble winx, GLdouble winy, GLdouble winz, const GLdouble modelMatrix[16], const GLdouble projMatrix[16], const GLint viewport[4], GLdouble* objx, GLdouble* objy, GLdouble* objz)
{
    XMFLOAT4X4 model4, proj4;
    for (int i = 0; i < 16; i++) { reinterpret_cast<float*>(&model4)[i] = static_cast<float>(modelMatrix[i]); reinterpret_cast<float*>(&proj4)[i] = static_cast<float>(projMatrix[i]); }

    XMMATRIX model = XMLoadFloat4x4(&model4);
    XMMATRIX proj = XMLoadFloat4x4(&proj4);
    XMMATRIX modelProj = XMMatrixMultiply(model, proj);
    XMVECTOR det;
    XMMATRIX inv = XMMatrixInverse(&det, modelProj);

    XMVECTOR win = XMVectorSet(
        static_cast<float>((winx - viewport[0]) / viewport[2] * 2 - 1),
        static_cast<float>((winy - viewport[1]) / viewport[3] * 2 - 1),
        static_cast<float>(winz * 2 - 1),
        1.0f);

    XMVECTOR source = XMVector4Transform(win, inv);
    XMFLOAT4 s; XMStoreFloat4(&s, source);
    if (s.w == 0.0f) return false;
    *objx = s.x / s.w; *objy = s.y / s.w; *objz = s.z / s.w;
    return true;
}

// ============================================================================
// Misc
// ============================================================================

void glPointSize(GLfloat size)
{
    GL_TRACE_IMP(" glPointSize(%10.4g)", (double) size);
    s_renderState.pointSize = size;
}

void glLineWidth(GLfloat width)
{
    GL_TRACE_IMP(" glLineWidth(%10.4g)", (double) width);
}

void glHint(GLenum target, GLenum mode)
{
    GL_TRACE_IMP(" glHint(%s, %s)", glEnumToString(target), glEnumToString(mode));
    switch (target)
    {
    case GL_FOG_HINT:            s_fogHint = mode; break;
    case GL_POLYGON_SMOOTH_HINT: s_polygonSmoothHint = mode; break;
    }
}

GLenum glGetError() { GL_TRACE_IMP(" glGetError()") return 0; }
const GLubyte* gluErrorString(GLenum errCode) { return (const GLubyte*)"[Error Reporting Not Yet Implemented]"; }

void glClipPlane(GLenum plane, const GLdouble* equation)
{
    GL_TRACE_IMP(" glClipPlane(%s, (const double *)%p)", glEnumToString(plane), equation);
    // TODO: implement via constant buffer clip planes in shader
}

void glFinish()
{
    GL_TRACE_IMP(" glFinish()");
    g_backend.WaitForGpu();
}

void glScissorRect(GLint x, GLint y, GLsizei width, GLsizei height)
{
    // TODO
}
