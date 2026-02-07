/*  Project Starshatter 4.5
    Destroyer Studios LLC
    Copyright © 1997-2005. All Rights Reserved.

    SUBSYSTEM:    Stars.exe
    FILE:         magic.fx
    AUTHOR:       John DiCamillo


    OVERVIEW
    ========
    DirectX9 rendering effects for solid objects
*/


float4   eye                              = { 0, 0, 0, 1 };
float4   black          : COLOR           = { 0, 0, 0, 1 };
float4   cheat          : COLOR           = { 0.1, 0.1, 0.1, 1 };
float4   white          : COLOR           = { 1, 1, 1, 1 };
float4   red            : COLOR           = { 1, 0, 0, 0 };

/*********** Material Properties ***********/

float4   Ka             : Ambient         = { 0.2f, 0.2f, 0.2f, 1.0f };
float4   Kd             : Diffuse         = { 1.0f, 1.0f, 1.0f, 1.0f };
float4   Ks             : Specular        = { 0.2f, 0.2f, 0.2f, 1.0f };
float    Ns             : SpecularPower   = 20;
float4   Ke             : Emissive        = { 0.0f, 0.0f, 0.0f, 0.0f };
float    offsetAmp                        = 0.001;
float    bias                             = -0.00001;

texture  tex_d          : DiffuseMap;
texture  tex_s          : SpecularMap;
texture  tex_n          : NormalMap;
texture  tex_e          : EmissiveMap;
texture  tex_x          : DiffuseMap;

/************** light info *****************/

float4   ambientColor   : LIGHTCOLOR      = { 0.2f, 0.2f, 0.2f, 1.0f };
float4   light1Pos      : POSITION        = { 100, 100, 100, 1 };
float4   light1Dir      : DIRECTION       = {-100,-100,-100, 1 };
float4   light1Color    : LIGHTCOLOR      = { 1, 0, 0, 1 };

/************** xform matrices *************/

float4x4 world          : World;
float4x4 view           : View;
float4x4 proj           : Projection;
float4x4 wvp            : WorldViewProjection;
float4x4 worldInv       : WorldInverse;

float4   eyeObj         : POSITION        = { 0, 0, 0, 1 };

/********** SAMPLERS ***********************/

sampler2D diffuseSampler = sampler_state
{
   Texture = <tex_d>;
   MinFilter = Linear;
   MagFilter = Linear;
   MipFilter = Linear;
};

sampler2D specularSampler = sampler_state
{
   Texture = <tex_s>;
   MinFilter = Linear;
   MagFilter = Linear;
   MipFilter = Linear;
};

sampler2D normalSampler = sampler_state
{
   Texture = <tex_n>;
   MinFilter = Linear;
   MagFilter = Linear;
   MipFilter = Linear;
};

sampler2D emissiveSampler = sampler_state
{
   Texture = <tex_e>;
   MinFilter = Linear;
   MagFilter = Linear;
   MipFilter = Linear;
};

sampler2D extraSampler = sampler_state
{
   Texture = <tex_x>;
   MinFilter = Linear;
   MagFilter = Linear;
   MipFilter = Linear;
};


/****************************************************/

struct VS_OUTPUT_NOPS
{
   float4 position      : POSITION;
   float3 lightVec      : COLOR0;
   float3 specular      : COLOR1;
   float2 tex0          : TEXCOORD0;
   float2 tex1          : TEXCOORD1;
   float2 tex2          : TEXCOORD2;
};

struct VS_OUTPUT
{
   float4 position      : POSITION;
   float2 tex0          : TEXCOORD0;
   float3 lightVec      : TEXCOORD1;
   float3 eyeVec        : TEXCOORD2;
};

struct VS_OUTPUT_SIMPLE
{
   float4 position      : POSITION;
   float4 diffuse       : COLOR0;
   float4 specular      : COLOR1;
   float2 tex0          : TEXCOORD0;
};

/**************************************/
/***** VERTEX SHADER ******************/
/**************************************/

