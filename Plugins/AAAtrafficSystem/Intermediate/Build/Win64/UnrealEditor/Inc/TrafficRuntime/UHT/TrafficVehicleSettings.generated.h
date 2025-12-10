// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

// IWYU pragma: private, include "TrafficVehicleSettings.h"

#ifdef TRAFFICRUNTIME_TrafficVehicleSettings_generated_h
#error "TrafficVehicleSettings.generated.h already included, missing '#pragma once' in TrafficVehicleSettings.h"
#endif
#define TRAFFICRUNTIME_TrafficVehicleSettings_generated_h

#include "UObject/ObjectMacros.h"
#include "UObject/ScriptMacros.h"

PRAGMA_DISABLE_DEPRECATION_WARNINGS

// ********** Begin Class UTrafficVehicleSettings **************************************************
struct Z_Construct_UClass_UTrafficVehicleSettings_Statics;
TRAFFICRUNTIME_API UClass* Z_Construct_UClass_UTrafficVehicleSettings_NoRegister();

#define FID_Users_rllax_L_E_O_Plugins_AAAtrafficSystem_Source_TrafficRuntime_Public_TrafficVehicleSettings_h_16_INCLASS_NO_PURE_DECLS \
private: \
	static void StaticRegisterNativesUTrafficVehicleSettings(); \
	friend struct ::Z_Construct_UClass_UTrafficVehicleSettings_Statics; \
	static UClass* GetPrivateStaticClass(); \
	friend TRAFFICRUNTIME_API UClass* ::Z_Construct_UClass_UTrafficVehicleSettings_NoRegister(); \
public: \
	DECLARE_CLASS2(UTrafficVehicleSettings, UDeveloperSettings, COMPILED_IN_FLAGS(0 | CLASS_DefaultConfig | CLASS_Config), CASTCLASS_None, TEXT("/Script/TrafficRuntime"), Z_Construct_UClass_UTrafficVehicleSettings_NoRegister) \
	DECLARE_SERIALIZER(UTrafficVehicleSettings) \
	static constexpr const TCHAR* StaticConfigName() {return TEXT("Game");} \



#define FID_Users_rllax_L_E_O_Plugins_AAAtrafficSystem_Source_TrafficRuntime_Public_TrafficVehicleSettings_h_16_ENHANCED_CONSTRUCTORS \
	/** Deleted move- and copy-constructors, should never be used */ \
	UTrafficVehicleSettings(UTrafficVehicleSettings&&) = delete; \
	UTrafficVehicleSettings(const UTrafficVehicleSettings&) = delete; \
	DECLARE_VTABLE_PTR_HELPER_CTOR(NO_API, UTrafficVehicleSettings); \
	DEFINE_VTABLE_PTR_HELPER_CTOR_CALLER(UTrafficVehicleSettings); \
	DEFINE_DEFAULT_CONSTRUCTOR_CALL(UTrafficVehicleSettings) \
	NO_API virtual ~UTrafficVehicleSettings();


#define FID_Users_rllax_L_E_O_Plugins_AAAtrafficSystem_Source_TrafficRuntime_Public_TrafficVehicleSettings_h_13_PROLOG
#define FID_Users_rllax_L_E_O_Plugins_AAAtrafficSystem_Source_TrafficRuntime_Public_TrafficVehicleSettings_h_16_GENERATED_BODY \
PRAGMA_DISABLE_DEPRECATION_WARNINGS \
public: \
	FID_Users_rllax_L_E_O_Plugins_AAAtrafficSystem_Source_TrafficRuntime_Public_TrafficVehicleSettings_h_16_INCLASS_NO_PURE_DECLS \
	FID_Users_rllax_L_E_O_Plugins_AAAtrafficSystem_Source_TrafficRuntime_Public_TrafficVehicleSettings_h_16_ENHANCED_CONSTRUCTORS \
private: \
PRAGMA_ENABLE_DEPRECATION_WARNINGS


class UTrafficVehicleSettings;

// ********** End Class UTrafficVehicleSettings ****************************************************

#undef CURRENT_FILE_ID
#define CURRENT_FILE_ID FID_Users_rllax_L_E_O_Plugins_AAAtrafficSystem_Source_TrafficRuntime_Public_TrafficVehicleSettings_h

PRAGMA_ENABLE_DEPRECATION_WARNINGS
