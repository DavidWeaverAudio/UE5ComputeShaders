#pragma once

#include "CoreMinimal.h"
#include "GenericPlatform/GenericPlatformMisc.h"
#include "Kismet/BlueprintAsyncActionBase.h"

#include "MySimpleComputeShader.generated.h"

struct FMySimpleComputeShaderDispatchParams
{
	int X;
	int Y;
	int Z;

	int Input[2];
	int Output;

	FMySimpleComputeShaderDispatchParams(int InX, int InY, int InZ)
		: X(InX), Y(InY), Z(InZ)
	{
	}
};

class FMySimpleComputeShaderInterface {
public:
	static void DispatchRenderThread(FRHICommandListImmediate& RHICmdList, FMySimpleComputeShaderDispatchParams Params, TFunction<void(int OutputVal)> AsyncCallback);

	static void DispatchGameThread(
		FMySimpleComputeShaderDispatchParams Params, 
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
		FMySimpleComputeShaderDispatchParams Params,
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

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMySimpleComputeShaderLibrary_AsyncExecutionCompleted, const int, Value);

UCLASS()
class UMySimpleComputeShaderLibrary_AsyncExecution : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()

public:
	virtual void Activate() override {
		FMySimpleComputeShaderDispatchParams Params(1, 1, 1);
		Params.Input[0] = Arg1;
		Params.Input[1] = Arg2;
		FMySimpleComputeShaderInterface::Dispatch(Params, [this](int OutputVal) {
			this->Completed.Broadcast(OutputVal);
		});
	}

	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", Category = "ComputeShader", WorldContext = "WorldContextObject"))
	static UMySimpleComputeShaderLibrary_AsyncExecution* ExecuteBaseComputeShader(UObject* WorldContextObject, int Arg1, int Arg2)
	{
		UMySimpleComputeShaderLibrary_AsyncExecution* Action = NewObject<UMySimpleComputeShaderLibrary_AsyncExecution>();
		Action->Arg1 = Arg2;
		Action->Arg2 = Arg2;
		Action->RegisterWithGameInstance(WorldContextObject);
		return Action;
	}

	UPROPERTY(BlueprintAssignable)
	FOnMySimpleComputeShaderLibrary_AsyncExecutionCompleted Completed;

	int Arg1;
	int Arg2;
};