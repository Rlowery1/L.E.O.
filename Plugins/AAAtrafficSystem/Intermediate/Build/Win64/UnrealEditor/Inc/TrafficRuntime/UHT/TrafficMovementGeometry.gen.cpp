// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "TrafficMovementGeometry.h"

PRAGMA_DISABLE_DEPRECATION_WARNINGS
static_assert(!UE_WITH_CONSTINIT_UOBJECT, "This generated code can only be compiled with !UE_WITH_CONSTINIT_OBJECT");
void EmptyLinkFunctionForGeneratedCodeTrafficMovementGeometry() {}

// ********** Begin Cross Module References ********************************************************
COREUOBJECT_API UScriptStruct* Z_Construct_UScriptStruct_FVector();
TRAFFICRUNTIME_API UScriptStruct* Z_Construct_UScriptStruct_FMovementSample();
UPackage* Z_Construct_UPackage__Script_TrafficRuntime();
// ********** End Cross Module References **********************************************************

// ********** Begin ScriptStruct FMovementSample ***************************************************
struct Z_Construct_UScriptStruct_FMovementSample_Statics
{
	static inline consteval int32 GetStructSize() { return sizeof(FMovementSample); }
	static inline consteval int16 GetStructAlignment() { return alignof(FMovementSample); }
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Struct_MetaDataParams[] = {
		{ "BlueprintType", "true" },
		{ "ModuleRelativePath", "Public/TrafficMovementGeometry.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_S_MetaData[] = {
		{ "ModuleRelativePath", "Public/TrafficMovementGeometry.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_Position_MetaData[] = {
		{ "ModuleRelativePath", "Public/TrafficMovementGeometry.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_Tangent_MetaData[] = {
		{ "ModuleRelativePath", "Public/TrafficMovementGeometry.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_Curvature_MetaData[] = {
		{ "ModuleRelativePath", "Public/TrafficMovementGeometry.h" },
	};
#endif // WITH_METADATA

// ********** Begin ScriptStruct FMovementSample constinit property declarations *******************
	static const UECodeGen_Private::FFloatPropertyParams NewProp_S;
	static const UECodeGen_Private::FStructPropertyParams NewProp_Position;
	static const UECodeGen_Private::FStructPropertyParams NewProp_Tangent;
	static const UECodeGen_Private::FFloatPropertyParams NewProp_Curvature;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
// ********** End ScriptStruct FMovementSample constinit property declarations *********************
	static void* NewStructOps()
	{
		return (UScriptStruct::ICppStructOps*)new UScriptStruct::TCppStructOps<FMovementSample>();
	}
	static const UECodeGen_Private::FStructParams StructParams;
}; // struct Z_Construct_UScriptStruct_FMovementSample_Statics
static FStructRegistrationInfo Z_Registration_Info_UScriptStruct_FMovementSample;
class UScriptStruct* FMovementSample::StaticStruct()
{
	if (!Z_Registration_Info_UScriptStruct_FMovementSample.OuterSingleton)
	{
		Z_Registration_Info_UScriptStruct_FMovementSample.OuterSingleton = GetStaticStruct(Z_Construct_UScriptStruct_FMovementSample, (UObject*)Z_Construct_UPackage__Script_TrafficRuntime(), TEXT("MovementSample"));
	}
	return Z_Registration_Info_UScriptStruct_FMovementSample.OuterSingleton;
	}

// ********** Begin ScriptStruct FMovementSample Property Definitions ******************************
const UECodeGen_Private::FFloatPropertyParams Z_Construct_UScriptStruct_FMovementSample_Statics::NewProp_S = { "S", nullptr, (EPropertyFlags)0x0010000000000000, UECodeGen_Private::EPropertyGenFlags::Float, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(FMovementSample, S), METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_S_MetaData), NewProp_S_MetaData) };
const UECodeGen_Private::FStructPropertyParams Z_Construct_UScriptStruct_FMovementSample_Statics::NewProp_Position = { "Position", nullptr, (EPropertyFlags)0x0010000000000000, UECodeGen_Private::EPropertyGenFlags::Struct, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(FMovementSample, Position), Z_Construct_UScriptStruct_FVector, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_Position_MetaData), NewProp_Position_MetaData) };
const UECodeGen_Private::FStructPropertyParams Z_Construct_UScriptStruct_FMovementSample_Statics::NewProp_Tangent = { "Tangent", nullptr, (EPropertyFlags)0x0010000000000000, UECodeGen_Private::EPropertyGenFlags::Struct, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(FMovementSample, Tangent), Z_Construct_UScriptStruct_FVector, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_Tangent_MetaData), NewProp_Tangent_MetaData) };
const UECodeGen_Private::FFloatPropertyParams Z_Construct_UScriptStruct_FMovementSample_Statics::NewProp_Curvature = { "Curvature", nullptr, (EPropertyFlags)0x0010000000000000, UECodeGen_Private::EPropertyGenFlags::Float, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(FMovementSample, Curvature), METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_Curvature_MetaData), NewProp_Curvature_MetaData) };
const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UScriptStruct_FMovementSample_Statics::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FMovementSample_Statics::NewProp_S,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FMovementSample_Statics::NewProp_Position,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FMovementSample_Statics::NewProp_Tangent,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FMovementSample_Statics::NewProp_Curvature,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FMovementSample_Statics::PropPointers) < 2048);
// ********** End ScriptStruct FMovementSample Property Definitions ********************************
const UECodeGen_Private::FStructParams Z_Construct_UScriptStruct_FMovementSample_Statics::StructParams = {
	(UObject* (*)())Z_Construct_UPackage__Script_TrafficRuntime,
	nullptr,
	&NewStructOps,
	"MovementSample",
	Z_Construct_UScriptStruct_FMovementSample_Statics::PropPointers,
	UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FMovementSample_Statics::PropPointers),
	sizeof(FMovementSample),
	alignof(FMovementSample),
	RF_Public|RF_Transient|RF_MarkAsNative,
	EStructFlags(0x00000201),
	METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FMovementSample_Statics::Struct_MetaDataParams), Z_Construct_UScriptStruct_FMovementSample_Statics::Struct_MetaDataParams)
};
UScriptStruct* Z_Construct_UScriptStruct_FMovementSample()
{
	if (!Z_Registration_Info_UScriptStruct_FMovementSample.InnerSingleton)
	{
		UECodeGen_Private::ConstructUScriptStruct(Z_Registration_Info_UScriptStruct_FMovementSample.InnerSingleton, Z_Construct_UScriptStruct_FMovementSample_Statics::StructParams);
	}
	return CastChecked<UScriptStruct>(Z_Registration_Info_UScriptStruct_FMovementSample.InnerSingleton);
}
// ********** End ScriptStruct FMovementSample *****************************************************

