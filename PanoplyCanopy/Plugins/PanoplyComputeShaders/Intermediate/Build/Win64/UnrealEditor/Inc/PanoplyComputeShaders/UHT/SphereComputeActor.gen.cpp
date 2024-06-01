// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "PanoplyComputeShaders/Private/SphereComputeActor.h"
PRAGMA_DISABLE_DEPRECATION_WARNINGS
void EmptyLinkFunctionForGeneratedCodeSphereComputeActor() {}
// Cross Module References
	ENGINE_API UClass* Z_Construct_UClass_AActor();
	ENGINE_API UClass* Z_Construct_UClass_UTextureRenderTarget2D_NoRegister();
	PANOPLYCOMPUTESHADERS_API UClass* Z_Construct_UClass_ASphereComputeActor();
	PANOPLYCOMPUTESHADERS_API UClass* Z_Construct_UClass_ASphereComputeActor_NoRegister();
	UPackage* Z_Construct_UPackage__Script_PanoplyComputeShaders();
// End Cross Module References
	void ASphereComputeActor::StaticRegisterNativesASphereComputeActor()
	{
	}
	IMPLEMENT_CLASS_NO_AUTO_REGISTRATION(ASphereComputeActor);
	UClass* Z_Construct_UClass_ASphereComputeActor_NoRegister()
	{
		return ASphereComputeActor::StaticClass();
	}
	struct Z_Construct_UClass_ASphereComputeActor_Statics
	{
		static UObject* (*const DependentSingletons[])();
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Class_MetaDataParams[];
#endif
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_RenderTarget_MetaData[];
#endif
		static const UECodeGen_Private::FObjectPropertyParams NewProp_RenderTarget;
		static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
		static const FCppClassTypeInfoStatic StaticCppClassTypeInfo;
		static const UECodeGen_Private::FClassParams ClassParams;
	};
	UObject* (*const Z_Construct_UClass_ASphereComputeActor_Statics::DependentSingletons[])() = {
		(UObject* (*)())Z_Construct_UClass_AActor,
		(UObject* (*)())Z_Construct_UPackage__Script_PanoplyComputeShaders,
	};
	static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_ASphereComputeActor_Statics::DependentSingletons) < 16);
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_ASphereComputeActor_Statics::Class_MetaDataParams[] = {
		{ "IncludePath", "SphereComputeActor.h" },
		{ "ModuleRelativePath", "Private/SphereComputeActor.h" },
	};
#endif
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_ASphereComputeActor_Statics::NewProp_RenderTarget_MetaData[] = {
		{ "Category", "ComputeShader" },
		{ "ModuleRelativePath", "Private/SphereComputeActor.h" },
	};
#endif
	const UECodeGen_Private::FObjectPropertyParams Z_Construct_UClass_ASphereComputeActor_Statics::NewProp_RenderTarget = { "RenderTarget", nullptr, (EPropertyFlags)0x0040000000000001, UECodeGen_Private::EPropertyGenFlags::Object, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(ASphereComputeActor, RenderTarget), Z_Construct_UClass_UTextureRenderTarget2D_NoRegister, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UClass_ASphereComputeActor_Statics::NewProp_RenderTarget_MetaData), Z_Construct_UClass_ASphereComputeActor_Statics::NewProp_RenderTarget_MetaData) };
	const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UClass_ASphereComputeActor_Statics::PropPointers[] = {
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_ASphereComputeActor_Statics::NewProp_RenderTarget,
	};
	const FCppClassTypeInfoStatic Z_Construct_UClass_ASphereComputeActor_Statics::StaticCppClassTypeInfo = {
		TCppClassTypeTraits<ASphereComputeActor>::IsAbstract,
	};
	const UECodeGen_Private::FClassParams Z_Construct_UClass_ASphereComputeActor_Statics::ClassParams = {
		&ASphereComputeActor::StaticClass,
		"Engine",
		&StaticCppClassTypeInfo,
		DependentSingletons,
		nullptr,
		Z_Construct_UClass_ASphereComputeActor_Statics::PropPointers,
		nullptr,
		UE_ARRAY_COUNT(DependentSingletons),
		0,
		UE_ARRAY_COUNT(Z_Construct_UClass_ASphereComputeActor_Statics::PropPointers),
		0,
		0x008000A4u,
		METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UClass_ASphereComputeActor_Statics::Class_MetaDataParams), Z_Construct_UClass_ASphereComputeActor_Statics::Class_MetaDataParams)
	};
	static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_ASphereComputeActor_Statics::PropPointers) < 2048);
	UClass* Z_Construct_UClass_ASphereComputeActor()
	{
		if (!Z_Registration_Info_UClass_ASphereComputeActor.OuterSingleton)
		{
			UECodeGen_Private::ConstructUClass(Z_Registration_Info_UClass_ASphereComputeActor.OuterSingleton, Z_Construct_UClass_ASphereComputeActor_Statics::ClassParams);
		}
		return Z_Registration_Info_UClass_ASphereComputeActor.OuterSingleton;
	}
	template<> PANOPLYCOMPUTESHADERS_API UClass* StaticClass<ASphereComputeActor>()
	{
		return ASphereComputeActor::StaticClass();
	}
	DEFINE_VTABLE_PTR_HELPER_CTOR(ASphereComputeActor);
	ASphereComputeActor::~ASphereComputeActor() {}
	struct Z_CompiledInDeferFile_FID_Users_david_repos_PanoplyCanopy_PanoplyCanopy_Plugins_PanoplyComputeShaders_Source_PanoplyComputeShaders_Private_SphereComputeActor_h_Statics
	{
		static const FClassRegisterCompiledInInfo ClassInfo[];
	};
	const FClassRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_david_repos_PanoplyCanopy_PanoplyCanopy_Plugins_PanoplyComputeShaders_Source_PanoplyComputeShaders_Private_SphereComputeActor_h_Statics::ClassInfo[] = {
		{ Z_Construct_UClass_ASphereComputeActor, ASphereComputeActor::StaticClass, TEXT("ASphereComputeActor"), &Z_Registration_Info_UClass_ASphereComputeActor, CONSTRUCT_RELOAD_VERSION_INFO(FClassReloadVersionInfo, sizeof(ASphereComputeActor), 848992777U) },
	};
	static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_david_repos_PanoplyCanopy_PanoplyCanopy_Plugins_PanoplyComputeShaders_Source_PanoplyComputeShaders_Private_SphereComputeActor_h_746802522(TEXT("/Script/PanoplyComputeShaders"),
		Z_CompiledInDeferFile_FID_Users_david_repos_PanoplyCanopy_PanoplyCanopy_Plugins_PanoplyComputeShaders_Source_PanoplyComputeShaders_Private_SphereComputeActor_h_Statics::ClassInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Users_david_repos_PanoplyCanopy_PanoplyCanopy_Plugins_PanoplyComputeShaders_Source_PanoplyComputeShaders_Private_SphereComputeActor_h_Statics::ClassInfo),
		nullptr, 0,
		nullptr, 0);
PRAGMA_ENABLE_DEPRECATION_WARNINGS
