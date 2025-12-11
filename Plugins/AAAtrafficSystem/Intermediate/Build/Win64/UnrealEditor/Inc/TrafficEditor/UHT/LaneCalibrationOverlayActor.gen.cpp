// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "LaneCalibrationOverlayActor.h"
#include "TrafficLaneCalibration.h"

PRAGMA_DISABLE_DEPRECATION_WARNINGS
static_assert(!UE_WITH_CONSTINIT_UOBJECT, "This generated code can only be compiled with !UE_WITH_CONSTINIT_OBJECT");
void EmptyLinkFunctionForGeneratedCodeLaneCalibrationOverlayActor() {}

// ********** Begin Cross Module References ********************************************************
COREUOBJECT_API UScriptStruct* Z_Construct_UScriptStruct_FTransform();
COREUOBJECT_API UScriptStruct* Z_Construct_UScriptStruct_FVector();
ENGINE_API UClass* Z_Construct_UClass_AActor();
ENGINE_API UClass* Z_Construct_UClass_UInstancedStaticMeshComponent_NoRegister();
ENGINE_API UClass* Z_Construct_UClass_UMaterialInterface_NoRegister();
ENGINE_API UClass* Z_Construct_UClass_UStaticMesh_NoRegister();
PROCEDURALMESHCOMPONENT_API UClass* Z_Construct_UClass_UProceduralMeshComponent_NoRegister();
TRAFFICEDITOR_API UClass* Z_Construct_UClass_ALaneCalibrationOverlayActor();
TRAFFICEDITOR_API UClass* Z_Construct_UClass_ALaneCalibrationOverlayActor_NoRegister();
TRAFFICRUNTIME_API UScriptStruct* Z_Construct_UScriptStruct_FTrafficLaneFamilyCalibration();
UPackage* Z_Construct_UPackage__Script_TrafficEditor();
// ********** End Cross Module References **********************************************************

// ********** Begin Class ALaneCalibrationOverlayActor Function Editor_RebuildFromCachedCenterline *
struct Z_Construct_UFunction_ALaneCalibrationOverlayActor_Editor_RebuildFromCachedCenterline_Statics
{
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[] = {
		{ "CallInEditor", "true" },
		{ "Category", "Traffic|Calibration" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "// Rebuild overlay from cached data (used by editor property edits).\n" },
#endif
		{ "ModuleRelativePath", "Public/LaneCalibrationOverlayActor.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Rebuild overlay from cached data (used by editor property edits)." },
#endif
	};
#endif // WITH_METADATA

// ********** Begin Function Editor_RebuildFromCachedCenterline constinit property declarations ****
// ********** End Function Editor_RebuildFromCachedCenterline constinit property declarations ******
	static const UECodeGen_Private::FFunctionParams FuncParams;
};
const UECodeGen_Private::FFunctionParams Z_Construct_UFunction_ALaneCalibrationOverlayActor_Editor_RebuildFromCachedCenterline_Statics::FuncParams = { { (UObject*(*)())Z_Construct_UClass_ALaneCalibrationOverlayActor, nullptr, "Editor_RebuildFromCachedCenterline", 	nullptr, 
	0, 
0,
RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x00020401, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UFunction_ALaneCalibrationOverlayActor_Editor_RebuildFromCachedCenterline_Statics::Function_MetaDataParams), Z_Construct_UFunction_ALaneCalibrationOverlayActor_Editor_RebuildFromCachedCenterline_Statics::Function_MetaDataParams)},  };
UFunction* Z_Construct_UFunction_ALaneCalibrationOverlayActor_Editor_RebuildFromCachedCenterline()
{
	static UFunction* ReturnFunction = nullptr;
	if (!ReturnFunction)
	{
		UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UFunction_ALaneCalibrationOverlayActor_Editor_RebuildFromCachedCenterline_Statics::FuncParams);
	}
	return ReturnFunction;
}
DEFINE_FUNCTION(ALaneCalibrationOverlayActor::execEditor_RebuildFromCachedCenterline)
{
	P_FINISH;
	P_NATIVE_BEGIN;
	P_THIS->Editor_RebuildFromCachedCenterline();
	P_NATIVE_END;
}
// ********** End Class ALaneCalibrationOverlayActor Function Editor_RebuildFromCachedCenterline ***

