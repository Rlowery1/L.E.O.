// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "TrafficVisualSettings.h"

PRAGMA_DISABLE_DEPRECATION_WARNINGS
static_assert(!UE_WITH_CONSTINIT_UOBJECT, "This generated code can only be compiled with !UE_WITH_CONSTINIT_OBJECT");
void EmptyLinkFunctionForGeneratedCodeTrafficVisualSettings() {}

// ********** Begin Cross Module References ********************************************************
DEVELOPERSETTINGS_API UClass* Z_Construct_UClass_UDeveloperSettings();
ENGINE_API UClass* Z_Construct_UClass_UMaterialInterface_NoRegister();
ENGINE_API UClass* Z_Construct_UClass_UStaticMesh_NoRegister();
TRAFFICRUNTIME_API UClass* Z_Construct_UClass_UTrafficVisualSettings();
TRAFFICRUNTIME_API UClass* Z_Construct_UClass_UTrafficVisualSettings_NoRegister();
UPackage* Z_Construct_UPackage__Script_TrafficRuntime();
// ********** End Cross Module References **********************************************************

// ********** Begin Class UTrafficVisualSettings ***************************************************
FClassRegistrationInfo Z_Registration_Info_UClass_UTrafficVisualSettings;
UClass* UTrafficVisualSettings::GetPrivateStaticClass()
{
	using TClass = UTrafficVisualSettings;
	if (!Z_Registration_Info_UClass_UTrafficVisualSettings.InnerSingleton)
	{
		GetPrivateStaticClassBody(
			TClass::StaticPackage(),
			TEXT("TrafficVisualSettings"),
			Z_Registration_Info_UClass_UTrafficVisualSettings.InnerSingleton,
			StaticRegisterNativesUTrafficVisualSettings,
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
	return Z_Registration_Info_UClass_UTrafficVisualSettings.InnerSingleton;
}
UClass* Z_Construct_UClass_UTrafficVisualSettings_NoRegister()
{
	return UTrafficVisualSettings::GetPrivateStaticClass();
}
struct Z_Construct_UClass_UTrafficVisualSettings_Statics
{
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Class_MetaDataParams[] = {
		{ "IncludePath", "TrafficVisualSettings.h" },
		{ "ModuleRelativePath", "Public/TrafficVisualSettings.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_ForwardLaneArrowMesh_MetaData[] = {
		{ "Category", "Lanes" },
		{ "ModuleRelativePath", "Public/TrafficVisualSettings.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_BackwardLaneArrowMesh_MetaData[] = {
		{ "Category", "Lanes" },
		{ "ModuleRelativePath", "Public/TrafficVisualSettings.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_RoadRibbonMaterial_MetaData[] = {
		{ "Category", "Road" },
		{ "ModuleRelativePath", "Public/TrafficVisualSettings.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_RoadRibbonUVScale_MetaData[] = {
		{ "Category", "Road" },
		{ "ClampMax", "100.0" },
		{ "ClampMin", "0.01" },
		{ "ModuleRelativePath", "Public/TrafficVisualSettings.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_ArrowLength_MetaData[] = {
		{ "Category", "Lane Arrows" },
		{ "ClampMax", "1000.0" },
		{ "ClampMin", "100.0" },
		{ "ModuleRelativePath", "Public/TrafficVisualSettings.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_ArrowWidth_MetaData[] = {
		{ "Category", "Lane Arrows" },
		{ "ClampMax", "500.0" },
		{ "ClampMin", "50.0" },
		{ "ModuleRelativePath", "Public/TrafficVisualSettings.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_ArrowSpacing_MetaData[] = {
		{ "Category", "Lane Arrows" },
		{ "ClampMax", "1500.0" },
		{ "ClampMin", "200.0" },
		{ "ModuleRelativePath", "Public/TrafficVisualSettings.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_bShowDebugSplines_MetaData[] = {
		{ "Category", "Debug" },
		{ "ModuleRelativePath", "Public/TrafficVisualSettings.h" },
	};
#endif // WITH_METADATA

// ********** Begin Class UTrafficVisualSettings constinit property declarations *******************
	static const UECodeGen_Private::FSoftObjectPropertyParams NewProp_ForwardLaneArrowMesh;
	static const UECodeGen_Private::FSoftObjectPropertyParams NewProp_BackwardLaneArrowMesh;
	static const UECodeGen_Private::FSoftObjectPropertyParams NewProp_RoadRibbonMaterial;
	static const UECodeGen_Private::FFloatPropertyParams NewProp_RoadRibbonUVScale;
	static const UECodeGen_Private::FFloatPropertyParams NewProp_ArrowLength;
	static const UECodeGen_Private::FFloatPropertyParams NewProp_ArrowWidth;
	static const UECodeGen_Private::FFloatPropertyParams NewProp_ArrowSpacing;
	static void NewProp_bShowDebugSplines_SetBit(void* Obj);
	static const UECodeGen_Private::FBoolPropertyParams NewProp_bShowDebugSplines;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
// ********** End Class UTrafficVisualSettings constinit property declarations *********************
	static UObject* (*const DependentSingletons[])();
	static constexpr FCppClassTypeInfoStatic StaticCppClassTypeInfo = {
		TCppClassTypeTraits<UTrafficVisualSettings>::IsAbstract,
	};
	static const UECodeGen_Private::FClassParams ClassParams;
}; // struct Z_Construct_UClass_UTrafficVisualSettings_Statics

// ********** Begin Class UTrafficVisualSettings Property Definitions ******************************
const UECodeGen_Private::FSoftObjectPropertyParams Z_Construct_UClass_UTrafficVisualSettings_Statics::NewProp_ForwardLaneArrowMesh = { "ForwardLaneArrowMesh", nullptr, (EPropertyFlags)0x0014000000004001, UECodeGen_Private::EPropertyGenFlags::SoftObject, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(UTrafficVisualSettings, ForwardLaneArrowMesh), Z_Construct_UClass_UStaticMesh_NoRegister, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_ForwardLaneArrowMesh_MetaData), NewProp_ForwardLaneArrowMesh_MetaData) };
const UECodeGen_Private::FSoftObjectPropertyParams Z_Construct_UClass_UTrafficVisualSettings_Statics::NewProp_BackwardLaneArrowMesh = { "BackwardLaneArrowMesh", nullptr, (EPropertyFlags)0x0014000000004001, UECodeGen_Private::EPropertyGenFlags::SoftObject, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(UTrafficVisualSettings, BackwardLaneArrowMesh), Z_Construct_UClass_UStaticMesh_NoRegister, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_BackwardLaneArrowMesh_MetaData), NewProp_BackwardLaneArrowMesh_MetaData) };
const UECodeGen_Private::FSoftObjectPropertyParams Z_Construct_UClass_UTrafficVisualSettings_Statics::NewProp_RoadRibbonMaterial = { "RoadRibbonMaterial", nullptr, (EPropertyFlags)0x0014000000004001, UECodeGen_Private::EPropertyGenFlags::SoftObject, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(UTrafficVisualSettings, RoadRibbonMaterial), Z_Construct_UClass_UMaterialInterface_NoRegister, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_RoadRibbonMaterial_MetaData), NewProp_RoadRibbonMaterial_MetaData) };
const UECodeGen_Private::FFloatPropertyParams Z_Construct_UClass_UTrafficVisualSettings_Statics::NewProp_RoadRibbonUVScale = { "RoadRibbonUVScale", nullptr, (EPropertyFlags)0x0010000000004001, UECodeGen_Private::EPropertyGenFlags::Float, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(UTrafficVisualSettings, RoadRibbonUVScale), METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_RoadRibbonUVScale_MetaData), NewProp_RoadRibbonUVScale_MetaData) };
const UECodeGen_Private::FFloatPropertyParams Z_Construct_UClass_UTrafficVisualSettings_Statics::NewProp_ArrowLength = { "ArrowLength", nullptr, (EPropertyFlags)0x0010000000004001, UECodeGen_Private::EPropertyGenFlags::Float, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(UTrafficVisualSettings, ArrowLength), METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_ArrowLength_MetaData), NewProp_ArrowLength_MetaData) };
const UECodeGen_Private::FFloatPropertyParams Z_Construct_UClass_UTrafficVisualSettings_Statics::NewProp_ArrowWidth = { "ArrowWidth", nullptr, (EPropertyFlags)0x0010000000004001, UECodeGen_Private::EPropertyGenFlags::Float, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(UTrafficVisualSettings, ArrowWidth), METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_ArrowWidth_MetaData), NewProp_ArrowWidth_MetaData) };
const UECodeGen_Private::FFloatPropertyParams Z_Construct_UClass_UTrafficVisualSettings_Statics::NewProp_ArrowSpacing = { "ArrowSpacing", nullptr, (EPropertyFlags)0x0010000000004001, UECodeGen_Private::EPropertyGenFlags::Float, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(UTrafficVisualSettings, ArrowSpacing), METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_ArrowSpacing_MetaData), NewProp_ArrowSpacing_MetaData) };
void Z_Construct_UClass_UTrafficVisualSettings_Statics::NewProp_bShowDebugSplines_SetBit(void* Obj)
{
	((UTrafficVisualSettings*)Obj)->bShowDebugSplines = 1;
}
const UECodeGen_Private::FBoolPropertyParams Z_Construct_UClass_UTrafficVisualSettings_Statics::NewProp_bShowDebugSplines = { "bShowDebugSplines", nullptr, (EPropertyFlags)0x0010000000004001, UECodeGen_Private::EPropertyGenFlags::Bool | UECodeGen_Private::EPropertyGenFlags::NativeBool, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, sizeof(bool), sizeof(UTrafficVisualSettings), &Z_Construct_UClass_UTrafficVisualSettings_Statics::NewProp_bShowDebugSplines_SetBit, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_bShowDebugSplines_MetaData), NewProp_bShowDebugSplines_MetaData) };
const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UClass_UTrafficVisualSettings_Statics::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UTrafficVisualSettings_Statics::NewProp_ForwardLaneArrowMesh,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UTrafficVisualSettings_Statics::NewProp_BackwardLaneArrowMesh,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UTrafficVisualSettings_Statics::NewProp_RoadRibbonMaterial,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UTrafficVisualSettings_Statics::NewProp_RoadRibbonUVScale,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UTrafficVisualSettings_Statics::NewProp_ArrowLength,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UTrafficVisualSettings_Statics::NewProp_ArrowWidth,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UTrafficVisualSettings_Statics::NewProp_ArrowSpacing,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UTrafficVisualSettings_Statics::NewProp_bShowDebugSplines,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_UTrafficVisualSettings_Statics::PropPointers) < 2048);
// ********** End Class UTrafficVisualSettings Property Definitions ********************************
UObject* (*const Z_Construct_UClass_UTrafficVisualSettings_Statics::DependentSingletons[])() = {
	(UObject* (*)())Z_Construct_UClass_UDeveloperSettings,
	(UObject* (*)())Z_Construct_UPackage__Script_TrafficRuntime,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_UTrafficVisualSettings_Statics::DependentSingletons) < 16);
