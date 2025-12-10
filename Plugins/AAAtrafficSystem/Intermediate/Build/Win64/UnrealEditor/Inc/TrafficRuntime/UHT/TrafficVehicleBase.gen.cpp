// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "TrafficVehicleBase.h"

PRAGMA_DISABLE_DEPRECATION_WARNINGS
static_assert(!UE_WITH_CONSTINIT_UOBJECT, "This generated code can only be compiled with !UE_WITH_CONSTINIT_OBJECT");
void EmptyLinkFunctionForGeneratedCodeTrafficVehicleBase() {}

// ********** Begin Cross Module References ********************************************************
ENGINE_API UClass* Z_Construct_UClass_AActor();
ENGINE_API UClass* Z_Construct_UClass_UStaticMeshComponent_NoRegister();
TRAFFICRUNTIME_API UClass* Z_Construct_UClass_ATrafficVehicleBase();
TRAFFICRUNTIME_API UClass* Z_Construct_UClass_ATrafficVehicleBase_NoRegister();
TRAFFICRUNTIME_API UClass* Z_Construct_UClass_UTrafficKinematicFollower_NoRegister();
UPackage* Z_Construct_UPackage__Script_TrafficRuntime();
// ********** End Cross Module References **********************************************************

// ********** Begin Class ATrafficVehicleBase ******************************************************
FClassRegistrationInfo Z_Registration_Info_UClass_ATrafficVehicleBase;
UClass* ATrafficVehicleBase::GetPrivateStaticClass()
{
	using TClass = ATrafficVehicleBase;
	if (!Z_Registration_Info_UClass_ATrafficVehicleBase.InnerSingleton)
	{
		GetPrivateStaticClassBody(
			TClass::StaticPackage(),
			TEXT("TrafficVehicleBase"),
			Z_Registration_Info_UClass_ATrafficVehicleBase.InnerSingleton,
			StaticRegisterNativesATrafficVehicleBase,
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
	return Z_Registration_Info_UClass_ATrafficVehicleBase.InnerSingleton;
}
UClass* Z_Construct_UClass_ATrafficVehicleBase_NoRegister()
{
	return ATrafficVehicleBase::GetPrivateStaticClass();
}
struct Z_Construct_UClass_ATrafficVehicleBase_Statics
{
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Class_MetaDataParams[] = {
		{ "IncludePath", "TrafficVehicleBase.h" },
		{ "ModuleRelativePath", "Public/TrafficVehicleBase.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_Body_MetaData[] = {
		{ "Category", "TrafficVehicleBase" },
		{ "EditInline", "true" },
		{ "ModuleRelativePath", "Public/TrafficVehicleBase.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_Follower_MetaData[] = {
		{ "ModuleRelativePath", "Public/TrafficVehicleBase.h" },
	};
#endif // WITH_METADATA

// ********** Begin Class ATrafficVehicleBase constinit property declarations **********************
	static const UECodeGen_Private::FObjectPropertyParams NewProp_Body;
	static const UECodeGen_Private::FObjectPropertyParams NewProp_Follower;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
// ********** End Class ATrafficVehicleBase constinit property declarations ************************
	static UObject* (*const DependentSingletons[])();
	static constexpr FCppClassTypeInfoStatic StaticCppClassTypeInfo = {
		TCppClassTypeTraits<ATrafficVehicleBase>::IsAbstract,
	};
	static const UECodeGen_Private::FClassParams ClassParams;
}; // struct Z_Construct_UClass_ATrafficVehicleBase_Statics

// ********** Begin Class ATrafficVehicleBase Property Definitions *********************************
const UECodeGen_Private::FObjectPropertyParams Z_Construct_UClass_ATrafficVehicleBase_Statics::NewProp_Body = { "Body", nullptr, (EPropertyFlags)0x00200800000a0009, UECodeGen_Private::EPropertyGenFlags::Object, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(ATrafficVehicleBase, Body), Z_Construct_UClass_UStaticMeshComponent_NoRegister, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_Body_MetaData), NewProp_Body_MetaData) };
const UECodeGen_Private::FObjectPropertyParams Z_Construct_UClass_ATrafficVehicleBase_Statics::NewProp_Follower = { "Follower", nullptr, (EPropertyFlags)0x0020080000000000, UECodeGen_Private::EPropertyGenFlags::Object, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(ATrafficVehicleBase, Follower), Z_Construct_UClass_UTrafficKinematicFollower_NoRegister, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_Follower_MetaData), NewProp_Follower_MetaData) };
const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UClass_ATrafficVehicleBase_Statics::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_ATrafficVehicleBase_Statics::NewProp_Body,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_ATrafficVehicleBase_Statics::NewProp_Follower,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_ATrafficVehicleBase_Statics::PropPointers) < 2048);
// ********** End Class ATrafficVehicleBase Property Definitions ***********************************
UObject* (*const Z_Construct_UClass_ATrafficVehicleBase_Statics::DependentSingletons[])() = {
	(UObject* (*)())Z_Construct_UClass_AActor,
	(UObject* (*)())Z_Construct_UPackage__Script_TrafficRuntime,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_ATrafficVehicleBase_Statics::DependentSingletons) < 16);
