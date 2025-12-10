// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

// IWYU pragma: private, include "TrafficRoadFamilySettings.h"

#ifdef TRAFFICRUNTIME_TrafficRoadFamilySettings_generated_h
#error "TrafficRoadFamilySettings.generated.h already included, missing '#pragma once' in TrafficRoadFamilySettings.h"
#endif
#define TRAFFICRUNTIME_TrafficRoadFamilySettings_generated_h

#include "UObject/ObjectMacros.h"
#include "UObject/ScriptMacros.h"

PRAGMA_DISABLE_DEPRECATION_WARNINGS

// ********** Begin ScriptStruct FTrafficLaneLayoutSide ********************************************
struct Z_Construct_UScriptStruct_FTrafficLaneLayoutSide_Statics;
#define FID_Users_rllax_L_E_O_Plugins_AAAtrafficSystem_Source_TrafficRuntime_Public_TrafficRoadFamilySettings_h_10_GENERATED_BODY \
	friend struct ::Z_Construct_UScriptStruct_FTrafficLaneLayoutSide_Statics; \
	static class UScriptStruct* StaticStruct();


struct FTrafficLaneLayoutSide;
// ********** End ScriptStruct FTrafficLaneLayoutSide **********************************************

// ********** Begin ScriptStruct FRoadFamilyDefinition *********************************************
struct Z_Construct_UScriptStruct_FRoadFamilyDefinition_Statics;
#define FID_Users_rllax_L_E_O_Plugins_AAAtrafficSystem_Source_TrafficRuntime_Public_TrafficRoadFamilySettings_h_25_GENERATED_BODY \
	friend struct ::Z_Construct_UScriptStruct_FRoadFamilyDefinition_Statics; \
	static class UScriptStruct* StaticStruct();


struct FRoadFamilyDefinition;
// ********** End ScriptStruct FRoadFamilyDefinition ***********************************************

// ********** Begin Class UTrafficRoadFamilySettings ***********************************************
struct Z_Construct_UClass_UTrafficRoadFamilySettings_Statics;
TRAFFICRUNTIME_API UClass* Z_Construct_UClass_UTrafficRoadFamilySettings_NoRegister();

#define FID_Users_rllax_L_E_O_Plugins_AAAtrafficSystem_Source_TrafficRuntime_Public_TrafficRoadFamilySettings_h_43_INCLASS_NO_PURE_DECLS \
private: \
	static void StaticRegisterNativesUTrafficRoadFamilySettings(); \
	friend struct ::Z_Construct_UClass_UTrafficRoadFamilySettings_Statics; \
	static UClass* GetPrivateStaticClass(); \
	friend TRAFFICRUNTIME_API UClass* ::Z_Construct_UClass_UTrafficRoadFamilySettings_NoRegister(); \
public: \
	DECLARE_CLASS2(UTrafficRoadFamilySettings, UDeveloperSettings, COMPILED_IN_FLAGS(0 | CLASS_DefaultConfig | CLASS_Config), CASTCLASS_None, TEXT("/Script/TrafficRuntime"), Z_Construct_UClass_UTrafficRoadFamilySettings_NoRegister) \
	DECLARE_SERIALIZER(UTrafficRoadFamilySettings) \
	static constexpr const TCHAR* StaticConfigName() {return TEXT("Game");} \



#define FID_Users_rllax_L_E_O_Plugins_AAAtrafficSystem_Source_TrafficRuntime_Public_TrafficRoadFamilySettings_h_43_ENHANCED_CONSTRUCTORS \
	/** Deleted move- and copy-constructors, should never be used */ \
	UTrafficRoadFamilySettings(UTrafficRoadFamilySettings&&) = delete; \
	UTrafficRoadFamilySettings(const UTrafficRoadFamilySettings&) = delete; \
	DECLARE_VTABLE_PTR_HELPER_CTOR(NO_API, UTrafficRoadFamilySettings); \
	DEFINE_VTABLE_PTR_HELPER_CTOR_CALLER(UTrafficRoadFamilySettings); \
	DEFINE_DEFAULT_CONSTRUCTOR_CALL(UTrafficRoadFamilySettings) \
	NO_API virtual ~UTrafficRoadFamilySettings();


#define FID_Users_rllax_L_E_O_Plugins_AAAtrafficSystem_Source_TrafficRuntime_Public_TrafficRoadFamilySettings_h_40_PROLOG
#define FID_Users_rllax_L_E_O_Plugins_AAAtrafficSystem_Source_TrafficRuntime_Public_TrafficRoadFamilySettings_h_43_GENERATED_BODY \
PRAGMA_DISABLE_DEPRECATION_WARNINGS \
public: \
	FID_Users_rllax_L_E_O_Plugins_AAAtrafficSystem_Source_TrafficRuntime_Public_TrafficRoadFamilySettings_h_43_INCLASS_NO_PURE_DECLS \
	FID_Users_rllax_L_E_O_Plugins_AAAtrafficSystem_Source_TrafficRuntime_Public_TrafficRoadFamilySettings_h_43_ENHANCED_CONSTRUCTORS \
private: \
PRAGMA_ENABLE_DEPRECATION_WARNINGS


class UTrafficRoadFamilySettings;

// ********** End Class UTrafficRoadFamilySettings *************************************************

#undef CURRENT_FILE_ID
#define CURRENT_FILE_ID FID_Users_rllax_L_E_O_Plugins_AAAtrafficSystem_Source_TrafficRuntime_Public_TrafficRoadFamilySettings_h

PRAGMA_ENABLE_DEPRECATION_WARNINGS
