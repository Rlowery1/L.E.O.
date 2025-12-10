// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "TrafficVehicleAdapter.h"

PRAGMA_DISABLE_DEPRECATION_WARNINGS
static_assert(!UE_WITH_CONSTINIT_UOBJECT, "This generated code can only be compiled with !UE_WITH_CONSTINIT_OBJECT");
void EmptyLinkFunctionForGeneratedCodeTrafficVehicleAdapter() {}

// ********** Begin Cross Module References ********************************************************
COREUOBJECT_API UClass* Z_Construct_UClass_UClass_NoRegister();
ENGINE_API UClass* Z_Construct_UClass_AActor_NoRegister();
TRAFFICRUNTIME_API UClass* Z_Construct_UClass_ATrafficVehicleAdapter();
TRAFFICRUNTIME_API UClass* Z_Construct_UClass_ATrafficVehicleAdapter_NoRegister();
TRAFFICRUNTIME_API UClass* Z_Construct_UClass_ATrafficVehicleBase();
UPackage* Z_Construct_UPackage__Script_TrafficRuntime();
// ********** End Cross Module References **********************************************************

// ********** Begin Class ATrafficVehicleAdapter ***************************************************
FClassRegistrationInfo Z_Registration_Info_UClass_ATrafficVehicleAdapter;
UClass* ATrafficVehicleAdapter::GetPrivateStaticClass()
{
	using TClass = ATrafficVehicleAdapter;
	if (!Z_Registration_Info_UClass_ATrafficVehicleAdapter.InnerSingleton)
	{
		GetPrivateStaticClassBody(
			TClass::StaticPackage(),
			TEXT("TrafficVehicleAdapter"),
			Z_Registration_Info_UClass_ATrafficVehicleAdapter.InnerSingleton,
			StaticRegisterNativesATrafficVehicleAdapter,
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
	return Z_Registration_Info_UClass_ATrafficVehicleAdapter.InnerSingleton;
}
UClass* Z_Construct_UClass_ATrafficVehicleAdapter_NoRegister()
{
	return ATrafficVehicleAdapter::GetPrivateStaticClass();
}
struct Z_Construct_UClass_ATrafficVehicleAdapter_Statics
{
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Class_MetaDataParams[] = {
#if !UE_BUILD_SHIPPING
		{ "Comment", "/**\n * Adapter vehicle that drives with the AAA kinematic follower but renders any user-supplied vehicle Blueprint/Class.\n * The spawned visual is attached and collision-disabled so it does not fight the kinematic pathing.\n */" },
#endif
		{ "IncludePath", "TrafficVehicleAdapter.h" },
		{ "ModuleRelativePath", "Public/TrafficVehicleAdapter.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Adapter vehicle that drives with the AAA kinematic follower but renders any user-supplied vehicle Blueprint/Class.\nThe spawned visual is attached and collision-disabled so it does not fight the kinematic pathing." },
#endif
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_ExternalVehicleClass_MetaData[] = {
		{ "Category", "Vehicle" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** Optional external vehicle class to spawn and attach for visuals (can be any Actor, e.g. Chaos vehicle BP). */" },
#endif
		{ "ModuleRelativePath", "Public/TrafficVehicleAdapter.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Optional external vehicle class to spawn and attach for visuals (can be any Actor, e.g. Chaos vehicle BP)." },
#endif
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_SpawnedVisual_MetaData[] = {
		{ "ModuleRelativePath", "Public/TrafficVehicleAdapter.h" },
	};
#endif // WITH_METADATA

// ********** Begin Class ATrafficVehicleAdapter constinit property declarations *******************
	static const UECodeGen_Private::FSoftClassPropertyParams NewProp_ExternalVehicleClass;
	static const UECodeGen_Private::FObjectPropertyParams NewProp_SpawnedVisual;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
// ********** End Class ATrafficVehicleAdapter constinit property declarations *********************
	static UObject* (*const DependentSingletons[])();
	static constexpr FCppClassTypeInfoStatic StaticCppClassTypeInfo = {
		TCppClassTypeTraits<ATrafficVehicleAdapter>::IsAbstract,
	};
	static const UECodeGen_Private::FClassParams ClassParams;
}; // struct Z_Construct_UClass_ATrafficVehicleAdapter_Statics

// ********** Begin Class ATrafficVehicleAdapter Property Definitions ******************************
const UECodeGen_Private::FSoftClassPropertyParams Z_Construct_UClass_ATrafficVehicleAdapter_Statics::NewProp_ExternalVehicleClass = { "ExternalVehicleClass", nullptr, (EPropertyFlags)0x0014000000000001, UECodeGen_Private::EPropertyGenFlags::SoftClass, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(ATrafficVehicleAdapter, ExternalVehicleClass), Z_Construct_UClass_AActor_NoRegister, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_ExternalVehicleClass_MetaData), NewProp_ExternalVehicleClass_MetaData) };
const UECodeGen_Private::FObjectPropertyParams Z_Construct_UClass_ATrafficVehicleAdapter_Statics::NewProp_SpawnedVisual = { "SpawnedVisual", nullptr, (EPropertyFlags)0x0040000000000000, UECodeGen_Private::EPropertyGenFlags::Object, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(ATrafficVehicleAdapter, SpawnedVisual), Z_Construct_UClass_AActor_NoRegister, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_SpawnedVisual_MetaData), NewProp_SpawnedVisual_MetaData) };
const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UClass_ATrafficVehicleAdapter_Statics::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_ATrafficVehicleAdapter_Statics::NewProp_ExternalVehicleClass,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_ATrafficVehicleAdapter_Statics::NewProp_SpawnedVisual,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_ATrafficVehicleAdapter_Statics::PropPointers) < 2048);
// ********** End Class ATrafficVehicleAdapter Property Definitions ********************************
UObject* (*const Z_Construct_UClass_ATrafficVehicleAdapter_Statics::DependentSingletons[])() = {
	(UObject* (*)())Z_Construct_UClass_ATrafficVehicleBase,
	(UObject* (*)())Z_Construct_UPackage__Script_TrafficRuntime,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_ATrafficVehicleAdapter_Statics::DependentSingletons) < 16);