VS_OUTPUT_SIMPLE
vtxSimple(
            float4 position      : POSITION,
            float3 normal        : NORMAL,
            float2 tex0          : TEXCOORD0)
{
   VS_OUTPUT_SIMPLE Out;

   // transform vertex position to homogeneous clip space
   Out.position = mul(position, wvp);

   float4x4 wv = mul(world, view);

   float3 L = normalize(mul(-light1Dir.xyz, (float3x3) view));
   float3 N = normalize(mul(normal,         (float3x3) wv));
   float3 P = mul(position, (float4x3) wv);           // position (view space)
   float3 R = normalize(2 * dot(N, L) * N - L);       // reflection vector (view space)
   float3 V = -normalize(P);                          // view direction (view space)

   Out.diffuse  = ambientColor * Ka + 
                  light1Color  * Kd * max(0, dot(N, L));
   Out.specular = light1Color  * Ks * pow(abs(max(0, dot(R, V))), Ns);
   Out.tex0     = tex0;

   return Out;
}

VS_OUTPUT_NOPS
vtxNormalNoPS(
            float4 position      : POSITION,
            float3 normal        : NORMAL,
            float2 tex0          : TEXCOORD0,
            float2 tex1          : TEXCOORD1,
            float3 tangent       : TANGENT2,
            float3 binormal      : BINORMAL3)
{
   VS_OUTPUT_NOPS Out;

   // transform vertex position to homogeneous clip space
   Out.position = mul(position, wvp);

   //pass texture coordinates
   Out.tex0 = tex0;
   Out.tex1 = tex0;
   Out.tex2 = tex0;

   // compute the 3x3 tranform from object space to tangent space
   float3x3 obj2tan;

   obj2tan[0] = tangent;
   obj2tan[1] = binormal;
   obj2tan[2] = normal;

   // Transform light vector to tangent space
   float3 L = mul(obj2tan, -light1Dir.xyz);

   // Normalize and bias transformed light vector
   L = normalize(L) * 0.5 + 0.5;

   Out.lightVec = L;

   float3 N       = normal;
   float3 V       = normalize(eyeObj.xyz - position.xyz);
          L       = normalize(-light1Dir.xyz);
   float3 H       = normalize(V+L);

   float  N_dot_H = dot(N,H);
   Out.specular = Ks * pow(abs(N_dot_H), Ns);
   return Out;
}

VS_OUTPUT vtxNormal(
            float4 position      : POSITION,
            float3 normal        : NORMAL,
            float2 tex0          : TEXCOORD0,
            float2 tex1          : TEXCOORD1,
            float3 tangent       : TANGENT2,
            float3 binormal      : BINORMAL3)
{
   VS_OUTPUT Out;

   // transform vertex position to homogeneous clip space
   Out.position = mul(position, wvp);

   //pass texture coordinates
   Out.tex0 = tex0;

   // compute the 3x3 tranform from object space to tangent space
   float3x3 obj2tan;

   obj2tan[0] = tangent;
   obj2tan[1] = binormal;
   obj2tan[2] = normal;

   // Transform light vector to tangent space
   float3 L = mul(obj2tan, -light1Dir.xyz);

   // Normalize transformed light vector
   Out.lightVec = normalize(L);

   // Transform eye vector from obj space to tangent space
   float3 objEyeVec = eyeObj.xyz - position.xyz;

   Out.eyeVec =  normalize(mul(obj2tan, objEyeVec));

   return Out;
}

VS_OUTPUT_SIMPLE
vtxAtmosphere(
            float4 position      : POSITION,
            float3 normal        : NORMAL,
            float2 tex0          : TEXCOORD0)
{
   VS_OUTPUT_SIMPLE Out;

   // transform vertex position to homogeneous clip space
   float4 tmp = position;
   tmp.xyz *= 1.04;
   Out.position = mul(tmp, wvp);

   float4x4 wv = mul(world, view);

   float3 L = normalize(mul(-light1Dir.xyz, (float3x3) view));
   float3 N = normalize(mul(normal,         (float3x3) wv));
   float3 V = normalize(eyeObj - position); // eye ray

   float  n = clamp(dot(V, normal), 0, 1);
   float  a = pow(1-n, 2);

   Out.diffuse  = Ka * abs(clamp(dot(N, L), -0.2, 1));
   Out.specular = black;
   Out.tex0     = float2(0, 1-a);

   return Out;
}

/**************************************/
/********* PIXEL SHADER ***************/
/**************************************/

float4 pixDiffuse(
      VS_OUTPUT_SIMPLE  In,
      uniform sampler2D diffTex
      ) : COLOR
{
   float4 diffuse    = tex2D(diffTex, In.tex0);

   return In.diffuse * diffuse + In.specular;
}

