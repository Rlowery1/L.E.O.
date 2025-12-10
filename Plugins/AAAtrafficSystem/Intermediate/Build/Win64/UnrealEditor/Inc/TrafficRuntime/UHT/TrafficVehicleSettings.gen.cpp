// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "TrafficVehicleSettings.h"

PRAGMA_DISABLE_DEPRECATION_WARNINGS
static_assert(!UE_WITH_CONSTINIT_UOBJECT, "This generated code can only be compiled with !UE_WITH_CONSTINIT_OBJECT");
void EmptyLinkFunctionForGeneratedCodeTrafficVehicleSettings() {}

// ********** Begin Cross Module References ********************************************************
COREUOBJECT_API UClass* Z_Construct_UClass_UClass_NoRegister();
DEVELOPERSETTINGS_API UClass* Z_Construct_UClass_UDeveloperSettings();
ENGINE_API UClass* Z_Construct_UClass_AActor_NoRegister();
TRAFFICRUNTIME_API UClass* Z_Construct_UClass_ATrafficVehicleBase_NoRegister();
TRAFFICRUNTIME_API UClass* Z_Construct_UClass_UTrafficVehicleSettings();
TRAFFICRUNTIME_API UClass* Z_Construct_UClass_UTrafficVehicleSettings_NoRegister();
UPackage* Z_Construct_UPackage__Script_TrafficRuntime();
// ********** End Cross Module References **********************************************************