const UECodeGen_Private::FClassParams Z_Construct_UClass_UTrafficVisualSettings_Statics::ClassParams = {
	&UTrafficVisualSettings::StaticClass,
	"Game",
	&StaticCppClassTypeInfo,
	DependentSingletons,
	nullptr,
	Z_Construct_UClass_UTrafficVisualSettings_Statics::PropPointers,
	nullptr,
	UE_ARRAY_COUNT(DependentSingletons),
	0,
	UE_ARRAY_COUNT(Z_Construct_UClass_UTrafficVisualSettings_Statics::PropPointers),
	0,
	0x001000A6u,
	METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UClass_UTrafficVisualSettings_Statics::Class_MetaDataParams), Z_Construct_UClass_UTrafficVisualSettings_Statics::Class_MetaDataParams)
};
void UTrafficVisualSettings::StaticRegisterNativesUTrafficVisualSettings()
{
}
UClass* Z_Construct_UClass_UTrafficVisualSettings()
{
	if (!Z_Registration_Info_UClass_UTrafficVisualSettings.OuterSingleton)
	{
		UECodeGen_Private::ConstructUClass(Z_Registration_Info_UClass_UTrafficVisualSettings.OuterSingleton, Z_Construct_UClass_UTrafficVisualSettings_Statics::ClassParams);
	}
	return Z_Registration_Info_UClass_UTrafficVisualSettings.OuterSingleton;
}
DEFINE_VTABLE_PTR_HELPER_CTOR_NS(, UTrafficVisualSettings);
UTrafficVisualSettings::~UTrafficVisualSettings() {}
// ********** End Class UTrafficVisualSettings *****************************************************

