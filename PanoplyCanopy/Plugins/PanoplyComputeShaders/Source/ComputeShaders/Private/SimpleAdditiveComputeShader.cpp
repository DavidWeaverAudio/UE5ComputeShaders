#include "SimpleAdditiveComputeShader.h"
#include "ComputeShaders/Public/SimpleAdditiveComputeShader.h"
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

DECLARE_STATS_GROUP(TEXT("SimpleAdditiveComputeShader"), STATGROUP_SimpleAdditiveComputeShader, STATCAT_Advanced);
DECLARE_CYCLE_STAT(TEXT("SimpleAdditiveComputeShader Execute"), STAT_SimpleAdditiveComputeShader_Execute, STATGROUP_SimpleAdditiveComputeShader);

// This class carries our parameter declarations and acts as the bridge between cpp and HLSL.
class COMPUTESHADERS_API FSimpleAdditiveComputeShader : public FGlobalShader
{
public:

	DECLARE_GLOBAL_SHADER(FSimpleAdditiveComputeShader);
	SHADER_USE_PARAMETER_STRUCT(FSimpleAdditiveComputeShader, FGlobalShader);
	class FMySimpleComputeShader_Perm_TEST : SHADER_PERMUTATION_INT("TEST", 1);
	using FPermutationDomain = TShaderPermutationDomain<FMySimpleComputeShader_Perm_TEST>;

#pragma region Examples
		// SHADER_PARAMETER(uint32, MyUint32) // On the shader side: uint32 MyUint32;
		//SHADER_PARAMETER(FVector3f, MyVector) // On the shader side: float3 MyVector;

		// SHADER_PARAMETER_TEXTURE(Texture2D, MyTexture) // On the shader side: Texture2D<float4> MyTexture; (float4 should be whatever you expect each pixel in the texture to be, in this case float4(R,G,B,A) for 4 channels)
		// SHADER_PARAMETER_SAMPLER(SamplerState, MyTextureSampler) // On the shader side: SamplerState MySampler; // CPP side: TStaticSamplerState<ESamplerFilter::SF_Bilinear>::GetRHI();

		// SHADER_PARAMETER_ARRAY(float, MyFloatArray, [3]) // On the shader side: float MyFloatArray[3];

		// SHADER_PARAMETER_UAV(RWTexture2D<FVector4f>, MyTextureUAV) // On the shader side: RWTexture2D<float4> MyTextureUAV;
		// SHADER_PARAMETER_UAV(RWStructuredBuffer<FMyCustomStruct>, MyCustomStructs) // On the shader side: RWStructuredBuffer<FMyCustomStruct> MyCustomStructs;
		// SHADER_PARAMETER_UAV(RWBuffer<FMyCustomStruct>, MyCustomStructs) // On the shader side: RWBuffer<FMyCustomStruct> MyCustomStructs;

		// SHADER_PARAMETER_SRV(StructuredBuffer<FMyCustomStruct>, MyCustomStructs) // On the shader side: StructuredBuffer<FMyCustomStruct> MyCustomStructs;
		// SHADER_PARAMETER_SRV(Buffer<FMyCustomStruct>, MyCustomStructs) // On the shader side: Buffer<FMyCustomStruct> MyCustomStructs;
		// SHADER_PARAMETER_SRV(Texture2D<FVector4f>, MyReadOnlyTexture) // On the shader side: Texture2D<float4> MyReadOnlyTexture;

		// SHADER_PARAMETER_STRUCT_REF(FMyCustomStruct, MyCustomStruct)
#pragma endregion
    BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
        SHADER_PARAMETER_RDG_BUFFER_SRV(Buffer<FVector3f>, Input)
		SHADER_PARAMETER_RDG_BUFFER_UAV(RWBuffer<float>, Output)
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
		
		OutEnvironment.SetDefine(TEXT("THREADS_X"), NUM_THREADS_SimpleAdditiveComputeShader_X);
		OutEnvironment.SetDefine(TEXT("THREADS_Y"), NUM_THREADS_SimpleAdditiveComputeShader_Y);
		OutEnvironment.SetDefine(TEXT("THREADS_Z"), NUM_THREADS_SimpleAdditiveComputeShader_Z);
	}
private:
};

// This will tell the engine to create the shader and where the shader entry point is.
//                            ShaderType                            ShaderPath                     Shader function name    Type
IMPLEMENT_GLOBAL_SHADER(FSimpleAdditiveComputeShader, "/ComputeShaders/SimpleAdditiveComputeShader.usf", "SimpleAdditiveComputeShader", SF_Compute);

void FSimpleAdditiveComputeShaderInterface::DispatchRenderThread(FRHICommandListImmediate& RHICmdList, FSimpleAdditiveComputeShaderDispatchParams Params, TFunction<void(int OutputVal)> AsyncCallback) {
	FRDGBuilder GraphBuilder(RHICmdList);
	{
		SCOPE_CYCLE_COUNTER(STAT_SimpleAdditiveComputeShader_Execute);
		DECLARE_GPU_STAT(SimpleAdditiveComputeShader)
		RDG_EVENT_SCOPE(GraphBuilder, "SimpleAdditiveComputeShader");
		RDG_GPU_STAT_SCOPE(GraphBuilder, SimpleAdditiveComputeShader);

		typename FSimpleAdditiveComputeShader::FPermutationDomain PermutationVector;
		TShaderMapRef<FSimpleAdditiveComputeShader> ComputeShader(GetGlobalShaderMap(GMaxRHIFeatureLevel), PermutationVector);
		bool bIsShaderValid = ComputeShader.IsValid();

		if (bIsShaderValid) {
			FSimpleAdditiveComputeShader::FParameters* PassParameters = GraphBuilder.AllocParameters<FSimpleAdditiveComputeShader::FParameters>();
			
			const void* RawData = (void*)Params.Input;
			int NumInputs = 2;
			int InputSize = sizeof(FVector3f);
			FRDGBufferRef InputBuffer = CreateUploadBuffer(GraphBuilder, TEXT("InputBuffer"), InputSize, NumInputs, RawData, InputSize * NumInputs);
			PassParameters->Input = GraphBuilder.CreateSRV(FRDGBufferSRVDesc(InputBuffer, PF_R32G32B32F));

			FRDGBufferRef OutputBuffer = GraphBuilder.CreateBuffer(
				FRDGBufferDesc::CreateBufferDesc(sizeof(int32), 1),
				TEXT("OutputBuffer"));
			PassParameters->Output = GraphBuilder.CreateUAV(FRDGBufferUAVDesc(OutputBuffer, PF_R32_SINT));

			auto GroupCount = FComputeShaderUtils::GetGroupCount(FIntVector(Params.X, Params.Y, Params.Z), FComputeShaderUtils::kGolden2DGroupSize);
			GraphBuilder.AddPass(
				RDG_EVENT_NAME("ExecuteSimpleAdditiveComputeShader"),
				PassParameters,
				ERDGPassFlags::AsyncCompute,
				[&PassParameters, ComputeShader, GroupCount](FRHIComputeCommandList& RHICmdList)
				{
					FComputeShaderUtils::Dispatch(RHICmdList, ComputeShader, *PassParameters, GroupCount);
				});

			FRHIGPUBufferReadback* GPUBufferReadback = new FRHIGPUBufferReadback(TEXT("ExecuteSimpleAdditiveComputeShaderOutput"));
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