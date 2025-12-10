// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "TrafficRoadTypes.h"

PRAGMA_DISABLE_DEPRECATION_WARNINGS
static_assert(!UE_WITH_CONSTINIT_UOBJECT, "This generated code can only be compiled with !UE_WITH_CONSTINIT_OBJECT");
void EmptyLinkFunctionForGeneratedCodeTrafficRoadTypes() {}

// ********** Begin Cross Module References ********************************************************
COREUOBJECT_API UScriptStruct* Z_Construct_UScriptStruct_FVector();
ENGINE_API UClass* Z_Construct_UClass_AActor_NoRegister();
TRAFFICRUNTIME_API UEnum* Z_Construct_UEnum_TrafficRuntime_ELaneDirection();
TRAFFICRUNTIME_API UEnum* Z_Construct_UEnum_TrafficRuntime_ETrafficTurnType();
TRAFFICRUNTIME_API UScriptStruct* Z_Construct_UScriptStruct_FTrafficIntersection();
TRAFFICRUNTIME_API UScriptStruct* Z_Construct_UScriptStruct_FTrafficLane();
TRAFFICRUNTIME_API UScriptStruct* Z_Construct_UScriptStruct_FTrafficMovement();
TRAFFICRUNTIME_API UScriptStruct* Z_Construct_UScriptStruct_FTrafficNetwork();
TRAFFICRUNTIME_API UScriptStruct* Z_Construct_UScriptStruct_FTrafficRoad();
UPackage* Z_Construct_UPackage__Script_TrafficRuntime();
// ********** End Cross Module References **********************************************************

// ********** Begin Enum ELaneDirection ************************************************************
static FEnumRegistrationInfo Z_Registration_Info_UEnum_ELaneDirection;
static UEnum* ELaneDirection_StaticEnum()
{
	if (!Z_Registration_Info_UEnum_ELaneDirection.OuterSingleton)
	{
		Z_Registration_Info_UEnum_ELaneDirection.OuterSingleton = GetStaticEnum(Z_Construct_UEnum_TrafficRuntime_ELaneDirection, (UObject*)Z_Construct_UPackage__Script_TrafficRuntime(), TEXT("ELaneDirection"));
	}
	return Z_Registration_Info_UEnum_ELaneDirection.OuterSingleton;
}
template<> TRAFFICRUNTIME_NON_ATTRIBUTED_API UEnum* StaticEnum<ELaneDirection>()
{
	return ELaneDirection_StaticEnum();
}
struct Z_Construct_UEnum_TrafficRuntime_ELaneDirection_Statics
{
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Enum_MetaDataParams[] = {
		{ "Backward.Name", "ELaneDirection::Backward" },
		{ "Bidirectional.Name", "ELaneDirection::Bidirectional" },
		{ "BlueprintType", "true" },
		{ "Forward.Name", "ELaneDirection::Forward" },
		{ "ModuleRelativePath", "Public/TrafficRoadTypes.h" },
	};
#endif // WITH_METADATA
	static constexpr UECodeGen_Private::FEnumeratorParam Enumerators[] = {
		{ "ELaneDirection::Forward", (int64)ELaneDirection::Forward },
		{ "ELaneDirection::Backward", (int64)ELaneDirection::Backward },
		{ "ELaneDirection::Bidirectional", (int64)ELaneDirection::Bidirectional },
	};
	static const UECodeGen_Private::FEnumParams EnumParams;
}; // struct Z_Construct_UEnum_TrafficRuntime_ELaneDirection_Statics 
const UECodeGen_Private::FEnumParams Z_Construct_UEnum_TrafficRuntime_ELaneDirection_Statics::EnumParams = {
	(UObject*(*)())Z_Construct_UPackage__Script_TrafficRuntime,
	nullptr,
	"ELaneDirection",
	"ELaneDirection",
	Z_Construct_UEnum_TrafficRuntime_ELaneDirection_Statics::Enumerators,
	RF_Public|RF_Transient|RF_MarkAsNative,
	UE_ARRAY_COUNT(Z_Construct_UEnum_TrafficRuntime_ELaneDirection_Statics::Enumerators),
	EEnumFlags::None,
	(uint8)UEnum::ECppForm::EnumClass,
	METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UEnum_TrafficRuntime_ELaneDirection_Statics::Enum_MetaDataParams), Z_Construct_UEnum_TrafficRuntime_ELaneDirection_Statics::Enum_MetaDataParams)
};
UEnum* Z_Construct_UEnum_TrafficRuntime_ELaneDirection()
{
	if (!Z_Registration_Info_UEnum_ELaneDirection.InnerSingleton)
	{
		UECodeGen_Private::ConstructUEnum(Z_Registration_Info_UEnum_ELaneDirection.InnerSingleton, Z_Construct_UEnum_TrafficRuntime_ELaneDirection_Statics::EnumParams);
	}
	return Z_Registration_Info_UEnum_ELaneDirection.InnerSingleton;
}
// ********** End Enum ELaneDirection **************************************************************

// ********** Begin Enum ETrafficTurnType **********************************************************
static FEnumRegistrationInfo Z_Registration_Info_UEnum_ETrafficTurnType;
static UEnum* ETrafficTurnType_StaticEnum()
{
	if (!Z_Registration_Info_UEnum_ETrafficTurnType.OuterSingleton)
	{
		Z_Registration_Info_UEnum_ETrafficTurnType.OuterSingleton = GetStaticEnum(Z_Construct_UEnum_TrafficRuntime_ETrafficTurnType, (UObject*)Z_Construct_UPackage__Script_TrafficRuntime(), TEXT("ETrafficTurnType"));
	}
	return Z_Registration_Info_UEnum_ETrafficTurnType.OuterSingleton;
}
template<> TRAFFICRUNTIME_NON_ATTRIBUTED_API UEnum* StaticEnum<ETrafficTurnType>()
{
	return ETrafficTurnType_StaticEnum();
}
struct Z_Construct_UEnum_TrafficRuntime_ETrafficTurnType_Statics
{
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Enum_MetaDataParams[] = {
		{ "BlueprintType", "true" },
		{ "Left.Name", "ETrafficTurnType::Left" },
		{ "ModuleRelativePath", "Public/TrafficRoadTypes.h" },
		{ "Right.Name", "ETrafficTurnType::Right" },
		{ "Through.Name", "ETrafficTurnType::Through" },
		{ "UTurn.Name", "ETrafficTurnType::UTurn" },
	};
#endif // WITH_METADATA
	static constexpr UECodeGen_Private::FEnumeratorParam Enumerators[] = {
		{ "ETrafficTurnType::Through", (int64)ETrafficTurnType::Through },
		{ "ETrafficTurnType::Left", (int64)ETrafficTurnType::Left },
		{ "ETrafficTurnType::Right", (int64)ETrafficTurnType::Right },
		{ "ETrafficTurnType::UTurn", (int64)ETrafficTurnType::UTurn },
	};
	static const UECodeGen_Private::FEnumParams EnumParams;
}; // struct Z_Construct_UEnum_TrafficRuntime_ETrafficTurnType_Statics 
const UECodeGen_Private::FEnumParams Z_Construct_UEnum_TrafficRuntime_ETrafficTurnType_Statics::EnumParams = {
	(UObject*(*)())Z_Construct_UPackage__Script_TrafficRuntime,
	nullptr,
	"ETrafficTurnType",
	"ETrafficTurnType",
	Z_Construct_UEnum_TrafficRuntime_ETrafficTurnType_Statics::Enumerators,
	RF_Public|RF_Transient|RF_MarkAsNative,
	UE_ARRAY_COUNT(Z_Construct_UEnum_TrafficRuntime_ETrafficTurnType_Statics::Enumerators),
	EEnumFlags::None,
	(uint8)UEnum::ECppForm::EnumClass,
	METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UEnum_TrafficRuntime_ETrafficTurnType_Statics::Enum_MetaDataParams), Z_Construct_UEnum_TrafficRuntime_ETrafficTurnType_Statics::Enum_MetaDataParams)
};
UEnum* Z_Construct_UEnum_TrafficRuntime_ETrafficTurnType()
{
	if (!Z_Registration_Info_UEnum_ETrafficTurnType.InnerSingleton)
	{
		UECodeGen_Private::ConstructUEnum(Z_Registration_Info_UEnum_ETrafficTurnType.InnerSingleton, Z_Construct_UEnum_TrafficRuntime_ETrafficTurnType_Statics::EnumParams);
	}
	return Z_Registration_Info_UEnum_ETrafficTurnType.InnerSingleton;
}
// ********** End Enum ETrafficTurnType ************************************************************

