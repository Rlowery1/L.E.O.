// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "TrafficRoadFamilySettings.h"

PRAGMA_DISABLE_DEPRECATION_WARNINGS
static_assert(!UE_WITH_CONSTINIT_UOBJECT, "This generated code can only be compiled with !UE_WITH_CONSTINIT_OBJECT");
void EmptyLinkFunctionForGeneratedCodeTrafficRoadFamilySettings() {}

// ********** Begin Cross Module References ********************************************************
DEVELOPERSETTINGS_API UClass* Z_Construct_UClass_UDeveloperSettings();
TRAFFICRUNTIME_API UClass* Z_Construct_UClass_UTrafficRoadFamilySettings();
TRAFFICRUNTIME_API UClass* Z_Construct_UClass_UTrafficRoadFamilySettings_NoRegister();
TRAFFICRUNTIME_API UScriptStruct* Z_Construct_UScriptStruct_FRoadFamilyDefinition();
TRAFFICRUNTIME_API UScriptStruct* Z_Construct_UScriptStruct_FTrafficLaneLayoutSide();
UPackage* Z_Construct_UPackage__Script_TrafficRuntime();
// ********** End Cross Module References **********************************************************

// ********** Begin ScriptStruct FTrafficLaneLayoutSide ********************************************
struct Z_Construct_UScriptStruct_FTrafficLaneLayoutSide_Statics
{
	static inline consteval int32 GetStructSize() { return sizeof(FTrafficLaneLayoutSide); }
	static inline consteval int16 GetStructAlignment() { return alignof(FTrafficLaneLayoutSide); }
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Struct_MetaDataParams[] = {
		{ "BlueprintType", "true" },
		{ "ModuleRelativePath", "Public/TrafficRoadFamilySettings.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_NumLanes_MetaData[] = {
		{ "Category", "Traffic" },
		{ "ModuleRelativePath", "Public/TrafficRoadFamilySettings.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_LaneWidthCm_MetaData[] = {
		{ "Category", "Traffic" },
		{ "ClampMax", "1000.0" },
		{ "ClampMin", "50.0" },
		{ "ModuleRelativePath", "Public/TrafficRoadFamilySettings.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_InnerLaneCenterOffsetCm_MetaData[] = {
		{ "Category", "Traffic" },
		{ "ModuleRelativePath", "Public/TrafficRoadFamilySettings.h" },
	};
#endif // WITH_METADATA

// ********** Begin ScriptStruct FTrafficLaneLayoutSide constinit property declarations ************
	static const UECodeGen_Private::FIntPropertyParams NewProp_NumLanes;
	static const UECodeGen_Private::FFloatPropertyParams NewProp_LaneWidthCm;
	static const UECodeGen_Private::FFloatPropertyParams NewProp_InnerLaneCenterOffsetCm;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
// ********** End ScriptStruct FTrafficLaneLayoutSide constinit property declarations **************
	static void* NewStructOps()
	{
		return (UScriptStruct::ICppStructOps*)new UScriptStruct::TCppStructOps<FTrafficLaneLayoutSide>();
	}
	static const UECodeGen_Private::FStructParams StructParams;
}; // struct Z_Construct_UScriptStruct_FTrafficLaneLayoutSide_Statics
static FStructRegistrationInfo Z_Registration_Info_UScriptStruct_FTrafficLaneLayoutSide;
class UScriptStruct* FTrafficLaneLayoutSide::StaticStruct()
{
	if (!Z_Registration_Info_UScriptStruct_FTrafficLaneLayoutSide.OuterSingleton)
	{
		Z_Registration_Info_UScriptStruct_FTrafficLaneLayoutSide.OuterSingleton = GetStaticStruct(Z_Construct_UScriptStruct_FTrafficLaneLayoutSide, (UObject*)Z_Construct_UPackage__Script_TrafficRuntime(), TEXT("TrafficLaneLayoutSide"));
	}
	return Z_Registration_Info_UScriptStruct_FTrafficLaneLayoutSide.OuterSingleton;
	}

// ********** Begin ScriptStruct FTrafficLaneLayoutSide Property Definitions ***********************
const UECodeGen_Private::FIntPropertyParams Z_Construct_UScriptStruct_FTrafficLaneLayoutSide_Statics::NewProp_NumLanes = { "NumLanes", nullptr, (EPropertyFlags)0x0010000000004001, UECodeGen_Private::EPropertyGenFlags::Int, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(FTrafficLaneLayoutSide, NumLanes), METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_NumLanes_MetaData), NewProp_NumLanes_MetaData) };
const UECodeGen_Private::FFloatPropertyParams Z_Construct_UScriptStruct_FTrafficLaneLayoutSide_Statics::NewProp_LaneWidthCm = { "LaneWidthCm", nullptr, (EPropertyFlags)0x0010000000004001, UECodeGen_Private::EPropertyGenFlags::Float, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(FTrafficLaneLayoutSide, LaneWidthCm), METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_LaneWidthCm_MetaData), NewProp_LaneWidthCm_MetaData) };
const UECodeGen_Private::FFloatPropertyParams Z_Construct_UScriptStruct_FTrafficLaneLayoutSide_Statics::NewProp_InnerLaneCenterOffsetCm = { "InnerLaneCenterOffsetCm", nullptr, (EPropertyFlags)0x0010000000004001, UECodeGen_Private::EPropertyGenFlags::Float, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(FTrafficLaneLayoutSide, InnerLaneCenterOffsetCm), METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_InnerLaneCenterOffsetCm_MetaData), NewProp_InnerLaneCenterOffsetCm_MetaData) };
const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UScriptStruct_FTrafficLaneLayoutSide_Statics::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FTrafficLaneLayoutSide_Statics::NewProp_NumLanes,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FTrafficLaneLayoutSide_Statics::NewProp_LaneWidthCm,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FTrafficLaneLayoutSide_Statics::NewProp_InnerLaneCenterOffsetCm,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FTrafficLaneLayoutSide_Statics::PropPointers) < 2048);
// ********** End ScriptStruct FTrafficLaneLayoutSide Property Definitions *************************
const UECodeGen_Private::FStructParams Z_Construct_UScriptStruct_FTrafficLaneLayoutSide_Statics::StructParams = {
	(UObject* (*)())Z_Construct_UPackage__Script_TrafficRuntime,
	nullptr,
	&NewStructOps,
	"TrafficLaneLayoutSide",
	Z_Construct_UScriptStruct_FTrafficLaneLayoutSide_Statics::PropPointers,
	UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FTrafficLaneLayoutSide_Statics::PropPointers),
	sizeof(FTrafficLaneLayoutSide),
	alignof(FTrafficLaneLayoutSide),
	RF_Public|RF_Transient|RF_MarkAsNative,
	EStructFlags(0x00000201),
	METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FTrafficLaneLayoutSide_Statics::Struct_MetaDataParams), Z_Construct_UScriptStruct_FTrafficLaneLayoutSide_Statics::Struct_MetaDataParams)
};
UScriptStruct* Z_Construct_UScriptStruct_FTrafficLaneLayoutSide()
{
	if (!Z_Registration_Info_UScriptStruct_FTrafficLaneLayoutSide.InnerSingleton)
	{
		UECodeGen_Private::ConstructUScriptStruct(Z_Registration_Info_UScriptStruct_FTrafficLaneLayoutSide.InnerSingleton, Z_Construct_UScriptStruct_FTrafficLaneLayoutSide_Statics::StructParams);
	}
	return CastChecked<UScriptStruct>(Z_Registration_Info_UScriptStruct_FTrafficLaneLayoutSide.InnerSingleton);
}
// ********** End ScriptStruct FTrafficLaneLayoutSide **********************************************

