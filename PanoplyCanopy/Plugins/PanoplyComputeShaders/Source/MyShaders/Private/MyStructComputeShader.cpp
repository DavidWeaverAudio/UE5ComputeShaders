#include "MyStructComputeShader.h"
#include "MyShaders/Public/MyStructComputeShader.h"
#include "PixelShaderUtils.h"
#include "MeshPassProcessor.inl"
#include "StaticMeshResources.h"
#include "DynamicMeshBuilder.h"
#include "RenderGraphResources.h"
#include "RHIGPUReadback.h"
#include "GlobalShader.h"
#include "UnifiedBuffer.h"
#include "CanvasTypes.h"
#include "MaterialShader.h"

DECLARE_STATS_GROUP(TEXT("MyStructComputeShader"), STATGROUP_MyStructComputeShader, STATCAT_Advanced);
DECLARE_CYCLE_STAT(TEXT("MyStructComputeShader Execute"), STAT_MyStructComputeShader_Execute, STATGROUP_MyStructComputeShader);

// This class carries our parameter declarations and acts as the bridge between cpp and HLSL.
class MYSHADERS_API FMyStructComputeShader : public FGlobalShader
{
public:

	DECLARE_GLOBAL_SHADER(FMyStructComputeShader);
	SHADER_USE_PARAMETER_STRUCT(FMyStructComputeShader, FGlobalShader);
	class FMyStructComputeShader_Perm_TEST : SHADER_PERMUTATION_INT("TEST", 1);
	using FPermutationDomain = TShaderPermutationDomain<FMyStructComputeShader_Perm_TEST>;

#pragma region Examples
	// SHADER_PARAMETER_SRV(Buffer<FMyCustomStruct>, MyCustomStructs) // On the shader side: Buffer<FMyCustomStruct> MyCustomStructs;
#pragma endregion
	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_RDG_BUFFER_SRV(Buffer<FVector3f>, Input)
		SHADER_PARAMETER_RDG_BUFFER_SRV(Buffer <FVector3f>, InputTarget)
		SHADER_PARAMETER_RDG_BUFFER_UAV(RWBuffer<float>, Output)
		SHADER_PARAMETER(int, NumInputs)
		END_SHADER_PARAMETER_STRUCT()

public:
	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		const FPermutationDomain PermutationVector(Parameters.PermutationId);
		return true;
	}

	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
		const FPermutationDomain PermutationVector(Parameters.PermutationId);

		OutEnvironment.SetDefine(TEXT("THREADS_X"), NUM_THREADS_MyStructComputeShader_X);
		OutEnvironment.SetDefine(TEXT("THREADS_Y"), NUM_THREADS_MyStructComputeShader_Y);
		OutEnvironment.SetDefine(TEXT("THREADS_Z"), NUM_THREADS_MyStructComputeShader_Z);
	}
private:
};

// This will tell the engine to create the shader and where the shader entry point is.
//                            ShaderType                            ShaderPath                     Shader function name    Type
IMPLEMENT_GLOBAL_SHADER(FMyStructComputeShader, "/MyShadersShaders/MyStructComputeShader.usf", "MyStructComputeShader", SF_Compute);

void FMyStructComputeShaderInterface::DispatchRenderThread(FRHICommandListImmediate& RHICmdList, FMyStructComputeShaderDispatchParams Params, TFunction<void(int OutputVal)> AsyncCallback) {
	FRDGBuilder GraphBuilder(RHICmdList);
	{
		SCOPE_CYCLE_COUNTER(STAT_MyStructComputeShader_Execute);
		DECLARE_GPU_STAT(MyStructComputeShader)
		RDG_EVENT_SCOPE(GraphBuilder, "MyStructComputeShader");
		RDG_GPU_STAT_SCOPE(GraphBuilder, MyStructComputeShader);

		typename FMyStructComputeShader::FPermutationDomain PermutationVector;
		TShaderMapRef<FMyStructComputeShader> ComputeShader(GetGlobalShaderMap(GMaxRHIFeatureLevel), PermutationVector);
		bool bIsShaderValid = ComputeShader.IsValid();

		if (bIsShaderValid) {
			FMyStructComputeShader::FParameters* PassParameters = GraphBuilder.AllocParameters<FMyStructComputeShader::FParameters>();

			const void* RawData = (void*)Params.Input.GetData();
			int NumInputs = Params.Input.Num();
			FVector3f InputTarget = Params.InputTarget;
			int InputSize = sizeof(FVector3f);
			FRDGBufferRef InputBuffer = CreateUploadBuffer(GraphBuilder, TEXT("InputBuffer"), InputSize, NumInputs, RawData, InputSize * NumInputs);
			FRDGBufferRef InputTargetBuffer = CreateUploadBuffer(GraphBuilder, TEXT("InputTargetBuffer"), sizeof(FVector3f), 1, &InputTarget, sizeof(FVector3f));
			PassParameters->Input = GraphBuilder.CreateSRV(FRDGBufferSRVDesc(InputBuffer, PF_R32G32B32F));
			PassParameters->NumInputs = NumInputs;
			PassParameters->InputTarget = GraphBuilder.CreateSRV(FRDGBufferSRVDesc(InputTargetBuffer, PF_R32G32B32F));

			FRDGBufferRef OutputBuffer = GraphBuilder.CreateBuffer(
				FRDGBufferDesc::CreateBufferDesc(sizeof(int32), 1),
				TEXT("OutputBuffer"));
			PassParameters->Output = GraphBuilder.CreateUAV(FRDGBufferUAVDesc(OutputBuffer, PF_R32_SINT));

			auto GroupCount = FComputeShaderUtils::GetGroupCount(FIntVector(Params.X, Params.Y, Params.Z), FComputeShaderUtils::kGolden2DGroupSize);
			GraphBuilder.AddPass(
				RDG_EVENT_NAME("ExecuteMyStructComputeShader"),
				PassParameters,
				ERDGPassFlags::AsyncCompute,
				[&PassParameters, ComputeShader, GroupCount](FRHIComputeCommandList& RHICmdList)
				{
					FComputeShaderUtils::Dispatch(RHICmdList, ComputeShader, *PassParameters, GroupCount);
				});

			FRHIGPUBufferReadback* GPUBufferReadback = new FRHIGPUBufferReadback(TEXT("ExecuteMyStructComputeShaderOutput"));
			AddEnqueueCopyPass(GraphBuilder, GPUBufferReadback, OutputBuffer, 0u);

			auto RunnerFunc = [GPUBufferReadback, AsyncCallback](auto&& RunnerFunc) -> void {
				if (GPUBufferReadback->IsReady()) {
					int32* Buffer = (int32*)GPUBufferReadback->Lock(1);
					int OutVal = Buffer[0];
					GPUBufferReadback->Unlock();
					AsyncTask(ENamedThreads::GameThread, [AsyncCallback, OutVal]() {
						AsyncCallback(OutVal);
						});
					delete GPUBufferReadback;
				}
				else {
					AsyncTask(ENamedThreads::ActualRenderingThread, [RunnerFunc]() {
						RunnerFunc(RunnerFunc);
						});
				}
				};

			AsyncTask(ENamedThreads::ActualRenderingThread, [RunnerFunc]() {
				RunnerFunc(RunnerFunc);
				});

		}
		else {
			// We silently exit here as we don't want to crash the game if the shader is not found or has an error.

		}
	}

	GraphBuilder.Execute();
}