// ********** Begin ScriptStruct FTrafficLane ******************************************************
struct Z_Construct_UScriptStruct_FTrafficLane_Statics
{
	static inline consteval int32 GetStructSize() { return sizeof(FTrafficLane); }
	static inline consteval int16 GetStructAlignment() { return alignof(FTrafficLane); }
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Struct_MetaDataParams[] = {
		{ "BlueprintType", "true" },
		{ "ModuleRelativePath", "Public/TrafficRoadTypes.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_LaneId_MetaData[] = {
		{ "Category", "TrafficLane" },
		{ "ModuleRelativePath", "Public/TrafficRoadTypes.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_RoadId_MetaData[] = {
		{ "Category", "TrafficLane" },
		{ "ModuleRelativePath", "Public/TrafficRoadTypes.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_SideIndex_MetaData[] = {
		{ "Category", "TrafficLane" },
		{ "ModuleRelativePath", "Public/TrafficRoadTypes.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_Width_MetaData[] = {
		{ "Category", "TrafficLane" },
		{ "ModuleRelativePath", "Public/TrafficRoadTypes.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_CenterlinePoints_MetaData[] = {
		{ "Category", "TrafficLane" },
		{ "ModuleRelativePath", "Public/TrafficRoadTypes.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_Direction_MetaData[] = {
		{ "Category", "TrafficLane" },
		{ "ModuleRelativePath", "Public/TrafficRoadTypes.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_PrevLaneId_MetaData[] = {
		{ "Category", "TrafficLane" },
		{ "ModuleRelativePath", "Public/TrafficRoadTypes.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_NextLaneId_MetaData[] = {
		{ "Category", "TrafficLane" },
		{ "ModuleRelativePath", "Public/TrafficRoadTypes.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_SpeedLimitKmh_MetaData[] = {
		{ "Category", "TrafficLane" },
		{ "ModuleRelativePath", "Public/TrafficRoadTypes.h" },
	};
#endif // WITH_METADATA

// ********** Begin ScriptStruct FTrafficLane constinit property declarations **********************
	static const UECodeGen_Private::FIntPropertyParams NewProp_LaneId;
	static const UECodeGen_Private::FIntPropertyParams NewProp_RoadId;
	static const UECodeGen_Private::FIntPropertyParams NewProp_SideIndex;
	static const UECodeGen_Private::FFloatPropertyParams NewProp_Width;
	static const UECodeGen_Private::FStructPropertyParams NewProp_CenterlinePoints_Inner;
	static const UECodeGen_Private::FArrayPropertyParams NewProp_CenterlinePoints;
	static const UECodeGen_Private::FBytePropertyParams NewProp_Direction_Underlying;
	static const UECodeGen_Private::FEnumPropertyParams NewProp_Direction;
	static const UECodeGen_Private::FIntPropertyParams NewProp_PrevLaneId;
	static const UECodeGen_Private::FIntPropertyParams NewProp_NextLaneId;
	static const UECodeGen_Private::FFloatPropertyParams NewProp_SpeedLimitKmh;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
// ********** End ScriptStruct FTrafficLane constinit property declarations ************************
	static void* NewStructOps()
	{
		return (UScriptStruct::ICppStructOps*)new UScriptStruct::TCppStructOps<FTrafficLane>();
	}
	static const UECodeGen_Private::FStructParams StructParams;
}; // struct Z_Construct_UScriptStruct_FTrafficLane_Statics
static FStructRegistrationInfo Z_Registration_Info_UScriptStruct_FTrafficLane;
class UScriptStruct* FTrafficLane::StaticStruct()
{
	if (!Z_Registration_Info_UScriptStruct_FTrafficLane.OuterSingleton)
	{
		Z_Registration_Info_UScriptStruct_FTrafficLane.OuterSingleton = GetStaticStruct(Z_Construct_UScriptStruct_FTrafficLane, (UObject*)Z_Construct_UPackage__Script_TrafficRuntime(), TEXT("TrafficLane"));
	}
	return Z_Registration_Info_UScriptStruct_FTrafficLane.OuterSingleton;
	}

// ********** Begin ScriptStruct FTrafficLane Property Definitions *********************************
const UECodeGen_Private::FIntPropertyParams Z_Construct_UScriptStruct_FTrafficLane_Statics::NewProp_LaneId = { "LaneId", nullptr, (EPropertyFlags)0x0010000000000005, UECodeGen_Private::EPropertyGenFlags::Int, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(FTrafficLane, LaneId), METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_LaneId_MetaData), NewProp_LaneId_MetaData) };
const UECodeGen_Private::FIntPropertyParams Z_Construct_UScriptStruct_FTrafficLane_Statics::NewProp_RoadId = { "RoadId", nullptr, (EPropertyFlags)0x0010000000000005, UECodeGen_Private::EPropertyGenFlags::Int, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(FTrafficLane, RoadId), METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_RoadId_MetaData), NewProp_RoadId_MetaData) };
const UECodeGen_Private::FIntPropertyParams Z_Construct_UScriptStruct_FTrafficLane_Statics::NewProp_SideIndex = { "SideIndex", nullptr, (EPropertyFlags)0x0010000000000005, UECodeGen_Private::EPropertyGenFlags::Int, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(FTrafficLane, SideIndex), METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_SideIndex_MetaData), NewProp_SideIndex_MetaData) };
const UECodeGen_Private::FFloatPropertyParams Z_Construct_UScriptStruct_FTrafficLane_Statics::NewProp_Width = { "Width", nullptr, (EPropertyFlags)0x0010000000000005, UECodeGen_Private::EPropertyGenFlags::Float, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(FTrafficLane, Width), METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_Width_MetaData), NewProp_Width_MetaData) };
const UECodeGen_Private::FStructPropertyParams Z_Construct_UScriptStruct_FTrafficLane_Statics::NewProp_CenterlinePoints_Inner = { "CenterlinePoints", nullptr, (EPropertyFlags)0x0000000000000000, UECodeGen_Private::EPropertyGenFlags::Struct, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, 0, Z_Construct_UScriptStruct_FVector, METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FArrayPropertyParams Z_Construct_UScriptStruct_FTrafficLane_Statics::NewProp_CenterlinePoints = { "CenterlinePoints", nullptr, (EPropertyFlags)0x0010000000000005, UECodeGen_Private::EPropertyGenFlags::Array, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(FTrafficLane, CenterlinePoints), EArrayPropertyFlags::None, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_CenterlinePoints_MetaData), NewProp_CenterlinePoints_MetaData) };
const UECodeGen_Private::FBytePropertyParams Z_Construct_UScriptStruct_FTrafficLane_Statics::NewProp_Direction_Underlying = { "UnderlyingType", nullptr, (EPropertyFlags)0x0000000000000000, UECodeGen_Private::EPropertyGenFlags::Byte, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, 0, nullptr, METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FEnumPropertyParams Z_Construct_UScriptStruct_FTrafficLane_Statics::NewProp_Direction = { "Direction", nullptr, (EPropertyFlags)0x0010000000000005, UECodeGen_Private::EPropertyGenFlags::Enum, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(FTrafficLane, Direction), Z_Construct_UEnum_TrafficRuntime_ELaneDirection, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_Direction_MetaData), NewProp_Direction_MetaData) }; // 162408219
const UECodeGen_Private::FIntPropertyParams Z_Construct_UScriptStruct_FTrafficLane_Statics::NewProp_PrevLaneId = { "PrevLaneId", nullptr, (EPropertyFlags)0x0010000000000005, UECodeGen_Private::EPropertyGenFlags::Int, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(FTrafficLane, PrevLaneId), METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_PrevLaneId_MetaData), NewProp_PrevLaneId_MetaData) };
const UECodeGen_Private::FIntPropertyParams Z_Construct_UScriptStruct_FTrafficLane_Statics::NewProp_NextLaneId = { "NextLaneId", nullptr, (EPropertyFlags)0x0010000000000005, UECodeGen_Private::EPropertyGenFlags::Int, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(FTrafficLane, NextLaneId), METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_NextLaneId_MetaData), NewProp_NextLaneId_MetaData) };
const UECodeGen_Private::FFloatPropertyParams Z_Construct_UScriptStruct_FTrafficLane_Statics::NewProp_SpeedLimitKmh = { "SpeedLimitKmh", nullptr, (EPropertyFlags)0x0010000000000005, UECodeGen_Private::EPropertyGenFlags::Float, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(FTrafficLane, SpeedLimitKmh), METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_SpeedLimitKmh_MetaData), NewProp_SpeedLimitKmh_MetaData) };
const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UScriptStruct_FTrafficLane_Statics::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FTrafficLane_Statics::NewProp_LaneId,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FTrafficLane_Statics::NewProp_RoadId,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FTrafficLane_Statics::NewProp_SideIndex,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FTrafficLane_Statics::NewProp_Width,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FTrafficLane_Statics::NewProp_CenterlinePoints_Inner,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FTrafficLane_Statics::NewProp_CenterlinePoints,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FTrafficLane_Statics::NewProp_Direction_Underlying,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FTrafficLane_Statics::NewProp_Direction,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FTrafficLane_Statics::NewProp_PrevLaneId,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FTrafficLane_Statics::NewProp_NextLaneId,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FTrafficLane_Statics::NewProp_SpeedLimitKmh,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FTrafficLane_Statics::PropPointers) < 2048);
// ********** End ScriptStruct FTrafficLane Property Definitions ***********************************
const UECodeGen_Private::FStructParams Z_Construct_UScriptStruct_FTrafficLane_Statics::StructParams = {
	(UObject* (*)())Z_Construct_UPackage__Script_TrafficRuntime,
	nullptr,
	&NewStructOps,
	"TrafficLane",
	Z_Construct_UScriptStruct_FTrafficLane_Statics::PropPointers,
	UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FTrafficLane_Statics::PropPointers),
	sizeof(FTrafficLane),
	alignof(FTrafficLane),
	RF_Public|RF_Transient|RF_MarkAsNative,
	EStructFlags(0x00000201),
	METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FTrafficLane_Statics::Struct_MetaDataParams), Z_Construct_UScriptStruct_FTrafficLane_Statics::Struct_MetaDataParams)
};
UScriptStruct* Z_Construct_UScriptStruct_FTrafficLane()
{
	if (!Z_Registration_Info_UScriptStruct_FTrafficLane.InnerSingleton)
	{
		UECodeGen_Private::ConstructUScriptStruct(Z_Registration_Info_UScriptStruct_FTrafficLane.InnerSingleton, Z_Construct_UScriptStruct_FTrafficLane_Statics::StructParams);
	}
	return CastChecked<UScriptStruct>(Z_Registration_Info_UScriptStruct_FTrafficLane.InnerSingleton);
}
// ********** End ScriptStruct FTrafficLane ********************************************************