// ********** Begin ScriptStruct FRoadFamilyDefinition *********************************************
struct Z_Construct_UScriptStruct_FRoadFamilyDefinition_Statics
{
	static inline consteval int32 GetStructSize() { return sizeof(FRoadFamilyDefinition); }
	static inline consteval int16 GetStructAlignment() { return alignof(FRoadFamilyDefinition); }
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Struct_MetaDataParams[] = {
		{ "BlueprintType", "true" },
		{ "ModuleRelativePath", "Public/TrafficRoadFamilySettings.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_FamilyName_MetaData[] = {
		{ "Category", "Traffic" },
		{ "ModuleRelativePath", "Public/TrafficRoadFamilySettings.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_Forward_MetaData[] = {
		{ "Category", "Traffic" },
		{ "ModuleRelativePath", "Public/TrafficRoadFamilySettings.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_Backward_MetaData[] = {
		{ "Category", "Traffic" },
		{ "ModuleRelativePath", "Public/TrafficRoadFamilySettings.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_DefaultSpeedLimitKmh_MetaData[] = {
		{ "Category", "Traffic" },
		{ "ClampMax", "300.0" },
		{ "ClampMin", "0.0" },
		{ "ModuleRelativePath", "Public/TrafficRoadFamilySettings.h" },
	};
#endif // WITH_METADATA

// ********** Begin ScriptStruct FRoadFamilyDefinition constinit property declarations *************
	static const UECodeGen_Private::FNamePropertyParams NewProp_FamilyName;
	static const UECodeGen_Private::FStructPropertyParams NewProp_Forward;
	static const UECodeGen_Private::FStructPropertyParams NewProp_Backward;
	static const UECodeGen_Private::FFloatPropertyParams NewProp_DefaultSpeedLimitKmh;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
// ********** End ScriptStruct FRoadFamilyDefinition constinit property declarations ***************
	static void* NewStructOps()
	{
		return (UScriptStruct::ICppStructOps*)new UScriptStruct::TCppStructOps<FRoadFamilyDefinition>();
	}
	static const UECodeGen_Private::FStructParams StructParams;
}; // struct Z_Construct_UScriptStruct_FRoadFamilyDefinition_Statics
static FStructRegistrationInfo Z_Registration_Info_UScriptStruct_FRoadFamilyDefinition;
class UScriptStruct* FRoadFamilyDefinition::StaticStruct()
{
	if (!Z_Registration_Info_UScriptStruct_FRoadFamilyDefinition.OuterSingleton)
	{
		Z_Registration_Info_UScriptStruct_FRoadFamilyDefinition.OuterSingleton = GetStaticStruct(Z_Construct_UScriptStruct_FRoadFamilyDefinition, (UObject*)Z_Construct_UPackage__Script_TrafficRuntime(), TEXT("RoadFamilyDefinition"));
	}
	return Z_Registration_Info_UScriptStruct_FRoadFamilyDefinition.OuterSingleton;
	}

// ********** Begin ScriptStruct FRoadFamilyDefinition Property Definitions ************************
const UECodeGen_Private::FNamePropertyParams Z_Construct_UScriptStruct_FRoadFamilyDefinition_Statics::NewProp_FamilyName = { "FamilyName", nullptr, (EPropertyFlags)0x0010000000004001, UECodeGen_Private::EPropertyGenFlags::Name, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(FRoadFamilyDefinition, FamilyName), METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_FamilyName_MetaData), NewProp_FamilyName_MetaData) };
const UECodeGen_Private::FStructPropertyParams Z_Construct_UScriptStruct_FRoadFamilyDefinition_Statics::NewProp_Forward = { "Forward", nullptr, (EPropertyFlags)0x0010000000004001, UECodeGen_Private::EPropertyGenFlags::Struct, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(FRoadFamilyDefinition, Forward), Z_Construct_UScriptStruct_FTrafficLaneLayoutSide, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_Forward_MetaData), NewProp_Forward_MetaData) }; // 3849306716
const UECodeGen_Private::FStructPropertyParams Z_Construct_UScriptStruct_FRoadFamilyDefinition_Statics::NewProp_Backward = { "Backward", nullptr, (EPropertyFlags)0x0010000000004001, UECodeGen_Private::EPropertyGenFlags::Struct, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(FRoadFamilyDefinition, Backward), Z_Construct_UScriptStruct_FTrafficLaneLayoutSide, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_Backward_MetaData), NewProp_Backward_MetaData) }; // 3849306716
const UECodeGen_Private::FFloatPropertyParams Z_Construct_UScriptStruct_FRoadFamilyDefinition_Statics::NewProp_DefaultSpeedLimitKmh = { "DefaultSpeedLimitKmh", nullptr, (EPropertyFlags)0x0010000000004001, UECodeGen_Private::EPropertyGenFlags::Float, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(FRoadFamilyDefinition, DefaultSpeedLimitKmh), METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_DefaultSpeedLimitKmh_MetaData), NewProp_DefaultSpeedLimitKmh_MetaData) };
const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UScriptStruct_FRoadFamilyDefinition_Statics::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FRoadFamilyDefinition_Statics::NewProp_FamilyName,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FRoadFamilyDefinition_Statics::NewProp_Forward,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FRoadFamilyDefinition_Statics::NewProp_Backward,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FRoadFamilyDefinition_Statics::NewProp_DefaultSpeedLimitKmh,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FRoadFamilyDefinition_Statics::PropPointers) < 2048);
// ********** End ScriptStruct FRoadFamilyDefinition Property Definitions **************************
const UECodeGen_Private::FStructParams Z_Construct_UScriptStruct_FRoadFamilyDefinition_Statics::StructParams = {
	(UObject* (*)())Z_Construct_UPackage__Script_TrafficRuntime,
	nullptr,
	&NewStructOps,
	"RoadFamilyDefinition",
	Z_Construct_UScriptStruct_FRoadFamilyDefinition_Statics::PropPointers,
	UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FRoadFamilyDefinition_Statics::PropPointers),
	sizeof(FRoadFamilyDefinition),
	alignof(FRoadFamilyDefinition),
	RF_Public|RF_Transient|RF_MarkAsNative,
	EStructFlags(0x00000201),
	METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FRoadFamilyDefinition_Statics::Struct_MetaDataParams), Z_Construct_UScriptStruct_FRoadFamilyDefinition_Statics::Struct_MetaDataParams)
};
UScriptStruct* Z_Construct_UScriptStruct_FRoadFamilyDefinition()
{
	if (!Z_Registration_Info_UScriptStruct_FRoadFamilyDefinition.InnerSingleton)
	{
		UECodeGen_Private::ConstructUScriptStruct(Z_Registration_Info_UScriptStruct_FRoadFamilyDefinition.InnerSingleton, Z_Construct_UScriptStruct_FRoadFamilyDefinition_Statics::StructParams);
	}
	return CastChecked<UScriptStruct>(Z_Registration_Info_UScriptStruct_FRoadFamilyDefinition.InnerSingleton);
}
// ********** End ScriptStruct FRoadFamilyDefinition ***********************************************