// ********** Begin Class UTrafficVehicleSettings **************************************************
FClassRegistrationInfo Z_Registration_Info_UClass_UTrafficVehicleSettings;
UClass* UTrafficVehicleSettings::GetPrivateStaticClass()
{
	using TClass = UTrafficVehicleSettings;
	if (!Z_Registration_Info_UClass_UTrafficVehicleSettings.InnerSingleton)
	{
		GetPrivateStaticClassBody(
			TClass::StaticPackage(),
			TEXT("TrafficVehicleSettings"),
			Z_Registration_Info_UClass_UTrafficVehicleSettings.InnerSingleton,
			StaticRegisterNativesUTrafficVehicleSettings,
			sizeof(TClass),
			alignof(TClass),
			TClass::StaticClassFlags,
			TClass::StaticClassCastFlags(),
			TClass::StaticConfigName(),
			(UClass::ClassConstructorType)InternalConstructor<TClass>,
			(UClass::ClassVTableHelperCtorCallerType)InternalVTableHelperCtorCaller<TClass>,
			UOBJECT_CPPCLASS_STATICFUNCTIONS_FORCLASS(TClass),
			&TClass::Super::StaticClass,
			&TClass::WithinClass::StaticClass
		);
	}
	return Z_Registration_Info_UClass_UTrafficVehicleSettings.InnerSingleton;
}
UClass* Z_Construct_UClass_UTrafficVehicleSettings_NoRegister()
{
	return UTrafficVehicleSettings::GetPrivateStaticClass();
}
struct Z_Construct_UClass_UTrafficVehicleSettings_Statics
{
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Class_MetaDataParams[] = {
#if !UE_BUILD_SHIPPING
		{ "Comment", "/**\n * Vehicle defaults for AAA Traffic (test vehicles, Chaos class selection, etc.)\n */" },
#endif
		{ "IncludePath", "TrafficVehicleSettings.h" },
		{ "ModuleRelativePath", "Public/TrafficVehicleSettings.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Vehicle defaults for AAA Traffic (test vehicles, Chaos class selection, etc.)" },
#endif
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_DefaultTestVehicleClass_MetaData[] = {
		{ "Category", "Vehicles" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** Default vehicle class to spawn for test traffic (e.g., a Chaos vehicle BP from CitySample). */" },
#endif
		{ "ModuleRelativePath", "Public/TrafficVehicleSettings.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Default vehicle class to spawn for test traffic (e.g., a Chaos vehicle BP from CitySample)." },
#endif
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_bUseExternalVehicleAdapter_MetaData[] = {
		{ "Category", "Vehicles" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** Use an external visual Chaos vehicle (any Actor class) attached to an adapter that follows traffic lanes. */" },
#endif
		{ "ModuleRelativePath", "Public/TrafficVehicleSettings.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Use an external visual Chaos vehicle (any Actor class) attached to an adapter that follows traffic lanes." },
#endif
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_ExternalVehicleClass_MetaData[] = {
		{ "Category", "Vehicles" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** The external vehicle Blueprint/Class to attach when using the adapter (can be City Sample or any Chaos vehicle). */" },
#endif
		{ "EditCondition", "bUseExternalVehicleAdapter" },
		{ "ModuleRelativePath", "Public/TrafficVehicleSettings.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "The external vehicle Blueprint/Class to attach when using the adapter (can be City Sample or any Chaos vehicle)." },
#endif
	};
#endif // WITH_METADATA

// ********** Begin Class UTrafficVehicleSettings constinit property declarations ******************
	static const UECodeGen_Private::FSoftClassPropertyParams NewProp_DefaultTestVehicleClass;
	static void NewProp_bUseExternalVehicleAdapter_SetBit(void* Obj);
	static const UECodeGen_Private::FBoolPropertyParams NewProp_bUseExternalVehicleAdapter;
	static const UECodeGen_Private::FSoftClassPropertyParams NewProp_ExternalVehicleClass;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
// ********** End Class UTrafficVehicleSettings constinit property declarations ********************
	static UObject* (*const DependentSingletons[])();
	static constexpr FCppClassTypeInfoStatic StaticCppClassTypeInfo = {
		TCppClassTypeTraits<UTrafficVehicleSettings>::IsAbstract,
	};
	static const UECodeGen_Private::FClassParams ClassParams;
}; // struct Z_Construct_UClass_UTrafficVehicleSettings_Statics

// ********** Begin Class UTrafficVehicleSettings Property Definitions *****************************
const UECodeGen_Private::FSoftClassPropertyParams Z_Construct_UClass_UTrafficVehicleSettings_Statics::NewProp_DefaultTestVehicleClass = { "DefaultTestVehicleClass", nullptr, (EPropertyFlags)0x0014000000004001, UECodeGen_Private::EPropertyGenFlags::SoftClass, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(UTrafficVehicleSettings, DefaultTestVehicleClass), Z_Construct_UClass_ATrafficVehicleBase_NoRegister, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_DefaultTestVehicleClass_MetaData), NewProp_DefaultTestVehicleClass_MetaData) };
void Z_Construct_UClass_UTrafficVehicleSettings_Statics::NewProp_bUseExternalVehicleAdapter_SetBit(void* Obj)
{
	((UTrafficVehicleSettings*)Obj)->bUseExternalVehicleAdapter = 1;
}
const UECodeGen_Private::FBoolPropertyParams Z_Construct_UClass_UTrafficVehicleSettings_Statics::NewProp_bUseExternalVehicleAdapter = { "bUseExternalVehicleAdapter", nullptr, (EPropertyFlags)0x0010000000004001, UECodeGen_Private::EPropertyGenFlags::Bool | UECodeGen_Private::EPropertyGenFlags::NativeBool, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, sizeof(bool), sizeof(UTrafficVehicleSettings), &Z_Construct_UClass_UTrafficVehicleSettings_Statics::NewProp_bUseExternalVehicleAdapter_SetBit, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_bUseExternalVehicleAdapter_MetaData), NewProp_bUseExternalVehicleAdapter_MetaData) };
const UECodeGen_Private::FSoftClassPropertyParams Z_Construct_UClass_UTrafficVehicleSettings_Statics::NewProp_ExternalVehicleClass = { "ExternalVehicleClass", nullptr, (EPropertyFlags)0x0014000000004001, UECodeGen_Private::EPropertyGenFlags::SoftClass, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(UTrafficVehicleSettings, ExternalVehicleClass), Z_Construct_UClass_AActor_NoRegister, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_ExternalVehicleClass_MetaData), NewProp_ExternalVehicleClass_MetaData) };
const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UClass_UTrafficVehicleSettings_Statics::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UTrafficVehicleSettings_Statics::NewProp_DefaultTestVehicleClass,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UTrafficVehicleSettings_Statics::NewProp_bUseExternalVehicleAdapter,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UTrafficVehicleSettings_Statics::NewProp_ExternalVehicleClass,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_UTrafficVehicleSettings_Statics::PropPointers) < 2048);
// ********** End Class UTrafficVehicleSettings Property Definitions *******************************
UObject* (*const Z_Construct_UClass_UTrafficVehicleSettings_Statics::DependentSingletons[])() = {
	(UObject* (*)())Z_Construct_UClass_UDeveloperSettings,
	(UObject* (*)())Z_Construct_UPackage__Script_TrafficRuntime,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_UTrafficVehicleSettings_Statics::DependentSingletons) < 16);
