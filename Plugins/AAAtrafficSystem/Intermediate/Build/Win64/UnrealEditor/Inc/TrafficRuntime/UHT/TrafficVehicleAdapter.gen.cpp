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
ENGINE_API UClass* Z_Construct_UClass_AActor();
ENGINE_API UClass* Z_Construct_UClass_APawn_NoRegister();
TRAFFICRUNTIME_API UClass* Z_Construct_UClass_ATrafficVehicleAdapter();
TRAFFICRUNTIME_API UClass* Z_Construct_UClass_ATrafficVehicleAdapter_NoRegister();
TRAFFICRUNTIME_API UClass* Z_Construct_UClass_ATrafficVehicleBase_NoRegister();
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
		{ "Comment", "/**\n * Adapter that binds a logic vehicle (ATrafficVehicleBase) to a visual/Chaos pawn.\n * For now, the adapter simply teleports the visual to the logic transform each tick.\n */" },
#endif
		{ "IncludePath", "TrafficVehicleAdapter.h" },
		{ "ModuleRelativePath", "Public/TrafficVehicleAdapter.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Adapter that binds a logic vehicle (ATrafficVehicleBase) to a visual/Chaos pawn.\nFor now, the adapter simply teleports the visual to the logic transform each tick." },
#endif
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_LogicVehicle_MetaData[] = {
		{ "Category", "TrafficVehicle" },
		{ "ModuleRelativePath", "Public/TrafficVehicleAdapter.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_ChaosVehicle_MetaData[] = {
		{ "Category", "TrafficVehicle" },
		{ "ModuleRelativePath", "Public/TrafficVehicleAdapter.h" },
	};
#endif // WITH_METADATA

// ********** Begin Class ATrafficVehicleAdapter constinit property declarations *******************
	static const UECodeGen_Private::FWeakObjectPropertyParams NewProp_LogicVehicle;
	static const UECodeGen_Private::FWeakObjectPropertyParams NewProp_ChaosVehicle;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
// ********** End Class ATrafficVehicleAdapter constinit property declarations *********************
	static UObject* (*const DependentSingletons[])();
	static constexpr FCppClassTypeInfoStatic StaticCppClassTypeInfo = {
		TCppClassTypeTraits<ATrafficVehicleAdapter>::IsAbstract,
	};
	static const UECodeGen_Private::FClassParams ClassParams;
}; // struct Z_Construct_UClass_ATrafficVehicleAdapter_Statics

// ********** Begin Class ATrafficVehicleAdapter Property Definitions ******************************
const UECodeGen_Private::FWeakObjectPropertyParams Z_Construct_UClass_ATrafficVehicleAdapter_Statics::NewProp_LogicVehicle = { "LogicVehicle", nullptr, (EPropertyFlags)0x0014000000020015, UECodeGen_Private::EPropertyGenFlags::WeakObject, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(ATrafficVehicleAdapter, LogicVehicle), Z_Construct_UClass_ATrafficVehicleBase_NoRegister, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_LogicVehicle_MetaData), NewProp_LogicVehicle_MetaData) };
const UECodeGen_Private::FWeakObjectPropertyParams Z_Construct_UClass_ATrafficVehicleAdapter_Statics::NewProp_ChaosVehicle = { "ChaosVehicle", nullptr, (EPropertyFlags)0x0014000000020015, UECodeGen_Private::EPropertyGenFlags::WeakObject, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(ATrafficVehicleAdapter, ChaosVehicle), Z_Construct_UClass_APawn_NoRegister, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_ChaosVehicle_MetaData), NewProp_ChaosVehicle_MetaData) };
const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UClass_ATrafficVehicleAdapter_Statics::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_ATrafficVehicleAdapter_Statics::NewProp_LogicVehicle,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_ATrafficVehicleAdapter_Statics::NewProp_ChaosVehicle,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_ATrafficVehicleAdapter_Statics::PropPointers) < 2048);
// ********** End Class ATrafficVehicleAdapter Property Definitions ********************************
UObject* (*const Z_Construct_UClass_ATrafficVehicleAdapter_Statics::DependentSingletons[])() = {
	(UObject* (*)())Z_Construct_UClass_AActor,
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
		{ Z_Construct_UClass_ATrafficVehicleAdapter, ATrafficVehicleAdapter::StaticClass, TEXT("ATrafficVehicleAdapter"), &Z_Registration_Info_UClass_ATrafficVehicleAdapter, CONSTRUCT_RELOAD_VERSION_INFO(FClassReloadVersionInfo, sizeof(ATrafficVehicleAdapter), 2714958072U) },
	};
}; // Z_CompiledInDeferFile_FID_Users_rllax_L_E_O_Plugins_AAAtrafficSystem_Source_TrafficRuntime_Public_TrafficVehicleAdapter_h__Script_TrafficRuntime_Statics 
static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_rllax_L_E_O_Plugins_AAAtrafficSystem_Source_TrafficRuntime_Public_TrafficVehicleAdapter_h__Script_TrafficRuntime_3244143516{
	TEXT("/Script/TrafficRuntime"),
	Z_CompiledInDeferFile_FID_Users_rllax_L_E_O_Plugins_AAAtrafficSystem_Source_TrafficRuntime_Public_TrafficVehicleAdapter_h__Script_TrafficRuntime_Statics::ClassInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Users_rllax_L_E_O_Plugins_AAAtrafficSystem_Source_TrafficRuntime_Public_TrafficVehicleAdapter_h__Script_TrafficRuntime_Statics::ClassInfo),
	nullptr, 0,
	nullptr, 0,
};
// ********** End Registration *********************************************************************

PRAGMA_ENABLE_DEPRECATION_WARNINGS