float4 pixSpecular(
      VS_OUTPUT_SIMPLE  In,
      uniform sampler2D diffTex,
      uniform sampler2D specTex
      ) : COLOR
{
   float4 diffuse    = tex2D(diffTex, In.tex0);
   float4 specular   = tex2D(specTex, In.tex0);

   return In.diffuse * diffuse + In.specular * specular;
}

float4 pixEmissive(
      VS_OUTPUT_SIMPLE  In,
      uniform sampler2D diffTex,
      uniform sampler2D glowTex
      ) : COLOR
{
   float4 diffuse    = tex2D(diffTex, In.tex0);
   float4 emissive   = tex2D(glowTex, In.tex0);

   return In.diffuse * diffuse + In.specular + emissive;
}

float4 pixEmissiveSpecular(
      VS_OUTPUT_SIMPLE  In,
      uniform sampler2D diffTex,
      uniform sampler2D glowTex,
      uniform sampler2D specTex
      ) : COLOR
{
   float4 diffuse    = tex2D(diffTex, In.tex0);
   float4 emissive   = tex2D(glowTex, In.tex0);
   float4 specular   = tex2D(specTex, In.tex0);

   return In.diffuse * diffuse + In.specular * specular + emissive;
}

float4 pixNightLightSpecular(
      VS_OUTPUT_SIMPLE  In,
      uniform sampler2D diffTex,
      uniform sampler2D glowTex,
      uniform sampler2D specTex
      ) : COLOR
{
   float4 diffuse    = tex2D(diffTex, In.tex0);
   float4 emissive   = tex2D(glowTex, In.tex0);
   float4 specular   = tex2D(specTex, In.tex0);
   float  inv_diff   = 1 - In.diffuse.b;
   float4 night      = float4(inv_diff, inv_diff, inv_diff, 0);

   return In.diffuse * diffuse + In.specular * specular + emissive * night;
}

float4 pixNormalSpecular(
      VS_OUTPUT         In,
      uniform sampler2D diffTex,
      uniform sampler2D specTex,
      uniform sampler2D normTex
      ) : COLOR
{
   float4 diffuse    = tex2D(diffTex, In.tex0);
   float4 specular   = tex2D(specTex, In.tex0);
   float3 normal     = tex2D(normTex, In.tex0).xyz * 2.0 - 1.0;
   float3 N          = normalize(normal);
   float3 V          = In.eyeVec;   //normalize(In.eyeVec);
   float3 L          = In.lightVec; //normalize(In.lightVec);
   float3 H          = normalize(V+L);
   float4 coeff      = lit(dot(N,L), dot(N,H), Ns);

   return light1Color * (Kd*diffuse*coeff.y + Ks*specular*coeff.z);
}

float4 pixNormal(
      VS_OUTPUT         In,
      uniform sampler2D diffTex,
      uniform sampler2D normTex
      ) : COLOR
{
   float4 diffuse    = tex2D(diffTex, In.tex0);
   float3 normal     = tex2D(normTex, In.tex0).xyz * 2.0 - 1.0;
   float3 N          = normalize(normal);
   float3 V          = normalize(In.eyeVec);
   float3 L          = normalize(In.lightVec);
   float3 H          = normalize(V+L);
   float4 coeff      = lit(dot(N,L), dot(N,H), Ns);

   return light1Color * (Kd*diffuse*coeff.y + Ks*coeff.z);
}

float4 pixAtmosphere(
      VS_OUTPUT_SIMPLE  In,
      uniform sampler2D limbTex
      ) : COLOR
{
   float4 limb = tex2D(limbTex, In.tex0);
   return In.diffuse * limb;
}


/****************************************************/
/********** TECHNIQUES ******************************/
/****************************************************/

technique SimplePix
{ 
   pass P0 
   {
      FogEnable         = false;
      VertexShader      = compile vs_1_1 vtxSimple();
      PixelShader       = compile ps_2_0 pixDiffuse(diffuseSampler);
   }
}

technique SpecMapPix
{ 
   pass P0 
   {
      FogEnable         = false;
      VertexShader      = compile vs_1_1 vtxSimple();
      PixelShader       = compile ps_2_0 pixSpecular(diffuseSampler,specularSampler);
   }
}

