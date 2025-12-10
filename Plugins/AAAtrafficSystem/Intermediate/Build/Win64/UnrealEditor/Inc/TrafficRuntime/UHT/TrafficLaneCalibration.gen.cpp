// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "TrafficLaneCalibration.h"

PRAGMA_DISABLE_DEPRECATION_WARNINGS
static_assert(!UE_WITH_CONSTINIT_UOBJECT, "This generated code can only be compiled with !UE_WITH_CONSTINIT_OBJECT");
void EmptyLinkFunctionForGeneratedCodeTrafficLaneCalibration() {}

// ********** Begin Cross Module References ********************************************************
TRAFFICRUNTIME_API UScriptStruct* Z_Construct_UScriptStruct_FLaneSurfaceCoverageMetrics();
TRAFFICRUNTIME_API UScriptStruct* Z_Construct_UScriptStruct_FTrafficLaneFamilyCalibration();
UPackage* Z_Construct_UPackage__Script_TrafficRuntime();
// ********** End Cross Module References **********************************************************

// ********** Begin ScriptStruct FLaneSurfaceCoverageMetrics ***************************************
struct Z_Construct_UScriptStruct_FLaneSurfaceCoverageMetrics_Statics
{
	static inline consteval int32 GetStructSize() { return sizeof(FLaneSurfaceCoverageMetrics); }
	static inline consteval int16 GetStructAlignment() { return alignof(FLaneSurfaceCoverageMetrics); }
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Struct_MetaDataParams[] = {
		{ "BlueprintType", "true" },
		{ "ModuleRelativePath", "Public/TrafficLaneCalibration.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_LaneId_MetaData[] = {
		{ "ModuleRelativePath", "Public/TrafficLaneCalibration.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_RoadId_MetaData[] = {
		{ "ModuleRelativePath", "Public/TrafficLaneCalibration.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_NumSamples_MetaData[] = {
		{ "ModuleRelativePath", "Public/TrafficLaneCalibration.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_NumSamplesOnRoad_MetaData[] = {
		{ "ModuleRelativePath", "Public/TrafficLaneCalibration.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_CoveragePercent_MetaData[] = {
		{ "ModuleRelativePath", "Public/TrafficLaneCalibration.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_MaxVerticalGapCm_MetaData[] = {
		{ "ModuleRelativePath", "Public/TrafficLaneCalibration.h" },
	};
#endif // WITH_METADATA

// ********** Begin ScriptStruct FLaneSurfaceCoverageMetrics constinit property declarations *******
	static const UECodeGen_Private::FIntPropertyParams NewProp_LaneId;
	static const UECodeGen_Private::FIntPropertyParams NewProp_RoadId;
	static const UECodeGen_Private::FIntPropertyParams NewProp_NumSamples;
	static const UECodeGen_Private::FIntPropertyParams NewProp_NumSamplesOnRoad;
	static const UECodeGen_Private::FFloatPropertyParams NewProp_CoveragePercent;
	static const UECodeGen_Private::FFloatPropertyParams NewProp_MaxVerticalGapCm;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
// ********** End ScriptStruct FLaneSurfaceCoverageMetrics constinit property declarations *********
	static void* NewStructOps()
	{
		return (UScriptStruct::ICppStructOps*)new UScriptStruct::TCppStructOps<FLaneSurfaceCoverageMetrics>();
	}
	static const UECodeGen_Private::FStructParams StructParams;
}; // struct Z_Construct_UScriptStruct_FLaneSurfaceCoverageMetrics_Statics
static FStructRegistrationInfo Z_Registration_Info_UScriptStruct_FLaneSurfaceCoverageMetrics;
class UScriptStruct* FLaneSurfaceCoverageMetrics::StaticStruct()
{
	if (!Z_Registration_Info_UScriptStruct_FLaneSurfaceCoverageMetrics.OuterSingleton)
	{
		Z_Registration_Info_UScriptStruct_FLaneSurfaceCoverageMetrics.OuterSingleton = GetStaticStruct(Z_Construct_UScriptStruct_FLaneSurfaceCoverageMetrics, (UObject*)Z_Construct_UPackage__Script_TrafficRuntime(), TEXT("LaneSurfaceCoverageMetrics"));
	}
	return Z_Registration_Info_UScriptStruct_FLaneSurfaceCoverageMetrics.OuterSingleton;
	}

// ********** Begin ScriptStruct FLaneSurfaceCoverageMetrics Property Definitions ******************
const UECodeGen_Private::FIntPropertyParams Z_Construct_UScriptStruct_FLaneSurfaceCoverageMetrics_Statics::NewProp_LaneId = { "LaneId", nullptr, (EPropertyFlags)0x0010000000000000, UECodeGen_Private::EPropertyGenFlags::Int, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(FLaneSurfaceCoverageMetrics, LaneId), METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_LaneId_MetaData), NewProp_LaneId_MetaData) };
const UECodeGen_Private::FIntPropertyParams Z_Construct_UScriptStruct_FLaneSurfaceCoverageMetrics_Statics::NewProp_RoadId = { "RoadId", nullptr, (EPropertyFlags)0x0010000000000000, UECodeGen_Private::EPropertyGenFlags::Int, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(FLaneSurfaceCoverageMetrics, RoadId), METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_RoadId_MetaData), NewProp_RoadId_MetaData) };
const UECodeGen_Private::FIntPropertyParams Z_Construct_UScriptStruct_FLaneSurfaceCoverageMetrics_Statics::NewProp_NumSamples = { "NumSamples", nullptr, (EPropertyFlags)0x0010000000000000, UECodeGen_Private::EPropertyGenFlags::Int, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(FLaneSurfaceCoverageMetrics, NumSamples), METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_NumSamples_MetaData), NewProp_NumSamples_MetaData) };
const UECodeGen_Private::FIntPropertyParams Z_Construct_UScriptStruct_FLaneSurfaceCoverageMetrics_Statics::NewProp_NumSamplesOnRoad = { "NumSamplesOnRoad", nullptr, (EPropertyFlags)0x0010000000000000, UECodeGen_Private::EPropertyGenFlags::Int, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(FLaneSurfaceCoverageMetrics, NumSamplesOnRoad), METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_NumSamplesOnRoad_MetaData), NewProp_NumSamplesOnRoad_MetaData) };
const UECodeGen_Private::FFloatPropertyParams Z_Construct_UScriptStruct_FLaneSurfaceCoverageMetrics_Statics::NewProp_CoveragePercent = { "CoveragePercent", nullptr, (EPropertyFlags)0x0010000000000000, UECodeGen_Private::EPropertyGenFlags::Float, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(FLaneSurfaceCoverageMetrics, CoveragePercent), METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_CoveragePercent_MetaData), NewProp_CoveragePercent_MetaData) };
const UECodeGen_Private::FFloatPropertyParams Z_Construct_UScriptStruct_FLaneSurfaceCoverageMetrics_Statics::NewProp_MaxVerticalGapCm = { "MaxVerticalGapCm", nullptr, (EPropertyFlags)0x0010000000000000, UECodeGen_Private::EPropertyGenFlags::Float, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(FLaneSurfaceCoverageMetrics, MaxVerticalGapCm), METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_MaxVerticalGapCm_MetaData), NewProp_MaxVerticalGapCm_MetaData) };
const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UScriptStruct_FLaneSurfaceCoverageMetrics_Statics::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FLaneSurfaceCoverageMetrics_Statics::NewProp_LaneId,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FLaneSurfaceCoverageMetrics_Statics::NewProp_RoadId,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FLaneSurfaceCoverageMetrics_Statics::NewProp_NumSamples,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FLaneSurfaceCoverageMetrics_Statics::NewProp_NumSamplesOnRoad,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FLaneSurfaceCoverageMetrics_Statics::NewProp_CoveragePercent,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FLaneSurfaceCoverageMetrics_Statics::NewProp_MaxVerticalGapCm,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FLaneSurfaceCoverageMetrics_Statics::PropPointers) < 2048);
// ********** End ScriptStruct FLaneSurfaceCoverageMetrics Property Definitions ********************
const UECodeGen_Private::FStructParams Z_Construct_UScriptStruct_FLaneSurfaceCoverageMetrics_Statics::StructParams = {
	(UObject* (*)())Z_Construct_UPackage__Script_TrafficRuntime,
	nullptr,
	&NewStructOps,
	"LaneSurfaceCoverageMetrics",
	Z_Construct_UScriptStruct_FLaneSurfaceCoverageMetrics_Statics::PropPointers,
	UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FLaneSurfaceCoverageMetrics_Statics::PropPointers),
	sizeof(FLaneSurfaceCoverageMetrics),
	alignof(FLaneSurfaceCoverageMetrics),
	RF_Public|RF_Transient|RF_MarkAsNative,
	EStructFlags(0x00000201),
	METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FLaneSurfaceCoverageMetrics_Statics::Struct_MetaDataParams), Z_Construct_UScriptStruct_FLaneSurfaceCoverageMetrics_Statics::Struct_MetaDataParams)
};
UScriptStruct* Z_Construct_UScriptStruct_FLaneSurfaceCoverageMetrics()
{
	if (!Z_Registration_Info_UScriptStruct_FLaneSurfaceCoverageMetrics.InnerSingleton)
	{
		UECodeGen_Private::ConstructUScriptStruct(Z_Registration_Info_UScriptStruct_FLaneSurfaceCoverageMetrics.InnerSingleton, Z_Construct_UScriptStruct_FLaneSurfaceCoverageMetrics_Statics::StructParams);
	}
	return CastChecked<UScriptStruct>(Z_Registration_Info_UScriptStruct_FLaneSurfaceCoverageMetrics.InnerSingleton);
}
// ********** End ScriptStruct FLaneSurfaceCoverageMetrics *****************************************