// ********** Begin ScriptStruct FTrafficRoad ******************************************************
struct Z_Construct_UScriptStruct_FTrafficRoad_Statics
{
	static inline consteval int32 GetStructSize() { return sizeof(FTrafficRoad); }
	static inline consteval int16 GetStructAlignment() { return alignof(FTrafficRoad); }
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Struct_MetaDataParams[] = {
		{ "BlueprintType", "true" },
		{ "ModuleRelativePath", "Public/TrafficRoadTypes.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_RoadId_MetaData[] = {
		{ "Category", "TrafficRoad" },
		{ "ModuleRelativePath", "Public/TrafficRoadTypes.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_FamilyId_MetaData[] = {
		{ "Category", "TrafficRoad" },
		{ "ModuleRelativePath", "Public/TrafficRoadTypes.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_CenterlinePoints_MetaData[] = {
		{ "Category", "TrafficRoad" },
		{ "ModuleRelativePath", "Public/TrafficRoadTypes.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_Lanes_MetaData[] = {
		{ "Category", "TrafficRoad" },
		{ "ModuleRelativePath", "Public/TrafficRoadTypes.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_SourceActor_MetaData[] = {
		{ "Category", "TrafficRoad" },
		{ "ModuleRelativePath", "Public/TrafficRoadTypes.h" },
	};
#endif // WITH_METADATA

// ********** Begin ScriptStruct FTrafficRoad constinit property declarations **********************
	static const UECodeGen_Private::FIntPropertyParams NewProp_RoadId;
	static const UECodeGen_Private::FIntPropertyParams NewProp_FamilyId;
	static const UECodeGen_Private::FStructPropertyParams NewProp_CenterlinePoints_Inner;
	static const UECodeGen_Private::FArrayPropertyParams NewProp_CenterlinePoints;
	static const UECodeGen_Private::FStructPropertyParams NewProp_Lanes_Inner;
	static const UECodeGen_Private::FArrayPropertyParams NewProp_Lanes;
	static const UECodeGen_Private::FSoftObjectPropertyParams NewProp_SourceActor;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
// ********** End ScriptStruct FTrafficRoad constinit property declarations ************************
	static void* NewStructOps()
	{
		return (UScriptStruct::ICppStructOps*)new UScriptStruct::TCppStructOps<FTrafficRoad>();
	}
	static const UECodeGen_Private::FStructParams StructParams;
}; // struct Z_Construct_UScriptStruct_FTrafficRoad_Statics
static FStructRegistrationInfo Z_Registration_Info_UScriptStruct_FTrafficRoad;
class UScriptStruct* FTrafficRoad::StaticStruct()
{
	if (!Z_Registration_Info_UScriptStruct_FTrafficRoad.OuterSingleton)
	{
		Z_Registration_Info_UScriptStruct_FTrafficRoad.OuterSingleton = GetStaticStruct(Z_Construct_UScriptStruct_FTrafficRoad, (UObject*)Z_Construct_UPackage__Script_TrafficRuntime(), TEXT("TrafficRoad"));
	}
	return Z_Registration_Info_UScriptStruct_FTrafficRoad.OuterSingleton;
	}

// ********** Begin ScriptStruct FTrafficRoad Property Definitions *********************************
const UECodeGen_Private::FIntPropertyParams Z_Construct_UScriptStruct_FTrafficRoad_Statics::NewProp_RoadId = { "RoadId", nullptr, (EPropertyFlags)0x0010000000000005, UECodeGen_Private::EPropertyGenFlags::Int, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(FTrafficRoad, RoadId), METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_RoadId_MetaData), NewProp_RoadId_MetaData) };
const UECodeGen_Private::FIntPropertyParams Z_Construct_UScriptStruct_FTrafficRoad_Statics::NewProp_FamilyId = { "FamilyId", nullptr, (EPropertyFlags)0x0010000000000005, UECodeGen_Private::EPropertyGenFlags::Int, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(FTrafficRoad, FamilyId), METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_FamilyId_MetaData), NewProp_FamilyId_MetaData) };
const UECodeGen_Private::FStructPropertyParams Z_Construct_UScriptStruct_FTrafficRoad_Statics::NewProp_CenterlinePoints_Inner = { "CenterlinePoints", nullptr, (EPropertyFlags)0x0000000000000000, UECodeGen_Private::EPropertyGenFlags::Struct, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, 0, Z_Construct_UScriptStruct_FVector, METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FArrayPropertyParams Z_Construct_UScriptStruct_FTrafficRoad_Statics::NewProp_CenterlinePoints = { "CenterlinePoints", nullptr, (EPropertyFlags)0x0010000000000005, UECodeGen_Private::EPropertyGenFlags::Array, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(FTrafficRoad, CenterlinePoints), EArrayPropertyFlags::None, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_CenterlinePoints_MetaData), NewProp_CenterlinePoints_MetaData) };
const UECodeGen_Private::FStructPropertyParams Z_Construct_UScriptStruct_FTrafficRoad_Statics::NewProp_Lanes_Inner = { "Lanes", nullptr, (EPropertyFlags)0x0000000000000000, UECodeGen_Private::EPropertyGenFlags::Struct, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, 0, Z_Construct_UScriptStruct_FTrafficLane, METADATA_PARAMS(0, nullptr) }; // 2577686271
const UECodeGen_Private::FArrayPropertyParams Z_Construct_UScriptStruct_FTrafficRoad_Statics::NewProp_Lanes = { "Lanes", nullptr, (EPropertyFlags)0x0010000000000005, UECodeGen_Private::EPropertyGenFlags::Array, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(FTrafficRoad, Lanes), EArrayPropertyFlags::None, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_Lanes_MetaData), NewProp_Lanes_MetaData) }; // 2577686271
const UECodeGen_Private::FSoftObjectPropertyParams Z_Construct_UScriptStruct_FTrafficRoad_Statics::NewProp_SourceActor = { "SourceActor", nullptr, (EPropertyFlags)0x0014000000000005, UECodeGen_Private::EPropertyGenFlags::SoftObject, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(FTrafficRoad, SourceActor), Z_Construct_UClass_AActor_NoRegister, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_SourceActor_MetaData), NewProp_SourceActor_MetaData) };
const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UScriptStruct_FTrafficRoad_Statics::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FTrafficRoad_Statics::NewProp_RoadId,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FTrafficRoad_Statics::NewProp_FamilyId,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FTrafficRoad_Statics::NewProp_CenterlinePoints_Inner,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FTrafficRoad_Statics::NewProp_CenterlinePoints,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FTrafficRoad_Statics::NewProp_Lanes_Inner,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FTrafficRoad_Statics::NewProp_Lanes,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FTrafficRoad_Statics::NewProp_SourceActor,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FTrafficRoad_Statics::PropPointers) < 2048);
// ********** End ScriptStruct FTrafficRoad Property Definitions ***********************************
const UECodeGen_Private::FStructParams Z_Construct_UScriptStruct_FTrafficRoad_Statics::StructParams = {
	(UObject* (*)())Z_Construct_UPackage__Script_TrafficRuntime,
	nullptr,
	&NewStructOps,
	"TrafficRoad",
	Z_Construct_UScriptStruct_FTrafficRoad_Statics::PropPointers,
	UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FTrafficRoad_Statics::PropPointers),
	sizeof(FTrafficRoad),
	alignof(FTrafficRoad),
	RF_Public|RF_Transient|RF_MarkAsNative,
	EStructFlags(0x00000201),
	METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FTrafficRoad_Statics::Struct_MetaDataParams), Z_Construct_UScriptStruct_FTrafficRoad_Statics::Struct_MetaDataParams)
};
UScriptStruct* Z_Construct_UScriptStruct_FTrafficRoad()
{
	if (!Z_Registration_Info_UScriptStruct_FTrafficRoad.InnerSingleton)
	{
		UECodeGen_Private::ConstructUScriptStruct(Z_Registration_Info_UScriptStruct_FTrafficRoad.InnerSingleton, Z_Construct_UScriptStruct_FTrafficRoad_Statics::StructParams);
	}
	return CastChecked<UScriptStruct>(Z_Registration_Info_UScriptStruct_FTrafficRoad.InnerSingleton);
}
// ********** End ScriptStruct FTrafficRoad ********************************************************

