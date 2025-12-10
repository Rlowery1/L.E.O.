// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "RoadLanePreviewActor.h"

PRAGMA_DISABLE_DEPRECATION_WARNINGS
static_assert(!UE_WITH_CONSTINIT_UOBJECT, "This generated code can only be compiled with !UE_WITH_CONSTINIT_OBJECT");
void EmptyLinkFunctionForGeneratedCodeRoadLanePreviewActor() {}

// ********** Begin Cross Module References ********************************************************
ENGINE_API UClass* Z_Construct_UClass_AActor();
ENGINE_API UClass* Z_Construct_UClass_USplineComponent_NoRegister();
TRAFFICEDITOR_API UClass* Z_Construct_UClass_ARoadLanePreviewActor();
TRAFFICEDITOR_API UClass* Z_Construct_UClass_ARoadLanePreviewActor_NoRegister();
UPackage* Z_Construct_UPackage__Script_TrafficEditor();
// ********** End Cross Module References **********************************************************

// ********** Begin Class ARoadLanePreviewActor ****************************************************
FClassRegistrationInfo Z_Registration_Info_UClass_ARoadLanePreviewActor;
UClass* ARoadLanePreviewActor::GetPrivateStaticClass()
{
	using TClass = ARoadLanePreviewActor;
	if (!Z_Registration_Info_UClass_ARoadLanePreviewActor.InnerSingleton)
	{
		GetPrivateStaticClassBody(
			TClass::StaticPackage(),
			TEXT("RoadLanePreviewActor"),
			Z_Registration_Info_UClass_ARoadLanePreviewActor.InnerSingleton,
			StaticRegisterNativesARoadLanePreviewActor,
			sizeof(TClass),
			alignof(TClass),
			TClass::StaticClassFlags,
			TClass::StaticClassCastFlags(),
			TClass::StaticConfigName(),
			(UClass::ClassConstructorType)InternalConstructor<TClass>,
			(UClass::ClassVTableHelperCtorCallerType)InternalVTableHelperCtorCaller<TClass>,
			UOBJECT_CPPCLASS_STATICFUNCTIONS_FORCLASS(TClass),
			&TClass::Super::StaticClass,
			&TClass::WithinClass::StaticClass
		);
	}
	return Z_Registration_Info_UClass_ARoadLanePreviewActor.InnerSingleton;
}
UClass* Z_Construct_UClass_ARoadLanePreviewActor_NoRegister()
{
	return ARoadLanePreviewActor::GetPrivateStaticClass();
}
struct Z_Construct_UClass_ARoadLanePreviewActor_Statics
{
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Class_MetaDataParams[] = {
		{ "IncludePath", "RoadLanePreviewActor.h" },
		{ "IsBlueprintBase", "false" },
		{ "ModuleRelativePath", "Public/RoadLanePreviewActor.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_CenterlinePreview_MetaData[] = {
		{ "EditInline", "true" },
		{ "ModuleRelativePath", "Public/RoadLanePreviewActor.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_LaneSplines_MetaData[] = {
		{ "EditInline", "true" },
		{ "ModuleRelativePath", "Public/RoadLanePreviewActor.h" },
	};
#endif // WITH_METADATA

// ********** Begin Class ARoadLanePreviewActor constinit property declarations ********************
	static const UECodeGen_Private::FObjectPropertyParams NewProp_CenterlinePreview;
	static const UECodeGen_Private::FObjectPropertyParams NewProp_LaneSplines_Inner;
	static const UECodeGen_Private::FArrayPropertyParams NewProp_LaneSplines;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
// ********** End Class ARoadLanePreviewActor constinit property declarations **********************
	static UObject* (*const DependentSingletons[])();
	static constexpr FCppClassTypeInfoStatic StaticCppClassTypeInfo = {
		TCppClassTypeTraits<ARoadLanePreviewActor>::IsAbstract,
	};
	static const UECodeGen_Private::FClassParams ClassParams;
}; // struct Z_Construct_UClass_ARoadLanePreviewActor_Statics

// ********** Begin Class ARoadLanePreviewActor Property Definitions *******************************
const UECodeGen_Private::FObjectPropertyParams Z_Construct_UClass_ARoadLanePreviewActor_Statics::NewProp_CenterlinePreview = { "CenterlinePreview", nullptr, (EPropertyFlags)0x0124080000080008, UECodeGen_Private::EPropertyGenFlags::Object | UECodeGen_Private::EPropertyGenFlags::ObjectPtr, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(ARoadLanePreviewActor, CenterlinePreview), Z_Construct_UClass_USplineComponent_NoRegister, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_CenterlinePreview_MetaData), NewProp_CenterlinePreview_MetaData) };
const UECodeGen_Private::FObjectPropertyParams Z_Construct_UClass_ARoadLanePreviewActor_Statics::NewProp_LaneSplines_Inner = { "LaneSplines", nullptr, (EPropertyFlags)0x0104000000080008, UECodeGen_Private::EPropertyGenFlags::Object | UECodeGen_Private::EPropertyGenFlags::ObjectPtr, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, 0, Z_Construct_UClass_USplineComponent_NoRegister, METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FArrayPropertyParams Z_Construct_UClass_ARoadLanePreviewActor_Statics::NewProp_LaneSplines = { "LaneSplines", nullptr, (EPropertyFlags)0x0124088000000008, UECodeGen_Private::EPropertyGenFlags::Array, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(ARoadLanePreviewActor, LaneSplines), EArrayPropertyFlags::None, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_LaneSplines_MetaData), NewProp_LaneSplines_MetaData) };
const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UClass_ARoadLanePreviewActor_Statics::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_ARoadLanePreviewActor_Statics::NewProp_CenterlinePreview,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_ARoadLanePreviewActor_Statics::NewProp_LaneSplines_Inner,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_ARoadLanePreviewActor_Statics::NewProp_LaneSplines,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_ARoadLanePreviewActor_Statics::PropPointers) < 2048);
// ********** End Class ARoadLanePreviewActor Property Definitions *********************************
UObject* (*const Z_Construct_UClass_ARoadLanePreviewActor_Statics::DependentSingletons[])() = {
	(UObject* (*)())Z_Construct_UClass_AActor,
	(UObject* (*)())Z_Construct_UPackage__Script_TrafficEditor,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_ARoadLanePreviewActor_Statics::DependentSingletons) < 16);
