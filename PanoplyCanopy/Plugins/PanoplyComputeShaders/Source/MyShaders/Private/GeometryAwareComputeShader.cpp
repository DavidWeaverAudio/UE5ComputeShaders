#include "GeometryAwareComputeShader.h"
#include "MyShaders/Public/GeometryAwareComputeShader.h"
#include "PixelShaderUtils.h"
#include "MeshPassProcessor.inl"
#include "StaticMeshResources.h"
#include "DynamicMeshBuilder.h"
#include "RHIGPUReadback.h"
#include "GlobalShader.h"
#include "UnifiedBuffer.h"
#include "CanvasTypes.h"
#include "MaterialShader.h"

DECLARE_STATS_GROUP(TEXT("GeometryAwareComputeShader"), STATGROUP_GeometryAwareComputeShader, STATCAT_Advanced);
DECLARE_CYCLE_STAT(TEXT("GeometryAwareComputeShader Execute"), STAT_GeometryAwareComputeShader_Execute, STATGROUP_GeometryAwareComputeShader);

// This class carries our parameter declarations and acts as the bridge between cpp and HLSL.
class MYSHADERS_API FGeometryAwareComputeShader : public FGlobalShader
{
public:

	DECLARE_GLOBAL_SHADER(FGeometryAwareComputeShader);
	SHADER_USE_PARAMETER_STRUCT(FGeometryAwareComputeShader, FGlobalShader);
	class FGeometryAwareComputeShader_Perm_TEST : SHADER_PERMUTATION_INT("TEST", 1);
	using FPermutationDomain = TShaderPermutationDomain<FGeometryAwareComputeShader_Perm_TEST>;

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_RDG_BUFFER_SRV(Buffer<float3>, Input)
		SHADER_PARAMETER(float, SurfaceThreshold)
		SHADER_PARAMETER_RDG_BUFFER_UAV(RWBuffer<int32>, Output)
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

		OutEnvironment.SetDefine(TEXT("THREADS_X"), NUM_THREADS_GeometryAwareComputeShader_X);
		OutEnvironment.SetDefine(TEXT("THREADS_Y"), NUM_THREADS_GeometryAwareComputeShader_Y);
		OutEnvironment.SetDefine(TEXT("THREADS_Z"), NUM_THREADS_GeometryAwareComputeShader_Z);
	}
private:
};

// This will tell the engine to create the shader and where the shader entry point is.
//                            ShaderType                            ShaderPath                     Shader function name    Type
IMPLEMENT_GLOBAL_SHADER(FGeometryAwareComputeShader, "/MyShadersShaders/GeometryAwareComputeShader.usf", "GeometryAwareComputeShader", SF_Compute);

void FGeometryAwareComputeShaderInterface::DispatchRenderThread(FRHICommandListImmediate& RHICmdList, FGeometryAwareComputeShaderDispatchParams Params, TFunction<void(int32 NearGeometry)> AsyncCallback) {
	FRDGBuilder GraphBuilder(RHICmdList);
	{
		SCOPE_CYCLE_COUNTER(STAT_GeometryAwareComputeShader_Execute);
		DECLARE_GPU_STAT(GeometryAwareComputeShader)
		RDG_EVENT_SCOPE(GraphBuilder, "GeometryAwareComputeShader");
		RDG_GPU_STAT_SCOPE(GraphBuilder, GeometryAwareComputeShader);

		typename FGeometryAwareComputeShader::FPermutationDomain PermutationVector;
		TShaderMapRef<FGeometryAwareComputeShader> ComputeShader(GetGlobalShaderMap(GMaxRHIFeatureLevel), PermutationVector);
		bool bIsShaderValid = ComputeShader.IsValid();

		if (bIsShaderValid) {
			FGeometryAwareComputeShader::FParameters* PassParameters = GraphBuilder.AllocParameters<FGeometryAwareComputeShader::FParameters>();
			int NumInputs = 1;
			int InputSize = sizeof(FVector3f);
			//Something something distance field
			FRDGBufferRef InputBuffer = CreateUploadBuffer(GraphBuilder, TEXT("InputBuffer"), InputSize, NumInputs, &Params.InputPosition, InputSize * NumInputs);
			PassParameters->Input = GraphBuilder.CreateSRV(FRDGBufferSRVDesc(InputBuffer, PF_R32G32B32F));

			PassParameters->SurfaceThreshold = Params.SurfaceThreshold;

			FRDGBufferRef OutputBuffer = GraphBuilder.CreateBuffer(FRDGBufferDesc::CreateBufferDesc(sizeof(int32), 1), TEXT("OutputBuffer"));
			PassParameters->Output = GraphBuilder.CreateUAV(FRDGBufferUAVDesc(OutputBuffer, PF_R32_SINT));

			auto GroupCount = FComputeShaderUtils::GetGroupCount(FIntVector(Params.X, Params.Y, Params.Z), FComputeShaderUtils::kGolden2DGroupSize);
			GraphBuilder.AddPass(
				RDG_EVENT_NAME("ExecuteGeometryAwareComputeShader"),
				PassParameters,
				ERDGPassFlags::AsyncCompute,
				[&PassParameters, ComputeShader, GroupCount](FRHIComputeCommandList& RHICmdList)
				{
					FComputeShaderUtils::Dispatch(RHICmdList, ComputeShader, *PassParameters, GroupCount);
				});

			FRHIGPUBufferReadback* GPUBufferReadback = new FRHIGPUBufferReadback(TEXT("ExecuteGeometryAwareComputeShaderOutput"));
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