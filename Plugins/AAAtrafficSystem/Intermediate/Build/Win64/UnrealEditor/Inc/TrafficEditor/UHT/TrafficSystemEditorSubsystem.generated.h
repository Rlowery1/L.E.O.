// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

// IWYU pragma: private, include "TrafficSystemEditorSubsystem.h"

#ifdef TRAFFICEDITOR_TrafficSystemEditorSubsystem_generated_h
#error "TrafficSystemEditorSubsystem.generated.h already included, missing '#pragma once' in TrafficSystemEditorSubsystem.h"
#endif
#define TRAFFICEDITOR_TrafficSystemEditorSubsystem_generated_h

#include "UObject/ObjectMacros.h"
#include "UObject/ScriptMacros.h"

PRAGMA_DISABLE_DEPRECATION_WARNINGS
struct FGuid;

// ********** Begin ScriptStruct FCalibrationSnippet ***********************************************
struct Z_Construct_UScriptStruct_FCalibrationSnippet_Statics;
#define FID_Users_rllax_L_E_O_Plugins_AAAtrafficSystem_Source_TrafficEditor_Public_TrafficSystemEditorSubsystem_h_17_GENERATED_BODY \
	friend struct ::Z_Construct_UScriptStruct_FCalibrationSnippet_Statics; \
	TRAFFICEDITOR_API static class UScriptStruct* StaticStruct();


struct FCalibrationSnippet;
// ********** End ScriptStruct FCalibrationSnippet *************************************************

// ********** Begin Class UTrafficSystemEditorSubsystem ********************************************
#define FID_Users_rllax_L_E_O_Plugins_AAAtrafficSystem_Source_TrafficEditor_Public_TrafficSystemEditorSubsystem_h_35_RPC_WRAPPERS_NO_PURE_DECLS \
	DECLARE_FUNCTION(execEditor_BakeCalibrationForActiveFamily); \
	DECLARE_FUNCTION(execEditor_BeginCalibrationForFamily);


struct Z_Construct_UClass_UTrafficSystemEditorSubsystem_Statics;
TRAFFICEDITOR_API UClass* Z_Construct_UClass_UTrafficSystemEditorSubsystem_NoRegister();

#define FID_Users_rllax_L_E_O_Plugins_AAAtrafficSystem_Source_TrafficEditor_Public_TrafficSystemEditorSubsystem_h_35_INCLASS_NO_PURE_DECLS \
private: \
	static void StaticRegisterNativesUTrafficSystemEditorSubsystem(); \
	friend struct ::Z_Construct_UClass_UTrafficSystemEditorSubsystem_Statics; \
	static UClass* GetPrivateStaticClass(); \
	friend TRAFFICEDITOR_API UClass* ::Z_Construct_UClass_UTrafficSystemEditorSubsystem_NoRegister(); \
public: \
	DECLARE_CLASS2(UTrafficSystemEditorSubsystem, UEditorSubsystem, COMPILED_IN_FLAGS(0), CASTCLASS_None, TEXT("/Script/TrafficEditor"), Z_Construct_UClass_UTrafficSystemEditorSubsystem_NoRegister) \
	DECLARE_SERIALIZER(UTrafficSystemEditorSubsystem)


#define FID_Users_rllax_L_E_O_Plugins_AAAtrafficSystem_Source_TrafficEditor_Public_TrafficSystemEditorSubsystem_h_35_ENHANCED_CONSTRUCTORS \
	/** Standard constructor, called after all reflected properties have been initialized */ \
	NO_API UTrafficSystemEditorSubsystem(); \
	/** Deleted move- and copy-constructors, should never be used */ \
	UTrafficSystemEditorSubsystem(UTrafficSystemEditorSubsystem&&) = delete; \
	UTrafficSystemEditorSubsystem(const UTrafficSystemEditorSubsystem&) = delete; \
	DECLARE_VTABLE_PTR_HELPER_CTOR(NO_API, UTrafficSystemEditorSubsystem); \
	DEFINE_VTABLE_PTR_HELPER_CTOR_CALLER(UTrafficSystemEditorSubsystem); \
	DEFINE_DEFAULT_CONSTRUCTOR_CALL(UTrafficSystemEditorSubsystem) \
	NO_API virtual ~UTrafficSystemEditorSubsystem();


#define FID_Users_rllax_L_E_O_Plugins_AAAtrafficSystem_Source_TrafficEditor_Public_TrafficSystemEditorSubsystem_h_32_PROLOG
#define FID_Users_rllax_L_E_O_Plugins_AAAtrafficSystem_Source_TrafficEditor_Public_TrafficSystemEditorSubsystem_h_35_GENERATED_BODY \
PRAGMA_DISABLE_DEPRECATION_WARNINGS \
public: \
	FID_Users_rllax_L_E_O_Plugins_AAAtrafficSystem_Source_TrafficEditor_Public_TrafficSystemEditorSubsystem_h_35_RPC_WRAPPERS_NO_PURE_DECLS \
	FID_Users_rllax_L_E_O_Plugins_AAAtrafficSystem_Source_TrafficEditor_Public_TrafficSystemEditorSubsystem_h_35_INCLASS_NO_PURE_DECLS \
	FID_Users_rllax_L_E_O_Plugins_AAAtrafficSystem_Source_TrafficEditor_Public_TrafficSystemEditorSubsystem_h_35_ENHANCED_CONSTRUCTORS \
private: \
PRAGMA_ENABLE_DEPRECATION_WARNINGS


class UTrafficSystemEditorSubsystem;

// ********** End Class UTrafficSystemEditorSubsystem **********************************************

#undef CURRENT_FILE_ID
#define CURRENT_FILE_ID FID_Users_rllax_L_E_O_Plugins_AAAtrafficSystem_Source_TrafficEditor_Public_TrafficSystemEditorSubsystem_h

PRAGMA_ENABLE_DEPRECATION_WARNINGS
