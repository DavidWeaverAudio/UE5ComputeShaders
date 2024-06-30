#include "VariableInputComputeShader.h"
#include "ComputeShaders/Public/VariableInputComputeShader.h"
#include "PixelShaderUtils.h"
#include "MeshPassProcessor.inl"
#include "StaticMeshResources.h"
#include "DynamicMeshBuilder.h"
#include "RHIGPUReadback.h"
#include "GlobalShader.h"
#include "UnifiedBuffer.h"
#include "CanvasTypes.h"
#include "MaterialShader.h"

DECLARE_STATS_GROUP(TEXT("VariableInputComputeShader"), STATGROUP_VariableInputComputeShader, STATCAT_Advanced);
DECLARE_CYCLE_STAT(TEXT("VariableInputComputeShader Execute"), STAT_VariableInputComputeShader_Execute, STATGROUP_VariableInputComputeShader);

// This class carries our parameter declarations and acts as the bridge between cpp and HLSL.
class COMPUTESHADERS_API FVariableInputComputeShader : public FGlobalShader
{
public:

	DECLARE_GLOBAL_SHADER(FVariableInputComputeShader);
	SHADER_USE_PARAMETER_STRUCT(FVariableInputComputeShader, FGlobalShader);
	class FVariableInputComputeShader_Perm_TEST : SHADER_PERMUTATION_INT("TEST", 1);
	using FPermutationDomain = TShaderPermutationDomain<FVariableInputComputeShader_Perm_TEST>;

    BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_RDG_BUFFER_SRV(Buffer<float3>, Input)
		SHADER_PARAMETER_RDG_BUFFER_UAV(RWBuffer<float>, Output)
		SHADER_PARAMETER(int, TotalInputs)
		SHADER_PARAMETER(FVector3f, InputTarget)
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
		
		OutEnvironment.SetDefine(TEXT("THREADS_X"), NUM_THREADS_VariableInputComputeShader_X);
		OutEnvironment.SetDefine(TEXT("THREADS_Y"), NUM_THREADS_VariableInputComputeShader_Y);
		OutEnvironment.SetDefine(TEXT("THREADS_Z"), NUM_THREADS_VariableInputComputeShader_Z);
	}
private:
};

// This will tell the engine to create the shader and where the shader entry point is.
//                            ShaderType                            ShaderPath                     Shader function name    Type
IMPLEMENT_GLOBAL_SHADER(FVariableInputComputeShader, "/ComputeShaders/VariableInputComputeShader.usf", "VariableInputComputeShader", SF_Compute);

void FVariableInputComputeShaderInterface::DispatchRenderThread(FRHICommandListImmediate& RHICmdList, FVariableInputComputeShaderDispatchParams Params, TFunction<void(int OutputVal)> AsyncCallback) {
	FRDGBuilder GraphBuilder(RHICmdList);
	{
		SCOPE_CYCLE_COUNTER(STAT_VariableInputComputeShader_Execute);
		DECLARE_GPU_STAT(VariableInputComputeShader)
		RDG_EVENT_SCOPE(GraphBuilder, "VariableInputComputeShader");
		RDG_GPU_STAT_SCOPE(GraphBuilder, VariableInputComputeShader);

		typename FVariableInputComputeShader::FPermutationDomain PermutationVector;
		TShaderMapRef<FVariableInputComputeShader> ComputeShader(GetGlobalShaderMap(GMaxRHIFeatureLevel), PermutationVector);
		bool bIsShaderValid = ComputeShader.IsValid();

		if (bIsShaderValid) {
			FVariableInputComputeShader::FParameters* PassParameters = GraphBuilder.AllocParameters<FVariableInputComputeShader::FParameters>();
			
			const void* RawData = (void*)Params.Input.GetData();

			int NumInputs = Params.TotalInputs;
			int InputSize = sizeof(FVector3f);
			FRDGBufferRef InputBuffer = CreateUploadBuffer(GraphBuilder, TEXT("InputBuffer"), InputSize, NumInputs, RawData, InputSize * NumInputs);
			PassParameters->Input = GraphBuilder.CreateSRV(FRDGBufferSRVDesc(InputBuffer, PF_R32G32B32F));

			PassParameters->InputTarget = Params.InputTarget;
			FRDGBufferRef OutputBuffer = GraphBuilder.CreateBuffer(FRDGBufferDesc::CreateBufferDesc(sizeof(int32), 1),TEXT("OutputBuffer"));
			PassParameters->Output = GraphBuilder.CreateUAV(FRDGBufferUAVDesc(OutputBuffer, PF_R32_SINT));
			PassParameters->TotalInputs = NumInputs;

			auto GroupCount = FComputeShaderUtils::GetGroupCount(FIntVector(Params.X, Params.Y, Params.Z), FComputeShaderUtils::kGolden2DGroupSize);
			GraphBuilder.AddPass(
				RDG_EVENT_NAME("ExecuteVariableInputComputeShader"),
				PassParameters,
				ERDGPassFlags::AsyncCompute,
				[&PassParameters, ComputeShader, GroupCount](FRHIComputeCommandList& RHICmdList)
				{
					FComputeShaderUtils::Dispatch(RHICmdList, ComputeShader, *PassParameters, GroupCount);
				});

			FRHIGPUBufferReadback* GPUBufferReadback = new FRHIGPUBufferReadback(TEXT("ExecuteVariableInputComputeShaderOutput"));
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