technique EmissivePix
{ 
   pass P0 
   {
      FogEnable         = false;
      VertexShader      = compile vs_1_1 vtxSimple();
      PixelShader       = compile ps_2_0 pixEmissive(diffuseSampler,emissiveSampler);
   }
}

technique EmissiveSpecMapPix
{ 
   pass P0 
   {
      FogEnable         = false;
      VertexShader      = compile vs_1_1 vtxSimple();
      PixelShader       = compile ps_2_0 pixEmissiveSpecular(diffuseSampler,emissiveSampler,specularSampler);
   }
}

technique BumpSpecMapPix
{ 
   pass P0 
   {
      TexCoordIndex[0]  = 0;
      TexCoordIndex[1]  = 1;
      TexCoordIndex[2]  = 2;
      TexCoordIndex[3]  = 3;

      DepthBias         = (bias);
      FogEnable         = false;
      VertexShader      = compile vs_1_1 vtxNormal();
      PixelShader       = compile ps_2_0 pixNormalSpecular(diffuseSampler,specularSampler,normalSampler);
   }
}

technique BumpSpecMap
{ 
   pass P0 
   {
      Sampler[0]        = (normalSampler);
      Sampler[1]        = (diffuseSampler);
      Sampler[2]        = (specularSampler);

      ColorOp[0]        = DOTPRODUCT3;
      ColorArg1[0]      = TEXTURE;
      ColorArg2[0]      = DIFFUSE;
      AlphaOp[0]        = SELECTARG1;
      AlphaArg1[0]      = TEXTURE;
      TexCoordIndex[0]  = 0;

      ColorOp[1]        = MODULATE;
      ColorArg1[1]      = TEXTURE;
      ColorArg2[1]      = CURRENT;
      AlphaOp[1]        = SELECTARG1;
      AlphaArg1[1]      = TEXTURE;
      TexCoordIndex[1]  = 1;

      ColorOp[2]        = MULTIPLYADD;
      ColorArg1[2]      = TEXTURE;
      ColorArg2[2]      = SPECULAR;
      AlphaOp[2]        = SELECTARG1;
      AlphaArg1[2]      = TEXTURE;
      TexCoordIndex[2]  = 2;

      ColorOp[3]        = DISABLE;
      AlphaOp[3]        = DISABLE;
      TexCoordIndex[3]  = 3;

      DepthBias         = (bias);
      FogEnable         = false;
      VertexShader      = compile vs_1_1 vtxNormalNoPS();
      PixelShader       = NULL;
   }
}

technique BumpMapPix
{ 
   pass P0 
   {
      TexCoordIndex[0]  = 0;
      TexCoordIndex[1]  = 1;
      TexCoordIndex[2]  = 2;
      TexCoordIndex[3]  = 3;

      DepthBias         = (bias);
      FogEnable         = false;
      VertexShader      = compile vs_1_1 vtxNormal();
      PixelShader       = compile ps_2_0 pixNormal(diffuseSampler,normalSampler);
   }
}

technique BumpMap
{ 
   pass P0 
   {
      Sampler[0]        = (normalSampler);
      Sampler[1]        = (diffuseSampler);

      ColorOp[0]        = DOTPRODUCT3;
      ColorArg1[0]      = TEXTURE;
      ColorArg2[0]      = DIFFUSE;
      AlphaOp[0]        = SELECTARG1;
      AlphaArg1[0]      = TEXTURE;
      TexCoordIndex[0]  = 0;

      ColorOp[1]        = MODULATE;
      ColorArg1[1]      = TEXTURE;
      ColorArg2[1]      = CURRENT;
      AlphaOp[1]        = SELECTARG1;
      AlphaArg1[1]      = TEXTURE;
      TexCoordIndex[1]  = 1;

      ColorOp[2]        = DISABLE;
      AlphaOp[2]        = DISABLE;
      TexCoordIndex[2]  = 2;

      DepthBias         = (bias);
      FogEnable         = false;
      VertexShader      = compile vs_1_1 vtxNormalNoPS();
      PixelShader       = NULL;
   }
}

