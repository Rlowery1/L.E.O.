// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "TrafficVehicleSettings.h"
#include "UObject/SoftObjectPath.h"

PRAGMA_DISABLE_DEPRECATION_WARNINGS
static_assert(!UE_WITH_CONSTINIT_UOBJECT, "This generated code can only be compiled with !UE_WITH_CONSTINIT_OBJECT");
void EmptyLinkFunctionForGeneratedCodeTrafficVehicleSettings() {}

// ********** Begin Cross Module References ********************************************************
COREUOBJECT_API UScriptStruct* Z_Construct_UScriptStruct_FSoftObjectPath();
DEVELOPERSETTINGS_API UClass* Z_Construct_UClass_UDeveloperSettings();
TRAFFICRUNTIME_API UClass* Z_Construct_UClass_UTrafficVehicleSettings();
TRAFFICRUNTIME_API UClass* Z_Construct_UClass_UTrafficVehicleSettings_NoRegister();
UPackage* Z_Construct_UPackage__Script_TrafficRuntime();
// ********** End Cross Module References **********************************************************

// ********** Begin Class UTrafficVehicleSettings **************************************************
FClassRegistrationInfo Z_Registration_Info_UClass_UTrafficVehicleSettings;
UClass* UTrafficVehicleSettings::GetPrivateStaticClass()
{
	using TClass = UTrafficVehicleSettings;
	if (!Z_Registration_Info_UClass_UTrafficVehicleSettings.InnerSingleton)
	{
		GetPrivateStaticClassBody(
			TClass::StaticPackage(),
			TEXT("TrafficVehicleSettings"),
			Z_Registration_Info_UClass_UTrafficVehicleSettings.InnerSingleton,
			StaticRegisterNativesUTrafficVehicleSettings,
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
	return Z_Registration_Info_UClass_UTrafficVehicleSettings.InnerSingleton;
}
UClass* Z_Construct_UClass_UTrafficVehicleSettings_NoRegister()
{
	return UTrafficVehicleSettings::GetPrivateStaticClass();
}
struct Z_Construct_UClass_UTrafficVehicleSettings_Statics
{
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Class_MetaDataParams[] = {
#if !UE_BUILD_SHIPPING
		{ "Comment", "/**\n * Vehicle defaults for AAA Traffic (test vehicles, Chaos class selection, etc.)\n */" },
#endif
		{ "IncludePath", "TrafficVehicleSettings.h" },
		{ "ModuleRelativePath", "Public/TrafficVehicleSettings.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Vehicle defaults for AAA Traffic (test vehicles, Chaos class selection, etc.)" },
#endif
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_DefaultVehicleProfile_MetaData[] = {
		{ "Category", "TrafficVehicle" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** Default vehicle profile data asset (can point to a Chaos vehicle profile in project content). */" },
#endif
		{ "ModuleRelativePath", "Public/TrafficVehicleSettings.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Default vehicle profile data asset (can point to a Chaos vehicle profile in project content)." },
#endif
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_AdditionalVehicleProfiles_MetaData[] = {
		{ "Category", "TrafficVehicle" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** Optional additional profiles for variety (not yet sampled automatically). */" },
#endif
		{ "ModuleRelativePath", "Public/TrafficVehicleSettings.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Optional additional profiles for variety (not yet sampled automatically)." },
#endif
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_MinSpawnSpacingMultiplier_MetaData[] = {
		{ "Category", "TrafficVehicle|Spawning" },
		{ "ClampMin", "1.0" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** Minimum spacing multiplier between spawned vehicles: min spacing = VehicleLengthCm * MinSpawnSpacingMultiplier. */" },
#endif
		{ "ModuleRelativePath", "Public/TrafficVehicleSettings.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Minimum spacing multiplier between spawned vehicles: min spacing = VehicleLengthCm * MinSpawnSpacingMultiplier." },
#endif
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_MinUsableLaneLengthCm_MetaData[] = {
		{ "Category", "TrafficVehicle|Spawning" },
		{ "ClampMin", "0.0" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** Minimum usable lane length to consider spawning vehicles (in cm). */" },
#endif
		{ "ModuleRelativePath", "Public/TrafficVehicleSettings.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Minimum usable lane length to consider spawning vehicles (in cm)." },
#endif
	};
#endif // WITH_METADATA

// ********** Begin Class UTrafficVehicleSettings constinit property declarations ******************
	static const UECodeGen_Private::FStructPropertyParams NewProp_DefaultVehicleProfile;
	static const UECodeGen_Private::FStructPropertyParams NewProp_AdditionalVehicleProfiles_Inner;
	static const UECodeGen_Private::FArrayPropertyParams NewProp_AdditionalVehicleProfiles;
	static const UECodeGen_Private::FFloatPropertyParams NewProp_MinSpawnSpacingMultiplier;
	static const UECodeGen_Private::FFloatPropertyParams NewProp_MinUsableLaneLengthCm;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
// ********** End Class UTrafficVehicleSettings constinit property declarations ********************
	static UObject* (*const DependentSingletons[])();
	static constexpr FCppClassTypeInfoStatic StaticCppClassTypeInfo = {
		TCppClassTypeTraits<UTrafficVehicleSettings>::IsAbstract,
	};
	static const UECodeGen_Private::FClassParams ClassParams;
}; // struct Z_Construct_UClass_UTrafficVehicleSettings_Statics

// ********** Begin Class UTrafficVehicleSettings Property Definitions *****************************
const UECodeGen_Private::FStructPropertyParams Z_Construct_UClass_UTrafficVehicleSettings_Statics::NewProp_DefaultVehicleProfile = { "DefaultVehicleProfile", nullptr, (EPropertyFlags)0x0010000000004001, UECodeGen_Private::EPropertyGenFlags::Struct, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(UTrafficVehicleSettings, DefaultVehicleProfile), Z_Construct_UScriptStruct_FSoftObjectPath, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_DefaultVehicleProfile_MetaData), NewProp_DefaultVehicleProfile_MetaData) }; // 2425717601
const UECodeGen_Private::FStructPropertyParams Z_Construct_UClass_UTrafficVehicleSettings_Statics::NewProp_AdditionalVehicleProfiles_Inner = { "AdditionalVehicleProfiles", nullptr, (EPropertyFlags)0x0000000000004000, UECodeGen_Private::EPropertyGenFlags::Struct, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, 0, Z_Construct_UScriptStruct_FSoftObjectPath, METADATA_PARAMS(0, nullptr) }; // 2425717601
const UECodeGen_Private::FArrayPropertyParams Z_Construct_UClass_UTrafficVehicleSettings_Statics::NewProp_AdditionalVehicleProfiles = { "AdditionalVehicleProfiles", nullptr, (EPropertyFlags)0x0010000000004001, UECodeGen_Private::EPropertyGenFlags::Array, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(UTrafficVehicleSettings, AdditionalVehicleProfiles), EArrayPropertyFlags::None, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_AdditionalVehicleProfiles_MetaData), NewProp_AdditionalVehicleProfiles_MetaData) }; // 2425717601
const UECodeGen_Private::FFloatPropertyParams Z_Construct_UClass_UTrafficVehicleSettings_Statics::NewProp_MinSpawnSpacingMultiplier = { "MinSpawnSpacingMultiplier", nullptr, (EPropertyFlags)0x0010000000004001, UECodeGen_Private::EPropertyGenFlags::Float, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(UTrafficVehicleSettings, MinSpawnSpacingMultiplier), METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_MinSpawnSpacingMultiplier_MetaData), NewProp_MinSpawnSpacingMultiplier_MetaData) };
const UECodeGen_Private::FFloatPropertyParams Z_Construct_UClass_UTrafficVehicleSettings_Statics::NewProp_MinUsableLaneLengthCm = { "MinUsableLaneLengthCm", nullptr, (EPropertyFlags)0x0010000000004001, UECodeGen_Private::EPropertyGenFlags::Float, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(UTrafficVehicleSettings, MinUsableLaneLengthCm), METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_MinUsableLaneLengthCm_MetaData), NewProp_MinUsableLaneLengthCm_MetaData) };
const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UClass_UTrafficVehicleSettings_Statics::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UTrafficVehicleSettings_Statics::NewProp_DefaultVehicleProfile,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UTrafficVehicleSettings_Statics::NewProp_AdditionalVehicleProfiles_Inner,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UTrafficVehicleSettings_Statics::NewProp_AdditionalVehicleProfiles,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UTrafficVehicleSettings_Statics::NewProp_MinSpawnSpacingMultiplier,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UTrafficVehicleSettings_Statics::NewProp_MinUsableLaneLengthCm,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_UTrafficVehicleSettings_Statics::PropPointers) < 2048);
// ********** End Class UTrafficVehicleSettings Property Definitions *******************************
UObject* (*const Z_Construct_UClass_UTrafficVehicleSettings_Statics::DependentSingletons[])() = {
	(UObject* (*)())Z_Construct_UClass_UDeveloperSettings,
	(UObject* (*)())Z_Construct_UPackage__Script_TrafficRuntime,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_UTrafficVehicleSettings_Statics::DependentSingletons) < 16);
