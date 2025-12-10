// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "TrafficVehicleManager.h"

PRAGMA_DISABLE_DEPRECATION_WARNINGS
static_assert(!UE_WITH_CONSTINIT_UOBJECT, "This generated code can only be compiled with !UE_WITH_CONSTINIT_OBJECT");
void EmptyLinkFunctionForGeneratedCodeTrafficVehicleManager() {}

// ********** Begin Cross Module References ********************************************************
ENGINE_API UClass* Z_Construct_UClass_AActor();
TRAFFICRUNTIME_API UClass* Z_Construct_UClass_ATrafficVehicleBase_NoRegister();
TRAFFICRUNTIME_API UClass* Z_Construct_UClass_ATrafficVehicleManager();
TRAFFICRUNTIME_API UClass* Z_Construct_UClass_ATrafficVehicleManager_NoRegister();
TRAFFICRUNTIME_API UClass* Z_Construct_UClass_UTrafficNetworkAsset_NoRegister();
UPackage* Z_Construct_UPackage__Script_TrafficRuntime();
// ********** End Cross Module References **********************************************************

// ********** Begin Class ATrafficVehicleManager ***************************************************
FClassRegistrationInfo Z_Registration_Info_UClass_ATrafficVehicleManager;
UClass* ATrafficVehicleManager::GetPrivateStaticClass()
{
	using TClass = ATrafficVehicleManager;
	if (!Z_Registration_Info_UClass_ATrafficVehicleManager.InnerSingleton)
	{
		GetPrivateStaticClassBody(
			TClass::StaticPackage(),
			TEXT("TrafficVehicleManager"),
			Z_Registration_Info_UClass_ATrafficVehicleManager.InnerSingleton,
			StaticRegisterNativesATrafficVehicleManager,
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
	return Z_Registration_Info_UClass_ATrafficVehicleManager.InnerSingleton;
}
UClass* Z_Construct_UClass_ATrafficVehicleManager_NoRegister()
{
	return ATrafficVehicleManager::GetPrivateStaticClass();
}
struct Z_Construct_UClass_ATrafficVehicleManager_Statics
{
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Class_MetaDataParams[] = {
		{ "IncludePath", "TrafficVehicleManager.h" },
		{ "ModuleRelativePath", "Public/TrafficVehicleManager.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_NetworkAsset_MetaData[] = {
		{ "ModuleRelativePath", "Public/TrafficVehicleManager.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_Vehicles_MetaData[] = {
		{ "ModuleRelativePath", "Public/TrafficVehicleManager.h" },
	};
#endif // WITH_METADATA

// ********** Begin Class ATrafficVehicleManager constinit property declarations *******************
	static const UECodeGen_Private::FObjectPropertyParams NewProp_NetworkAsset;
	static const UECodeGen_Private::FObjectPropertyParams NewProp_Vehicles_Inner;
	static const UECodeGen_Private::FArrayPropertyParams NewProp_Vehicles;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
// ********** End Class ATrafficVehicleManager constinit property declarations *********************
	static UObject* (*const DependentSingletons[])();
	static constexpr FCppClassTypeInfoStatic StaticCppClassTypeInfo = {
		TCppClassTypeTraits<ATrafficVehicleManager>::IsAbstract,
	};
	static const UECodeGen_Private::FClassParams ClassParams;
}; // struct Z_Construct_UClass_ATrafficVehicleManager_Statics

// ********** Begin Class ATrafficVehicleManager Property Definitions ******************************
const UECodeGen_Private::FObjectPropertyParams Z_Construct_UClass_ATrafficVehicleManager_Statics::NewProp_NetworkAsset = { "NetworkAsset", nullptr, (EPropertyFlags)0x0040000000000000, UECodeGen_Private::EPropertyGenFlags::Object, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(ATrafficVehicleManager, NetworkAsset), Z_Construct_UClass_UTrafficNetworkAsset_NoRegister, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_NetworkAsset_MetaData), NewProp_NetworkAsset_MetaData) };
const UECodeGen_Private::FObjectPropertyParams Z_Construct_UClass_ATrafficVehicleManager_Statics::NewProp_Vehicles_Inner = { "Vehicles", nullptr, (EPropertyFlags)0x0000000000000000, UECodeGen_Private::EPropertyGenFlags::Object, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, 0, Z_Construct_UClass_ATrafficVehicleBase_NoRegister, METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FArrayPropertyParams Z_Construct_UClass_ATrafficVehicleManager_Statics::NewProp_Vehicles = { "Vehicles", nullptr, (EPropertyFlags)0x0040000000000000, UECodeGen_Private::EPropertyGenFlags::Array, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(ATrafficVehicleManager, Vehicles), EArrayPropertyFlags::None, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_Vehicles_MetaData), NewProp_Vehicles_MetaData) };
const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UClass_ATrafficVehicleManager_Statics::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_ATrafficVehicleManager_Statics::NewProp_NetworkAsset,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_ATrafficVehicleManager_Statics::NewProp_Vehicles_Inner,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_ATrafficVehicleManager_Statics::NewProp_Vehicles,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_ATrafficVehicleManager_Statics::PropPointers) < 2048);
// ********** End Class ATrafficVehicleManager Property Definitions ********************************
UObject* (*const Z_Construct_UClass_ATrafficVehicleManager_Statics::DependentSingletons[])() = {
	(UObject* (*)())Z_Construct_UClass_AActor,
	(UObject* (*)())Z_Construct_UPackage__Script_TrafficRuntime,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_ATrafficVehicleManager_Statics::DependentSingletons) < 16);