// ********** Begin Class UTrafficRoadFamilySettings ***********************************************
FClassRegistrationInfo Z_Registration_Info_UClass_UTrafficRoadFamilySettings;
UClass* UTrafficRoadFamilySettings::GetPrivateStaticClass()
{
	using TClass = UTrafficRoadFamilySettings;
	if (!Z_Registration_Info_UClass_UTrafficRoadFamilySettings.InnerSingleton)
	{
		GetPrivateStaticClassBody(
			TClass::StaticPackage(),
			TEXT("TrafficRoadFamilySettings"),
			Z_Registration_Info_UClass_UTrafficRoadFamilySettings.InnerSingleton,
			StaticRegisterNativesUTrafficRoadFamilySettings,
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
	return Z_Registration_Info_UClass_UTrafficRoadFamilySettings.InnerSingleton;
}
UClass* Z_Construct_UClass_UTrafficRoadFamilySettings_NoRegister()
{
	return UTrafficRoadFamilySettings::GetPrivateStaticClass();
}
struct Z_Construct_UClass_UTrafficRoadFamilySettings_Statics
{
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Class_MetaDataParams[] = {
		{ "IncludePath", "TrafficRoadFamilySettings.h" },
		{ "ModuleRelativePath", "Public/TrafficRoadFamilySettings.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_Families_MetaData[] = {
		{ "Category", "Families" },
		{ "ModuleRelativePath", "Public/TrafficRoadFamilySettings.h" },
	};
#endif // WITH_METADATA

// ********** Begin Class UTrafficRoadFamilySettings constinit property declarations ***************
	static const UECodeGen_Private::FStructPropertyParams NewProp_Families_Inner;
	static const UECodeGen_Private::FArrayPropertyParams NewProp_Families;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
// ********** End Class UTrafficRoadFamilySettings constinit property declarations *****************
	static UObject* (*const DependentSingletons[])();
	static constexpr FCppClassTypeInfoStatic StaticCppClassTypeInfo = {
		TCppClassTypeTraits<UTrafficRoadFamilySettings>::IsAbstract,
	};
	static const UECodeGen_Private::FClassParams ClassParams;
}; // struct Z_Construct_UClass_UTrafficRoadFamilySettings_Statics

// ********** Begin Class UTrafficRoadFamilySettings Property Definitions **************************
const UECodeGen_Private::FStructPropertyParams Z_Construct_UClass_UTrafficRoadFamilySettings_Statics::NewProp_Families_Inner = { "Families", nullptr, (EPropertyFlags)0x0000000000004000, UECodeGen_Private::EPropertyGenFlags::Struct, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, 0, Z_Construct_UScriptStruct_FRoadFamilyDefinition, METADATA_PARAMS(0, nullptr) }; // 3947480440
const UECodeGen_Private::FArrayPropertyParams Z_Construct_UClass_UTrafficRoadFamilySettings_Statics::NewProp_Families = { "Families", nullptr, (EPropertyFlags)0x0010000000004001, UECodeGen_Private::EPropertyGenFlags::Array, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(UTrafficRoadFamilySettings, Families), EArrayPropertyFlags::None, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_Families_MetaData), NewProp_Families_MetaData) }; // 3947480440
const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UClass_UTrafficRoadFamilySettings_Statics::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UTrafficRoadFamilySettings_Statics::NewProp_Families_Inner,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UTrafficRoadFamilySettings_Statics::NewProp_Families,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_UTrafficRoadFamilySettings_Statics::PropPointers) < 2048);
// ********** End Class UTrafficRoadFamilySettings Property Definitions ****************************
UObject* (*const Z_Construct_UClass_UTrafficRoadFamilySettings_Statics::DependentSingletons[])() = {
	(UObject* (*)())Z_Construct_UClass_UDeveloperSettings,
	(UObject* (*)())Z_Construct_UPackage__Script_TrafficRuntime,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_UTrafficRoadFamilySettings_Statics::DependentSingletons) < 16);