const UECodeGen_Private::FClassParams Z_Construct_UClass_UTrafficVehicleSettings_Statics::ClassParams = {
	&UTrafficVehicleSettings::StaticClass,
	"Game",
	&StaticCppClassTypeInfo,
	DependentSingletons,
	nullptr,
	Z_Construct_UClass_UTrafficVehicleSettings_Statics::PropPointers,
	nullptr,
	UE_ARRAY_COUNT(DependentSingletons),
	0,
	UE_ARRAY_COUNT(Z_Construct_UClass_UTrafficVehicleSettings_Statics::PropPointers),
	0,
	0x001000A6u,
	METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UClass_UTrafficVehicleSettings_Statics::Class_MetaDataParams), Z_Construct_UClass_UTrafficVehicleSettings_Statics::Class_MetaDataParams)
};
void UTrafficVehicleSettings::StaticRegisterNativesUTrafficVehicleSettings()
{
}
UClass* Z_Construct_UClass_UTrafficVehicleSettings()
{
	if (!Z_Registration_Info_UClass_UTrafficVehicleSettings.OuterSingleton)
	{
		UECodeGen_Private::ConstructUClass(Z_Registration_Info_UClass_UTrafficVehicleSettings.OuterSingleton, Z_Construct_UClass_UTrafficVehicleSettings_Statics::ClassParams);
	}
	return Z_Registration_Info_UClass_UTrafficVehicleSettings.OuterSingleton;
}
DEFINE_VTABLE_PTR_HELPER_CTOR_NS(, UTrafficVehicleSettings);
UTrafficVehicleSettings::~UTrafficVehicleSettings() {}
// ********** End Class UTrafficVehicleSettings ****************************************************