// ********** Begin ScriptStruct FTrafficIntersection **********************************************
struct Z_Construct_UScriptStruct_FTrafficIntersection_Statics
{
	static inline consteval int32 GetStructSize() { return sizeof(FTrafficIntersection); }
	static inline consteval int16 GetStructAlignment() { return alignof(FTrafficIntersection); }
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Struct_MetaDataParams[] = {
		{ "BlueprintType", "true" },
		{ "ModuleRelativePath", "Public/TrafficRoadTypes.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_IntersectionId_MetaData[] = {
		{ "Category", "TrafficIntersection" },
		{ "ModuleRelativePath", "Public/TrafficRoadTypes.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_IncomingLaneIds_MetaData[] = {
		{ "Category", "TrafficIntersection" },
		{ "ModuleRelativePath", "Public/TrafficRoadTypes.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_OutgoingLaneIds_MetaData[] = {
		{ "Category", "TrafficIntersection" },
		{ "ModuleRelativePath", "Public/TrafficRoadTypes.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_Center_MetaData[] = {
		{ "Category", "TrafficIntersection" },
		{ "ModuleRelativePath", "Public/TrafficRoadTypes.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_Radius_MetaData[] = {
		{ "Category", "TrafficIntersection" },
		{ "ModuleRelativePath", "Public/TrafficRoadTypes.h" },
	};
#endif // WITH_METADATA

// ********** Begin ScriptStruct FTrafficIntersection constinit property declarations **************
	static const UECodeGen_Private::FIntPropertyParams NewProp_IntersectionId;
	static const UECodeGen_Private::FIntPropertyParams NewProp_IncomingLaneIds_Inner;
	static const UECodeGen_Private::FArrayPropertyParams NewProp_IncomingLaneIds;
	static const UECodeGen_Private::FIntPropertyParams NewProp_OutgoingLaneIds_Inner;
	static const UECodeGen_Private::FArrayPropertyParams NewProp_OutgoingLaneIds;
	static const UECodeGen_Private::FStructPropertyParams NewProp_Center;
	static const UECodeGen_Private::FFloatPropertyParams NewProp_Radius;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
// ********** End ScriptStruct FTrafficIntersection constinit property declarations ****************
	static void* NewStructOps()
	{
		return (UScriptStruct::ICppStructOps*)new UScriptStruct::TCppStructOps<FTrafficIntersection>();
	}
	static const UECodeGen_Private::FStructParams StructParams;
}; // struct Z_Construct_UScriptStruct_FTrafficIntersection_Statics
static FStructRegistrationInfo Z_Registration_Info_UScriptStruct_FTrafficIntersection;
class UScriptStruct* FTrafficIntersection::StaticStruct()
{
	if (!Z_Registration_Info_UScriptStruct_FTrafficIntersection.OuterSingleton)
	{
		Z_Registration_Info_UScriptStruct_FTrafficIntersection.OuterSingleton = GetStaticStruct(Z_Construct_UScriptStruct_FTrafficIntersection, (UObject*)Z_Construct_UPackage__Script_TrafficRuntime(), TEXT("TrafficIntersection"));
	}
	return Z_Registration_Info_UScriptStruct_FTrafficIntersection.OuterSingleton;
	}

// ********** Begin ScriptStruct FTrafficIntersection Property Definitions *************************
const UECodeGen_Private::FIntPropertyParams Z_Construct_UScriptStruct_FTrafficIntersection_Statics::NewProp_IntersectionId = { "IntersectionId", nullptr, (EPropertyFlags)0x0010000000000005, UECodeGen_Private::EPropertyGenFlags::Int, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(FTrafficIntersection, IntersectionId), METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_IntersectionId_MetaData), NewProp_IntersectionId_MetaData) };
const UECodeGen_Private::FIntPropertyParams Z_Construct_UScriptStruct_FTrafficIntersection_Statics::NewProp_IncomingLaneIds_Inner = { "IncomingLaneIds", nullptr, (EPropertyFlags)0x0000000000000000, UECodeGen_Private::EPropertyGenFlags::Int, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, 0, METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FArrayPropertyParams Z_Construct_UScriptStruct_FTrafficIntersection_Statics::NewProp_IncomingLaneIds = { "IncomingLaneIds", nullptr, (EPropertyFlags)0x0010000000000005, UECodeGen_Private::EPropertyGenFlags::Array, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(FTrafficIntersection, IncomingLaneIds), EArrayPropertyFlags::None, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_IncomingLaneIds_MetaData), NewProp_IncomingLaneIds_MetaData) };
const UECodeGen_Private::FIntPropertyParams Z_Construct_UScriptStruct_FTrafficIntersection_Statics::NewProp_OutgoingLaneIds_Inner = { "OutgoingLaneIds", nullptr, (EPropertyFlags)0x0000000000000000, UECodeGen_Private::EPropertyGenFlags::Int, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, 0, METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FArrayPropertyParams Z_Construct_UScriptStruct_FTrafficIntersection_Statics::NewProp_OutgoingLaneIds = { "OutgoingLaneIds", nullptr, (EPropertyFlags)0x0010000000000005, UECodeGen_Private::EPropertyGenFlags::Array, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(FTrafficIntersection, OutgoingLaneIds), EArrayPropertyFlags::None, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_OutgoingLaneIds_MetaData), NewProp_OutgoingLaneIds_MetaData) };
const UECodeGen_Private::FStructPropertyParams Z_Construct_UScriptStruct_FTrafficIntersection_Statics::NewProp_Center = { "Center", nullptr, (EPropertyFlags)0x0010000000000005, UECodeGen_Private::EPropertyGenFlags::Struct, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(FTrafficIntersection, Center), Z_Construct_UScriptStruct_FVector, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_Center_MetaData), NewProp_Center_MetaData) };
const UECodeGen_Private::FFloatPropertyParams Z_Construct_UScriptStruct_FTrafficIntersection_Statics::NewProp_Radius = { "Radius", nullptr, (EPropertyFlags)0x0010000000000005, UECodeGen_Private::EPropertyGenFlags::Float, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(FTrafficIntersection, Radius), METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_Radius_MetaData), NewProp_Radius_MetaData) };
const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UScriptStruct_FTrafficIntersection_Statics::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FTrafficIntersection_Statics::NewProp_IntersectionId,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FTrafficIntersection_Statics::NewProp_IncomingLaneIds_Inner,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FTrafficIntersection_Statics::NewProp_IncomingLaneIds,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FTrafficIntersection_Statics::NewProp_OutgoingLaneIds_Inner,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FTrafficIntersection_Statics::NewProp_OutgoingLaneIds,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FTrafficIntersection_Statics::NewProp_Center,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FTrafficIntersection_Statics::NewProp_Radius,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FTrafficIntersection_Statics::PropPointers) < 2048);
// ********** End ScriptStruct FTrafficIntersection Property Definitions ***************************
const UECodeGen_Private::FStructParams Z_Construct_UScriptStruct_FTrafficIntersection_Statics::StructParams = {
	(UObject* (*)())Z_Construct_UPackage__Script_TrafficRuntime,
	nullptr,
	&NewStructOps,
	"TrafficIntersection",
	Z_Construct_UScriptStruct_FTrafficIntersection_Statics::PropPointers,
	UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FTrafficIntersection_Statics::PropPointers),
	sizeof(FTrafficIntersection),
	alignof(FTrafficIntersection),
	RF_Public|RF_Transient|RF_MarkAsNative,
	EStructFlags(0x00000201),
	METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FTrafficIntersection_Statics::Struct_MetaDataParams), Z_Construct_UScriptStruct_FTrafficIntersection_Statics::Struct_MetaDataParams)
};
UScriptStruct* Z_Construct_UScriptStruct_FTrafficIntersection()
{
	if (!Z_Registration_Info_UScriptStruct_FTrafficIntersection.InnerSingleton)
	{
		UECodeGen_Private::ConstructUScriptStruct(Z_Registration_Info_UScriptStruct_FTrafficIntersection.InnerSingleton, Z_Construct_UScriptStruct_FTrafficIntersection_Statics::StructParams);
	}
	return CastChecked<UScriptStruct>(Z_Registration_Info_UScriptStruct_FTrafficIntersection.InnerSingleton);
}
// ********** End ScriptStruct FTrafficIntersection ************************************************

