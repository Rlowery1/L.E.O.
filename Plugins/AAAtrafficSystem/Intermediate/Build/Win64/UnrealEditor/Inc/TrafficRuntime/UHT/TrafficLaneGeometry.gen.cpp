// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "TrafficLaneGeometry.h"

PRAGMA_DISABLE_DEPRECATION_WARNINGS
static_assert(!UE_WITH_CONSTINIT_UOBJECT, "This generated code can only be compiled with !UE_WITH_CONSTINIT_OBJECT");
void EmptyLinkFunctionForGeneratedCodeTrafficLaneGeometry() {}

// ********** Begin Cross Module References ********************************************************
COREUOBJECT_API UScriptStruct* Z_Construct_UScriptStruct_FVector();
TRAFFICRUNTIME_API UScriptStruct* Z_Construct_UScriptStruct_FLaneProjectionResult();
UPackage* Z_Construct_UPackage__Script_TrafficRuntime();
// ********** End Cross Module References **********************************************************

// ********** Begin ScriptStruct FLaneProjectionResult *********************************************
struct Z_Construct_UScriptStruct_FLaneProjectionResult_Statics
{
	static inline consteval int32 GetStructSize() { return sizeof(FLaneProjectionResult); }
	static inline consteval int16 GetStructAlignment() { return alignof(FLaneProjectionResult); }
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Struct_MetaDataParams[] = {
		{ "BlueprintType", "true" },
		{ "ModuleRelativePath", "Public/TrafficLaneGeometry.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_LaneId_MetaData[] = {
		{ "ModuleRelativePath", "Public/TrafficLaneGeometry.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_S_MetaData[] = {
		{ "ModuleRelativePath", "Public/TrafficLaneGeometry.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_ClosestPoint_MetaData[] = {
		{ "ModuleRelativePath", "Public/TrafficLaneGeometry.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_LateralOffsetCm_MetaData[] = {
		{ "ModuleRelativePath", "Public/TrafficLaneGeometry.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_HeadingErrorDeg_MetaData[] = {
		{ "ModuleRelativePath", "Public/TrafficLaneGeometry.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_SegmentIndex_MetaData[] = {
		{ "ModuleRelativePath", "Public/TrafficLaneGeometry.h" },
	};
#endif // WITH_METADATA

// ********** Begin ScriptStruct FLaneProjectionResult constinit property declarations *************
	static const UECodeGen_Private::FIntPropertyParams NewProp_LaneId;
	static const UECodeGen_Private::FFloatPropertyParams NewProp_S;
	static const UECodeGen_Private::FStructPropertyParams NewProp_ClosestPoint;
	static const UECodeGen_Private::FFloatPropertyParams NewProp_LateralOffsetCm;
	static const UECodeGen_Private::FFloatPropertyParams NewProp_HeadingErrorDeg;
	static const UECodeGen_Private::FIntPropertyParams NewProp_SegmentIndex;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
// ********** End ScriptStruct FLaneProjectionResult constinit property declarations ***************
	static void* NewStructOps()
	{
		return (UScriptStruct::ICppStructOps*)new UScriptStruct::TCppStructOps<FLaneProjectionResult>();
	}
	static const UECodeGen_Private::FStructParams StructParams;
}; // struct Z_Construct_UScriptStruct_FLaneProjectionResult_Statics
static FStructRegistrationInfo Z_Registration_Info_UScriptStruct_FLaneProjectionResult;
class UScriptStruct* FLaneProjectionResult::StaticStruct()
{
	if (!Z_Registration_Info_UScriptStruct_FLaneProjectionResult.OuterSingleton)
	{
		Z_Registration_Info_UScriptStruct_FLaneProjectionResult.OuterSingleton = GetStaticStruct(Z_Construct_UScriptStruct_FLaneProjectionResult, (UObject*)Z_Construct_UPackage__Script_TrafficRuntime(), TEXT("LaneProjectionResult"));
	}
	return Z_Registration_Info_UScriptStruct_FLaneProjectionResult.OuterSingleton;
	}

// ********** Begin ScriptStruct FLaneProjectionResult Property Definitions ************************
const UECodeGen_Private::FIntPropertyParams Z_Construct_UScriptStruct_FLaneProjectionResult_Statics::NewProp_LaneId = { "LaneId", nullptr, (EPropertyFlags)0x0010000000000000, UECodeGen_Private::EPropertyGenFlags::Int, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(FLaneProjectionResult, LaneId), METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_LaneId_MetaData), NewProp_LaneId_MetaData) };
const UECodeGen_Private::FFloatPropertyParams Z_Construct_UScriptStruct_FLaneProjectionResult_Statics::NewProp_S = { "S", nullptr, (EPropertyFlags)0x0010000000000000, UECodeGen_Private::EPropertyGenFlags::Float, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(FLaneProjectionResult, S), METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_S_MetaData), NewProp_S_MetaData) };
const UECodeGen_Private::FStructPropertyParams Z_Construct_UScriptStruct_FLaneProjectionResult_Statics::NewProp_ClosestPoint = { "ClosestPoint", nullptr, (EPropertyFlags)0x0010000000000000, UECodeGen_Private::EPropertyGenFlags::Struct, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(FLaneProjectionResult, ClosestPoint), Z_Construct_UScriptStruct_FVector, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_ClosestPoint_MetaData), NewProp_ClosestPoint_MetaData) };
const UECodeGen_Private::FFloatPropertyParams Z_Construct_UScriptStruct_FLaneProjectionResult_Statics::NewProp_LateralOffsetCm = { "LateralOffsetCm", nullptr, (EPropertyFlags)0x0010000000000000, UECodeGen_Private::EPropertyGenFlags::Float, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(FLaneProjectionResult, LateralOffsetCm), METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_LateralOffsetCm_MetaData), NewProp_LateralOffsetCm_MetaData) };
const UECodeGen_Private::FFloatPropertyParams Z_Construct_UScriptStruct_FLaneProjectionResult_Statics::NewProp_HeadingErrorDeg = { "HeadingErrorDeg", nullptr, (EPropertyFlags)0x0010000000000000, UECodeGen_Private::EPropertyGenFlags::Float, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(FLaneProjectionResult, HeadingErrorDeg), METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_HeadingErrorDeg_MetaData), NewProp_HeadingErrorDeg_MetaData) };
const UECodeGen_Private::FIntPropertyParams Z_Construct_UScriptStruct_FLaneProjectionResult_Statics::NewProp_SegmentIndex = { "SegmentIndex", nullptr, (EPropertyFlags)0x0010000000000000, UECodeGen_Private::EPropertyGenFlags::Int, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(FLaneProjectionResult, SegmentIndex), METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_SegmentIndex_MetaData), NewProp_SegmentIndex_MetaData) };
const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UScriptStruct_FLaneProjectionResult_Statics::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FLaneProjectionResult_Statics::NewProp_LaneId,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FLaneProjectionResult_Statics::NewProp_S,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FLaneProjectionResult_Statics::NewProp_ClosestPoint,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FLaneProjectionResult_Statics::NewProp_LateralOffsetCm,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FLaneProjectionResult_Statics::NewProp_HeadingErrorDeg,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FLaneProjectionResult_Statics::NewProp_SegmentIndex,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FLaneProjectionResult_Statics::PropPointers) < 2048);
// ********** End ScriptStruct FLaneProjectionResult Property Definitions **************************
const UECodeGen_Private::FStructParams Z_Construct_UScriptStruct_FLaneProjectionResult_Statics::StructParams = {
	(UObject* (*)())Z_Construct_UPackage__Script_TrafficRuntime,
	nullptr,
	&NewStructOps,
	"LaneProjectionResult",
	Z_Construct_UScriptStruct_FLaneProjectionResult_Statics::PropPointers,
	UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FLaneProjectionResult_Statics::PropPointers),
	sizeof(FLaneProjectionResult),
	alignof(FLaneProjectionResult),
	RF_Public|RF_Transient|RF_MarkAsNative,
	EStructFlags(0x00000201),
	METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FLaneProjectionResult_Statics::Struct_MetaDataParams), Z_Construct_UScriptStruct_FLaneProjectionResult_Statics::Struct_MetaDataParams)
};
UScriptStruct* Z_Construct_UScriptStruct_FLaneProjectionResult()
{
	if (!Z_Registration_Info_UScriptStruct_FLaneProjectionResult.InnerSingleton)
	{
		UECodeGen_Private::ConstructUScriptStruct(Z_Registration_Info_UScriptStruct_FLaneProjectionResult.InnerSingleton, Z_Construct_UScriptStruct_FLaneProjectionResult_Statics::StructParams);
	}
	return CastChecked<UScriptStruct>(Z_Registration_Info_UScriptStruct_FLaneProjectionResult.InnerSingleton);
}
// ********** End ScriptStruct FLaneProjectionResult ***********************************************