technique SimpleMaterial
{
   pass P0
   {
      //FogEnable         = false;
      MaterialAmbient   = (Ka); 
      MaterialDiffuse   = (Kd); 
      MaterialSpecular  = (Ks); 
      MaterialPower     = (Ns);
      MaterialEmissive  = (Ke);

      Lighting          = TRUE;
      SpecularEnable    = TRUE;

      // NO textures

      ColorOp[0]        = DISABLE;
      AlphaOp[0]        = DISABLE;

      // NO shaders

      VertexShader      = NULL;
      PixelShader       = NULL;
   }
}

technique SimpleTexture
{
   pass P0
   {
      //FogEnable         = false;
      MaterialAmbient   = (Ka); 
      MaterialDiffuse   = (Kd); 
      MaterialSpecular  = (Ks); 
      MaterialPower     = (Ns);
      MaterialEmissive  = (black);

      Lighting          = TRUE;
      SpecularEnable    = TRUE;

      Sampler[0]        = (diffuseSampler);


      // texture stages

      ColorOp[0]        = MODULATE;
      ColorArg1[0]      = TEXTURE;
      ColorArg2[0]      = DIFFUSE;
      AlphaOp[0]        = MODULATE;
      AlphaArg1[0]      = TEXTURE;
      AlphaArg2[0]      = DIFFUSE;
      TexCoordIndex[0]  = 0;

      ColorOp[1]        = DISABLE;
      AlphaOp[1]        = DISABLE;


      // NO shaders

      VertexShader      = NULL;
      PixelShader       = NULL;
   }
}


technique SpecularTexture
{
   pass P0
   {
      //FogEnable         = false;
      MaterialAmbient   = (Ka); 
      MaterialDiffuse   = (Kd); 
      MaterialSpecular  = (Ks); 
      MaterialPower     = (Ns);
      MaterialEmissive  = (Ke);

      Lighting          = TRUE;
      SpecularEnable    = TRUE;

      Sampler[0]        = (diffuseSampler);
      Sampler[1]        = (specularSampler);

      // texture stages

      ColorOp[0]        = MODULATE;
      ColorArg1[0]      = TEXTURE;
      ColorArg2[0]      = DIFFUSE;
      AlphaOp[0]        = MODULATE;
      AlphaArg1[0]      = TEXTURE;
      AlphaArg2[0]      = DIFFUSE;
      TexCoordIndex[0]  = 0;

      ColorOp[1]        = MULTIPLYADD;
      ColorArg1[1]      = TEXTURE;
      ColorArg2[1]      = SPECULAR;
      AlphaOp[1]        = SELECTARG1;
      AlphaArg1[1]      = CURRENT;
      TexCoordIndex[1]  = 0;

      ColorOp[2]        = DISABLE;
      AlphaOp[2]        = DISABLE;

      // NO shaders

      VertexShader      = NULL;
      PixelShader       = NULL;
   }
}

technique EmissiveTexture
{
   pass P0
   {
      //FogEnable         = false;
      MaterialAmbient   = (Ka); 
      MaterialDiffuse   = (Kd); 
      MaterialSpecular  = (Ks); 
      MaterialPower     = (Ns);
      MaterialEmissive  = (black);

      Lighting          = TRUE;
      SpecularEnable    = FALSE;

      Sampler[0]        = (diffuseSampler);
      Sampler[1]        = (emissiveSampler);

      // texture stages

      ColorOp[0]        = MODULATE;
      ColorArg1[0]      = TEXTURE;
      ColorArg2[0]      = DIFFUSE;
      AlphaOp[0]        = MODULATE;
      AlphaArg1[0]      = TEXTURE;
      AlphaArg2[0]      = DIFFUSE;
      TexCoordIndex[0]  = 0;

      ColorOp[1]        = ADD;
      ColorArg1[1]      = TEXTURE;
      ColorArg2[1]      = CURRENT;
      AlphaOp[1]        = SELECTARG1;
      AlphaArg1[1]      = CURRENT;
      TexCoordIndex[1]  = 0;

      ColorOp[2]        = DISABLE;
      AlphaOp[2]        = DISABLE;

      // NO shaders

      VertexShader      = NULL;
      PixelShader       = NULL;
   }
}