// ********** Begin Registration *******************************************************************
struct Z_CompiledInDeferFile_FID_Users_rllax_L_E_O_Plugins_AAAtrafficSystem_Source_TrafficRuntime_Public_TrafficVisualSettings_h__Script_TrafficRuntime_Statics
{
	static constexpr FClassRegisterCompiledInInfo ClassInfo[] = {
		{ Z_Construct_UClass_UTrafficVisualSettings, UTrafficVisualSettings::StaticClass, TEXT("UTrafficVisualSettings"), &Z_Registration_Info_UClass_UTrafficVisualSettings, CONSTRUCT_RELOAD_VERSION_INFO(FClassReloadVersionInfo, sizeof(UTrafficVisualSettings), 3179370998U) },
	};
}; // Z_CompiledInDeferFile_FID_Users_rllax_L_E_O_Plugins_AAAtrafficSystem_Source_TrafficRuntime_Public_TrafficVisualSettings_h__Script_TrafficRuntime_Statics 
static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_rllax_L_E_O_Plugins_AAAtrafficSystem_Source_TrafficRuntime_Public_TrafficVisualSettings_h__Script_TrafficRuntime_309995383{
	TEXT("/Script/TrafficRuntime"),
	Z_CompiledInDeferFile_FID_Users_rllax_L_E_O_Plugins_AAAtrafficSystem_Source_TrafficRuntime_Public_TrafficVisualSettings_h__Script_TrafficRuntime_Statics::ClassInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Users_rllax_L_E_O_Plugins_AAAtrafficSystem_Source_TrafficRuntime_Public_TrafficVisualSettings_h__Script_TrafficRuntime_Statics::ClassInfo),
	nullptr, 0,
	nullptr, 0,
};
// ********** End Registration *********************************************************************

PRAGMA_ENABLE_DEPRECATION_WARNINGS
