#include "MyShaderTest.h"
#include "GlobalShader.h"
#include "PipelineStateCache.h"
#include "Engine/TextureRenderTarget2D.h"
#include "RenderGraphUtils.h"
#include "ShaderParameterUtils.h"
#include "RHIStaticStates.h"


#define LOCTEXT_NAMESPACE "CustomRenderingModul"

IMPLEMENT_GLOBAL_SHADER_PARAMETER_STRUCT(FMyShaderParameterBuffer, "MyShaderUniformBuffer");

//-----------------------------
// Global Shader Class
// Refer to FComposeSeparateTranslucencyPS in PostProcessing.cpp
//-----------------------------

class FMyShaderRDGTest : public FGlobalShader
{
	DECLARE_GLOBAL_SHADER(FMyShaderRDGTest);
	SHADER_USE_PARAMETER_STRUCT(FMyShaderRDGTest, FGlobalShader);

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters,)
		SHADER_PARAMETER(FVector4, SimpleColor)
		SHADER_PARAMETER_RDG_TEXTURE(Texture2D, SimpleTexture)
		SHADER_PARAMETER_SAMPLER(SamplerState, SimpleTextureSampler)
		SHADER_PARAMETER_STRUCT_REF(FMyShaderParameterBuffer, MyShaderUniformBuffer)
		RENDER_TARGET_BINDING_SLOTS() // Use this macro when use raster pipeline, this will expose input of the reandering target and depth template
	END_SHADER_PARAMETER_STRUCT()

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
	}
};

class FMyShaderRDGTestVS : public FMyShaderRDGTest
{
	DECLARE_GLOBAL_SHADER(FMyShaderRDGTestVS);

	FMyShaderRDGTestVS() {}

	FMyShaderRDGTestVS(const ShaderMetaType::CompiledShaderInitializerType& Initializer) : FMyShaderRDGTest(Initializer) {}
};
IMPLEMENT_GLOBAL_SHADER(FMyShaderRDGTestVS, "/MyShaders/Private/MyTestShader.usf", "MainVS", SF_Vertex);

class FMyShaderRDGTestPS : public FMyShaderRDGTest
{
	DECLARE_GLOBAL_SHADER(FMyShaderRDGTestPS);

	FMyShaderRDGTestPS() {}

	FMyShaderRDGTestPS(const ShaderMetaType::CompiledShaderInitializerType& Initializer) : FMyShaderRDGTest(Initializer) {}
};
IMPLEMENT_GLOBAL_SHADER(FMyShaderRDGTestPS, "/MyShaders/Private/MyTestShader.usf", "MainPS", SF_Pixel);

// Refer to FGenerateMipsCS in GenerateMips.cpp
class FMyShaderRDGTestCS : public FGlobalShader
{
	DECLARE_GLOBAL_SHADER(FMyShaderRDGTestCS);
	SHADER_USE_PARAMETER_STRUCT(FMyShaderRDGTestCS, FGlobalShader);

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_STRUCT_REF(FMyShaderParameterBuffer, MyShaderUniformBuffer)
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<float4>, OutputTexture)
	END_SHADER_PARAMETER_STRUCT()

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return RHISupportsComputeShaders(Parameters.Platform);
	}
};
IMPLEMENT_GLOBAL_SHADER(FMyShaderRDGTestCS, "/MyShaders/Private/MyTestComputeShader.usf", "MainCS", SF_Compute);


#undef LOCTEXT_NAMESPACE
