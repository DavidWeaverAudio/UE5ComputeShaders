#pragma once

#include "CoreMinimal.h"
#include "GenericPlatform/GenericPlatformMisc.h"
#include "Kismet/BlueprintAsyncActionBase.h"

#include "MyStructComputeShader.generated.h"

struct FMyStructComputeShaderDispatchParams
{
	int X;
	int Y;
	int Z;

	TArray<FVector3f> Input;
	int Output;
	int InputNum;  
	FVector3f InputTarget;

	FMyStructComputeShaderDispatchParams(int InX, int InY, int InZ)
		: X(InX), Y(InY), Z(InZ)
	{
	}
};

class FMyStructComputeShaderInterface {
public:
	static void DispatchRenderThread(FRHICommandListImmediate& RHICmdList, FMyStructComputeShaderDispatchParams Params, TFunction<void(int OutputVal)> AsyncCallback);

	static void DispatchGameThread(
		FMyStructComputeShaderDispatchParams Params,
		TFunction<void(int OutputVal)> AsyncCallback
	)
	{
		ENQUEUE_RENDER_COMMAND(SceneDrawCompletion)(
			[Params, AsyncCallback](FRHICommandListImmediate& RHICmdList)
			{
				DispatchRenderThread(RHICmdList, Params, AsyncCallback);
			});
	}
	static void Dispatch(
		FMyStructComputeShaderDispatchParams Params,
		TFunction<void(int OutputVal)> AsyncCallback
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

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMyStructComputeShaderLibrary_AsyncExecutionCompleted, const int, Value);

UCLASS()
class UMyStructComputeShaderLibrary_AsyncExecution : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()

public:
	virtual void Activate() override {
		FMyStructComputeShaderDispatchParams Params(1, 1, 1);
		Params.Input = Args;
		Params.InputNum = Args.Num();
		Params.InputTarget = InputTarget;

		FMyStructComputeShaderInterface::Dispatch(Params, [this](int OutputVal) {
			this->Completed.Broadcast(OutputVal);
			});
	}

	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", Category = "ComputeShader", WorldContext = "WorldContextObject"))
	static UMyStructComputeShaderLibrary_AsyncExecution* ExecuteBaseComputeShader(UObject* WorldContextObject, TArray<FVector3f> Args, FVector3f InputTarget)
	{
		UMyStructComputeShaderLibrary_AsyncExecution* Action = NewObject<UMyStructComputeShaderLibrary_AsyncExecution>();
		Action->Args = Args;
		Action->InputTarget = InputTarget;
		Action->RegisterWithGameInstance(WorldContextObject);
		return Action;
	}

	UPROPERTY(BlueprintAssignable)
	FOnMyStructComputeShaderLibrary_AsyncExecutionCompleted Completed;

	TArray<FVector3f> Args;
	FVector3f InputTarget;
};