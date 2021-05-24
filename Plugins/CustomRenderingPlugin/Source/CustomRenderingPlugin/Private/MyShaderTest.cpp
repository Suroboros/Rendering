#include "MyShaderTest.h"
#include "Engine/TextureRenderTarget2D.h"
#include "PipelineStateCache.h"
#include "GlobalShader.h"
#include "RenderGraph.h"

#define LOCTEXT_NAMESPACE "CustomRenderingModul"

//IMPLEMENT_GLOBAL_SHADER_PARAMETER_STRUCT(FMyShaderParameterBuffer, "MyShaderUniformBuffer");

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
		SetTextureParameter(RHICmdList, Shader, SimpleTextureVal, SimpleTextureSamplerVal, TStaticSamplerState<SF_Trilinear, AM_Clamp>::GetRHI(), MyTexture);
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
// Data struct
//-----------------------------
struct FTextureVertex
{
	FVector4    Position;
	FVector2D   UV;
};

class FQuadVertexBuffer : public FVertexBuffer
{
public:
	void InitRHI() override
	{
		TResourceArray<FTextureVertex, VERTEXBUFFER_ALIGNMENT> Vertices;
		Vertices.SetNumUninitialized(6);
		
		// Left up
		Vertices[0].Position = FVector4(1.0f, 1.0f, 0, 1.0f);
		Vertices[0].UV = FVector2D(1.0f, 1.0f);

		// Right up
		Vertices[1].Position = FVector4(-1.0f, 1.0f, 0, 1.0f);
		Vertices[1].UV = FVector2D(0, 1.0f);

		// Left bottom
		Vertices[2].Position = FVector4(1.0f, -1.0f, 0, 1.0f);
		Vertices[2].UV = FVector2D(1.0f, 0);

		// Right bottom
		Vertices[3].Position = FVector4(-1.0f, -1.0f, 0, 1.0f);
		Vertices[3].UV = FVector2D(0, 0);

		// For the triangle optimization
		Vertices[4].Position = FVector4(-1.0f, 1.0f, 0, 1.0f);
		Vertices[4].UV = FVector2D(-1.0f, 1.0f);

		Vertices[5].Position = FVector4(1.0f, -1.0f, 0, 1.0f);
		Vertices[5].UV = FVector2D(1.0f, -1.0f);

		FRHIResourceCreateInfo CreateInfo(&Vertices);
		VertexBufferRHI = RHICreateVertexBuffer(Vertices.GetResourceDataSize(), BUF_Static, CreateInfo);
	}
};

class FQuadIndexBuffer : public FIndexBuffer
{
public:
	void InitRHI() override
	{
		const uint16 Indices[] = { 0, 1, 2, 2, 1, 3, 0, 4, 5 };

		TResourceArray<uint16, INDEXBUFFER_ALIGNMENT> IndexBuffer;
		uint32 IndicesCount = UE_ARRAY_COUNT(Indices);
		IndexBuffer.AddUninitialized(IndicesCount);
		FMemory::Memcpy(IndexBuffer.GetData(), Indices, IndicesCount * sizeof(uint16));

		// Create index buffer.
		FRHIResourceCreateInfo CreateIndexInfo(&IndexBuffer);
		IndexBufferRHI = RHICreateIndexBuffer(sizeof(uint16), IndexBuffer.GetResourceDataSize(), BUF_Static, CreateIndexInfo);
	}
};

class FTextureVertexDeclaration : public FRenderResource
{
public:
	FVertexDeclarationRHIRef  VertexDeclarationRHI;

	virtual void InitRHI() override
	{
		FVertexDeclarationElementList Elements;
		uint32 Stride = sizeof(FTextureVertex);
		Elements.Add(FVertexElement(0, STRUCT_OFFSET(FTextureVertex, Position), VET_Float2, 0, Stride));
		Elements.Add(FVertexElement(0, STRUCT_OFFSET(FTextureVertex, UV), VET_Float2, 1, Stride));
		VertexDeclarationRHI = RHICreateVertexDeclaration(Elements);
	}

	virtual void ReleaseRHI() override
	{
		VertexDeclarationRHI.SafeRelease();
	}
};

TGlobalResource<FQuadVertexBuffer> QuadVertex;
TGlobalResource<FQuadIndexBuffer> QuadIndex;
TGlobalResource<FTextureVertexDeclaration> VertexDeclaration;

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

		// Get Data
		//FQuadVertexBuffer quadVertex;
		//quadVertex.InitRHI();
		//FQuadIndexBuffer quadIndex;
		//quadIndex.InitRHI();

		// Texture vertex
		//FTextureVertexDeclaration VertexDeclaration;
		//VertexDeclaration.InitRHI();

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