// ********** Begin ScriptStruct FTrafficMovement **************************************************
struct Z_Construct_UScriptStruct_FTrafficMovement_Statics
{
	static inline consteval int32 GetStructSize() { return sizeof(FTrafficMovement); }
	static inline consteval int16 GetStructAlignment() { return alignof(FTrafficMovement); }
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Struct_MetaDataParams[] = {
		{ "BlueprintType", "true" },
		{ "ModuleRelativePath", "Public/TrafficRoadTypes.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_MovementId_MetaData[] = {
		{ "Category", "TrafficMovement" },
		{ "ModuleRelativePath", "Public/TrafficRoadTypes.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_IntersectionId_MetaData[] = {
		{ "Category", "TrafficMovement" },
		{ "ModuleRelativePath", "Public/TrafficRoadTypes.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_IncomingLaneId_MetaData[] = {
		{ "Category", "TrafficMovement" },
		{ "ModuleRelativePath", "Public/TrafficRoadTypes.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_OutgoingLaneId_MetaData[] = {
		{ "Category", "TrafficMovement" },
		{ "ModuleRelativePath", "Public/TrafficRoadTypes.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_TurnType_MetaData[] = {
		{ "Category", "TrafficMovement" },
		{ "ModuleRelativePath", "Public/TrafficRoadTypes.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_PathPoints_MetaData[] = {
		{ "Category", "TrafficMovement" },
		{ "ModuleRelativePath", "Public/TrafficRoadTypes.h" },
	};
#endif // WITH_METADATA

// ********** Begin ScriptStruct FTrafficMovement constinit property declarations ******************
	static const UECodeGen_Private::FIntPropertyParams NewProp_MovementId;
	static const UECodeGen_Private::FIntPropertyParams NewProp_IntersectionId;
	static const UECodeGen_Private::FIntPropertyParams NewProp_IncomingLaneId;
	static const UECodeGen_Private::FIntPropertyParams NewProp_OutgoingLaneId;
	static const UECodeGen_Private::FBytePropertyParams NewProp_TurnType_Underlying;
	static const UECodeGen_Private::FEnumPropertyParams NewProp_TurnType;
	static const UECodeGen_Private::FStructPropertyParams NewProp_PathPoints_Inner;
	static const UECodeGen_Private::FArrayPropertyParams NewProp_PathPoints;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
// ********** End ScriptStruct FTrafficMovement constinit property declarations ********************
	static void* NewStructOps()
	{
		return (UScriptStruct::ICppStructOps*)new UScriptStruct::TCppStructOps<FTrafficMovement>();
	}
	static const UECodeGen_Private::FStructParams StructParams;
}; // struct Z_Construct_UScriptStruct_FTrafficMovement_Statics
static FStructRegistrationInfo Z_Registration_Info_UScriptStruct_FTrafficMovement;
class UScriptStruct* FTrafficMovement::StaticStruct()
{
	if (!Z_Registration_Info_UScriptStruct_FTrafficMovement.OuterSingleton)
	{
		Z_Registration_Info_UScriptStruct_FTrafficMovement.OuterSingleton = GetStaticStruct(Z_Construct_UScriptStruct_FTrafficMovement, (UObject*)Z_Construct_UPackage__Script_TrafficRuntime(), TEXT("TrafficMovement"));
	}
	return Z_Registration_Info_UScriptStruct_FTrafficMovement.OuterSingleton;
	}

// ********** Begin ScriptStruct FTrafficMovement Property Definitions *****************************
const UECodeGen_Private::FIntPropertyParams Z_Construct_UScriptStruct_FTrafficMovement_Statics::NewProp_MovementId = { "MovementId", nullptr, (EPropertyFlags)0x0010000000000005, UECodeGen_Private::EPropertyGenFlags::Int, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(FTrafficMovement, MovementId), METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_MovementId_MetaData), NewProp_MovementId_MetaData) };
const UECodeGen_Private::FIntPropertyParams Z_Construct_UScriptStruct_FTrafficMovement_Statics::NewProp_IntersectionId = { "IntersectionId", nullptr, (EPropertyFlags)0x0010000000000005, UECodeGen_Private::EPropertyGenFlags::Int, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(FTrafficMovement, IntersectionId), METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_IntersectionId_MetaData), NewProp_IntersectionId_MetaData) };
const UECodeGen_Private::FIntPropertyParams Z_Construct_UScriptStruct_FTrafficMovement_Statics::NewProp_IncomingLaneId = { "IncomingLaneId", nullptr, (EPropertyFlags)0x0010000000000005, UECodeGen_Private::EPropertyGenFlags::Int, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(FTrafficMovement, IncomingLaneId), METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_IncomingLaneId_MetaData), NewProp_IncomingLaneId_MetaData) };
const UECodeGen_Private::FIntPropertyParams Z_Construct_UScriptStruct_FTrafficMovement_Statics::NewProp_OutgoingLaneId = { "OutgoingLaneId", nullptr, (EPropertyFlags)0x0010000000000005, UECodeGen_Private::EPropertyGenFlags::Int, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(FTrafficMovement, OutgoingLaneId), METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_OutgoingLaneId_MetaData), NewProp_OutgoingLaneId_MetaData) };
const UECodeGen_Private::FBytePropertyParams Z_Construct_UScriptStruct_FTrafficMovement_Statics::NewProp_TurnType_Underlying = { "UnderlyingType", nullptr, (EPropertyFlags)0x0000000000000000, UECodeGen_Private::EPropertyGenFlags::Byte, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, 0, nullptr, METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FEnumPropertyParams Z_Construct_UScriptStruct_FTrafficMovement_Statics::NewProp_TurnType = { "TurnType", nullptr, (EPropertyFlags)0x0010000000000005, UECodeGen_Private::EPropertyGenFlags::Enum, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(FTrafficMovement, TurnType), Z_Construct_UEnum_TrafficRuntime_ETrafficTurnType, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_TurnType_MetaData), NewProp_TurnType_MetaData) }; // 2962274943
const UECodeGen_Private::FStructPropertyParams Z_Construct_UScriptStruct_FTrafficMovement_Statics::NewProp_PathPoints_Inner = { "PathPoints", nullptr, (EPropertyFlags)0x0000000000000000, UECodeGen_Private::EPropertyGenFlags::Struct, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, 0, Z_Construct_UScriptStruct_FVector, METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FArrayPropertyParams Z_Construct_UScriptStruct_FTrafficMovement_Statics::NewProp_PathPoints = { "PathPoints", nullptr, (EPropertyFlags)0x0010000000000005, UECodeGen_Private::EPropertyGenFlags::Array, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(FTrafficMovement, PathPoints), EArrayPropertyFlags::None, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_PathPoints_MetaData), NewProp_PathPoints_MetaData) };
const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UScriptStruct_FTrafficMovement_Statics::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FTrafficMovement_Statics::NewProp_MovementId,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FTrafficMovement_Statics::NewProp_IntersectionId,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FTrafficMovement_Statics::NewProp_IncomingLaneId,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FTrafficMovement_Statics::NewProp_OutgoingLaneId,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FTrafficMovement_Statics::NewProp_TurnType_Underlying,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FTrafficMovement_Statics::NewProp_TurnType,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FTrafficMovement_Statics::NewProp_PathPoints_Inner,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FTrafficMovement_Statics::NewProp_PathPoints,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FTrafficMovement_Statics::PropPointers) < 2048);
// ********** End ScriptStruct FTrafficMovement Property Definitions *******************************
const UECodeGen_Private::FStructParams Z_Construct_UScriptStruct_FTrafficMovement_Statics::StructParams = {
	(UObject* (*)())Z_Construct_UPackage__Script_TrafficRuntime,
	nullptr,
	&NewStructOps,
	"TrafficMovement",
	Z_Construct_UScriptStruct_FTrafficMovement_Statics::PropPointers,
	UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FTrafficMovement_Statics::PropPointers),
	sizeof(FTrafficMovement),
	alignof(FTrafficMovement),
	RF_Public|RF_Transient|RF_MarkAsNative,
	EStructFlags(0x00000201),
	METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FTrafficMovement_Statics::Struct_MetaDataParams), Z_Construct_UScriptStruct_FTrafficMovement_Statics::Struct_MetaDataParams)
};
UScriptStruct* Z_Construct_UScriptStruct_FTrafficMovement()
{
	if (!Z_Registration_Info_UScriptStruct_FTrafficMovement.InnerSingleton)
	{
		UECodeGen_Private::ConstructUScriptStruct(Z_Registration_Info_UScriptStruct_FTrafficMovement.InnerSingleton, Z_Construct_UScriptStruct_FTrafficMovement_Statics::StructParams);
	}
	return CastChecked<UScriptStruct>(Z_Registration_Info_UScriptStruct_FTrafficMovement.InnerSingleton);
}
// ********** End ScriptStruct FTrafficMovement ****************************************************