// ********** Begin Registration *******************************************************************
struct Z_CompiledInDeferFile_FID_Users_rllax_L_E_O_Plugins_AAAtrafficSystem_Source_TrafficRuntime_Public_TrafficLaneGeometry_h__Script_TrafficRuntime_Statics
{
	static constexpr FStructRegisterCompiledInInfo ScriptStructInfo[] = {
		{ FLaneProjectionResult::StaticStruct, Z_Construct_UScriptStruct_FLaneProjectionResult_Statics::NewStructOps, TEXT("LaneProjectionResult"),&Z_Registration_Info_UScriptStruct_FLaneProjectionResult, CONSTRUCT_RELOAD_VERSION_INFO(FStructReloadVersionInfo, sizeof(FLaneProjectionResult), 2725863686U) },
	};
}; // Z_CompiledInDeferFile_FID_Users_rllax_L_E_O_Plugins_AAAtrafficSystem_Source_TrafficRuntime_Public_TrafficLaneGeometry_h__Script_TrafficRuntime_Statics 
static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_rllax_L_E_O_Plugins_AAAtrafficSystem_Source_TrafficRuntime_Public_TrafficLaneGeometry_h__Script_TrafficRuntime_279088338{
	TEXT("/Script/TrafficRuntime"),
	nullptr, 0,
	Z_CompiledInDeferFile_FID_Users_rllax_L_E_O_Plugins_AAAtrafficSystem_Source_TrafficRuntime_Public_TrafficLaneGeometry_h__Script_TrafficRuntime_Statics::ScriptStructInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Users_rllax_L_E_O_Plugins_AAAtrafficSystem_Source_TrafficRuntime_Public_TrafficLaneGeometry_h__Script_TrafficRuntime_Statics::ScriptStructInfo),
	nullptr, 0,
};
// ********** End Registration *********************************************************************

PRAGMA_ENABLE_DEPRECATION_WARNINGS