const UECodeGen_Private::FClassParams Z_Construct_UClass_UTrafficRoadFamilySettings_Statics::ClassParams = {
	&UTrafficRoadFamilySettings::StaticClass,
	"Game",
	&StaticCppClassTypeInfo,
	DependentSingletons,
	nullptr,
	Z_Construct_UClass_UTrafficRoadFamilySettings_Statics::PropPointers,
	nullptr,
	UE_ARRAY_COUNT(DependentSingletons),
	0,
	UE_ARRAY_COUNT(Z_Construct_UClass_UTrafficRoadFamilySettings_Statics::PropPointers),
	0,
	0x001000A6u,
	METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UClass_UTrafficRoadFamilySettings_Statics::Class_MetaDataParams), Z_Construct_UClass_UTrafficRoadFamilySettings_Statics::Class_MetaDataParams)
};
void UTrafficRoadFamilySettings::StaticRegisterNativesUTrafficRoadFamilySettings()
{
}
UClass* Z_Construct_UClass_UTrafficRoadFamilySettings()
{
	if (!Z_Registration_Info_UClass_UTrafficRoadFamilySettings.OuterSingleton)
	{
		UECodeGen_Private::ConstructUClass(Z_Registration_Info_UClass_UTrafficRoadFamilySettings.OuterSingleton, Z_Construct_UClass_UTrafficRoadFamilySettings_Statics::ClassParams);
	}
	return Z_Registration_Info_UClass_UTrafficRoadFamilySettings.OuterSingleton;
}
DEFINE_VTABLE_PTR_HELPER_CTOR_NS(, UTrafficRoadFamilySettings);
UTrafficRoadFamilySettings::~UTrafficRoadFamilySettings() {}
// ********** End Class UTrafficRoadFamilySettings *************************************************

