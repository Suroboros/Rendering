#include "MyShaderTest.h"
#include "Engine/TextureRenderTarget2D.h"
#include "PipelineStateCache.h"
#include "GlobalShader.h"
#include "RenderGraph.h"

#define LOCTEXT_NAMESPACE "CustomRenderingModul"

//IMPLEMENT_GLOBAL_SHADER_PARAMETER_STRUCT(FMyShaderParameterBuffer, "MyShaderUniformBuffer");

TGlobalResource<FQuadVertexBuffer> QuadVertex;
TGlobalResource<FQuadIndexBuffer> QuadIndex;
TGlobalResource<FTextureVertexDeclaration> VertexDeclaration;

//-----------------------------
// Global Shader Class
//-----------------------------

class FMyShaderTest : public FGlobalShader
{
	DECLARE_INLINE_TYPE_LAYOUT(FMyShaderTest, NonVirtual);
public:
	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
	}

	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
		OutEnvironment.SetDefine(TEXT("TEST_MICRO"), 1);
	}

	FMyShaderTest() {}
	FMyShaderTest(const ShaderMetaType::CompiledShaderInitializerType& Initializer) : FGlobalShader(Initializer)
	{
		SimpleColorVal.Bind(Initializer.ParameterMap, TEXT("SimpleColor"));
		SimpleTextureVal.Bind(Initializer.ParameterMap, TEXT("SimpleTexture"));
		SimpleTextureSamplerVal.Bind(Initializer.ParameterMap, TEXT("SimpleTextureSampler"));
	}

	template<typename TRHIShader>
	void SetParameters(FRHICommandListImmediate& RHICmdList, TRHIShader Shader, const FLinearColor& MyColor, FTexture2DRHIRef MyTexture, FMyShaderParameterStruct MyShaderUnifomBuffer)
	{
		// Set uniform buffer
		FMyShaderParameterBuffer MyShaderParameter;
		MyShaderParameter.MyColor1 = MyShaderUnifomBuffer.MyColor1;
		MyShaderParameter.MyColor2 = MyShaderUnifomBuffer.MyColor2;
		MyShaderParameter.MyColor3 = MyShaderUnifomBuffer.MyColor3;
		MyShaderParameter.MyColor4 = MyShaderUnifomBuffer.MyColor4;
		MyShaderParameter.ColorIndex = MyShaderUnifomBuffer.ColorIndex;

		SetUniformBufferParameterImmediate(RHICmdList, Shader, GetUniformBufferParameter<FMyShaderParameterBuffer>(), MyShaderParameter);
		SetShaderValue(RHICmdList, Shader, SimpleColorVal, MyColor);
		SetTextureParameter(RHICmdList, Shader, SimpleTextureVal, SimpleTextureSamplerVal, TStaticSamplerState<SF_Trilinear, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI(), MyTexture);
	}

private:
	// Use LAYOUT_FIELD after ver.4.25
	LAYOUT_FIELD(FShaderParameter, SimpleColorVal);
	LAYOUT_FIELD(FShaderResourceParameter, SimpleTextureVal);
	LAYOUT_FIELD(FShaderResourceParameter, SimpleTextureSamplerVal);
};

class FMyShaderTestVS : public FMyShaderTest
{
	DECLARE_GLOBAL_SHADER(FMyShaderTestVS)
public:
	FMyShaderTestVS() {}
	FMyShaderTestVS(const ShaderMetaType::CompiledShaderInitializerType& Initializer) : FMyShaderTest(Initializer)
	{
	}
};

class FMyShaderTestPS : public FMyShaderTest
{
	DECLARE_GLOBAL_SHADER(FMyShaderTestPS)
public:
	FMyShaderTestPS(){}
	FMyShaderTestPS(const ShaderMetaType::CompiledShaderInitializerType& Initializer) : FMyShaderTest(Initializer)
	{
	}
};

