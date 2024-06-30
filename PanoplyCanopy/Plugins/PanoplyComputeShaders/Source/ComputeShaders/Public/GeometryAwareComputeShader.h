//Compute shader that sends back a boolean when near an object in the world.

#pragma once

#include "CoreMinimal.h"
#include "GenericPlatform/GenericPlatformMisc.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "GlobalDistanceFieldParameters.h"

#include "GeometryAwareComputeShader.generated.h"

struct FGeometryAwareComputeShaderDispatchParams
{
	int X;
	int Y;
	int Z;

	FVector3f InputPosition;
	//Some geometry input somehow
	float SurfaceThreshold;
	int bNearGeometry;

	FGeometryAwareComputeShaderDispatchParams(int InX, int InY, int InZ)
		: X(InX), Y(InY), Z(InZ)
	{
	}
};

class FGeometryAwareComputeShaderInterface {
public:
	static void DispatchRenderThread(FRHICommandListImmediate& RHICmdList, FGeometryAwareComputeShaderDispatchParams Params, TFunction<void(int NearGeometry)> AsyncCallback);
	static void DispatchGameThread(FGeometryAwareComputeShaderDispatchParams Params,
		TFunction<void(int NearGeometry)> AsyncCallback
	)
	{
		ENQUEUE_RENDER_COMMAND(SceneDrawCompletion)(
			[Params, AsyncCallback](FRHICommandListImmediate& RHICmdList)
			{
				DispatchRenderThread(RHICmdList, Params, AsyncCallback);
			});
	}
	static void Dispatch(
		FGeometryAwareComputeShaderDispatchParams Params,
		TFunction<void(int NearGeometry)> AsyncCallback
	)
	{
		if (IsInRenderingThread()) {
			DispatchRenderThread(GetImmediateCommandList_ForRenderCommand(), Params, AsyncCallback);
		}
		else {
			DispatchGameThread(Params, AsyncCallback);
		}
	}
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnGeometryAware_AsyncExecutionCompleted, const int, NearGeometry);

UCLASS()
class UGeometryAwareComputeShaderLibrary_AsyncExecution : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()

public:
	virtual void Activate() override {
		FGeometryAwareComputeShaderDispatchParams Params(1, 1, 1);
		Params.InputPosition = InputPosition;
		Params.SurfaceThreshold = SurfaceThreshold;
		FGeometryAwareComputeShaderInterface::Dispatch(Params, [this](int NearGeometry) {
			this->OnAsyncExecutionCompleted.Broadcast(NearGeometry);
			});
	}
	UFUNCTION(BlueprintCallable, meta = (BlueprintInternaUseOnly = "true", Category = "ComputeShader", WorldContext = "WorldContextObject"))
	static UGeometryAwareComputeShaderLibrary_AsyncExecution* ComputeGeometryAwareShader(UObject* WorldContextObject, FVector3f InputPosition, float SurfaceThreshold) {
		UGeometryAwareComputeShaderLibrary_AsyncExecution* Action = NewObject<UGeometryAwareComputeShaderLibrary_AsyncExecution>();
		Action->InputPosition = InputPosition;
		Action->SurfaceThreshold = SurfaceThreshold;
		return Action;
	}

	UPROPERTY(BlueprintAssignable)
	FOnGeometryAware_AsyncExecutionCompleted OnAsyncExecutionCompleted;

	FVector3f InputPosition;
	float SurfaceThreshold;

};

