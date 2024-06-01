#include "SphereComputeActor.h"
#include "SphereComputeShader.h"
#include "Engine/TextureRenderTarget2D.h"
#include "RenderingThread.h"
#include "RHICommandList.h"
#include "RenderUtils.h"
#include "RHIStaticStates.h"
#include "PipelineStateCache.h"
#include "RenderGraphUtils.h"
#include "RHI.h"
#include "RenderCore.h"
#include "RenderTargetPool.h"
#include "Engine/EngineTypes.h" // Include this header for EPixelFormat::PF_FloatRGBA

ASphereComputeActor::ASphereComputeActor()
{
	PrimaryActorTick.bCanEverTick = true;
    RenderTarget = nullptr;
}

void ASphereComputeActor::BeginPlay()
{
	Super::BeginPlay();
	CreateRenderTarget();
	DispatchComputeShader();
}

void ASphereComputeActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
    DispatchComputeShader();
}

void ASphereComputeActor::CreateRenderTarget()
{
	if (RenderTarget == nullptr)
	{
		RenderTarget = NewObject<UTextureRenderTarget2D>();
		RenderTarget->ClearColor = FLinearColor::Black;
		RenderTarget->InitCustomFormat(256, 256, EPixelFormat::PF_FloatRGBA, false);
		RenderTarget->UpdateResourceImmediate();
	}
}

void ASphereComputeActor::DispatchComputeShader()
{
	if (RenderTarget == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("Invalid Render Target"));
		return;
	}

	FRHICommandListImmediate& RHICmdList = GRHICommandList.GetImmediateCommandList();

	// Check if the texture supports UAV
	if (!(RenderTarget->GameThread_GetRenderTargetResource()->TextureRHI->GetFlags() & TexCreate_UAV))
	{
		UE_LOG(LogTemp, Error, TEXT("Texture does not support UAV"));
		return;
	}

	FRDGBuilder GraphBuilder(RHICmdList);

	// Convert FTextureRHIRef to IPooledRenderTarget
	TRefCountPtr<IPooledRenderTarget> PooledRenderTarget;
	FPooledRenderTargetDesc Desc = FPooledRenderTargetDesc::Create2DDesc(
		FIntPoint(RenderTarget->GetSurfaceWidth(), RenderTarget->GetSurfaceHeight()), 
		RenderTarget->GetFormat(), 
		FClearValueBinding::None, 
		TexCreate_ShaderResource | TexCreate_UAV, 
		TexCreate_None, 
		false);

	GRenderTargetPool.FindFreeElement(RHICmdList, Desc, PooledRenderTarget, TEXT("SphereComputeShaderRT"));

	FRDGTextureRef RDGTexture = GraphBuilder.RegisterExternalTexture(PooledRenderTarget);

	if (!RDGTexture)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to register external texture"));
		return;
	}

	FSphereComputeShader::FParameters* PassParameters = GraphBuilder.AllocParameters<FSphereComputeShader::FParameters>();
	PassParameters->OutputTexture = GraphBuilder.CreateUAV(FRDGTextureUAVDesc(RDGTexture));

	if (!PassParameters->OutputTexture)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to create UAV"));
		return;
	}

	TShaderMapRef<FSphereComputeShader> ComputeShader(GetGlobalShaderMap(GMaxRHIFeatureLevel));
	FComputeShaderUtils::AddPass(GraphBuilder, RDG_EVENT_NAME("SphereComputeShader"), ComputeShader, PassParameters, FIntVector(256 / 8, 256 / 8, 1));

	GraphBuilder.Execute();
}