class FMyComputeShaderTest : public FGlobalShader
{
	DECLARE_GLOBAL_SHADER(FMyComputeShaderTest)
public :
	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
	}

	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
		//OutEnvironment.CompilerFlags.Add(CFLAG_StandardOptimization);
	}

	FMyComputeShaderTest() {};
	FMyComputeShaderTest(const ShaderMetaType::CompiledShaderInitializerType& Initializer) : FGlobalShader(Initializer)
	{
		OutputTexture.Bind(Initializer.ParameterMap, TEXT("OutputTexture"));
	}

	void SetParameters(FRHICommandListImmediate& RHICmdList, FRHIUnorderedAccessView* InOutUAV, FMyShaderParameterStruct MyShaderParameterData)
	{
		FRHIComputeShader* ComputeShaderRHI = RHICmdList.GetBoundComputeShader();
		if (OutputTexture.IsBound())
		{
			RHICmdList.SetUAVParameter(ComputeShaderRHI, OutputTexture.GetBaseIndex(), InOutUAV);
		}

		FMyShaderParameterBuffer MyShaderParameter;
		MyShaderParameter.MyColor1 = MyShaderParameterData.MyColor1;
		MyShaderParameter.MyColor2 = MyShaderParameterData.MyColor2;
		MyShaderParameter.MyColor3 = MyShaderParameterData.MyColor3;
		MyShaderParameter.MyColor4 = MyShaderParameterData.MyColor4;
		MyShaderParameter.ColorIndex = MyShaderParameterData.ColorIndex;
		SetUniformBufferParameterImmediate(RHICmdList, ComputeShaderRHI, GetUniformBufferParameter<FMyShaderParameterBuffer>(), MyShaderParameter);
	}

	void UnsetParameters(FRHICommandListImmediate& RHICmdList)
	{
		FRHIComputeShader* ComputeShaderRHI = RHICmdList.GetBoundComputeShader();
		if (OutputTexture.IsBound())
		{
			RHICmdList.SetUAVParameter(ComputeShaderRHI, OutputTexture.GetBaseIndex(), nullptr);
		}
	}

private:
	LAYOUT_FIELD(FShaderResourceParameter, OutputTexture);
};

IMPLEMENT_SHADER_TYPE(, FMyShaderTestVS, TEXT("/MyShaders/Private/MyTestShader.usf"), TEXT("MainVS"), SF_Vertex)
IMPLEMENT_SHADER_TYPE(, FMyShaderTestPS, TEXT("/MyShaders/Private/MyTestShader.usf"), TEXT("MainPS"), SF_Pixel)
IMPLEMENT_SHADER_TYPE(, FMyComputeShaderTest, TEXT("/MyShaders/Private/MyTestComputeShader.usf"), TEXT("MainCS"), SF_Compute)

//-----------------------------
// Blueprint
//-----------------------------

static void DrawMyShaderTestRenderTarget_RenderThread(FRHICommandListImmediate& RHICmdList, FTexture2DRHIRef RenderTargetTexture, ERHIFeatureLevel::Type FeatureLevel, FLinearColor MyColor, FTexture2DRHIRef MyTexture, FMyShaderParameterStruct MyShaderUniformBuffer, FName TextureRenderTargetName)
{
	check(IsInRenderingThread());

#if WANTS_DRAW_MESH_EVENTS
	FString EventName;
	TextureRenderTargetName.ToString(EventName);
	SCOPED_DRAW_EVENTF(RHICmdList, SceneCapture, TEXT("ShaderTest %s"), *EventName);
#else
	SCOPED_DRAW_EVENT(RHICmdList, DrawMyShaderTestRenderTarget_RenderThread);
#endif

	RHICmdList.TransitionResource(EResourceTransitionAccess::EWritable, RenderTargetTexture);

	// Set Render Pass Info
	FRHIRenderPassInfo RPInfo(RenderTargetTexture, ERenderTargetActions::DontLoad_Store, RenderTargetTexture);

	RHICmdList.BeginRenderPass(RPInfo, TEXT("MyShaderTest"));
	{
		// Get shaders.
		FeatureLevel = GMaxRHIFeatureLevel;
		FGlobalShaderMap* GlobalShaderMap = GetGlobalShaderMap(FeatureLevel);
		TShaderMapRef<FMyShaderTestVS> VertexShader(GlobalShaderMap);
		TShaderMapRef<FMyShaderTestPS> PixelShader(GlobalShaderMap);

		// Set the graphic pipeline state.
		FGraphicsPipelineStateInitializer GraphicsPSOInit;
		RHICmdList.ApplyCachedRenderTargets(GraphicsPSOInit);
		GraphicsPSOInit.DepthStencilState = TStaticDepthStencilState<false, CF_Always>::GetRHI();
		GraphicsPSOInit.BlendState = TStaticBlendState<>::GetRHI();
		GraphicsPSOInit.RasterizerState = TStaticRasterizerState<>::GetRHI();
		GraphicsPSOInit.PrimitiveType = PT_TriangleList;
		// Bind Sahder
		GraphicsPSOInit.BoundShaderState.VertexDeclarationRHI = VertexDeclaration.VertexDeclarationRHI;
		GraphicsPSOInit.BoundShaderState.VertexShaderRHI = VertexShader.GetVertexShader();
		GraphicsPSOInit.BoundShaderState.PixelShaderRHI = PixelShader.GetPixelShader();

		SetGraphicsPipelineState(RHICmdList, GraphicsPSOInit);

		// Update viewport.
		FIntPoint DisplacementMapResolution(RenderTargetTexture->GetSizeX(), RenderTargetTexture->GetSizeY());
		RHICmdList.SetViewport(0, 0, 0.f, DisplacementMapResolution.X, DisplacementMapResolution.Y, 1.f);


		// Update shader uniform parameters.
		VertexShader->SetParameters(RHICmdList, PixelShader.GetPixelShader(), MyColor, MyTexture, MyShaderUniformBuffer);
		PixelShader->SetParameters(RHICmdList, PixelShader.GetPixelShader(), MyColor, MyTexture, MyShaderUniformBuffer);

		RHICmdList.SetStreamSource(0, QuadVertex.VertexBufferRHI, 0);

		// Draw
		RHICmdList.DrawIndexedPrimitive(QuadIndex.IndexBufferRHI, 0, 0, 4, 0, 2, 1);
	}
	RHICmdList.EndRenderPass();
}

