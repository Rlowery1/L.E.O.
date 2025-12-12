// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "TrafficCityBLDAdapterSettings.h"

PRAGMA_DISABLE_DEPRECATION_WARNINGS
static_assert(!UE_WITH_CONSTINIT_UOBJECT, "This generated code can only be compiled with !UE_WITH_CONSTINIT_OBJECT");
void EmptyLinkFunctionForGeneratedCodeTrafficCityBLDAdapterSettings() {}

// ********** Begin Cross Module References ********************************************************
DEVELOPERSETTINGS_API UClass* Z_Construct_UClass_UDeveloperSettings();
TRAFFICRUNTIME_API UClass* Z_Construct_UClass_UTrafficCityBLDAdapterSettings();
TRAFFICRUNTIME_API UClass* Z_Construct_UClass_UTrafficCityBLDAdapterSettings_NoRegister();
UPackage* Z_Construct_UPackage__Script_TrafficRuntime();
// ********** End Cross Module References **********************************************************

// ********** Begin Class UTrafficCityBLDAdapterSettings *******************************************
FClassRegistrationInfo Z_Registration_Info_UClass_UTrafficCityBLDAdapterSettings;
UClass* UTrafficCityBLDAdapterSettings::GetPrivateStaticClass()
{
	using TClass = UTrafficCityBLDAdapterSettings;
	if (!Z_Registration_Info_UClass_UTrafficCityBLDAdapterSettings.InnerSingleton)
	{
		GetPrivateStaticClassBody(
			TClass::StaticPackage(),
			TEXT("TrafficCityBLDAdapterSettings"),
			Z_Registration_Info_UClass_UTrafficCityBLDAdapterSettings.InnerSingleton,
			StaticRegisterNativesUTrafficCityBLDAdapterSettings,
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
	return Z_Registration_Info_UClass_UTrafficCityBLDAdapterSettings.InnerSingleton;
}
UClass* Z_Construct_UClass_UTrafficCityBLDAdapterSettings_NoRegister()
{
	return UTrafficCityBLDAdapterSettings::GetPrivateStaticClass();
}
struct Z_Construct_UClass_UTrafficCityBLDAdapterSettings_Statics
{
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Class_MetaDataParams[] = {
		{ "IncludePath", "TrafficCityBLDAdapterSettings.h" },
		{ "ModuleRelativePath", "Public/TrafficCityBLDAdapterSettings.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_RoadActorTag_MetaData[] = {
		{ "Category", "Detection" },
		{ "ModuleRelativePath", "Public/TrafficCityBLDAdapterSettings.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_RoadClassNameContains_MetaData[] = {
		{ "Category", "Detection" },
		{ "ModuleRelativePath", "Public/TrafficCityBLDAdapterSettings.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_RoadSplineTag_MetaData[] = {
		{ "Category", "Detection" },
		{ "ModuleRelativePath", "Public/TrafficCityBLDAdapterSettings.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_DefaultFamilyName_MetaData[] = {
		{ "Category", "Families" },
		{ "ModuleRelativePath", "Public/TrafficCityBLDAdapterSettings.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_DrivableMaterialKeywords_MetaData[] = {
		{ "Category", "Filtering" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** List of substrings to match drivable materials (e.g. \"Asphalt\", \"Road\"). If empty, material names are ignored. */" },
#endif
		{ "ModuleRelativePath", "Public/TrafficCityBLDAdapterSettings.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "List of substrings to match drivable materials (e.g. \"Asphalt\", \"Road\"). If empty, material names are ignored." },
#endif
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_MaxMeshLateralOffsetCm_MetaData[] = {
		{ "Category", "Filtering" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** Maximum lateral offset (cm) from actor forward/right for a mesh to be considered part of the road. Set to 0 to disable. */" },
#endif
		{ "ModuleRelativePath", "Public/TrafficCityBLDAdapterSettings.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Maximum lateral offset (cm) from actor forward/right for a mesh to be considered part of the road. Set to 0 to disable." },
#endif
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_MaxMeshHeightCm_MetaData[] = {
		{ "Category", "Filtering" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** Maximum mesh height (in cm) to consider it part of the drivable surface. Set to 0 to disable. */" },
#endif
		{ "ModuleRelativePath", "Public/TrafficCityBLDAdapterSettings.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Maximum mesh height (in cm) to consider it part of the drivable surface. Set to 0 to disable." },
#endif
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_ExcludedMeshNameKeywords_MetaData[] = {
		{ "Category", "Filtering" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** Negative keywords: exclude meshes if the name (component or mesh) contains any of these substrings. */" },
#endif
		{ "ModuleRelativePath", "Public/TrafficCityBLDAdapterSettings.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Negative keywords: exclude meshes if the name (component or mesh) contains any of these substrings." },
#endif
	};
#endif // WITH_METADATA

// ********** Begin Class UTrafficCityBLDAdapterSettings constinit property declarations ***********
	static const UECodeGen_Private::FNamePropertyParams NewProp_RoadActorTag;
	static const UECodeGen_Private::FStrPropertyParams NewProp_RoadClassNameContains_Inner;
	static const UECodeGen_Private::FArrayPropertyParams NewProp_RoadClassNameContains;
	static const UECodeGen_Private::FNamePropertyParams NewProp_RoadSplineTag;
	static const UECodeGen_Private::FNamePropertyParams NewProp_DefaultFamilyName;
	static const UECodeGen_Private::FStrPropertyParams NewProp_DrivableMaterialKeywords_Inner;
	static const UECodeGen_Private::FArrayPropertyParams NewProp_DrivableMaterialKeywords;
	static const UECodeGen_Private::FFloatPropertyParams NewProp_MaxMeshLateralOffsetCm;
	static const UECodeGen_Private::FFloatPropertyParams NewProp_MaxMeshHeightCm;
	static const UECodeGen_Private::FStrPropertyParams NewProp_ExcludedMeshNameKeywords_Inner;
	static const UECodeGen_Private::FArrayPropertyParams NewProp_ExcludedMeshNameKeywords;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
// ********** End Class UTrafficCityBLDAdapterSettings constinit property declarations *************
	static UObject* (*const DependentSingletons[])();
	static constexpr FCppClassTypeInfoStatic StaticCppClassTypeInfo = {
		TCppClassTypeTraits<UTrafficCityBLDAdapterSettings>::IsAbstract,
	};
	static const UECodeGen_Private::FClassParams ClassParams;
}; // struct Z_Construct_UClass_UTrafficCityBLDAdapterSettings_Statics

// ********** Begin Class UTrafficCityBLDAdapterSettings Property Definitions **********************
const UECodeGen_Private::FNamePropertyParams Z_Construct_UClass_UTrafficCityBLDAdapterSettings_Statics::NewProp_RoadActorTag = { "RoadActorTag", nullptr, (EPropertyFlags)0x0010000000004001, UECodeGen_Private::EPropertyGenFlags::Name, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(UTrafficCityBLDAdapterSettings, RoadActorTag), METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_RoadActorTag_MetaData), NewProp_RoadActorTag_MetaData) };
const UECodeGen_Private::FStrPropertyParams Z_Construct_UClass_UTrafficCityBLDAdapterSettings_Statics::NewProp_RoadClassNameContains_Inner = { "RoadClassNameContains", nullptr, (EPropertyFlags)0x0000000000004000, UECodeGen_Private::EPropertyGenFlags::Str, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, 0, METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FArrayPropertyParams Z_Construct_UClass_UTrafficCityBLDAdapterSettings_Statics::NewProp_RoadClassNameContains = { "RoadClassNameContains", nullptr, (EPropertyFlags)0x0010000000004001, UECodeGen_Private::EPropertyGenFlags::Array, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(UTrafficCityBLDAdapterSettings, RoadClassNameContains), EArrayPropertyFlags::None, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_RoadClassNameContains_MetaData), NewProp_RoadClassNameContains_MetaData) };
const UECodeGen_Private::FNamePropertyParams Z_Construct_UClass_UTrafficCityBLDAdapterSettings_Statics::NewProp_RoadSplineTag = { "RoadSplineTag", nullptr, (EPropertyFlags)0x0010000000004001, UECodeGen_Private::EPropertyGenFlags::Name, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(UTrafficCityBLDAdapterSettings, RoadSplineTag), METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_RoadSplineTag_MetaData), NewProp_RoadSplineTag_MetaData) };
const UECodeGen_Private::FNamePropertyParams Z_Construct_UClass_UTrafficCityBLDAdapterSettings_Statics::NewProp_DefaultFamilyName = { "DefaultFamilyName", nullptr, (EPropertyFlags)0x0010000000004001, UECodeGen_Private::EPropertyGenFlags::Name, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(UTrafficCityBLDAdapterSettings, DefaultFamilyName), METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_DefaultFamilyName_MetaData), NewProp_DefaultFamilyName_MetaData) };
const UECodeGen_Private::FStrPropertyParams Z_Construct_UClass_UTrafficCityBLDAdapterSettings_Statics::NewProp_DrivableMaterialKeywords_Inner = { "DrivableMaterialKeywords", nullptr, (EPropertyFlags)0x0000000000004000, UECodeGen_Private::EPropertyGenFlags::Str, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, 0, METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FArrayPropertyParams Z_Construct_UClass_UTrafficCityBLDAdapterSettings_Statics::NewProp_DrivableMaterialKeywords = { "DrivableMaterialKeywords", nullptr, (EPropertyFlags)0x0010000000004001, UECodeGen_Private::EPropertyGenFlags::Array, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(UTrafficCityBLDAdapterSettings, DrivableMaterialKeywords), EArrayPropertyFlags::None, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_DrivableMaterialKeywords_MetaData), NewProp_DrivableMaterialKeywords_MetaData) };
const UECodeGen_Private::FFloatPropertyParams Z_Construct_UClass_UTrafficCityBLDAdapterSettings_Statics::NewProp_MaxMeshLateralOffsetCm = { "MaxMeshLateralOffsetCm", nullptr, (EPropertyFlags)0x0010000000004001, UECodeGen_Private::EPropertyGenFlags::Float, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(UTrafficCityBLDAdapterSettings, MaxMeshLateralOffsetCm), METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_MaxMeshLateralOffsetCm_MetaData), NewProp_MaxMeshLateralOffsetCm_MetaData) };
const UECodeGen_Private::FFloatPropertyParams Z_Construct_UClass_UTrafficCityBLDAdapterSettings_Statics::NewProp_MaxMeshHeightCm = { "MaxMeshHeightCm", nullptr, (EPropertyFlags)0x0010000000004001, UECodeGen_Private::EPropertyGenFlags::Float, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(UTrafficCityBLDAdapterSettings, MaxMeshHeightCm), METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_MaxMeshHeightCm_MetaData), NewProp_MaxMeshHeightCm_MetaData) };
const UECodeGen_Private::FStrPropertyParams Z_Construct_UClass_UTrafficCityBLDAdapterSettings_Statics::NewProp_ExcludedMeshNameKeywords_Inner = { "ExcludedMeshNameKeywords", nullptr, (EPropertyFlags)0x0000000000004000, UECodeGen_Private::EPropertyGenFlags::Str, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, 0, METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FArrayPropertyParams Z_Construct_UClass_UTrafficCityBLDAdapterSettings_Statics::NewProp_ExcludedMeshNameKeywords = { "ExcludedMeshNameKeywords", nullptr, (EPropertyFlags)0x0010000000004001, UECodeGen_Private::EPropertyGenFlags::Array, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(UTrafficCityBLDAdapterSettings, ExcludedMeshNameKeywords), EArrayPropertyFlags::None, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_ExcludedMeshNameKeywords_MetaData), NewProp_ExcludedMeshNameKeywords_MetaData) };
const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UClass_UTrafficCityBLDAdapterSettings_Statics::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UTrafficCityBLDAdapterSettings_Statics::NewProp_RoadActorTag,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UTrafficCityBLDAdapterSettings_Statics::NewProp_RoadClassNameContains_Inner,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UTrafficCityBLDAdapterSettings_Statics::NewProp_RoadClassNameContains,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UTrafficCityBLDAdapterSettings_Statics::NewProp_RoadSplineTag,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UTrafficCityBLDAdapterSettings_Statics::NewProp_DefaultFamilyName,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UTrafficCityBLDAdapterSettings_Statics::NewProp_DrivableMaterialKeywords_Inner,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UTrafficCityBLDAdapterSettings_Statics::NewProp_DrivableMaterialKeywords,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UTrafficCityBLDAdapterSettings_Statics::NewProp_MaxMeshLateralOffsetCm,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UTrafficCityBLDAdapterSettings_Statics::NewProp_MaxMeshHeightCm,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UTrafficCityBLDAdapterSettings_Statics::NewProp_ExcludedMeshNameKeywords_Inner,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UTrafficCityBLDAdapterSettings_Statics::NewProp_ExcludedMeshNameKeywords,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_UTrafficCityBLDAdapterSettings_Statics::PropPointers) < 2048);
// ********** End Class UTrafficCityBLDAdapterSettings Property Definitions ************************
UObject* (*const Z_Construct_UClass_UTrafficCityBLDAdapterSettings_Statics::DependentSingletons[])() = {
	(UObject* (*)())Z_Construct_UClass_UDeveloperSettings,
	(UObject* (*)())Z_Construct_UPackage__Script_TrafficRuntime,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_UTrafficCityBLDAdapterSettings_Statics::DependentSingletons) < 16);