// ********** Begin ScriptStruct FTrafficNetwork ***************************************************
struct Z_Construct_UScriptStruct_FTrafficNetwork_Statics
{
	static inline consteval int32 GetStructSize() { return sizeof(FTrafficNetwork); }
	static inline consteval int16 GetStructAlignment() { return alignof(FTrafficNetwork); }
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Struct_MetaDataParams[] = {
		{ "BlueprintType", "true" },
		{ "ModuleRelativePath", "Public/TrafficRoadTypes.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_Roads_MetaData[] = {
		{ "Category", "TrafficNetwork" },
		{ "ModuleRelativePath", "Public/TrafficRoadTypes.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_Lanes_MetaData[] = {
		{ "Category", "TrafficNetwork" },
		{ "ModuleRelativePath", "Public/TrafficRoadTypes.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_Intersections_MetaData[] = {
		{ "Category", "TrafficNetwork" },
		{ "ModuleRelativePath", "Public/TrafficRoadTypes.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_Movements_MetaData[] = {
		{ "Category", "TrafficNetwork" },
		{ "ModuleRelativePath", "Public/TrafficRoadTypes.h" },
	};
#endif // WITH_METADATA

// ********** Begin ScriptStruct FTrafficNetwork constinit property declarations *******************
	static const UECodeGen_Private::FStructPropertyParams NewProp_Roads_Inner;
	static const UECodeGen_Private::FArrayPropertyParams NewProp_Roads;
	static const UECodeGen_Private::FStructPropertyParams NewProp_Lanes_Inner;
	static const UECodeGen_Private::FArrayPropertyParams NewProp_Lanes;
	static const UECodeGen_Private::FStructPropertyParams NewProp_Intersections_Inner;
	static const UECodeGen_Private::FArrayPropertyParams NewProp_Intersections;
	static const UECodeGen_Private::FStructPropertyParams NewProp_Movements_Inner;
	static const UECodeGen_Private::FArrayPropertyParams NewProp_Movements;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
// ********** End ScriptStruct FTrafficNetwork constinit property declarations *********************
	static void* NewStructOps()
	{
		return (UScriptStruct::ICppStructOps*)new UScriptStruct::TCppStructOps<FTrafficNetwork>();
	}
	static const UECodeGen_Private::FStructParams StructParams;
}; // struct Z_Construct_UScriptStruct_FTrafficNetwork_Statics
static FStructRegistrationInfo Z_Registration_Info_UScriptStruct_FTrafficNetwork;
class UScriptStruct* FTrafficNetwork::StaticStruct()
{
	if (!Z_Registration_Info_UScriptStruct_FTrafficNetwork.OuterSingleton)
	{
		Z_Registration_Info_UScriptStruct_FTrafficNetwork.OuterSingleton = GetStaticStruct(Z_Construct_UScriptStruct_FTrafficNetwork, (UObject*)Z_Construct_UPackage__Script_TrafficRuntime(), TEXT("TrafficNetwork"));
	}
	return Z_Registration_Info_UScriptStruct_FTrafficNetwork.OuterSingleton;
	}

// ********** Begin ScriptStruct FTrafficNetwork Property Definitions ******************************
const UECodeGen_Private::FStructPropertyParams Z_Construct_UScriptStruct_FTrafficNetwork_Statics::NewProp_Roads_Inner = { "Roads", nullptr, (EPropertyFlags)0x0000000000000000, UECodeGen_Private::EPropertyGenFlags::Struct, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, 0, Z_Construct_UScriptStruct_FTrafficRoad, METADATA_PARAMS(0, nullptr) }; // 3477913646
const UECodeGen_Private::FArrayPropertyParams Z_Construct_UScriptStruct_FTrafficNetwork_Statics::NewProp_Roads = { "Roads", nullptr, (EPropertyFlags)0x0010000000000005, UECodeGen_Private::EPropertyGenFlags::Array, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(FTrafficNetwork, Roads), EArrayPropertyFlags::None, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_Roads_MetaData), NewProp_Roads_MetaData) }; // 3477913646
const UECodeGen_Private::FStructPropertyParams Z_Construct_UScriptStruct_FTrafficNetwork_Statics::NewProp_Lanes_Inner = { "Lanes", nullptr, (EPropertyFlags)0x0000000000000000, UECodeGen_Private::EPropertyGenFlags::Struct, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, 0, Z_Construct_UScriptStruct_FTrafficLane, METADATA_PARAMS(0, nullptr) }; // 2577686271
const UECodeGen_Private::FArrayPropertyParams Z_Construct_UScriptStruct_FTrafficNetwork_Statics::NewProp_Lanes = { "Lanes", nullptr, (EPropertyFlags)0x0010000000000005, UECodeGen_Private::EPropertyGenFlags::Array, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(FTrafficNetwork, Lanes), EArrayPropertyFlags::None, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_Lanes_MetaData), NewProp_Lanes_MetaData) }; // 2577686271
const UECodeGen_Private::FStructPropertyParams Z_Construct_UScriptStruct_FTrafficNetwork_Statics::NewProp_Intersections_Inner = { "Intersections", nullptr, (EPropertyFlags)0x0000000000000000, UECodeGen_Private::EPropertyGenFlags::Struct, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, 0, Z_Construct_UScriptStruct_FTrafficIntersection, METADATA_PARAMS(0, nullptr) }; // 1890181855
const UECodeGen_Private::FArrayPropertyParams Z_Construct_UScriptStruct_FTrafficNetwork_Statics::NewProp_Intersections = { "Intersections", nullptr, (EPropertyFlags)0x0010000000000005, UECodeGen_Private::EPropertyGenFlags::Array, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(FTrafficNetwork, Intersections), EArrayPropertyFlags::None, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_Intersections_MetaData), NewProp_Intersections_MetaData) }; // 1890181855
const UECodeGen_Private::FStructPropertyParams Z_Construct_UScriptStruct_FTrafficNetwork_Statics::NewProp_Movements_Inner = { "Movements", nullptr, (EPropertyFlags)0x0000000000000000, UECodeGen_Private::EPropertyGenFlags::Struct, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, 0, Z_Construct_UScriptStruct_FTrafficMovement, METADATA_PARAMS(0, nullptr) }; // 2051957598
const UECodeGen_Private::FArrayPropertyParams Z_Construct_UScriptStruct_FTrafficNetwork_Statics::NewProp_Movements = { "Movements", nullptr, (EPropertyFlags)0x0010000000000005, UECodeGen_Private::EPropertyGenFlags::Array, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(FTrafficNetwork, Movements), EArrayPropertyFlags::None, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_Movements_MetaData), NewProp_Movements_MetaData) }; // 2051957598
const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UScriptStruct_FTrafficNetwork_Statics::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FTrafficNetwork_Statics::NewProp_Roads_Inner,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FTrafficNetwork_Statics::NewProp_Roads,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FTrafficNetwork_Statics::NewProp_Lanes_Inner,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FTrafficNetwork_Statics::NewProp_Lanes,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FTrafficNetwork_Statics::NewProp_Intersections_Inner,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FTrafficNetwork_Statics::NewProp_Intersections,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FTrafficNetwork_Statics::NewProp_Movements_Inner,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FTrafficNetwork_Statics::NewProp_Movements,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FTrafficNetwork_Statics::PropPointers) < 2048);
// ********** End ScriptStruct FTrafficNetwork Property Definitions ********************************
const UECodeGen_Private::FStructParams Z_Construct_UScriptStruct_FTrafficNetwork_Statics::StructParams = {
	(UObject* (*)())Z_Construct_UPackage__Script_TrafficRuntime,
	nullptr,
	&NewStructOps,
	"TrafficNetwork",
	Z_Construct_UScriptStruct_FTrafficNetwork_Statics::PropPointers,
	UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FTrafficNetwork_Statics::PropPointers),
	sizeof(FTrafficNetwork),
	alignof(FTrafficNetwork),
	RF_Public|RF_Transient|RF_MarkAsNative,
	EStructFlags(0x00000201),
	METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FTrafficNetwork_Statics::Struct_MetaDataParams), Z_Construct_UScriptStruct_FTrafficNetwork_Statics::Struct_MetaDataParams)
};
UScriptStruct* Z_Construct_UScriptStruct_FTrafficNetwork()
{
	if (!Z_Registration_Info_UScriptStruct_FTrafficNetwork.InnerSingleton)
	{
		UECodeGen_Private::ConstructUScriptStruct(Z_Registration_Info_UScriptStruct_FTrafficNetwork.InnerSingleton, Z_Construct_UScriptStruct_FTrafficNetwork_Statics::StructParams);
	}
	return CastChecked<UScriptStruct>(Z_Registration_Info_UScriptStruct_FTrafficNetwork.InnerSingleton);
}
// ********** End ScriptStruct FTrafficNetwork *****************************************************