// ********** Begin ScriptStruct FTrafficLaneFamilyCalibration *************************************
struct Z_Construct_UScriptStruct_FTrafficLaneFamilyCalibration_Statics
{
	static inline consteval int32 GetStructSize() { return sizeof(FTrafficLaneFamilyCalibration); }
	static inline consteval int16 GetStructAlignment() { return alignof(FTrafficLaneFamilyCalibration); }
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Struct_MetaDataParams[] = {
		{ "BlueprintType", "true" },
		{ "ModuleRelativePath", "Public/TrafficLaneCalibration.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_NumLanesPerSideForward_MetaData[] = {
		{ "Category", "Traffic" },
		{ "ModuleRelativePath", "Public/TrafficLaneCalibration.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_NumLanesPerSideBackward_MetaData[] = {
		{ "Category", "Traffic" },
		{ "ModuleRelativePath", "Public/TrafficLaneCalibration.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_LaneWidthCm_MetaData[] = {
		{ "Category", "Traffic" },
		{ "ClampMax", "1000.0" },
		{ "ClampMin", "50.0" },
		{ "ModuleRelativePath", "Public/TrafficLaneCalibration.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_CenterlineOffsetCm_MetaData[] = {
		{ "Category", "Traffic" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** Lateral offset from center for the first lane on each side. */" },
#endif
		{ "ModuleRelativePath", "Public/TrafficLaneCalibration.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Lateral offset from center for the first lane on each side." },
#endif
	};
#endif // WITH_METADATA

// ********** Begin ScriptStruct FTrafficLaneFamilyCalibration constinit property declarations *****
	static const UECodeGen_Private::FIntPropertyParams NewProp_NumLanesPerSideForward;
	static const UECodeGen_Private::FIntPropertyParams NewProp_NumLanesPerSideBackward;
	static const UECodeGen_Private::FFloatPropertyParams NewProp_LaneWidthCm;
	static const UECodeGen_Private::FFloatPropertyParams NewProp_CenterlineOffsetCm;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
// ********** End ScriptStruct FTrafficLaneFamilyCalibration constinit property declarations *******
	static void* NewStructOps()
	{
		return (UScriptStruct::ICppStructOps*)new UScriptStruct::TCppStructOps<FTrafficLaneFamilyCalibration>();
	}
	static const UECodeGen_Private::FStructParams StructParams;
}; // struct Z_Construct_UScriptStruct_FTrafficLaneFamilyCalibration_Statics
static FStructRegistrationInfo Z_Registration_Info_UScriptStruct_FTrafficLaneFamilyCalibration;
class UScriptStruct* FTrafficLaneFamilyCalibration::StaticStruct()
{
	if (!Z_Registration_Info_UScriptStruct_FTrafficLaneFamilyCalibration.OuterSingleton)
	{
		Z_Registration_Info_UScriptStruct_FTrafficLaneFamilyCalibration.OuterSingleton = GetStaticStruct(Z_Construct_UScriptStruct_FTrafficLaneFamilyCalibration, (UObject*)Z_Construct_UPackage__Script_TrafficRuntime(), TEXT("TrafficLaneFamilyCalibration"));
	}
	return Z_Registration_Info_UScriptStruct_FTrafficLaneFamilyCalibration.OuterSingleton;
	}

// ********** Begin ScriptStruct FTrafficLaneFamilyCalibration Property Definitions ****************
const UECodeGen_Private::FIntPropertyParams Z_Construct_UScriptStruct_FTrafficLaneFamilyCalibration_Statics::NewProp_NumLanesPerSideForward = { "NumLanesPerSideForward", nullptr, (EPropertyFlags)0x0010000000004001, UECodeGen_Private::EPropertyGenFlags::Int, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(FTrafficLaneFamilyCalibration, NumLanesPerSideForward), METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_NumLanesPerSideForward_MetaData), NewProp_NumLanesPerSideForward_MetaData) };
const UECodeGen_Private::FIntPropertyParams Z_Construct_UScriptStruct_FTrafficLaneFamilyCalibration_Statics::NewProp_NumLanesPerSideBackward = { "NumLanesPerSideBackward", nullptr, (EPropertyFlags)0x0010000000004001, UECodeGen_Private::EPropertyGenFlags::Int, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(FTrafficLaneFamilyCalibration, NumLanesPerSideBackward), METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_NumLanesPerSideBackward_MetaData), NewProp_NumLanesPerSideBackward_MetaData) };
const UECodeGen_Private::FFloatPropertyParams Z_Construct_UScriptStruct_FTrafficLaneFamilyCalibration_Statics::NewProp_LaneWidthCm = { "LaneWidthCm", nullptr, (EPropertyFlags)0x0010000000004001, UECodeGen_Private::EPropertyGenFlags::Float, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(FTrafficLaneFamilyCalibration, LaneWidthCm), METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_LaneWidthCm_MetaData), NewProp_LaneWidthCm_MetaData) };
const UECodeGen_Private::FFloatPropertyParams Z_Construct_UScriptStruct_FTrafficLaneFamilyCalibration_Statics::NewProp_CenterlineOffsetCm = { "CenterlineOffsetCm", nullptr, (EPropertyFlags)0x0010000000004001, UECodeGen_Private::EPropertyGenFlags::Float, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(FTrafficLaneFamilyCalibration, CenterlineOffsetCm), METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_CenterlineOffsetCm_MetaData), NewProp_CenterlineOffsetCm_MetaData) };
const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UScriptStruct_FTrafficLaneFamilyCalibration_Statics::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FTrafficLaneFamilyCalibration_Statics::NewProp_NumLanesPerSideForward,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FTrafficLaneFamilyCalibration_Statics::NewProp_NumLanesPerSideBackward,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FTrafficLaneFamilyCalibration_Statics::NewProp_LaneWidthCm,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FTrafficLaneFamilyCalibration_Statics::NewProp_CenterlineOffsetCm,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FTrafficLaneFamilyCalibration_Statics::PropPointers) < 2048);
// ********** End ScriptStruct FTrafficLaneFamilyCalibration Property Definitions ******************
const UECodeGen_Private::FStructParams Z_Construct_UScriptStruct_FTrafficLaneFamilyCalibration_Statics::StructParams = {
	(UObject* (*)())Z_Construct_UPackage__Script_TrafficRuntime,
	nullptr,
	&NewStructOps,
	"TrafficLaneFamilyCalibration",
	Z_Construct_UScriptStruct_FTrafficLaneFamilyCalibration_Statics::PropPointers,
	UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FTrafficLaneFamilyCalibration_Statics::PropPointers),
	sizeof(FTrafficLaneFamilyCalibration),
	alignof(FTrafficLaneFamilyCalibration),
	RF_Public|RF_Transient|RF_MarkAsNative,
	EStructFlags(0x00000201),
	METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FTrafficLaneFamilyCalibration_Statics::Struct_MetaDataParams), Z_Construct_UScriptStruct_FTrafficLaneFamilyCalibration_Statics::Struct_MetaDataParams)
};
UScriptStruct* Z_Construct_UScriptStruct_FTrafficLaneFamilyCalibration()
{
	if (!Z_Registration_Info_UScriptStruct_FTrafficLaneFamilyCalibration.InnerSingleton)
	{
		UECodeGen_Private::ConstructUScriptStruct(Z_Registration_Info_UScriptStruct_FTrafficLaneFamilyCalibration.InnerSingleton, Z_Construct_UScriptStruct_FTrafficLaneFamilyCalibration_Statics::StructParams);
	}
	return CastChecked<UScriptStruct>(Z_Registration_Info_UScriptStruct_FTrafficLaneFamilyCalibration.InnerSingleton);
}
// ********** End ScriptStruct FTrafficLaneFamilyCalibration ***************************************