// ********** Begin Registration *******************************************************************
struct Z_CompiledInDeferFile_FID_Users_rllax_L_E_O_Plugins_AAAtrafficSystem_Source_TrafficRuntime_Public_TrafficMovementGeometry_h__Script_TrafficRuntime_Statics
{
	static constexpr FStructRegisterCompiledInInfo ScriptStructInfo[] = {
		{ FMovementSample::StaticStruct, Z_Construct_UScriptStruct_FMovementSample_Statics::NewStructOps, TEXT("MovementSample"),&Z_Registration_Info_UScriptStruct_FMovementSample, CONSTRUCT_RELOAD_VERSION_INFO(FStructReloadVersionInfo, sizeof(FMovementSample), 3747208988U) },
	};
}; // Z_CompiledInDeferFile_FID_Users_rllax_L_E_O_Plugins_AAAtrafficSystem_Source_TrafficRuntime_Public_TrafficMovementGeometry_h__Script_TrafficRuntime_Statics 
static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_rllax_L_E_O_Plugins_AAAtrafficSystem_Source_TrafficRuntime_Public_TrafficMovementGeometry_h__Script_TrafficRuntime_1987109822{
	TEXT("/Script/TrafficRuntime"),
	nullptr, 0,
	Z_CompiledInDeferFile_FID_Users_rllax_L_E_O_Plugins_AAAtrafficSystem_Source_TrafficRuntime_Public_TrafficMovementGeometry_h__Script_TrafficRuntime_Statics::ScriptStructInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Users_rllax_L_E_O_Plugins_AAAtrafficSystem_Source_TrafficRuntime_Public_TrafficMovementGeometry_h__Script_TrafficRuntime_Statics::ScriptStructInfo),
	nullptr, 0,
};
// ********** End Registration *********************************************************************

PRAGMA_ENABLE_DEPRECATION_WARNINGS