void UMyShaderTestBlueprintLibrary::DrawMyShaderTestRenderTarget(
	class UTextureRenderTarget2D* OutputRenderTarget,
	AActor* Actor,
	FLinearColor MyColor,
	UTexture* MyTexture,
	FMyShaderParameterStruct MyShaderParameter
)
{
	check(IsInGameThread());

	if (!OutputRenderTarget)
	{
		return;
	}

	const FName TextureRenderTargetName = OutputRenderTarget->GetFName();
	FTexture2DRHIRef TextureRenderTargetTexure = OutputRenderTarget->GameThread_GetRenderTargetResource()->GetRenderTargetTexture();

	FTexture2DRHIRef MyTextureRHI = MyTexture->TextureReference.TextureReferenceRHI->GetTexture2D();

	ERHIFeatureLevel::Type FeatureLevel = Actor->GetWorld()->Scene->GetFeatureLevel();

	// Put render command into render thread
	ENQUEUE_RENDER_COMMAND(CaptureCommand)(
		[TextureRenderTargetTexure, FeatureLevel, MyColor, MyTextureRHI, MyShaderParameter, TextureRenderTargetName](FRHICommandListImmediate& RHICmdList)
		{
			DrawMyShaderTestRenderTarget_RenderThread(RHICmdList, TextureRenderTargetTexure, FeatureLevel, MyColor, MyTextureRHI, MyShaderParameter, TextureRenderTargetName);
		}
	);
}

static void DrawMyComputeShader_RenderThread(FRHICommandListImmediate& RHICmdList, FTexture2DRHIRef RenderTargetRHI, FMyShaderParameterStruct MyShaderParameter)
{
	check(IsInRenderingThread());

	// Get shaders and set
	const ERHIFeatureLevel::Type FeatureLevel = GMaxRHIFeatureLevel;
	TShaderMapRef<FMyComputeShaderTest> ComputeShader(GetGlobalShaderMap(FeatureLevel));
	RHICmdList.SetComputeShader(RHICmdList.GetBoundComputeShader());

	// Get RT size.
	FIntPoint DisplacementMapResolution(RenderTargetRHI->GetSizeX(), RenderTargetRHI->GetSizeY());

	// Create output texture and bind to unordered access view(UAV)
	FRHIResourceCreateInfo RCreateInfo;
	FTexture2DRHIRef Texture = RHICreateTexture2D(DisplacementMapResolution.X, DisplacementMapResolution.Y, PF_A32B32G32R32F, 1, 1, TexCreate_ShaderResource | TexCreate_UAV, RCreateInfo);
	FUnorderedAccessViewRHIRef TextureUAV = RHICreateUnorderedAccessView(Texture);

	// Set compute shader
	ComputeShader->SetParameters(RHICmdList, TextureUAV, MyShaderParameter);
	DispatchComputeShader(RHICmdList, ComputeShader, DisplacementMapResolution.X / 32, DisplacementMapResolution.Y / 32, 1);
	ComputeShader->UnsetParameters(RHICmdList);

	DrawMyShaderTestRenderTarget_RenderThread(RHICmdList, RenderTargetRHI, FeatureLevel, FLinearColor(), Texture, MyShaderParameter, RenderTargetRHI->GetName());
}

void UMyShaderTestBlueprintLibrary::DrawMyComputeShader(class UTextureRenderTarget2D* OutputRenderTarget, FMyShaderParameterStruct MyShaderParameter)
{
	check(IsInGameThread());

	FTexture2DRHIRef RenderTargetTexure = OutputRenderTarget->GameThread_GetRenderTargetResource()->GetRenderTargetTexture();

	ENQUEUE_RENDER_COMMAND(CaptureCommand)(
		[RenderTargetTexure, MyShaderParameter](FRHICommandListImmediate& RHICmdList)
		{
			DrawMyComputeShader_RenderThread(RHICmdList, RenderTargetTexure, MyShaderParameter);
		}
	);
}

#undef LOCTEXT_NAMESPACE
