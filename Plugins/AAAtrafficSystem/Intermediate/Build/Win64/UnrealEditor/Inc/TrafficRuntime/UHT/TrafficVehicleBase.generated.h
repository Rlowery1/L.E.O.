// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

// IWYU pragma: private, include "TrafficVehicleBase.h"

#ifdef TRAFFICRUNTIME_TrafficVehicleBase_generated_h
#error "TrafficVehicleBase.generated.h already included, missing '#pragma once' in TrafficVehicleBase.h"
#endif
#define TRAFFICRUNTIME_TrafficVehicleBase_generated_h

#include "UObject/ObjectMacros.h"
#include "UObject/ScriptMacros.h"

PRAGMA_DISABLE_DEPRECATION_WARNINGS

// ********** Begin Class ATrafficVehicleBase ******************************************************
struct Z_Construct_UClass_ATrafficVehicleBase_Statics;
TRAFFICRUNTIME_API UClass* Z_Construct_UClass_ATrafficVehicleBase_NoRegister();

#define FID_Users_rllax_L_E_O_Plugins_AAAtrafficSystem_Source_TrafficRuntime_Public_TrafficVehicleBase_h_16_INCLASS_NO_PURE_DECLS \
private: \
	static void StaticRegisterNativesATrafficVehicleBase(); \
	friend struct ::Z_Construct_UClass_ATrafficVehicleBase_Statics; \
	static UClass* GetPrivateStaticClass(); \
	friend TRAFFICRUNTIME_API UClass* ::Z_Construct_UClass_ATrafficVehicleBase_NoRegister(); \
public: \
	DECLARE_CLASS2(ATrafficVehicleBase, AActor, COMPILED_IN_FLAGS(0 | CLASS_Config), CASTCLASS_None, TEXT("/Script/TrafficRuntime"), Z_Construct_UClass_ATrafficVehicleBase_NoRegister) \
	DECLARE_SERIALIZER(ATrafficVehicleBase)


#define FID_Users_rllax_L_E_O_Plugins_AAAtrafficSystem_Source_TrafficRuntime_Public_TrafficVehicleBase_h_16_ENHANCED_CONSTRUCTORS \
	/** Deleted move- and copy-constructors, should never be used */ \
	ATrafficVehicleBase(ATrafficVehicleBase&&) = delete; \
	ATrafficVehicleBase(const ATrafficVehicleBase&) = delete; \
	DECLARE_VTABLE_PTR_HELPER_CTOR(NO_API, ATrafficVehicleBase); \
	DEFINE_VTABLE_PTR_HELPER_CTOR_CALLER(ATrafficVehicleBase); \
	DEFINE_DEFAULT_CONSTRUCTOR_CALL(ATrafficVehicleBase) \
	NO_API virtual ~ATrafficVehicleBase();


#define FID_Users_rllax_L_E_O_Plugins_AAAtrafficSystem_Source_TrafficRuntime_Public_TrafficVehicleBase_h_13_PROLOG
#define FID_Users_rllax_L_E_O_Plugins_AAAtrafficSystem_Source_TrafficRuntime_Public_TrafficVehicleBase_h_16_GENERATED_BODY \
PRAGMA_DISABLE_DEPRECATION_WARNINGS \
public: \
	FID_Users_rllax_L_E_O_Plugins_AAAtrafficSystem_Source_TrafficRuntime_Public_TrafficVehicleBase_h_16_INCLASS_NO_PURE_DECLS \
	FID_Users_rllax_L_E_O_Plugins_AAAtrafficSystem_Source_TrafficRuntime_Public_TrafficVehicleBase_h_16_ENHANCED_CONSTRUCTORS \
private: \
PRAGMA_ENABLE_DEPRECATION_WARNINGS


class ATrafficVehicleBase;

// ********** End Class ATrafficVehicleBase ********************************************************

#undef CURRENT_FILE_ID
#define CURRENT_FILE_ID FID_Users_rllax_L_E_O_Plugins_AAAtrafficSystem_Source_TrafficRuntime_Public_TrafficVehicleBase_h

PRAGMA_ENABLE_DEPRECATION_WARNINGS