const UECodeGen_Private::FClassParams Z_Construct_UClass_UTrafficVehicleSettings_Statics::ClassParams = {
	&UTrafficVehicleSettings::StaticClass,
	"Game",
	&StaticCppClassTypeInfo,
	DependentSingletons,
	nullptr,
	Z_Construct_UClass_UTrafficVehicleSettings_Statics::PropPointers,
	nullptr,
	UE_ARRAY_COUNT(DependentSingletons),
	0,
	UE_ARRAY_COUNT(Z_Construct_UClass_UTrafficVehicleSettings_Statics::PropPointers),
	0,
	0x001000A6u,
	METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UClass_UTrafficVehicleSettings_Statics::Class_MetaDataParams), Z_Construct_UClass_UTrafficVehicleSettings_Statics::Class_MetaDataParams)
};
void UTrafficVehicleSettings::StaticRegisterNativesUTrafficVehicleSettings()
{
}
UClass* Z_Construct_UClass_UTrafficVehicleSettings()
{
	if (!Z_Registration_Info_UClass_UTrafficVehicleSettings.OuterSingleton)
	{
		UECodeGen_Private::ConstructUClass(Z_Registration_Info_UClass_UTrafficVehicleSettings.OuterSingleton, Z_Construct_UClass_UTrafficVehicleSettings_Statics::ClassParams);
	}
	return Z_Registration_Info_UClass_UTrafficVehicleSettings.OuterSingleton;
}
DEFINE_VTABLE_PTR_HELPER_CTOR_NS(, UTrafficVehicleSettings);
UTrafficVehicleSettings::~UTrafficVehicleSettings() {}
// ********** End Class UTrafficVehicleSettings ****************************************************

// ********** Begin Registration *******************************************************************
struct Z_CompiledInDeferFile_FID_Users_rllax_L_E_O_Plugins_AAAtrafficSystem_Source_TrafficRuntime_Public_TrafficVehicleSettings_h__Script_TrafficRuntime_Statics
{
	static constexpr FClassRegisterCompiledInInfo ClassInfo[] = {
		{ Z_Construct_UClass_UTrafficVehicleSettings, UTrafficVehicleSettings::StaticClass, TEXT("UTrafficVehicleSettings"), &Z_Registration_Info_UClass_UTrafficVehicleSettings, CONSTRUCT_RELOAD_VERSION_INFO(FClassReloadVersionInfo, sizeof(UTrafficVehicleSettings), 1728041763U) },
	};
}; // Z_CompiledInDeferFile_FID_Users_rllax_L_E_O_Plugins_AAAtrafficSystem_Source_TrafficRuntime_Public_TrafficVehicleSettings_h__Script_TrafficRuntime_Statics 
static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_rllax_L_E_O_Plugins_AAAtrafficSystem_Source_TrafficRuntime_Public_TrafficVehicleSettings_h__Script_TrafficRuntime_4143899843{
	TEXT("/Script/TrafficRuntime"),
	Z_CompiledInDeferFile_FID_Users_rllax_L_E_O_Plugins_AAAtrafficSystem_Source_TrafficRuntime_Public_TrafficVehicleSettings_h__Script_TrafficRuntime_Statics::ClassInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Users_rllax_L_E_O_Plugins_AAAtrafficSystem_Source_TrafficRuntime_Public_TrafficVehicleSettings_h__Script_TrafficRuntime_Statics::ClassInfo),
	nullptr, 0,
	nullptr, 0,
};
// ********** End Registration *********************************************************************

PRAGMA_ENABLE_DEPRECATION_WARNINGS