// ********** Begin Registration *******************************************************************
struct Z_CompiledInDeferFile_FID_Users_rllax_L_E_O_Plugins_AAAtrafficSystem_Source_TrafficRuntime_Public_TrafficLaneCalibration_h__Script_TrafficRuntime_Statics
{
	static constexpr FStructRegisterCompiledInInfo ScriptStructInfo[] = {
		{ FLaneSurfaceCoverageMetrics::StaticStruct, Z_Construct_UScriptStruct_FLaneSurfaceCoverageMetrics_Statics::NewStructOps, TEXT("LaneSurfaceCoverageMetrics"),&Z_Registration_Info_UScriptStruct_FLaneSurfaceCoverageMetrics, CONSTRUCT_RELOAD_VERSION_INFO(FStructReloadVersionInfo, sizeof(FLaneSurfaceCoverageMetrics), 1656254902U) },
		{ FTrafficLaneFamilyCalibration::StaticStruct, Z_Construct_UScriptStruct_FTrafficLaneFamilyCalibration_Statics::NewStructOps, TEXT("TrafficLaneFamilyCalibration"),&Z_Registration_Info_UScriptStruct_FTrafficLaneFamilyCalibration, CONSTRUCT_RELOAD_VERSION_INFO(FStructReloadVersionInfo, sizeof(FTrafficLaneFamilyCalibration), 1801600758U) },
	};
}; // Z_CompiledInDeferFile_FID_Users_rllax_L_E_O_Plugins_AAAtrafficSystem_Source_TrafficRuntime_Public_TrafficLaneCalibration_h__Script_TrafficRuntime_Statics 
static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_rllax_L_E_O_Plugins_AAAtrafficSystem_Source_TrafficRuntime_Public_TrafficLaneCalibration_h__Script_TrafficRuntime_181367527{
	TEXT("/Script/TrafficRuntime"),
	nullptr, 0,
	Z_CompiledInDeferFile_FID_Users_rllax_L_E_O_Plugins_AAAtrafficSystem_Source_TrafficRuntime_Public_TrafficLaneCalibration_h__Script_TrafficRuntime_Statics::ScriptStructInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Users_rllax_L_E_O_Plugins_AAAtrafficSystem_Source_TrafficRuntime_Public_TrafficLaneCalibration_h__Script_TrafficRuntime_Statics::ScriptStructInfo),
	nullptr, 0,
};
// ********** End Registration *********************************************************************

PRAGMA_ENABLE_DEPRECATION_WARNINGS