const UECodeGen_Private::FClassParams Z_Construct_UClass_UTrafficCityBLDAdapterSettings_Statics::ClassParams = {
	&UTrafficCityBLDAdapterSettings::StaticClass,
	"Game",
	&StaticCppClassTypeInfo,
	DependentSingletons,
	nullptr,
	Z_Construct_UClass_UTrafficCityBLDAdapterSettings_Statics::PropPointers,
	nullptr,
	UE_ARRAY_COUNT(DependentSingletons),
	0,
	UE_ARRAY_COUNT(Z_Construct_UClass_UTrafficCityBLDAdapterSettings_Statics::PropPointers),
	0,
	0x001000A6u,
	METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UClass_UTrafficCityBLDAdapterSettings_Statics::Class_MetaDataParams), Z_Construct_UClass_UTrafficCityBLDAdapterSettings_Statics::Class_MetaDataParams)
};
void UTrafficCityBLDAdapterSettings::StaticRegisterNativesUTrafficCityBLDAdapterSettings()
{
}
UClass* Z_Construct_UClass_UTrafficCityBLDAdapterSettings()
{
	if (!Z_Registration_Info_UClass_UTrafficCityBLDAdapterSettings.OuterSingleton)
	{
		UECodeGen_Private::ConstructUClass(Z_Registration_Info_UClass_UTrafficCityBLDAdapterSettings.OuterSingleton, Z_Construct_UClass_UTrafficCityBLDAdapterSettings_Statics::ClassParams);
	}
	return Z_Registration_Info_UClass_UTrafficCityBLDAdapterSettings.OuterSingleton;
}
DEFINE_VTABLE_PTR_HELPER_CTOR_NS(, UTrafficCityBLDAdapterSettings);
UTrafficCityBLDAdapterSettings::~UTrafficCityBLDAdapterSettings() {}
// ********** End Class UTrafficCityBLDAdapterSettings *********************************************

