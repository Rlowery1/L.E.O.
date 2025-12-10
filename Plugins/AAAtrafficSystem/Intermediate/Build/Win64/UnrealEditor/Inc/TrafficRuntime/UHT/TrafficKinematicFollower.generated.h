// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

// IWYU pragma: private, include "TrafficKinematicFollower.h"

#ifdef TRAFFICRUNTIME_TrafficKinematicFollower_generated_h
#error "TrafficKinematicFollower.generated.h already included, missing '#pragma once' in TrafficKinematicFollower.h"
#endif
#define TRAFFICRUNTIME_TrafficKinematicFollower_generated_h

#include "UObject/ObjectMacros.h"
#include "UObject/ScriptMacros.h"

PRAGMA_DISABLE_DEPRECATION_WARNINGS

// ********** Begin ScriptStruct FPathFollowState **************************************************
struct Z_Construct_UScriptStruct_FPathFollowState_Statics;
#define FID_Users_rllax_L_E_O_Plugins_AAAtrafficSystem_Source_TrafficRuntime_Public_TrafficKinematicFollower_h_19_GENERATED_BODY \
	friend struct ::Z_Construct_UScriptStruct_FPathFollowState_Statics; \
	static class UScriptStruct* StaticStruct();


struct FPathFollowState;
// ********** End ScriptStruct FPathFollowState ****************************************************

// ********** Begin Class UTrafficKinematicFollower ************************************************
struct Z_Construct_UClass_UTrafficKinematicFollower_Statics;
TRAFFICRUNTIME_API UClass* Z_Construct_UClass_UTrafficKinematicFollower_NoRegister();

#define FID_Users_rllax_L_E_O_Plugins_AAAtrafficSystem_Source_TrafficRuntime_Public_TrafficKinematicFollower_h_37_INCLASS_NO_PURE_DECLS \
private: \
	static void StaticRegisterNativesUTrafficKinematicFollower(); \
	friend struct ::Z_Construct_UClass_UTrafficKinematicFollower_Statics; \
	static UClass* GetPrivateStaticClass(); \
	friend TRAFFICRUNTIME_API UClass* ::Z_Construct_UClass_UTrafficKinematicFollower_NoRegister(); \
public: \
	DECLARE_CLASS2(UTrafficKinematicFollower, UObject, COMPILED_IN_FLAGS(0), CASTCLASS_None, TEXT("/Script/TrafficRuntime"), Z_Construct_UClass_UTrafficKinematicFollower_NoRegister) \
	DECLARE_SERIALIZER(UTrafficKinematicFollower)


#define FID_Users_rllax_L_E_O_Plugins_AAAtrafficSystem_Source_TrafficRuntime_Public_TrafficKinematicFollower_h_37_ENHANCED_CONSTRUCTORS \
	/** Standard constructor, called after all reflected properties have been initialized */ \
	NO_API UTrafficKinematicFollower(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get()); \
	/** Deleted move- and copy-constructors, should never be used */ \
	UTrafficKinematicFollower(UTrafficKinematicFollower&&) = delete; \
	UTrafficKinematicFollower(const UTrafficKinematicFollower&) = delete; \
	DECLARE_VTABLE_PTR_HELPER_CTOR(NO_API, UTrafficKinematicFollower); \
	DEFINE_VTABLE_PTR_HELPER_CTOR_CALLER(UTrafficKinematicFollower); \
	DEFINE_DEFAULT_OBJECT_INITIALIZER_CONSTRUCTOR_CALL(UTrafficKinematicFollower) \
	NO_API virtual ~UTrafficKinematicFollower();


#define FID_Users_rllax_L_E_O_Plugins_AAAtrafficSystem_Source_TrafficRuntime_Public_TrafficKinematicFollower_h_34_PROLOG
#define FID_Users_rllax_L_E_O_Plugins_AAAtrafficSystem_Source_TrafficRuntime_Public_TrafficKinematicFollower_h_37_GENERATED_BODY \
PRAGMA_DISABLE_DEPRECATION_WARNINGS \
public: \
	FID_Users_rllax_L_E_O_Plugins_AAAtrafficSystem_Source_TrafficRuntime_Public_TrafficKinematicFollower_h_37_INCLASS_NO_PURE_DECLS \
	FID_Users_rllax_L_E_O_Plugins_AAAtrafficSystem_Source_TrafficRuntime_Public_TrafficKinematicFollower_h_37_ENHANCED_CONSTRUCTORS \
private: \
PRAGMA_ENABLE_DEPRECATION_WARNINGS


class UTrafficKinematicFollower;

// ********** End Class UTrafficKinematicFollower **************************************************

#undef CURRENT_FILE_ID
#define CURRENT_FILE_ID FID_Users_rllax_L_E_O_Plugins_AAAtrafficSystem_Source_TrafficRuntime_Public_TrafficKinematicFollower_h

// ********** Begin Enum EPathFollowTargetType *****************************************************
#define FOREACH_ENUM_EPATHFOLLOWTARGETTYPE(op) \
	op(EPathFollowTargetType::None) \
	op(EPathFollowTargetType::Lane) \
	op(EPathFollowTargetType::Movement) 

enum class EPathFollowTargetType : uint8;
template<> struct TIsUEnumClass<EPathFollowTargetType> { enum { Value = true }; };
template<> TRAFFICRUNTIME_NON_ATTRIBUTED_API UEnum* StaticEnum<EPathFollowTargetType>();
// ********** End Enum EPathFollowTargetType *******************************************************

PRAGMA_ENABLE_DEPRECATION_WARNINGS
