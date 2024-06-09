#pragma once

#include "CoreMinimal.h"
#include "GenericPlatform/GenericPlatformMisc.h"
#include "Kismet/BlueprintAsyncActionBase.h"

#include "VariableOutputComputeShader.generated.h"

struct FVariableOutputComputeShaderDispatchParams
{
	int X;
	int Y;
	int Z;

	FVector3f Input;
	FVector3f SurfaceNormal;
	int TotalOutputs;
	TArray<FVector3f> Output;

	FVariableOutputComputeShaderDispatchParams(int InX, int InY, int InZ)
		: X(InX), Y(InY), Z(InZ)
	{
	}
};

class FVariableOutputComputeShaderInterface {
public:
	static void DispatchRenderThread(FRHICommandListImmediate& RHICmdList, FVariableOutputComputeShaderDispatchParams Params, TFunction<void(TArray<FVector3f> OutputVal)> AsyncCallback);

	static void DispatchGameThread(
		FVariableOutputComputeShaderDispatchParams Params,
		TFunction<void(TArray<FVector3f> OutputVal)> AsyncCallback
	)
	{
		ENQUEUE_RENDER_COMMAND(SceneDrawCompletion)(
			[Params, AsyncCallback](FRHICommandListImmediate& RHICmdList)
			{
				DispatchRenderThread(RHICmdList, Params, AsyncCallback);
			});
	}
	static void Dispatch(
		FVariableOutputComputeShaderDispatchParams Params,
		TFunction<void(TArray<FVector3f> OutputVal)> AsyncCallback
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

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnVariableOutput_AsyncExecutionCompleted, const TArray<FVector3f>&, Value);

UCLASS()
class UVariableOutput_AsyncExecution : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()

public:
	virtual void Activate() override {
		FVariableOutputComputeShaderDispatchParams Params(1, 1, 1);
		Params.Input = Input;
		Params.SurfaceNormal = SurfaceNormal;
		Params.TotalOutputs = TotalOutputs;
		Params.Output.SetNumZeroed(TotalOutputs);
		FVariableOutputComputeShaderInterface::Dispatch(Params, [this](TArray<FVector3f> OutputVal) {
			this->Completed.Broadcast(OutputVal);
		});
	}

	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", Category = "ComputeShader", WorldContext = "WorldContextObject"))
	static UVariableOutput_AsyncExecution* ExecuteVariableOutputComputeShader(UObject* WorldContextObject, FVector3f Input, FVector3f SurfaceNormal, int TotalOutputs) {
		UVariableOutput_AsyncExecution* Action = NewObject<UVariableOutput_AsyncExecution>();
		Action->Input = Input;
		Action->SurfaceNormal = SurfaceNormal;
		Action->TotalOutputs = TotalOutputs;
		Action->RegisterWithGameInstance(WorldContextObject);
		return Action;
	}

	UPROPERTY(BlueprintAssignable)
	FOnVariableOutput_AsyncExecutionCompleted Completed;

	FVector3f Input;
	FVector3f SurfaceNormal;
	int TotalOutputs;
};