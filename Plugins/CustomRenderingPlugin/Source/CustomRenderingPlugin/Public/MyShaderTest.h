#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
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
};
