// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

// IWYU pragma: private, include "TrafficAutomationLogger.h"

#ifdef TRAFFICRUNTIME_TrafficAutomationLogger_generated_h
#error "TrafficAutomationLogger.generated.h already included, missing '#pragma once' in TrafficAutomationLogger.h"
#endif
#define TRAFFICRUNTIME_TrafficAutomationLogger_generated_h

#include "UObject/ObjectMacros.h"
#include "UObject/ScriptMacros.h"

PRAGMA_DISABLE_DEPRECATION_WARNINGS

// ********** Begin Class UTrafficAutomationLogger *************************************************
#define FID_Users_rllax_L_E_O_Plugins_AAAtrafficSystem_Source_TrafficRuntime_Public_TrafficAutomationLogger_h_9_RPC_WRAPPERS_NO_PURE_DECLS \
	DECLARE_FUNCTION(execEndTestLog); \
	DECLARE_FUNCTION(execLogMetricFloat); \
	DECLARE_FUNCTION(execLogMetric); \
	DECLARE_FUNCTION(execLogLine); \
	DECLARE_FUNCTION(execBeginTestLog);


struct Z_Construct_UClass_UTrafficAutomationLogger_Statics;
TRAFFICRUNTIME_API UClass* Z_Construct_UClass_UTrafficAutomationLogger_NoRegister();

#define FID_Users_rllax_L_E_O_Plugins_AAAtrafficSystem_Source_TrafficRuntime_Public_TrafficAutomationLogger_h_9_INCLASS_NO_PURE_DECLS \
private: \
	static void StaticRegisterNativesUTrafficAutomationLogger(); \
	friend struct ::Z_Construct_UClass_UTrafficAutomationLogger_Statics; \
	static UClass* GetPrivateStaticClass(); \
	friend TRAFFICRUNTIME_API UClass* ::Z_Construct_UClass_UTrafficAutomationLogger_NoRegister(); \
public: \
	DECLARE_CLASS2(UTrafficAutomationLogger, UObject, COMPILED_IN_FLAGS(0), CASTCLASS_None, TEXT("/Script/TrafficRuntime"), Z_Construct_UClass_UTrafficAutomationLogger_NoRegister) \
	DECLARE_SERIALIZER(UTrafficAutomationLogger)


#define FID_Users_rllax_L_E_O_Plugins_AAAtrafficSystem_Source_TrafficRuntime_Public_TrafficAutomationLogger_h_9_ENHANCED_CONSTRUCTORS \
	/** Standard constructor, called after all reflected properties have been initialized */ \
	NO_API UTrafficAutomationLogger(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get()); \
	/** Deleted move- and copy-constructors, should never be used */ \
	UTrafficAutomationLogger(UTrafficAutomationLogger&&) = delete; \
	UTrafficAutomationLogger(const UTrafficAutomationLogger&) = delete; \
	DECLARE_VTABLE_PTR_HELPER_CTOR(NO_API, UTrafficAutomationLogger); \
	DEFINE_VTABLE_PTR_HELPER_CTOR_CALLER(UTrafficAutomationLogger); \
	DEFINE_DEFAULT_OBJECT_INITIALIZER_CONSTRUCTOR_CALL(UTrafficAutomationLogger) \
	NO_API virtual ~UTrafficAutomationLogger();


#define FID_Users_rllax_L_E_O_Plugins_AAAtrafficSystem_Source_TrafficRuntime_Public_TrafficAutomationLogger_h_6_PROLOG
#define FID_Users_rllax_L_E_O_Plugins_AAAtrafficSystem_Source_TrafficRuntime_Public_TrafficAutomationLogger_h_9_GENERATED_BODY \
PRAGMA_DISABLE_DEPRECATION_WARNINGS \
public: \
	FID_Users_rllax_L_E_O_Plugins_AAAtrafficSystem_Source_TrafficRuntime_Public_TrafficAutomationLogger_h_9_RPC_WRAPPERS_NO_PURE_DECLS \
	FID_Users_rllax_L_E_O_Plugins_AAAtrafficSystem_Source_TrafficRuntime_Public_TrafficAutomationLogger_h_9_INCLASS_NO_PURE_DECLS \
	FID_Users_rllax_L_E_O_Plugins_AAAtrafficSystem_Source_TrafficRuntime_Public_TrafficAutomationLogger_h_9_ENHANCED_CONSTRUCTORS \
private: \
PRAGMA_ENABLE_DEPRECATION_WARNINGS


class UTrafficAutomationLogger;

// ********** End Class UTrafficAutomationLogger ***************************************************

#undef CURRENT_FILE_ID
#define CURRENT_FILE_ID FID_Users_rllax_L_E_O_Plugins_AAAtrafficSystem_Source_TrafficRuntime_Public_TrafficAutomationLogger_h

PRAGMA_ENABLE_DEPRECATION_WARNINGS