// ********** Begin Registration *******************************************************************
struct Z_CompiledInDeferFile_FID_Users_rllax_L_E_O_Plugins_AAAtrafficSystem_Source_TrafficRuntime_Public_TrafficCityBLDAdapterSettings_h__Script_TrafficRuntime_Statics
{
	static constexpr FClassRegisterCompiledInInfo ClassInfo[] = {
		{ Z_Construct_UClass_UTrafficCityBLDAdapterSettings, UTrafficCityBLDAdapterSettings::StaticClass, TEXT("UTrafficCityBLDAdapterSettings"), &Z_Registration_Info_UClass_UTrafficCityBLDAdapterSettings, CONSTRUCT_RELOAD_VERSION_INFO(FClassReloadVersionInfo, sizeof(UTrafficCityBLDAdapterSettings), 1956011281U) },
	};
}; // Z_CompiledInDeferFile_FID_Users_rllax_L_E_O_Plugins_AAAtrafficSystem_Source_TrafficRuntime_Public_TrafficCityBLDAdapterSettings_h__Script_TrafficRuntime_Statics 
static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_rllax_L_E_O_Plugins_AAAtrafficSystem_Source_TrafficRuntime_Public_TrafficCityBLDAdapterSettings_h__Script_TrafficRuntime_2889127703{
	TEXT("/Script/TrafficRuntime"),
	Z_CompiledInDeferFile_FID_Users_rllax_L_E_O_Plugins_AAAtrafficSystem_Source_TrafficRuntime_Public_TrafficCityBLDAdapterSettings_h__Script_TrafficRuntime_Statics::ClassInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Users_rllax_L_E_O_Plugins_AAAtrafficSystem_Source_TrafficRuntime_Public_TrafficCityBLDAdapterSettings_h__Script_TrafficRuntime_Statics::ClassInfo),
	nullptr, 0,
	nullptr, 0,
};
// ********** End Registration *********************************************************************

PRAGMA_ENABLE_DEPRECATION_WARNINGS
