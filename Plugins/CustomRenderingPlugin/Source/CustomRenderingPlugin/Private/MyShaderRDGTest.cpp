#include "MyShaderTest.h"
#include "GlobalShader.h"
#include "PipelineStateCache.h"
#include "Engine/TextureRenderTarget2D.h"
#include "RenderGraphUtils.h"
#include "ShaderParameterUtils.h"
#include "RHIStaticStates.h"
#include "RenderTargetPool.h"

#define LOCTEXT_NAMESPACE "CustomRenderingModul"

IMPLEMENT_GLOBAL_SHADER_PARAMETER_STRUCT(FMyShaderParameterBuffer, "MyShaderUniformBuffer");

TGlobalResource<FQuadVertexBuffer> QuadVertexRDG;
TGlobalResource<FQuadIndexBuffer> QuadIndexRDG;
TGlobalResource<FTextureVertexDeclaration> VertexDeclarationRDG;

//-----------------------------
// Global Shader Class
// Refer to FComposeSeparateTranslucencyPS in PostProcessing.cpp
//-----------------------------

class FMyShaderRDGTestVS : public FGlobalShader
{
public:
	DECLARE_GLOBAL_SHADER(FMyShaderRDGTestVS);

	FMyShaderRDGTestVS() {}

	FMyShaderRDGTestVS(const ShaderMetaType::CompiledShaderInitializerType& Initializer) : FGlobalShader(Initializer) {}

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
	}
};
IMPLEMENT_GLOBAL_SHADER(FMyShaderRDGTestVS, "/MyShaders/Private/MyTestShader.usf", "MainVS", SF_Vertex);

class FMyShaderRDGTestPS : public FGlobalShader
{
public:
	DECLARE_GLOBAL_SHADER(FMyShaderRDGTestPS);

	SHADER_USE_PARAMETER_STRUCT(FMyShaderRDGTestPS, FGlobalShader);

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
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
IMPLEMENT_GLOBAL_SHADER(FMyShaderRDGTestPS, "/MyShaders/Private/MyTestShader.usf", "MainPS", SF_Pixel);

// Refer to FGenerateMipsCS in GenerateMips.cpp
class FMyShaderRDGTestCS : public FGlobalShader
{
	DECLARE_GLOBAL_SHADER(FMyShaderRDGTestCS);
	SHADER_USE_PARAMETER_STRUCT(FMyShaderRDGTestCS, FGlobalShader);

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<float4>, OutputTexture)
		//SHADER_PARAMETER_STRUCT_REF(FMyShaderParameterBuffer, MyShaderUniformBuffer)
	END_SHADER_PARAMETER_STRUCT()

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return RHISupportsComputeShaders(Parameters.Platform);
	}
};
IMPLEMENT_GLOBAL_SHADER(FMyShaderRDGTestCS, "/MyShaders/Private/MyTestComputeShader.usf", "MainCS", SF_Compute);

static void DrawQuad(FRHICommandList& RHICmdList, TShaderMapRef<FMyShaderRDGTestVS> VS, TShaderMapRef<FMyShaderRDGTestPS> PS, FMyShaderRDGTestPS::FParameters* Parameters)
{
	// Set the graphic pipeline state.
	FGraphicsPipelineStateInitializer GraphicsPSOInit;
	GraphicsPSOInit.DepthStencilState = TStaticDepthStencilState<false, CF_Always>::GetRHI();
	GraphicsPSOInit.BlendState = TStaticBlendState<>::GetRHI();
	GraphicsPSOInit.RasterizerState = TStaticRasterizerState<>::GetRHI();
	GraphicsPSOInit.PrimitiveType = PT_TriangleList;
	// Bind Sahder
	GraphicsPSOInit.BoundShaderState.VertexDeclarationRHI = VertexDeclarationRDG.VertexDeclarationRHI;
	GraphicsPSOInit.BoundShaderState.VertexShaderRHI = VS.GetVertexShader();
	GraphicsPSOInit.BoundShaderState.PixelShaderRHI = PS.GetPixelShader();
	SetGraphicsPipelineState(RHICmdList, GraphicsPSOInit);

	SetShaderParameters(RHICmdList, PS, PS.GetPixelShader(), *Parameters);

	RHICmdList.SetStreamSource(0, QuadVertexRDG.VertexBufferRHI, 0);

	// Draw
	RHICmdList.DrawIndexedPrimitive(QuadIndexRDG.IndexBufferRHI, 0, 0, 4, 0, 2, 1);
}

static FRDGTextureRef AddComputePass(FRDGBuilder& GraphBuilder, FTexture2DRHIRef RenderTargetRHI)
{
	// Create render target texture
	FRDGTextureDesc RenderTargetDesc = FRDGTextureDesc::Create2DDesc(
		RenderTargetRHI->GetSizeXY(),
		RenderTargetRHI->GetFormat(),
		FClearValueBinding::Black,
		TexCreate_None,
		TexCreate_RenderTargetable | TexCreate_ShaderResource | TexCreate_UAV,
		false
	);

	FRDGTextureRef RenderTargetTexture = GraphBuilder.CreateTexture(RenderTargetDesc, TEXT("RenderTarget"));

	// Create UAV for texture
	FRDGTextureUAVDesc RenderTargetUAVDesc(RenderTargetTexture);
	FRDGTextureUAVRef RenderTargetUAV = GraphBuilder.CreateUAV(RenderTargetUAVDesc);

	// Setup parameters
	FMyShaderRDGTestCS::FParameters* PassParameters = GraphBuilder.AllocParameters<FMyShaderRDGTestCS::FParameters>();
	PassParameters->OutputTexture = RenderTargetUAV;

	// Get compute shader
	TShaderMapRef<FMyShaderRDGTestCS> ComputeShader(GetGlobalShaderMap(GMaxRHIFeatureLevel));

	// Setup thread group count
	FIntVector ThreadGroupCount(RenderTargetRHI->GetSizeX() / 32, RenderTargetRHI->GetSizeY() / 32, 1);

	GraphBuilder.AddPass(
		RDG_EVENT_NAME("MyComputeShaderRDGTest"),
		PassParameters,
		ERDGPassFlags::Compute,
		[PassParameters, ComputeShader, ThreadGroupCount](FRHICommandList& RHICmdList) {
		FComputeShaderUtils::Dispatch(RHICmdList, ComputeShader, *PassParameters, ThreadGroupCount);
	});

	return RenderTargetTexture;
}

