#include "MyShaderTest.h"
#include "Engine/TextureRenderTarget2D.h"
#include "PipelineStateCache.h"
#include "GlobalShader.h"

#define LOCTEXT_NAMESPACE "CustomRenderingModul"

//-----------------------------
// Uniform Buffer(Const Buffer)
//-----------------------------
BEGIN_GLOBAL_SHADER_PARAMETER_STRUCT(FMyShaderParameterBuffer, )
SHADER_PARAMETER(FVector4, MyColor1)
SHADER_PARAMETER(FVector4, MyColor2)
SHADER_PARAMETER(FVector4, MyColor3)
SHADER_PARAMETER(FVector4, MyColor4)
SHADER_PARAMETER(uint32, ColorIndex)
END_GLOBAL_SHADER_PARAMETER_STRUCT()

IMPLEMENT_GLOBAL_SHADER_PARAMETER_STRUCT(FMyShaderParameterBuffer, "MyShaderParameter");


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
	void SetParameters(FRHICommandListImmediate& RHICmdList, TRHIShader Shader, const FLinearColor& MyColor, FTexture2DRHIRef MyTexture)
	{
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

IMPLEMENT_SHADER_TYPE(, FMyShaderTestVS, TEXT("/MyShaders/Private/MyTestShader.usf"), TEXT("MainVS"), SF_Vertex)
IMPLEMENT_SHADER_TYPE(, FMyShaderTestPS, TEXT("/MyShaders/Private/MyTestShader.usf"), TEXT("MainPS"), SF_Pixel)


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
		VertexDeclarationRHI->Release();
	}
};

//-----------------------------
// Blueprint
//-----------------------------

static void DrawMyShaderTestRenderTarget_RenderThread(FRHICommandListImmediate& RHICmdList, FTextureRenderTargetResource* OutputRenderTargetResource, ERHIFeatureLevel::Type FeatureLevel, FLinearColor MyColor, FTextureRHIRef MyTexture, FMyShaderParameterStruct MyShaderStructData, FName TextureRenderTargetName)
{
	check(IsInRenderingThread());

#if WANTS_DRAW_MESH_EVENTS
	FString EventName;
	TextureRenderTargetName.ToString(EventName);
	SCOPED_DRAW_EVENTF(RHICmdList, SceneCapture, TEXT("ShaderTest %s"), *EventName);
#else
	SCOPED_DRAW_EVENT(RHICmdList, DrawMyShaderTestRenderTarget_RenderThread);
#endif

	// Set Render Target
	FTexture2DRHIRef RenderTargetTexture = OutputRenderTargetResource->GetRenderTargetTexture();

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
		FQuadVertexBuffer quadVertex;
		quadVertex.InitRHI();
		FQuadIndexBuffer quadIndex;
		quadIndex.InitRHI();

		// Texture vertex
		FTextureVertexDeclaration VertexDeclaration;
		VertexDeclaration.InitRHI();

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

		// Set uniform buffer
		FMyShaderParameterBuffer MyShaderParameter;
		MyShaderParameter.MyColor1 = MyShaderStructData.MyColor1;
		MyShaderParameter.MyColor2 = MyShaderStructData.MyColor2;
		MyShaderParameter.MyColor3 = MyShaderStructData.MyColor3;
		MyShaderParameter.MyColor4 = MyShaderStructData.MyColor4;
		MyShaderParameter.ColorIndex = MyShaderStructData.ColorIndex;

		// Update shader uniform parameters.
		SetUniformBufferParameterImmediate(RHICmdList, PixelShader.GetPixelShader(), PixelShader->GetUniformBufferParameter<FMyShaderParameterBuffer>(), MyShaderParameter);
		VertexShader->SetParameters(RHICmdList, PixelShader.GetPixelShader(), MyColor, MyTexture->GetTexture2D());
		PixelShader->SetParameters(RHICmdList, PixelShader.GetPixelShader(), MyColor, MyTexture->GetTexture2D());

		RHICmdList.SetStreamSource(0, quadVertex.VertexBufferRHI, 0);

		// Draw
		RHICmdList.DrawIndexedPrimitive(quadIndex.IndexBufferRHI, 0, 0, 4, 0, 2, 1);
	}
	RHICmdList.EndRenderPass();
}

void UMyShaderTestBlueprintLibrary::DrawMyShaderTestRenderTarget(
	class UTextureRenderTarget2D* OutputRenderTarget,
	AActor* Actor,
	FLinearColor MyColor,
	UTexture* MyTexture,
	FMyShaderParameterStruct MyShaderStructData
)
{
	check(IsInGameThread());

	if (!OutputRenderTarget)
	{
		return;
	}

	const FName TextureRenderTargetName = OutputRenderTarget->GetFName();
	FTextureRenderTargetResource* TextureRenderTargetResource = OutputRenderTarget->GameThread_GetRenderTargetResource();

	FTextureRHIRef MyTextureRHI = MyTexture->TextureReference.TextureReferenceRHI;

	ERHIFeatureLevel::Type FeatureLevel = Actor->GetWorld()->Scene->GetFeatureLevel();

	// Put render command into render thread
	ENQUEUE_RENDER_COMMAND(CaptureCommand)(
		[TextureRenderTargetResource, FeatureLevel, MyColor, MyTextureRHI, MyShaderStructData, TextureRenderTargetName](FRHICommandListImmediate& RHICmdList)
		{
			DrawMyShaderTestRenderTarget_RenderThread(RHICmdList, TextureRenderTargetResource, FeatureLevel, MyColor, MyTextureRHI, MyShaderStructData, TextureRenderTargetName);
		}
	);
}

#undef LOCTEXT_NAMESPACE
