#include "MyShaderTest.h"
#include "Engine/TextureRenderTarget2D.h"
#include "PipelineStateCache.h"
#include "GlobalShader.h"

#define LOCTEXT_NAMESPACE "CustomRenderingModul"


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
	}

	template<typename TShaderRHIParamRef>
	void SetParameters(FRHICommandListImmediate& RHICmdList, const TShaderRHIParamRef Shader, const FLinearColor& MyColor)
	{
		SetShaderValue(RHICmdList, Shader, SimpleColorVal, MyColor);
	}

private:
	// Use LAYOUT_FIELD after ver.4.25
	LAYOUT_FIELD(FShaderParameter, SimpleColorVal);
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

IMPLEMENT_SHADER_TYPE(, FMyShaderTestVS, TEXT("/MyShaders/Private/MyTestShader.usf"), TEXT("MainVS"), SF_Vertex)
IMPLEMENT_SHADER_TYPE(, FMyShaderTestPS, TEXT("/MyShaders/Private/MyTestShader.usf"), TEXT("MainPS"), SF_Pixel)


//-----------------------------
// Data struct
//-----------------------------

class FQuadVertexBuffer : public FVertexBuffer
{
public:
	void InitRHI() override
	{
		TResourceArray<FVector4, VERTEXBUFFER_ALIGNMENT> Vertices;
		Vertices.SetNumUninitialized(4);
		Vertices[0] = FVector4(-1.0f, 1.0f, 0, 1.0f);
		Vertices[1] = FVector4(1.0f, 1.0f, 0, 1.0f);
		Vertices[2] = FVector4(-1.0f, -1.0f, 0, 1.0f);
		Vertices[3] = FVector4(1.0f, -1.0f, 0, 1.0f);

		FRHIResourceCreateInfo CreateInfo(&Vertices);
		VertexBufferRHI = RHICreateVertexBuffer(Vertices.GetResourceDataSize(), BUF_Static, CreateInfo);
	}
};
class FQuadIndexBuffer : public FIndexBuffer
{
public:
	void InitRHI() override
	{
		const uint16 Indices[] = { 0, 1, 2, 2, 1, 3 };

		TResourceArray<uint16, INDEXBUFFER_ALIGNMENT> IndexBuffer;
		IndexBuffer.AddUninitialized(6);
		FMemory::Memcpy(IndexBuffer.GetData(), Indices, 6 * sizeof(uint16));

		FRHIResourceCreateInfo CreateIndexInfo(&IndexBuffer);
		IndexBufferRHI = RHICreateIndexBuffer(sizeof(uint16), IndexBuffer.GetResourceDataSize(), BUF_Static, CreateIndexInfo);
	}
};

//-----------------------------
// Blueprint
//-----------------------------

static void DrawMyShaderTestRenderTarget_RenderThread(FRHICommandListImmediate& RHICmdList, FTextureRenderTargetResource* OutputRenderTargetResource, ERHIFeatureLevel::Type FeatureLevel, FLinearColor MyColor, FName TextureRenderTargetName)
{
	check(IsInRenderingThread());

#if WANTS_DRAW_MESH_EVENTS
	FString EventName;
	TextureRenderTargetName.ToString(EventName);
	SCOPED_DRAW_EVENTF(RHICmdList, SceneCapture, TEXT("ShaderTest %s"), *EventName);
#else
	SCOPED_DRAW_EVENT(RHICmdList, DrawUVDisplacementToRenderTarget_RenderThread);
#endif

	// Set Render Target
	FTexture2DRHIRef RenderTargetTexture = OutputRenderTargetResource->GetRenderTargetTexture();

	RHICmdList.TransitionResource(EResourceTransitionAccess::EWritable, RenderTargetTexture);

	// Set Render Pass Info
	FRHIRenderPassInfo RPInfo(RenderTargetTexture, ERenderTargetActions::DontLoad_Store, RenderTargetTexture);

	RHICmdList.BeginRenderPass(RPInfo, TEXT("MyShaderTest"));
	{
		// Get shaders.
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
		GraphicsPSOInit.BoundShaderState.VertexDeclarationRHI = GetVertexDeclarationFVector4();
		GraphicsPSOInit.BoundShaderState.VertexShaderRHI = VertexShader.GetVertexShader();
		GraphicsPSOInit.BoundShaderState.PixelShaderRHI = PixelShader.GetPixelShader();

		SetGraphicsPipelineState(RHICmdList, GraphicsPSOInit);

		// Update viewport.
		FIntPoint DisplacementMapResolution(RenderTargetTexture->GetSizeX(), RenderTargetTexture->GetSizeY());
		RHICmdList.SetViewport(0, 0, 0.f, DisplacementMapResolution.X, DisplacementMapResolution.Y, 1.f);

		// Update shader uniform parameters.
		PixelShader->SetParameters(RHICmdList, PixelShader.GetPixelShader(), MyColor);

		// Get Data
		FQuadVertexBuffer quadVertex;
		quadVertex.InitRHI();
		FQuadIndexBuffer quadIndex;
		quadIndex.InitRHI();

		RHICmdList.SetStreamSource(0, quadVertex.VertexBufferRHI, 0);

		// Draw
		RHICmdList.DrawIndexedPrimitive(quadIndex.IndexBufferRHI, 0, 0, 4, 0, 2, 1);
	}
	RHICmdList.EndRenderPass();
}

void UMyShaderTestBlueprintLibrary::DrawMyShaderTestRenderTarget(UTextureRenderTarget2D* OutputRenderTarget, AActor* Actor, FLinearColor MyColor)
{
	check(IsInGameThread());

	if (!OutputRenderTarget)
	{
		return;
	}

	const FName TextureRenderTargetName = OutputRenderTarget->GetFName();
	FTextureRenderTargetResource* TextureRenderTargetResource = OutputRenderTarget->GameThread_GetRenderTargetResource();

	ERHIFeatureLevel::Type FeatureLevel = Actor->GetWorld()->Scene->GetFeatureLevel();

	// Put render command into render thread
	ENQUEUE_RENDER_COMMAND(CaptureCommand)(
		[TextureRenderTargetResource, FeatureLevel, MyColor, TextureRenderTargetName](FRHICommandListImmediate& RHICmdList)
		{
			DrawMyShaderTestRenderTarget_RenderThread(RHICmdList, TextureRenderTargetResource, FeatureLevel, MyColor, TextureRenderTargetName);
		}
	);
}

#undef LOCTEXT_NAMESPACE
