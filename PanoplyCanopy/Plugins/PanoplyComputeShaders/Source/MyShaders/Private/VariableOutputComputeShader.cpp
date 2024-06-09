#include "VariableOutputComputeShader.h"
#include "MyShaders/Public/VariableOutputComputeShader.h"
#include "PixelShaderUtils.h"
#include "MeshPassProcessor.inl"
#include "StaticMeshResources.h"
#include "DynamicMeshBuilder.h"
#include "RHIGPUReadback.h"
#include "GlobalShader.h"
#include "UnifiedBuffer.h"
#include "CanvasTypes.h"
#include "MaterialShader.h"

DECLARE_STATS_GROUP(TEXT("VariableOutputComputeShader"), STATGROUP_VariableOutputComputeShader, STATCAT_Advanced);
DECLARE_CYCLE_STAT(TEXT("VariableOutputComputeShader Execute"), STAT_VariableOutputComputeShader_Execute, STATGROUP_VariableOutputComputeShader);

// This class carries our parameter declarations and acts as the bridge between cpp and HLSL.
class MYSHADERS_API FVariableOutputComputeShader : public FGlobalShader
{
public:

	DECLARE_GLOBAL_SHADER(FVariableOutputComputeShader);
	SHADER_USE_PARAMETER_STRUCT(FVariableOutputComputeShader, FGlobalShader);
	class FVariableOutputComputeShader_Perm_TEST : SHADER_PERMUTATION_INT("TEST", 1);
	using FPermutationDomain = TShaderPermutationDomain<FVariableOutputComputeShader_Perm_TEST>;

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER(FVector3f, Input)
		SHADER_PARAMETER(FVector3f, SurfaceNormal)
		SHADER_PARAMETER(int, TotalOutputs)
		SHADER_PARAMETER_RDG_BUFFER_UAV_ARRAY(RWBuffer<float3>, Output, [16])
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

		OutEnvironment.SetDefine(TEXT("THREADS_X"), NUM_THREADS_VariableOutputComputeShader_X);
		OutEnvironment.SetDefine(TEXT("THREADS_Y"), NUM_THREADS_VariableOutputComputeShader_Y);
		OutEnvironment.SetDefine(TEXT("THREADS_Z"), NUM_THREADS_VariableOutputComputeShader_Z);
	}
private:
};

// This will tell the engine to create the shader and where the shader entry point is.
//                            ShaderType                            ShaderPath                     Shader function name    Type
IMPLEMENT_GLOBAL_SHADER(FVariableOutputComputeShader, "/MyShadersShaders/VariableOutputComputeShader.usf", "VariableOutputComputeShader", SF_Compute);

void FVariableOutputComputeShaderInterface::DispatchRenderThread(FRHICommandListImmediate& RHICmdList, FVariableOutputComputeShaderDispatchParams Params, TFunction<void(TArray<FVector3f> OutputVal)> AsyncCallback) {
	FRDGBuilder GraphBuilder(RHICmdList);
	{
		SCOPE_CYCLE_COUNTER(STAT_VariableOutputComputeShader_Execute);
		DECLARE_GPU_STAT(VariableOutputComputeShader)
		RDG_EVENT_SCOPE(GraphBuilder, "VariableOutputComputeShader");
		RDG_GPU_STAT_SCOPE(GraphBuilder, VariableOutputComputeShader);

		typename FVariableOutputComputeShader::FPermutationDomain PermutationVector;
		TShaderMapRef<FVariableOutputComputeShader> ComputeShader(GetGlobalShaderMap(GMaxRHIFeatureLevel), PermutationVector);
		bool bIsShaderValid = ComputeShader.IsValid();

		if (bIsShaderValid) {
			FVariableOutputComputeShader::FParameters* PassParameters = GraphBuilder.AllocParameters<FVariableOutputComputeShader::FParameters>();

			PassParameters->Input = Params.Input;
			PassParameters->SurfaceNormal = Params.SurfaceNormal;
			PassParameters->TotalOutputs = Params.TotalOutputs;

			TArray<FRDGBufferRef> OutputBuffers;
			TArray<FRHIGPUBufferReadback*> GPUBufferReadbacks;

			OutputBuffers.Reserve(16);
			GPUBufferReadbacks.Reserve(16);

			for (int i = 0; i < 16; i++) {
				const TCHAR* OutputName = *FString::Printf(TEXT("Output%d"), i);
				FRDGBufferRef OutputBuffer = GraphBuilder.CreateBuffer(FRDGBufferDesc::CreateBufferDesc(sizeof(FVector3f), 1), OutputName);
				OutputBuffers.Add(OutputBuffer);
				PassParameters->Output[i] = GraphBuilder.CreateUAV(OutputBuffer);

				FRHIGPUBufferReadback* GPUBufferReadback = new FRHIGPUBufferReadback(OutputName);
				GPUBufferReadbacks.Add(GPUBufferReadback);
				AddEnqueueCopyPass(GraphBuilder, GPUBufferReadback, OutputBuffer, 0);
			}

			FComputeShaderUtils::AddPass(
				GraphBuilder,
				RDG_EVENT_NAME("ExecuteVariableOutputComputeShader"),
				ComputeShader,
				PassParameters,
				FComputeShaderUtils::GetGroupCount(FIntVector(Params.X, Params.Y, Params.Z), FComputeShaderUtils::kGolden2DGroupSize)
			);
			
			auto RunnerFunc = [GPUBufferReadbacks, AsyncCallback](auto&& RunnerFunc) -> void {
				bool bAllReady = true;
				TArray<FVector3f> AllOutputs;

				for (FRHIGPUBufferReadback* GPUBufferReadback : GPUBufferReadbacks) {
					if (GPUBufferReadback->IsReady()) {
						FVector3f* Buffer = (FVector3f*)GPUBufferReadback->Lock(1);
						AllOutputs.Add(*Buffer);
						GPUBufferReadback->Unlock();
					}
					else {
						bAllReady = false;
						break;
					}
				}

				if (bAllReady) {
					AsyncTask(ENamedThreads::GameThread, [AsyncCallback, AllOutputs]() {
						AsyncCallback(AllOutputs);
						});
					for (FRHIGPUBufferReadback* GPUBufferReadback : GPUBufferReadbacks) {
						delete GPUBufferReadback;
					}
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
