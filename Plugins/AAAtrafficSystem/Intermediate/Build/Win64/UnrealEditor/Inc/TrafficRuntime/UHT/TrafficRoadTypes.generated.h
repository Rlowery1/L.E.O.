// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

// IWYU pragma: private, include "TrafficRoadTypes.h"

#ifdef TRAFFICRUNTIME_TrafficRoadTypes_generated_h
#error "TrafficRoadTypes.generated.h already included, missing '#pragma once' in TrafficRoadTypes.h"
#endif
#define TRAFFICRUNTIME_TrafficRoadTypes_generated_h

#include "UObject/ObjectMacros.h"
#include "UObject/ScriptMacros.h"

PRAGMA_DISABLE_DEPRECATION_WARNINGS

// ********** Begin ScriptStruct FTrafficLane ******************************************************
struct Z_Construct_UScriptStruct_FTrafficLane_Statics;
#define FID_Users_rllax_L_E_O_Plugins_AAAtrafficSystem_Source_TrafficRuntime_Public_TrafficRoadTypes_h_26_GENERATED_BODY \
	friend struct ::Z_Construct_UScriptStruct_FTrafficLane_Statics; \
	static class UScriptStruct* StaticStruct();


struct FTrafficLane;
// ********** End ScriptStruct FTrafficLane ********************************************************

// ********** Begin ScriptStruct FTrafficRoad ******************************************************
struct Z_Construct_UScriptStruct_FTrafficRoad_Statics;
#define FID_Users_rllax_L_E_O_Plugins_AAAtrafficSystem_Source_TrafficRuntime_Public_TrafficRoadTypes_h_59_GENERATED_BODY \
	friend struct ::Z_Construct_UScriptStruct_FTrafficRoad_Statics; \
	static class UScriptStruct* StaticStruct();


struct FTrafficRoad;
// ********** End ScriptStruct FTrafficRoad ********************************************************

// ********** Begin ScriptStruct FTrafficIntersection **********************************************
struct Z_Construct_UScriptStruct_FTrafficIntersection_Statics;
#define FID_Users_rllax_L_E_O_Plugins_AAAtrafficSystem_Source_TrafficRuntime_Public_TrafficRoadTypes_h_80_GENERATED_BODY \
	friend struct ::Z_Construct_UScriptStruct_FTrafficIntersection_Statics; \
	static class UScriptStruct* StaticStruct();


struct FTrafficIntersection;
// ********** End ScriptStruct FTrafficIntersection ************************************************

// ********** Begin ScriptStruct FTrafficMovement **************************************************
struct Z_Construct_UScriptStruct_FTrafficMovement_Statics;
#define FID_Users_rllax_L_E_O_Plugins_AAAtrafficSystem_Source_TrafficRuntime_Public_TrafficRoadTypes_h_101_GENERATED_BODY \
	friend struct ::Z_Construct_UScriptStruct_FTrafficMovement_Statics; \
	static class UScriptStruct* StaticStruct();


struct FTrafficMovement;
// ********** End ScriptStruct FTrafficMovement ****************************************************

// ********** Begin ScriptStruct FTrafficNetwork ***************************************************
struct Z_Construct_UScriptStruct_FTrafficNetwork_Statics;
#define FID_Users_rllax_L_E_O_Plugins_AAAtrafficSystem_Source_TrafficRuntime_Public_TrafficRoadTypes_h_125_GENERATED_BODY \
	friend struct ::Z_Construct_UScriptStruct_FTrafficNetwork_Statics; \
	static class UScriptStruct* StaticStruct();


struct FTrafficNetwork;
// ********** End ScriptStruct FTrafficNetwork *****************************************************

#undef CURRENT_FILE_ID
#define CURRENT_FILE_ID FID_Users_rllax_L_E_O_Plugins_AAAtrafficSystem_Source_TrafficRuntime_Public_TrafficRoadTypes_h

// ********** Begin Enum ELaneDirection ************************************************************
#define FOREACH_ENUM_ELANEDIRECTION(op) \
	op(ELaneDirection::Forward) \
	op(ELaneDirection::Backward) \
	op(ELaneDirection::Bidirectional) 

enum class ELaneDirection : uint8;
template<> struct TIsUEnumClass<ELaneDirection> { enum { Value = true }; };
template<> TRAFFICRUNTIME_NON_ATTRIBUTED_API UEnum* StaticEnum<ELaneDirection>();
// ********** End Enum ELaneDirection **************************************************************

// ********** Begin Enum ETrafficTurnType **********************************************************
#define FOREACH_ENUM_ETRAFFICTURNTYPE(op) \
	op(ETrafficTurnType::Through) \
	op(ETrafficTurnType::Left) \
	op(ETrafficTurnType::Right) \
	op(ETrafficTurnType::UTurn) 

enum class ETrafficTurnType : uint8;
template<> struct TIsUEnumClass<ETrafficTurnType> { enum { Value = true }; };
template<> TRAFFICRUNTIME_NON_ATTRIBUTED_API UEnum* StaticEnum<ETrafficTurnType>();
// ********** End Enum ETrafficTurnType ************************************************************

PRAGMA_ENABLE_DEPRECATION_WARNINGS