static FRDGTextureRef AddDrawPass(FRDGBuilder& GraphBuilder, FRDGTextureRef Texture, FMyShaderParameterStruct MyShaderParameter)
{
	// Create render target texture
	FRDGTextureDesc RenderTargetDesc = FRDGTextureDesc::Create2DDesc(
		Texture->Desc.Extent,
		Texture->Desc.Format,
		FClearValueBinding::Black,
		TexCreate_None,
		TexCreate_RenderTargetable | TexCreate_ShaderResource | TexCreate_UAV,
		false
	);

	FRDGTextureRef RenderTargetTexture = GraphBuilder.CreateTexture(RenderTargetDesc, TEXT("RenderTarget"));

	// Create UAV for texture
	FRDGTextureUAVDesc RenderTargetUAVDesc(RenderTargetTexture);
	FRDGTextureUAVRef RenderTargetUAV = GraphBuilder.CreateUAV(RenderTargetUAVDesc);

	// Setup parameters
	FMyShaderParameterBuffer ParameterBuffer;
	ParameterBuffer.MyColor1 = MyShaderParameter.MyColor1;
	ParameterBuffer.MyColor2 = MyShaderParameter.MyColor2;
	ParameterBuffer.MyColor3 = MyShaderParameter.MyColor3;
	ParameterBuffer.MyColor4 = MyShaderParameter.MyColor4;
	ParameterBuffer.ColorIndex = MyShaderParameter.ColorIndex;

	FMyShaderRDGTestPS::FParameters* PassParameters = GraphBuilder.AllocParameters<FMyShaderRDGTestPS::FParameters>();
	PassParameters->SimpleColor = FLinearColor();
	PassParameters->SimpleTexture = Texture;
	PassParameters->SimpleTextureSampler = TStaticSamplerState<SF_Trilinear, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI();
	PassParameters->MyShaderUniformBuffer = TUniformBufferRef<FMyShaderParameterBuffer>::CreateUniformBufferImmediate(ParameterBuffer, UniformBuffer_SingleFrame);
	PassParameters->RenderTargets[0] = FRenderTargetBinding(RenderTargetTexture, ERenderTargetLoadAction::ENoAction);


	// Get shader
	TShaderMapRef<FMyShaderRDGTestVS> VertexShader(GetGlobalShaderMap(GMaxRHIFeatureLevel));
	TShaderMapRef<FMyShaderRDGTestPS> PixelShader(GetGlobalShaderMap(GMaxRHIFeatureLevel));

	GraphBuilder.AddPass(
		RDG_EVENT_NAME("MyComputeShaderRDGDrawTest"),
		PassParameters,
		ERDGPassFlags::Raster,
		[PassParameters, VertexShader, PixelShader](FRHICommandList& RHICmdList) 
		{
			DrawQuad(RHICmdList, VertexShader, PixelShader, PassParameters);
		});

	return RenderTargetTexture;
}

static void DrawMyTestShaderWithRDG_RenderThread(FRHICommandListImmediate& RHICmdList, FTexture2DRHIRef RenderTargetRHI, FMyShaderParameterStruct MyShaderParameter)
{
	check(IsInRenderingThread());

	// Create render graph
	FRDGBuilder GraphBuilder(RHICmdList);

	// Add Pass
	FRDGTextureRef ComputeTexture = AddComputePass(GraphBuilder, RenderTargetRHI);
	FRDGTextureRef FinalTexture = AddDrawPass(GraphBuilder, ComputeTexture, MyShaderParameter);

	TRefCountPtr<IPooledRenderTarget> PooledRenderTargetPtr;
	GraphBuilder.QueueTextureExtraction(FinalTexture, &PooledRenderTargetPtr);

	GraphBuilder.Execute();

	// Copy the result to render target
	RHICmdList.CopyTexture(PooledRenderTargetPtr->GetRenderTargetItem().ShaderResourceTexture, RenderTargetRHI->GetTexture2D(), FRHICopyTextureInfo());
}

void UMyShaderTestBlueprintLibrary::DrawMyTestShaderWithRDG(class UTextureRenderTarget2D* OutputRenderTarget, FMyShaderParameterStruct MyShaderParameter)
{
	check(IsInGameThread());

	if (!OutputRenderTarget)
	{
		return;
	}

	FTexture2DRHIRef RenderTargetTexure = OutputRenderTarget->GameThread_GetRenderTargetResource()->GetRenderTargetTexture();

	ENQUEUE_RENDER_COMMAND(CaptureCommand)(
		[RenderTargetTexure, MyShaderParameter](FRHICommandListImmediate& RHICmdList)
	{
		DrawMyTestShaderWithRDG_RenderThread(RHICmdList, RenderTargetTexure, MyShaderParameter);
	}
	);
}

#undef LOCTEXT_NAMESPACE