// ********** Begin Registration *******************************************************************
struct Z_CompiledInDeferFile_FID_Users_rllax_L_E_O_Plugins_AAAtrafficSystem_Source_TrafficRuntime_Public_TrafficRoadFamilySettings_h__Script_TrafficRuntime_Statics
{
	static constexpr FStructRegisterCompiledInInfo ScriptStructInfo[] = {
		{ FTrafficLaneLayoutSide::StaticStruct, Z_Construct_UScriptStruct_FTrafficLaneLayoutSide_Statics::NewStructOps, TEXT("TrafficLaneLayoutSide"),&Z_Registration_Info_UScriptStruct_FTrafficLaneLayoutSide, CONSTRUCT_RELOAD_VERSION_INFO(FStructReloadVersionInfo, sizeof(FTrafficLaneLayoutSide), 3849306716U) },
		{ FRoadFamilyDefinition::StaticStruct, Z_Construct_UScriptStruct_FRoadFamilyDefinition_Statics::NewStructOps, TEXT("RoadFamilyDefinition"),&Z_Registration_Info_UScriptStruct_FRoadFamilyDefinition, CONSTRUCT_RELOAD_VERSION_INFO(FStructReloadVersionInfo, sizeof(FRoadFamilyDefinition), 3947480440U) },
	};
	static constexpr FClassRegisterCompiledInInfo ClassInfo[] = {
		{ Z_Construct_UClass_UTrafficRoadFamilySettings, UTrafficRoadFamilySettings::StaticClass, TEXT("UTrafficRoadFamilySettings"), &Z_Registration_Info_UClass_UTrafficRoadFamilySettings, CONSTRUCT_RELOAD_VERSION_INFO(FClassReloadVersionInfo, sizeof(UTrafficRoadFamilySettings), 2172041001U) },
	};
}; // Z_CompiledInDeferFile_FID_Users_rllax_L_E_O_Plugins_AAAtrafficSystem_Source_TrafficRuntime_Public_TrafficRoadFamilySettings_h__Script_TrafficRuntime_Statics 
static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_rllax_L_E_O_Plugins_AAAtrafficSystem_Source_TrafficRuntime_Public_TrafficRoadFamilySettings_h__Script_TrafficRuntime_3353187969{
	TEXT("/Script/TrafficRuntime"),
	Z_CompiledInDeferFile_FID_Users_rllax_L_E_O_Plugins_AAAtrafficSystem_Source_TrafficRuntime_Public_TrafficRoadFamilySettings_h__Script_TrafficRuntime_Statics::ClassInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Users_rllax_L_E_O_Plugins_AAAtrafficSystem_Source_TrafficRuntime_Public_TrafficRoadFamilySettings_h__Script_TrafficRuntime_Statics::ClassInfo),
	Z_CompiledInDeferFile_FID_Users_rllax_L_E_O_Plugins_AAAtrafficSystem_Source_TrafficRuntime_Public_TrafficRoadFamilySettings_h__Script_TrafficRuntime_Statics::ScriptStructInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Users_rllax_L_E_O_Plugins_AAAtrafficSystem_Source_TrafficRuntime_Public_TrafficRoadFamilySettings_h__Script_TrafficRuntime_Statics::ScriptStructInfo),
	nullptr, 0,
};
// ********** End Registration *********************************************************************

PRAGMA_ENABLE_DEPRECATION_WARNINGS
