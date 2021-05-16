#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "MyShaderTest.generated.h"

UCLASS(MinimalAPI, meta=(ScriptName="CustomRenderingLibary"))
class UMyShaderTestBlueprintLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

	UFUNCTION(BlueprintCallable, Category = "CustomRenderingPlugin", meta = (WorldContext = "WorldContextObject"))
		static void DrawMyShaderTestRenderTarget(class UTextureRenderTarget2D* OutputRenderTarget, AActor * Actor, FLinearColor myColor);
};