// ********** Begin Class ALaneCalibrationOverlayActor *********************************************
FClassRegistrationInfo Z_Registration_Info_UClass_ALaneCalibrationOverlayActor;
UClass* ALaneCalibrationOverlayActor::GetPrivateStaticClass()
{
	using TClass = ALaneCalibrationOverlayActor;
	if (!Z_Registration_Info_UClass_ALaneCalibrationOverlayActor.InnerSingleton)
	{
		GetPrivateStaticClassBody(
			TClass::StaticPackage(),
			TEXT("LaneCalibrationOverlayActor"),
			Z_Registration_Info_UClass_ALaneCalibrationOverlayActor.InnerSingleton,
			StaticRegisterNativesALaneCalibrationOverlayActor,
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
	return Z_Registration_Info_UClass_ALaneCalibrationOverlayActor.InnerSingleton;
}
UClass* Z_Construct_UClass_ALaneCalibrationOverlayActor_NoRegister()
{
	return ALaneCalibrationOverlayActor::GetPrivateStaticClass();
}
struct Z_Construct_UClass_ALaneCalibrationOverlayActor_Statics
{
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Class_MetaDataParams[] = {
		{ "IncludePath", "LaneCalibrationOverlayActor.h" },
		{ "ModuleRelativePath", "Public/LaneCalibrationOverlayActor.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_NumLanesPerSideForward_MetaData[] = {
		{ "Category", "Traffic|Calibration" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "// Editable preview settings exposed for calibration UI.\n" },
#endif
		{ "ModuleRelativePath", "Public/LaneCalibrationOverlayActor.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Editable preview settings exposed for calibration UI." },
#endif
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_NumLanesPerSideBackward_MetaData[] = {
		{ "Category", "Traffic|Calibration" },
		{ "ModuleRelativePath", "Public/LaneCalibrationOverlayActor.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_LaneWidthCm_MetaData[] = {
		{ "Category", "Traffic|Calibration" },
		{ "ClampMax", "1000.0" },
		{ "ClampMin", "50.0" },
		{ "ModuleRelativePath", "Public/LaneCalibrationOverlayActor.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_CenterlineOffsetCm_MetaData[] = {
		{ "Category", "Traffic|Calibration" },
		{ "ModuleRelativePath", "Public/LaneCalibrationOverlayActor.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_LaneRibbonMeshes_MetaData[] = {
		{ "EditInline", "true" },
		{ "ModuleRelativePath", "Public/LaneCalibrationOverlayActor.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_ChevronArrowComponents_MetaData[] = {
		{ "EditInline", "true" },
		{ "ModuleRelativePath", "Public/LaneCalibrationOverlayActor.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_FallbackArrowMesh_MetaData[] = {
		{ "ModuleRelativePath", "Public/LaneCalibrationOverlayActor.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_DefaultArrowMaterial_MetaData[] = {
		{ "ModuleRelativePath", "Public/LaneCalibrationOverlayActor.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_CachedCenterlinePoints_MetaData[] = {
		{ "ModuleRelativePath", "Public/LaneCalibrationOverlayActor.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_CachedCalibration_MetaData[] = {
		{ "ModuleRelativePath", "Public/LaneCalibrationOverlayActor.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_CachedRoadTransform_MetaData[] = {
		{ "ModuleRelativePath", "Public/LaneCalibrationOverlayActor.h" },
	};
#endif // WITH_METADATA

// ********** Begin Class ALaneCalibrationOverlayActor constinit property declarations *************
	static const UECodeGen_Private::FIntPropertyParams NewProp_NumLanesPerSideForward;
	static const UECodeGen_Private::FIntPropertyParams NewProp_NumLanesPerSideBackward;
	static const UECodeGen_Private::FFloatPropertyParams NewProp_LaneWidthCm;
	static const UECodeGen_Private::FFloatPropertyParams NewProp_CenterlineOffsetCm;
	static const UECodeGen_Private::FObjectPropertyParams NewProp_LaneRibbonMeshes_Inner;
	static const UECodeGen_Private::FArrayPropertyParams NewProp_LaneRibbonMeshes;
	static const UECodeGen_Private::FObjectPropertyParams NewProp_ChevronArrowComponents_Inner;
	static const UECodeGen_Private::FArrayPropertyParams NewProp_ChevronArrowComponents;
	static const UECodeGen_Private::FObjectPropertyParams NewProp_FallbackArrowMesh;
	static const UECodeGen_Private::FObjectPropertyParams NewProp_DefaultArrowMaterial;
	static const UECodeGen_Private::FStructPropertyParams NewProp_CachedCenterlinePoints_Inner;
	static const UECodeGen_Private::FArrayPropertyParams NewProp_CachedCenterlinePoints;
	static const UECodeGen_Private::FStructPropertyParams NewProp_CachedCalibration;
	static const UECodeGen_Private::FStructPropertyParams NewProp_CachedRoadTransform;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
// ********** End Class ALaneCalibrationOverlayActor constinit property declarations ***************
	static constexpr UE::CodeGen::FClassNativeFunction Funcs[] = {
		{ .NameUTF8 = UTF8TEXT("Editor_RebuildFromCachedCenterline"), .Pointer = &ALaneCalibrationOverlayActor::execEditor_RebuildFromCachedCenterline },
	};
	static UObject* (*const DependentSingletons[])();
	static constexpr FClassFunctionLinkInfo FuncInfo[] = {
		{ &Z_Construct_UFunction_ALaneCalibrationOverlayActor_Editor_RebuildFromCachedCenterline, "Editor_RebuildFromCachedCenterline" }, // 1084112074
	};
	static_assert(UE_ARRAY_COUNT(FuncInfo) < 2048);
	static constexpr FCppClassTypeInfoStatic StaticCppClassTypeInfo = {
		TCppClassTypeTraits<ALaneCalibrationOverlayActor>::IsAbstract,
	};
	static const UECodeGen_Private::FClassParams ClassParams;
}; // struct Z_Construct_UClass_ALaneCalibrationOverlayActor_Statics

// ********** Begin Class ALaneCalibrationOverlayActor Property Definitions ************************
const UECodeGen_Private::FIntPropertyParams Z_Construct_UClass_ALaneCalibrationOverlayActor_Statics::NewProp_NumLanesPerSideForward = { "NumLanesPerSideForward", nullptr, (EPropertyFlags)0x0010000000000005, UECodeGen_Private::EPropertyGenFlags::Int, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(ALaneCalibrationOverlayActor, NumLanesPerSideForward), METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_NumLanesPerSideForward_MetaData), NewProp_NumLanesPerSideForward_MetaData) };
const UECodeGen_Private::FIntPropertyParams Z_Construct_UClass_ALaneCalibrationOverlayActor_Statics::NewProp_NumLanesPerSideBackward = { "NumLanesPerSideBackward", nullptr, (EPropertyFlags)0x0010000000000005, UECodeGen_Private::EPropertyGenFlags::Int, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(ALaneCalibrationOverlayActor, NumLanesPerSideBackward), METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_NumLanesPerSideBackward_MetaData), NewProp_NumLanesPerSideBackward_MetaData) };
const UECodeGen_Private::FFloatPropertyParams Z_Construct_UClass_ALaneCalibrationOverlayActor_Statics::NewProp_LaneWidthCm = { "LaneWidthCm", nullptr, (EPropertyFlags)0x0010000000000005, UECodeGen_Private::EPropertyGenFlags::Float, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(ALaneCalibrationOverlayActor, LaneWidthCm), METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_LaneWidthCm_MetaData), NewProp_LaneWidthCm_MetaData) };
const UECodeGen_Private::FFloatPropertyParams Z_Construct_UClass_ALaneCalibrationOverlayActor_Statics::NewProp_CenterlineOffsetCm = { "CenterlineOffsetCm", nullptr, (EPropertyFlags)0x0010000000000005, UECodeGen_Private::EPropertyGenFlags::Float, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(ALaneCalibrationOverlayActor, CenterlineOffsetCm), METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_CenterlineOffsetCm_MetaData), NewProp_CenterlineOffsetCm_MetaData) };
const UECodeGen_Private::FObjectPropertyParams Z_Construct_UClass_ALaneCalibrationOverlayActor_Statics::NewProp_LaneRibbonMeshes_Inner = { "LaneRibbonMeshes", nullptr, (EPropertyFlags)0x0000000000080008, UECodeGen_Private::EPropertyGenFlags::Object, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, 0, Z_Construct_UClass_UProceduralMeshComponent_NoRegister, METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FArrayPropertyParams Z_Construct_UClass_ALaneCalibrationOverlayActor_Statics::NewProp_LaneRibbonMeshes = { "LaneRibbonMeshes", nullptr, (EPropertyFlags)0x0040008000000008, UECodeGen_Private::EPropertyGenFlags::Array, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(ALaneCalibrationOverlayActor, LaneRibbonMeshes), EArrayPropertyFlags::None, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_LaneRibbonMeshes_MetaData), NewProp_LaneRibbonMeshes_MetaData) };
const UECodeGen_Private::FObjectPropertyParams Z_Construct_UClass_ALaneCalibrationOverlayActor_Statics::NewProp_ChevronArrowComponents_Inner = { "ChevronArrowComponents", nullptr, (EPropertyFlags)0x0000000000080008, UECodeGen_Private::EPropertyGenFlags::Object, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, 0, Z_Construct_UClass_UInstancedStaticMeshComponent_NoRegister, METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FArrayPropertyParams Z_Construct_UClass_ALaneCalibrationOverlayActor_Statics::NewProp_ChevronArrowComponents = { "ChevronArrowComponents", nullptr, (EPropertyFlags)0x0040008000000008, UECodeGen_Private::EPropertyGenFlags::Array, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(ALaneCalibrationOverlayActor, ChevronArrowComponents), EArrayPropertyFlags::None, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_ChevronArrowComponents_MetaData), NewProp_ChevronArrowComponents_MetaData) };
const UECodeGen_Private::FObjectPropertyParams Z_Construct_UClass_ALaneCalibrationOverlayActor_Statics::NewProp_FallbackArrowMesh = { "FallbackArrowMesh", nullptr, (EPropertyFlags)0x0040000000000000, UECodeGen_Private::EPropertyGenFlags::Object, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(ALaneCalibrationOverlayActor, FallbackArrowMesh), Z_Construct_UClass_UStaticMesh_NoRegister, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_FallbackArrowMesh_MetaData), NewProp_FallbackArrowMesh_MetaData) };
const UECodeGen_Private::FObjectPropertyParams Z_Construct_UClass_ALaneCalibrationOverlayActor_Statics::NewProp_DefaultArrowMaterial = { "DefaultArrowMaterial", nullptr, (EPropertyFlags)0x0040000000000000, UECodeGen_Private::EPropertyGenFlags::Object, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(ALaneCalibrationOverlayActor, DefaultArrowMaterial), Z_Construct_UClass_UMaterialInterface_NoRegister, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_DefaultArrowMaterial_MetaData), NewProp_DefaultArrowMaterial_MetaData) };
const UECodeGen_Private::FStructPropertyParams Z_Construct_UClass_ALaneCalibrationOverlayActor_Statics::NewProp_CachedCenterlinePoints_Inner = { "CachedCenterlinePoints", nullptr, (EPropertyFlags)0x0000000000000000, UECodeGen_Private::EPropertyGenFlags::Struct, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, 0, Z_Construct_UScriptStruct_FVector, METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FArrayPropertyParams Z_Construct_UClass_ALaneCalibrationOverlayActor_Statics::NewProp_CachedCenterlinePoints = { "CachedCenterlinePoints", nullptr, (EPropertyFlags)0x0040000000000000, UECodeGen_Private::EPropertyGenFlags::Array, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(ALaneCalibrationOverlayActor, CachedCenterlinePoints), EArrayPropertyFlags::None, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_CachedCenterlinePoints_MetaData), NewProp_CachedCenterlinePoints_MetaData) };
const UECodeGen_Private::FStructPropertyParams Z_Construct_UClass_ALaneCalibrationOverlayActor_Statics::NewProp_CachedCalibration = { "CachedCalibration", nullptr, (EPropertyFlags)0x0040000000000000, UECodeGen_Private::EPropertyGenFlags::Struct, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(ALaneCalibrationOverlayActor, CachedCalibration), Z_Construct_UScriptStruct_FTrafficLaneFamilyCalibration, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_CachedCalibration_MetaData), NewProp_CachedCalibration_MetaData) }; // 1801600758
const UECodeGen_Private::FStructPropertyParams Z_Construct_UClass_ALaneCalibrationOverlayActor_Statics::NewProp_CachedRoadTransform = { "CachedRoadTransform", nullptr, (EPropertyFlags)0x0040000000000000, UECodeGen_Private::EPropertyGenFlags::Struct, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(ALaneCalibrationOverlayActor, CachedRoadTransform), Z_Construct_UScriptStruct_FTransform, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_CachedRoadTransform_MetaData), NewProp_CachedRoadTransform_MetaData) };
const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UClass_ALaneCalibrationOverlayActor_Statics::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_ALaneCalibrationOverlayActor_Statics::NewProp_NumLanesPerSideForward,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_ALaneCalibrationOverlayActor_Statics::NewProp_NumLanesPerSideBackward,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_ALaneCalibrationOverlayActor_Statics::NewProp_LaneWidthCm,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_ALaneCalibrationOverlayActor_Statics::NewProp_CenterlineOffsetCm,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_ALaneCalibrationOverlayActor_Statics::NewProp_LaneRibbonMeshes_Inner,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_ALaneCalibrationOverlayActor_Statics::NewProp_LaneRibbonMeshes,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_ALaneCalibrationOverlayActor_Statics::NewProp_ChevronArrowComponents_Inner,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_ALaneCalibrationOverlayActor_Statics::NewProp_ChevronArrowComponents,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_ALaneCalibrationOverlayActor_Statics::NewProp_FallbackArrowMesh,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_ALaneCalibrationOverlayActor_Statics::NewProp_DefaultArrowMaterial,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_ALaneCalibrationOverlayActor_Statics::NewProp_CachedCenterlinePoints_Inner,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_ALaneCalibrationOverlayActor_Statics::NewProp_CachedCenterlinePoints,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_ALaneCalibrationOverlayActor_Statics::NewProp_CachedCalibration,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_ALaneCalibrationOverlayActor_Statics::NewProp_CachedRoadTransform,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_ALaneCalibrationOverlayActor_Statics::PropPointers) < 2048);
// ********** End Class ALaneCalibrationOverlayActor Property Definitions **************************
UObject* (*const Z_Construct_UClass_ALaneCalibrationOverlayActor_Statics::DependentSingletons[])() = {
	(UObject* (*)())Z_Construct_UClass_AActor,
	(UObject* (*)())Z_Construct_UPackage__Script_TrafficEditor,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_ALaneCalibrationOverlayActor_Statics::DependentSingletons) < 16);