technique EmissiveSpecularTexture
{
   pass P0
   {
      //FogEnable         = false;
      MaterialAmbient   = (Ka); 
      MaterialDiffuse   = (Kd); 
      MaterialSpecular  = (Ks); 
      MaterialPower     = (Ns);
      MaterialEmissive  = (black);

      Lighting          = TRUE;
      SpecularEnable    = TRUE;

      Sampler[0]        = (diffuseSampler);
      Sampler[1]        = (specularSampler);
      Sampler[2]        = (emissiveSampler);

      // texture stages

      ColorOp[0]        = MODULATE;
      ColorArg1[0]      = TEXTURE;
      ColorArg2[0]      = DIFFUSE;
      AlphaOp[0]        = MODULATE;
      AlphaArg1[0]      = TEXTURE;
      AlphaArg2[0]      = DIFFUSE;
      TexCoordIndex[0]  = 0;

      ColorOp[1]        = MULTIPLYADD;
      ColorArg1[1]      = TEXTURE;
      ColorArg2[1]      = SPECULAR;
      AlphaOp[1]        = SELECTARG1;
      AlphaArg1[1]      = CURRENT;
      TexCoordIndex[1]  = 0;

      ColorOp[2]        = ADD;
      ColorArg1[2]      = TEXTURE;
      ColorArg2[2]      = CURRENT;
      AlphaOp[2]        = SELECTARG1;
      AlphaArg1[2]      = CURRENT;
      TexCoordIndex[2]  = 0;

      ColorOp[3]        = DISABLE;
      AlphaOp[3]        = DISABLE;

      // NO shaders

      VertexShader      = NULL;
      PixelShader       = NULL;
   }
}


technique PlanetSurf
{
   pass P0
   {
      AlphaBlendEnable  = true;
      BlendOp           = ADD;
      SrcBlend          = ONE;
      DestBlend         = ONE;
      FogEnable         = false;
      VertexShader      = compile vs_1_1 vtxSimple();
      PixelShader       = compile ps_2_0 pixEmissiveSpecular(diffuseSampler,emissiveSampler,specularSampler);
   }
}

technique PlanetSurfNightLight
{
   pass P0
   {
      AlphaBlendEnable  = true;
      BlendOp           = ADD;
      SrcBlend          = ONE;
      DestBlend         = ONE;
      FogEnable         = false;
      VertexShader      = compile vs_1_1 vtxSimple();
      PixelShader       = compile ps_2_0 pixNightLightSpecular(diffuseSampler,emissiveSampler,specularSampler);
   }
}

technique PlanetLimb
{
   pass P0
   {
      AlphaBlendEnable  = true;
      SrcBlend          = SRCALPHA;
      DestBlend         = INVSRCALPHA;
      FogEnable         = false;

      VertexShader      = compile vs_1_1 vtxAtmosphere();
      PixelShader       = compile ps_2_0 pixAtmosphere(diffuseSampler);
   }
}


matrix      mID;    // Identity transform
matrix      env_matrix; // Environment map transform
textureCUBE env_cube;   // Cubic environment map

technique WaterReflections
{
   pass P0
   {        
      VertexShader = null;

      MaterialAmbient   = (Ka); 
      MaterialDiffuse   = (Kd); 
      MaterialSpecular  = (Ks); 
      MaterialPower     = (Ns);
      MaterialEmissive  = (black);

      Lighting          = TRUE;
      SpecularEnable    = TRUE;

      Sampler[0]        = (diffuseSampler);


      ColorOp[0]        = MODULATE;
      ColorArg1[0]      = TEXTURE;
      ColorArg2[0]      = DIFFUSE;
      AlphaOp[0]        = MODULATE;
      AlphaArg1[0]      = TEXTURE;
      AlphaArg2[0]      = DIFFUSE;
      TexCoordIndex[0]  = 0;

      ColorOp[1]        = DISABLE;
      AlphaOp[1]        = DISABLE;



      // Stage2
      /*
      ColorOp[1]   = BlendCurrentAlpha;
      ColorArg1[1] = Texture;
      ColorArg2[1] = Current;
      AlphaOp[1]   = SelectArg2;
      AlphaArg2[1] = Current;

      MinFilter[1] = Linear;
      MagFilter[1] = Linear;
      MipFilter[1] = Point;

      Texture[1] = <env_cube>;
      TextureTransform[1] = <env_matrix>;
      TextureTransformFlags[1] = Count3;
      TexCoordIndex[1] = CameraSpaceReflectionVector;

      // Stage3
      ColorOp[2] = Disable;
      AlphaOp[2] = Disable;
      */
   }
}