// ********** Begin Registration *******************************************************************
struct Z_CompiledInDeferFile_FID_Users_rllax_L_E_O_Plugins_AAAtrafficSystem_Source_TrafficRuntime_Public_TrafficRoadTypes_h__Script_TrafficRuntime_Statics
{
	static constexpr FEnumRegisterCompiledInInfo EnumInfo[] = {
		{ ELaneDirection_StaticEnum, TEXT("ELaneDirection"), &Z_Registration_Info_UEnum_ELaneDirection, CONSTRUCT_RELOAD_VERSION_INFO(FEnumReloadVersionInfo, 162408219U) },
		{ ETrafficTurnType_StaticEnum, TEXT("ETrafficTurnType"), &Z_Registration_Info_UEnum_ETrafficTurnType, CONSTRUCT_RELOAD_VERSION_INFO(FEnumReloadVersionInfo, 2962274943U) },
	};
	static constexpr FStructRegisterCompiledInInfo ScriptStructInfo[] = {
		{ FTrafficLane::StaticStruct, Z_Construct_UScriptStruct_FTrafficLane_Statics::NewStructOps, TEXT("TrafficLane"),&Z_Registration_Info_UScriptStruct_FTrafficLane, CONSTRUCT_RELOAD_VERSION_INFO(FStructReloadVersionInfo, sizeof(FTrafficLane), 2577686271U) },
		{ FTrafficRoad::StaticStruct, Z_Construct_UScriptStruct_FTrafficRoad_Statics::NewStructOps, TEXT("TrafficRoad"),&Z_Registration_Info_UScriptStruct_FTrafficRoad, CONSTRUCT_RELOAD_VERSION_INFO(FStructReloadVersionInfo, sizeof(FTrafficRoad), 3477913646U) },
		{ FTrafficIntersection::StaticStruct, Z_Construct_UScriptStruct_FTrafficIntersection_Statics::NewStructOps, TEXT("TrafficIntersection"),&Z_Registration_Info_UScriptStruct_FTrafficIntersection, CONSTRUCT_RELOAD_VERSION_INFO(FStructReloadVersionInfo, sizeof(FTrafficIntersection), 1890181855U) },
		{ FTrafficMovement::StaticStruct, Z_Construct_UScriptStruct_FTrafficMovement_Statics::NewStructOps, TEXT("TrafficMovement"),&Z_Registration_Info_UScriptStruct_FTrafficMovement, CONSTRUCT_RELOAD_VERSION_INFO(FStructReloadVersionInfo, sizeof(FTrafficMovement), 2051957598U) },
		{ FTrafficNetwork::StaticStruct, Z_Construct_UScriptStruct_FTrafficNetwork_Statics::NewStructOps, TEXT("TrafficNetwork"),&Z_Registration_Info_UScriptStruct_FTrafficNetwork, CONSTRUCT_RELOAD_VERSION_INFO(FStructReloadVersionInfo, sizeof(FTrafficNetwork), 1665217710U) },
	};
}; // Z_CompiledInDeferFile_FID_Users_rllax_L_E_O_Plugins_AAAtrafficSystem_Source_TrafficRuntime_Public_TrafficRoadTypes_h__Script_TrafficRuntime_Statics 
static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_rllax_L_E_O_Plugins_AAAtrafficSystem_Source_TrafficRuntime_Public_TrafficRoadTypes_h__Script_TrafficRuntime_2027869492{
	TEXT("/Script/TrafficRuntime"),
	nullptr, 0,
	Z_CompiledInDeferFile_FID_Users_rllax_L_E_O_Plugins_AAAtrafficSystem_Source_TrafficRuntime_Public_TrafficRoadTypes_h__Script_TrafficRuntime_Statics::ScriptStructInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Users_rllax_L_E_O_Plugins_AAAtrafficSystem_Source_TrafficRuntime_Public_TrafficRoadTypes_h__Script_TrafficRuntime_Statics::ScriptStructInfo),
	Z_CompiledInDeferFile_FID_Users_rllax_L_E_O_Plugins_AAAtrafficSystem_Source_TrafficRuntime_Public_TrafficRoadTypes_h__Script_TrafficRuntime_Statics::EnumInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Users_rllax_L_E_O_Plugins_AAAtrafficSystem_Source_TrafficRuntime_Public_TrafficRoadTypes_h__Script_TrafficRuntime_Statics::EnumInfo),
};
// ********** End Registration *********************************************************************

PRAGMA_ENABLE_DEPRECATION_WARNINGS