const UECodeGen_Private::FClassParams Z_Construct_UClass_ALaneCalibrationOverlayActor_Statics::ClassParams = {
	&ALaneCalibrationOverlayActor::StaticClass,
	"Engine",
	&StaticCppClassTypeInfo,
	DependentSingletons,
	FuncInfo,
	Z_Construct_UClass_ALaneCalibrationOverlayActor_Statics::PropPointers,
	nullptr,
	UE_ARRAY_COUNT(DependentSingletons),
	UE_ARRAY_COUNT(FuncInfo),
	UE_ARRAY_COUNT(Z_Construct_UClass_ALaneCalibrationOverlayActor_Statics::PropPointers),
	0,
	0x009000A4u,
	METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UClass_ALaneCalibrationOverlayActor_Statics::Class_MetaDataParams), Z_Construct_UClass_ALaneCalibrationOverlayActor_Statics::Class_MetaDataParams)
};
void ALaneCalibrationOverlayActor::StaticRegisterNativesALaneCalibrationOverlayActor()
{
	UClass* Class = ALaneCalibrationOverlayActor::StaticClass();
	FNativeFunctionRegistrar::RegisterFunctions(Class, MakeConstArrayView(Z_Construct_UClass_ALaneCalibrationOverlayActor_Statics::Funcs));
}
UClass* Z_Construct_UClass_ALaneCalibrationOverlayActor()
{
	if (!Z_Registration_Info_UClass_ALaneCalibrationOverlayActor.OuterSingleton)
	{
		UECodeGen_Private::ConstructUClass(Z_Registration_Info_UClass_ALaneCalibrationOverlayActor.OuterSingleton, Z_Construct_UClass_ALaneCalibrationOverlayActor_Statics::ClassParams);
	}
	return Z_Registration_Info_UClass_ALaneCalibrationOverlayActor.OuterSingleton;
}
DEFINE_VTABLE_PTR_HELPER_CTOR_NS(, ALaneCalibrationOverlayActor);
ALaneCalibrationOverlayActor::~ALaneCalibrationOverlayActor() {}
// ********** End Class ALaneCalibrationOverlayActor ***********************************************

