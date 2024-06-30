#pragma once

#include "CoreMinimal.h"
#include "GenericPlatform/GenericPlatformMisc.h"
#include "Kismet/BlueprintAsyncActionBase.h"

#include "VariableInputComputeShader.generated.h"

struct FVariableInputComputeShaderDispatchParams
{
	int X;
	int Y;
	int Z;

	TArray<FVector3f> Input;
	int TotalInputs;
	int Output;
	FVector3f InputTarget;

	FVariableInputComputeShaderDispatchParams(int InX, int InY, int InZ)
		: X(InX), Y(InY), Z(InZ), TotalInputs(0), Output(0)
	{
	}
};

class FVariableInputComputeShaderInterface {
public:
	static void DispatchRenderThread(FRHICommandListImmediate& RHICmdList, FVariableInputComputeShaderDispatchParams Params, TFunction<void(int OutputVal)> AsyncCallback);

	static void DispatchGameThread(
		FVariableInputComputeShaderDispatchParams Params,
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
		FVariableInputComputeShaderDispatchParams Params,
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

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnVariableInput_AsyncExecutionCompleted, const int, Value);

UCLASS()
class UVariableInput_AsyncExecution : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()

public:
	virtual void Activate() override {
		FVariableInputComputeShaderDispatchParams Params(1, 1, 1);
		Params.Input.SetNumZeroed(Args.Num());
		for (int i = 0; i < Args.Num(); i++) {
			Params.Input[i].Set(Args[i].X, Args[i].Y, Args[i].Z);
		}
		Params.InputTarget = InputTarget;
		Params.TotalInputs = Args.Num();
		FVariableInputComputeShaderInterface::Dispatch(Params, [this](int OutputVal) {
			this->Completed.Broadcast(OutputVal);
			});
	}

	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", Category = "ComputeShader", WorldContext = "WorldContextObject"))
	static UVariableInput_AsyncExecution* ExecuteVariableInputComputeShader(UObject* WorldContextObject, TArray<FVector3f> Args, FVector3f InputTarget)
	{
		UVariableInput_AsyncExecution* Action = NewObject<UVariableInput_AsyncExecution>();
		Action->Args = Args;
		Action->InputTarget = InputTarget;
		Action->RegisterWithGameInstance(WorldContextObject);
		return Action;
	}

	UPROPERTY(BlueprintAssignable)
	FOnVariableInput_AsyncExecutionCompleted Completed;

	TArray<FVector3f> Args;
	FVector3f InputTarget;
};