// ********** Begin Registration *******************************************************************
struct Z_CompiledInDeferFile_FID_Users_rllax_L_E_O_Plugins_AAAtrafficSystem_Source_TrafficRuntime_Public_TrafficVehicleSettings_h__Script_TrafficRuntime_Statics
{
	static constexpr FClassRegisterCompiledInInfo ClassInfo[] = {
		{ Z_Construct_UClass_UTrafficVehicleSettings, UTrafficVehicleSettings::StaticClass, TEXT("UTrafficVehicleSettings"), &Z_Registration_Info_UClass_UTrafficVehicleSettings, CONSTRUCT_RELOAD_VERSION_INFO(FClassReloadVersionInfo, sizeof(UTrafficVehicleSettings), 2018944423U) },
	};
}; // Z_CompiledInDeferFile_FID_Users_rllax_L_E_O_Plugins_AAAtrafficSystem_Source_TrafficRuntime_Public_TrafficVehicleSettings_h__Script_TrafficRuntime_Statics 
static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_rllax_L_E_O_Plugins_AAAtrafficSystem_Source_TrafficRuntime_Public_TrafficVehicleSettings_h__Script_TrafficRuntime_3320993247{
	TEXT("/Script/TrafficRuntime"),
	Z_CompiledInDeferFile_FID_Users_rllax_L_E_O_Plugins_AAAtrafficSystem_Source_TrafficRuntime_Public_TrafficVehicleSettings_h__Script_TrafficRuntime_Statics::ClassInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Users_rllax_L_E_O_Plugins_AAAtrafficSystem_Source_TrafficRuntime_Public_TrafficVehicleSettings_h__Script_TrafficRuntime_Statics::ClassInfo),
	nullptr, 0,
	nullptr, 0,
};
// ********** End Registration *********************************************************************

PRAGMA_ENABLE_DEPRECATION_WARNINGS