// ********** Begin Registration *******************************************************************
struct Z_CompiledInDeferFile_FID_Users_rllax_L_E_O_Plugins_AAAtrafficSystem_Source_TrafficEditor_Public_LaneCalibrationOverlayActor_h__Script_TrafficEditor_Statics
{
	static constexpr FClassRegisterCompiledInInfo ClassInfo[] = {
		{ Z_Construct_UClass_ALaneCalibrationOverlayActor, ALaneCalibrationOverlayActor::StaticClass, TEXT("ALaneCalibrationOverlayActor"), &Z_Registration_Info_UClass_ALaneCalibrationOverlayActor, CONSTRUCT_RELOAD_VERSION_INFO(FClassReloadVersionInfo, sizeof(ALaneCalibrationOverlayActor), 4066278098U) },
	};
}; // Z_CompiledInDeferFile_FID_Users_rllax_L_E_O_Plugins_AAAtrafficSystem_Source_TrafficEditor_Public_LaneCalibrationOverlayActor_h__Script_TrafficEditor_Statics 
static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_rllax_L_E_O_Plugins_AAAtrafficSystem_Source_TrafficEditor_Public_LaneCalibrationOverlayActor_h__Script_TrafficEditor_4175190645{
	TEXT("/Script/TrafficEditor"),
	Z_CompiledInDeferFile_FID_Users_rllax_L_E_O_Plugins_AAAtrafficSystem_Source_TrafficEditor_Public_LaneCalibrationOverlayActor_h__Script_TrafficEditor_Statics::ClassInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Users_rllax_L_E_O_Plugins_AAAtrafficSystem_Source_TrafficEditor_Public_LaneCalibrationOverlayActor_h__Script_TrafficEditor_Statics::ClassInfo),
	nullptr, 0,
	nullptr, 0,
};
// ********** End Registration *********************************************************************

PRAGMA_ENABLE_DEPRECATION_WARNINGS
