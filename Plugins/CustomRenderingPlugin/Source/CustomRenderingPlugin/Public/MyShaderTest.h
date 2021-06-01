#pragma once
#include "RenderGraph.h"
#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "CustomRenderingPlugin.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "MyShaderTest.generated.h"

USTRUCT(BlueprintType)
struct FMyShaderParameterStruct
{
	GENERATED_USTRUCT_BODY()

		UPROPERTY(BlueprintReadWrite, VisibleAnywhere, meta = (WorldContext = "WorldContextObject"))
		FLinearColor MyColor1;

	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, meta = (WorldContext = "WorldContextObject"))
		FLinearColor MyColor2;

	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, meta = (WorldContext = "WorldContextObject"))
		FLinearColor MyColor3;

	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, meta = (WorldContext = "WorldContextObject"))
		FLinearColor MyColor4;

	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, meta = (WorldContext = "WorldContextObject"))
		int32 ColorIndex;
};

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

//-----------------------------
// Blueprint Function
//-----------------------------
UCLASS(MinimalAPI, meta=(ScriptName="CustomRenderingLibary"))
class UMyShaderTestBlueprintLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

	UFUNCTION(BlueprintCallable, Category = "CustomRenderingPlugin", meta = (WorldContext = "WorldContextObject"))
	static void DrawMyShaderTestRenderTarget(
		class UTextureRenderTarget2D* OutputRenderTarget,
		AActor * Actor,
		FLinearColor MyColor,
		UTexture* MyTexture,
		FMyShaderParameterStruct MyShaderParameter
	);

	UFUNCTION(BlueprintCallable, Category = "CustomRenderingPlugin", meta = (WorldContext = "WorldContextObject"))
	static void DrawMyComputeShader(class UTextureRenderTarget2D* OutputRenderTarget, FMyShaderParameterStruct MyShaderParameter);

	UFUNCTION(BlueprintCallable, Category = "CustomRenderingPlugin", meta = (WorldContext = "WorldContextObject"))
		static void DrawMyTestShaderWithRDG(class UTextureRenderTarget2D* OutputRenderTarget, FMyShaderParameterStruct MyShaderParameter);
};

//-----------------------------
// Data struct
//-----------------------------
struct FTextureVertex
{
	FVector4    Position;
	FVector2D   UV;
};

// FScreenRectangleVertexBuffer in CommonRenderResources.cpp
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

// FScreenRectangleIndexBuffer in CommonRenderResources.cpp
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

// FFilterVertexDeclaration in CommonRenderResources.h
class FTextureVertexDeclaration : public FRenderResource
{
public:
	FVertexDeclarationRHIRef  VertexDeclarationRHI;

	virtual void InitRHI() override
	{
		FVertexDeclarationElementList Elements;
		uint32 Stride = sizeof(FTextureVertex);
		Elements.Add(FVertexElement(0, STRUCT_OFFSET(FTextureVertex, Position), VET_Float4, 0, Stride));
		Elements.Add(FVertexElement(0, STRUCT_OFFSET(FTextureVertex, UV), VET_Float2, 1, Stride));
		VertexDeclarationRHI = RHICreateVertexDeclaration(Elements);
	}

	virtual void ReleaseRHI() override
	{
		VertexDeclarationRHI.SafeRelease();
	}
};