const UECodeGen_Private::FClassParams Z_Construct_UClass_ATrafficVehicleManager_Statics::ClassParams = {
	&ATrafficVehicleManager::StaticClass,
	"Engine",
	&StaticCppClassTypeInfo,
	DependentSingletons,
	nullptr,
	Z_Construct_UClass_ATrafficVehicleManager_Statics::PropPointers,
	nullptr,
	UE_ARRAY_COUNT(DependentSingletons),
	0,
	UE_ARRAY_COUNT(Z_Construct_UClass_ATrafficVehicleManager_Statics::PropPointers),
	0,
	0x009000A4u,
	METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UClass_ATrafficVehicleManager_Statics::Class_MetaDataParams), Z_Construct_UClass_ATrafficVehicleManager_Statics::Class_MetaDataParams)
};
void ATrafficVehicleManager::StaticRegisterNativesATrafficVehicleManager()
{
}
UClass* Z_Construct_UClass_ATrafficVehicleManager()
{
	if (!Z_Registration_Info_UClass_ATrafficVehicleManager.OuterSingleton)
	{
		UECodeGen_Private::ConstructUClass(Z_Registration_Info_UClass_ATrafficVehicleManager.OuterSingleton, Z_Construct_UClass_ATrafficVehicleManager_Statics::ClassParams);
	}
	return Z_Registration_Info_UClass_ATrafficVehicleManager.OuterSingleton;
}
DEFINE_VTABLE_PTR_HELPER_CTOR_NS(, ATrafficVehicleManager);
ATrafficVehicleManager::~ATrafficVehicleManager() {}
// ********** End Class ATrafficVehicleManager *****************************************************

// ********** Begin Registration *******************************************************************
struct Z_CompiledInDeferFile_FID_Users_rllax_L_E_O_Plugins_AAAtrafficSystem_Source_TrafficRuntime_Public_TrafficVehicleManager_h__Script_TrafficRuntime_Statics
{
	static constexpr FClassRegisterCompiledInInfo ClassInfo[] = {
		{ Z_Construct_UClass_ATrafficVehicleManager, ATrafficVehicleManager::StaticClass, TEXT("ATrafficVehicleManager"), &Z_Registration_Info_UClass_ATrafficVehicleManager, CONSTRUCT_RELOAD_VERSION_INFO(FClassReloadVersionInfo, sizeof(ATrafficVehicleManager), 2620571195U) },
	};
}; // Z_CompiledInDeferFile_FID_Users_rllax_L_E_O_Plugins_AAAtrafficSystem_Source_TrafficRuntime_Public_TrafficVehicleManager_h__Script_TrafficRuntime_Statics 
static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_rllax_L_E_O_Plugins_AAAtrafficSystem_Source_TrafficRuntime_Public_TrafficVehicleManager_h__Script_TrafficRuntime_3309748177{
	TEXT("/Script/TrafficRuntime"),
	Z_CompiledInDeferFile_FID_Users_rllax_L_E_O_Plugins_AAAtrafficSystem_Source_TrafficRuntime_Public_TrafficVehicleManager_h__Script_TrafficRuntime_Statics::ClassInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Users_rllax_L_E_O_Plugins_AAAtrafficSystem_Source_TrafficRuntime_Public_TrafficVehicleManager_h__Script_TrafficRuntime_Statics::ClassInfo),
	nullptr, 0,
	nullptr, 0,
};
// ********** End Registration *********************************************************************

PRAGMA_ENABLE_DEPRECATION_WARNINGS