const UECodeGen_Private::FClassParams Z_Construct_UClass_ARoadLanePreviewActor_Statics::ClassParams = {
	&ARoadLanePreviewActor::StaticClass,
	"Engine",
	&StaticCppClassTypeInfo,
	DependentSingletons,
	nullptr,
	Z_Construct_UClass_ARoadLanePreviewActor_Statics::PropPointers,
	nullptr,
	UE_ARRAY_COUNT(DependentSingletons),
	0,
	UE_ARRAY_COUNT(Z_Construct_UClass_ARoadLanePreviewActor_Statics::PropPointers),
	0,
	0x009000A4u,
	METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UClass_ARoadLanePreviewActor_Statics::Class_MetaDataParams), Z_Construct_UClass_ARoadLanePreviewActor_Statics::Class_MetaDataParams)
};
void ARoadLanePreviewActor::StaticRegisterNativesARoadLanePreviewActor()
{
}
UClass* Z_Construct_UClass_ARoadLanePreviewActor()
{
	if (!Z_Registration_Info_UClass_ARoadLanePreviewActor.OuterSingleton)
	{
		UECodeGen_Private::ConstructUClass(Z_Registration_Info_UClass_ARoadLanePreviewActor.OuterSingleton, Z_Construct_UClass_ARoadLanePreviewActor_Statics::ClassParams);
	}
	return Z_Registration_Info_UClass_ARoadLanePreviewActor.OuterSingleton;
}
DEFINE_VTABLE_PTR_HELPER_CTOR_NS(, ARoadLanePreviewActor);
ARoadLanePreviewActor::~ARoadLanePreviewActor() {}
// ********** End Class ARoadLanePreviewActor ******************************************************

// ********** Begin Registration *******************************************************************
struct Z_CompiledInDeferFile_FID_Users_rllax_L_E_O_Plugins_AAAtrafficSystem_Source_TrafficEditor_Public_RoadLanePreviewActor_h__Script_TrafficEditor_Statics
{
	static constexpr FClassRegisterCompiledInInfo ClassInfo[] = {
		{ Z_Construct_UClass_ARoadLanePreviewActor, ARoadLanePreviewActor::StaticClass, TEXT("ARoadLanePreviewActor"), &Z_Registration_Info_UClass_ARoadLanePreviewActor, CONSTRUCT_RELOAD_VERSION_INFO(FClassReloadVersionInfo, sizeof(ARoadLanePreviewActor), 427364107U) },
	};
}; // Z_CompiledInDeferFile_FID_Users_rllax_L_E_O_Plugins_AAAtrafficSystem_Source_TrafficEditor_Public_RoadLanePreviewActor_h__Script_TrafficEditor_Statics 
static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_rllax_L_E_O_Plugins_AAAtrafficSystem_Source_TrafficEditor_Public_RoadLanePreviewActor_h__Script_TrafficEditor_3421621887{
	TEXT("/Script/TrafficEditor"),
	Z_CompiledInDeferFile_FID_Users_rllax_L_E_O_Plugins_AAAtrafficSystem_Source_TrafficEditor_Public_RoadLanePreviewActor_h__Script_TrafficEditor_Statics::ClassInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Users_rllax_L_E_O_Plugins_AAAtrafficSystem_Source_TrafficEditor_Public_RoadLanePreviewActor_h__Script_TrafficEditor_Statics::ClassInfo),
	nullptr, 0,
	nullptr, 0,
};
// ********** End Registration *********************************************************************

PRAGMA_ENABLE_DEPRECATION_WARNINGS
