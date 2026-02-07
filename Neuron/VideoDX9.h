#ifndef VideoDX9_h
#define VideoDX9_h

#include "List.h"
#include "Video.h"
#include "VideoSettings.h"

class VideoDX9;
class VideoDX9Enum;
class VideoDX9VertexBuffer;
class VideoDX9IndexBuffer;
struct VideoDX9ScreenVertex;
class Surface;
class Segment;

struct VideoDX9SolidVertex;
struct VideoDX9LuminousVertex;
struct VideoDX9LineVertex;

class VideoDX9 : public Video
{
  public:
    VideoDX9(const HWND& window, VideoSettings* vs);
    ~VideoDX9() override;

    const VideoSettings* GetVideoSettings() const override { return &video_settings; }

    bool SetVideoSettings(const VideoSettings* vs) override;

    bool SetBackgroundColor(Color c) override;
    bool SetGammaLevel(int g) override;
    bool SetObjTransform(const Matrix& o, const Point& l) override;

    virtual bool SetupParams();
    bool Reset(const VideoSettings* vs) override;

    bool StartFrame() override;
    bool EndFrame() override;

    int Width() const override { return width; }
    int Height() const override { return height; }
    int Depth() const override { return bpp; }

    void RecoverSurfaces() override;

    bool ClearAll() override;
    bool ClearDepthBuffer() override;
    bool Present() override;
    bool Pause() override;
    bool Resume() override;

    virtual IDirect3D9* Direct3D() const { return d3d; }
    virtual IDirect3DDevice9* D3DDevice() const { return d3ddevice; }
    static IDirect3DDevice9* GetD3DDevice9();

    bool IsModeSupported(int width, int height, int bpp) const override;
    bool IsHardware() const override { return true; }
    int ZDepth() const override { return zdepth; }
    DWORD VidMemFree() const override;
    int D3DLevel() const override { return 9; }
    int MaxTexSize() const override;
    int MaxTexAspect() const override;
    int GammaLevel() const override { return gamma; }

    bool Capture(Bitmap& bmp) override;
    bool GetWindowRect(Rect& r) override;
    bool SetWindowRect(const Rect& r) override;
    bool SetViewport(int x, int y, int w, int h) override;
    bool SetCamera(const Camera* cam) override;
    bool SetEnvironment(Bitmap** faces) override;
    bool SetAmbient(Color c) override;
    bool SetLights(const List<Light>& lights) override;
    bool SetProjection(float fov, float znear = 1.0f, float zfar = 1.0e6f,
                       DWORD type = PROJECTION_PERSPECTIVE) override;
    bool SetRenderState(RENDER_STATE state, DWORD value) override;
    bool SetBlendType(int blend_type) override;

    bool DrawPolys(int npolys, Poly* p) override;
    bool DrawScreenPolys(int npolys, Poly* p, int blend = 0) override;
    bool DrawSolid(Solid* s, DWORD blend_modes = 0xf) override;
    bool DrawShadow(Solid* s, int nverts, Vec3* verts, bool vis = false) override;
    bool DrawLines(int nlines, Vec3* v, Color c, int blend = 0) override;
    bool DrawScreenLines(int nlines, float* v, Color c, int blend = 0) override;
    bool DrawPoints(VertexSet* v) override;
    bool DrawPolyOutline(Poly* p) override;
    bool UseMaterial(Material* m) override;

    bool UseXFont(const char* name, int size, bool b, bool i) override;
    bool DrawText(const char* text, int count, const Rect& rect, DWORD format, Color c) override;

    void PreloadTexture(Bitmap* bmp) override;
    void PreloadSurface(Surface* s) override;
    void InvalidateCache() override;

    static void CreateD3DMatrix(D3DMATRIX& result, const Matrix& m, const Point& p);
    static void CreateD3DMatrix(D3DMATRIX& result, const Matrix& m, const Vec3& v);
    static void CreateD3DMaterial(D3DMATERIAL9& result, const Material& mtl);

  private:
    bool CreateBuffers();
    bool DestroyBuffers();
    bool PopulateScreenVerts(VertexSet* vset);
    bool PrepareSurface(Surface* s);
    bool DrawSegment(Segment* s);

    int PrepareMaterial(Material* m);
    bool SetupPass(int n);

    HWND hwnd;
    int width;
    int height;
    int bpp;
    int gamma;
    int zdepth;
    Color background;

    VideoDX9Enum* dx9enum;
    VideoSettings video_settings;

    IDirect3D9* d3d;
    IDirect3DDevice9* d3ddevice;
    D3DPRESENT_PARAMETERS d3dparams;
    D3DSURFACE_DESC back_buffer_desc;
    bool device_lost;

    BYTE* surface;

    DWORD texture_format[3];
    D3DGAMMARAMP gamma_ramp;
    double fade;

    Rect rect;

    IDirect3DVertexDeclaration9* vertex_declaration;
    ID3DXEffect* magic_fx;
    BYTE* magic_fx_code;
    int magic_fx_code_len;

    IDirect3DTexture9* current_texture;
    int current_blend_state;
    int scene_active;
    DWORD render_state[RENDER_STATE_MAX];
    Material* use_material;

    Material* segment_material;
    int strategy;
    int passes;

    ID3DXFont* d3dx_font;
    char font_name[64];
    int font_size;
    bool font_bold;
    bool font_ital;

    Color ambient;
    int nlights;

    int first_vert;
    int num_verts;

    VideoDX9VertexBuffer* screen_vbuf;
    VideoDX9IndexBuffer* screen_ibuf;
    VideoDX9ScreenVertex* font_verts;
    WORD* font_indices;
    int font_nverts;

    VideoDX9ScreenVertex* screen_line_verts;
    VideoDX9LineVertex* line_verts;
};

#endif VideoDX9_h