const UECodeGen_Private::FClassParams Z_Construct_UClass_ATrafficVehicleAdapter_Statics::ClassParams = {
	&ATrafficVehicleAdapter::StaticClass,
	"Engine",
	&StaticCppClassTypeInfo,
	DependentSingletons,
	nullptr,
	Z_Construct_UClass_ATrafficVehicleAdapter_Statics::PropPointers,
	nullptr,
	UE_ARRAY_COUNT(DependentSingletons),
	0,
	UE_ARRAY_COUNT(Z_Construct_UClass_ATrafficVehicleAdapter_Statics::PropPointers),
	0,
	0x009000A4u,
	METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UClass_ATrafficVehicleAdapter_Statics::Class_MetaDataParams), Z_Construct_UClass_ATrafficVehicleAdapter_Statics::Class_MetaDataParams)
};
void ATrafficVehicleAdapter::StaticRegisterNativesATrafficVehicleAdapter()
{
}
UClass* Z_Construct_UClass_ATrafficVehicleAdapter()
{
	if (!Z_Registration_Info_UClass_ATrafficVehicleAdapter.OuterSingleton)
	{
		UECodeGen_Private::ConstructUClass(Z_Registration_Info_UClass_ATrafficVehicleAdapter.OuterSingleton, Z_Construct_UClass_ATrafficVehicleAdapter_Statics::ClassParams);
	}
	return Z_Registration_Info_UClass_ATrafficVehicleAdapter.OuterSingleton;
}
DEFINE_VTABLE_PTR_HELPER_CTOR_NS(, ATrafficVehicleAdapter);
ATrafficVehicleAdapter::~ATrafficVehicleAdapter() {}
// ********** End Class ATrafficVehicleAdapter *****************************************************

// ********** Begin Registration *******************************************************************
struct Z_CompiledInDeferFile_FID_Users_rllax_L_E_O_Plugins_AAAtrafficSystem_Source_TrafficRuntime_Public_TrafficVehicleAdapter_h__Script_TrafficRuntime_Statics
{
	static constexpr FClassRegisterCompiledInInfo ClassInfo[] = {
		{ Z_Construct_UClass_ATrafficVehicleAdapter, ATrafficVehicleAdapter::StaticClass, TEXT("ATrafficVehicleAdapter"), &Z_Registration_Info_UClass_ATrafficVehicleAdapter, CONSTRUCT_RELOAD_VERSION_INFO(FClassReloadVersionInfo, sizeof(ATrafficVehicleAdapter), 946681702U) },
	};
}; // Z_CompiledInDeferFile_FID_Users_rllax_L_E_O_Plugins_AAAtrafficSystem_Source_TrafficRuntime_Public_TrafficVehicleAdapter_h__Script_TrafficRuntime_Statics 
static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_rllax_L_E_O_Plugins_AAAtrafficSystem_Source_TrafficRuntime_Public_TrafficVehicleAdapter_h__Script_TrafficRuntime_1080529452{
	TEXT("/Script/TrafficRuntime"),
	Z_CompiledInDeferFile_FID_Users_rllax_L_E_O_Plugins_AAAtrafficSystem_Source_TrafficRuntime_Public_TrafficVehicleAdapter_h__Script_TrafficRuntime_Statics::ClassInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Users_rllax_L_E_O_Plugins_AAAtrafficSystem_Source_TrafficRuntime_Public_TrafficVehicleAdapter_h__Script_TrafficRuntime_Statics::ClassInfo),
	nullptr, 0,
	nullptr, 0,
};
// ********** End Registration *********************************************************************

PRAGMA_ENABLE_DEPRECATION_WARNINGS