const UECodeGen_Private::FClassParams Z_Construct_UClass_ATrafficVehicleBase_Statics::ClassParams = {
	&ATrafficVehicleBase::StaticClass,
	"Engine",
	&StaticCppClassTypeInfo,
	DependentSingletons,
	nullptr,
	Z_Construct_UClass_ATrafficVehicleBase_Statics::PropPointers,
	nullptr,
	UE_ARRAY_COUNT(DependentSingletons),
	0,
	UE_ARRAY_COUNT(Z_Construct_UClass_ATrafficVehicleBase_Statics::PropPointers),
	0,
	0x009000A4u,
	METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UClass_ATrafficVehicleBase_Statics::Class_MetaDataParams), Z_Construct_UClass_ATrafficVehicleBase_Statics::Class_MetaDataParams)
};
void ATrafficVehicleBase::StaticRegisterNativesATrafficVehicleBase()
{
}
UClass* Z_Construct_UClass_ATrafficVehicleBase()
{
	if (!Z_Registration_Info_UClass_ATrafficVehicleBase.OuterSingleton)
	{
		UECodeGen_Private::ConstructUClass(Z_Registration_Info_UClass_ATrafficVehicleBase.OuterSingleton, Z_Construct_UClass_ATrafficVehicleBase_Statics::ClassParams);
	}
	return Z_Registration_Info_UClass_ATrafficVehicleBase.OuterSingleton;
}
DEFINE_VTABLE_PTR_HELPER_CTOR_NS(, ATrafficVehicleBase);
ATrafficVehicleBase::~ATrafficVehicleBase() {}
// ********** End Class ATrafficVehicleBase ********************************************************

// ********** Begin Registration *******************************************************************
struct Z_CompiledInDeferFile_FID_Users_rllax_L_E_O_Plugins_AAAtrafficSystem_Source_TrafficRuntime_Public_TrafficVehicleBase_h__Script_TrafficRuntime_Statics
{
	static constexpr FClassRegisterCompiledInInfo ClassInfo[] = {
		{ Z_Construct_UClass_ATrafficVehicleBase, ATrafficVehicleBase::StaticClass, TEXT("ATrafficVehicleBase"), &Z_Registration_Info_UClass_ATrafficVehicleBase, CONSTRUCT_RELOAD_VERSION_INFO(FClassReloadVersionInfo, sizeof(ATrafficVehicleBase), 3942687223U) },
	};
}; // Z_CompiledInDeferFile_FID_Users_rllax_L_E_O_Plugins_AAAtrafficSystem_Source_TrafficRuntime_Public_TrafficVehicleBase_h__Script_TrafficRuntime_Statics 
static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_rllax_L_E_O_Plugins_AAAtrafficSystem_Source_TrafficRuntime_Public_TrafficVehicleBase_h__Script_TrafficRuntime_1089504080{
	TEXT("/Script/TrafficRuntime"),
	Z_CompiledInDeferFile_FID_Users_rllax_L_E_O_Plugins_AAAtrafficSystem_Source_TrafficRuntime_Public_TrafficVehicleBase_h__Script_TrafficRuntime_Statics::ClassInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Users_rllax_L_E_O_Plugins_AAAtrafficSystem_Source_TrafficRuntime_Public_TrafficVehicleBase_h__Script_TrafficRuntime_Statics::ClassInfo),
	nullptr, 0,
	nullptr, 0,
};
// ********** End Registration *********************************************************************

PRAGMA_ENABLE_DEPRECATION_WARNINGS
