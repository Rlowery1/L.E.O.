// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "TrafficGeometryProviderFactory.h"

PRAGMA_DISABLE_DEPRECATION_WARNINGS
static_assert(!UE_WITH_CONSTINIT_UOBJECT, "This generated code can only be compiled with !UE_WITH_CONSTINIT_OBJECT");
void EmptyLinkFunctionForGeneratedCodeTrafficGeometryProviderFactory() {}

// ********** Begin Cross Module References ********************************************************
COREUOBJECT_API UClass* Z_Construct_UClass_UObject();
TRAFFICRUNTIME_API UClass* Z_Construct_UClass_UTrafficGeometryProviderFactory();
TRAFFICRUNTIME_API UClass* Z_Construct_UClass_UTrafficGeometryProviderFactory_NoRegister();
UPackage* Z_Construct_UPackage__Script_TrafficRuntime();
// ********** End Cross Module References **********************************************************

// ********** Begin Class UTrafficGeometryProviderFactory ******************************************
FClassRegistrationInfo Z_Registration_Info_UClass_UTrafficGeometryProviderFactory;
UClass* UTrafficGeometryProviderFactory::GetPrivateStaticClass()
{
	using TClass = UTrafficGeometryProviderFactory;
	if (!Z_Registration_Info_UClass_UTrafficGeometryProviderFactory.InnerSingleton)
	{
		GetPrivateStaticClassBody(
			TClass::StaticPackage(),
			TEXT("TrafficGeometryProviderFactory"),
			Z_Registration_Info_UClass_UTrafficGeometryProviderFactory.InnerSingleton,
			StaticRegisterNativesUTrafficGeometryProviderFactory,
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
	return Z_Registration_Info_UClass_UTrafficGeometryProviderFactory.InnerSingleton;
}
UClass* Z_Construct_UClass_UTrafficGeometryProviderFactory_NoRegister()
{
	return UTrafficGeometryProviderFactory::GetPrivateStaticClass();
}
struct Z_Construct_UClass_UTrafficGeometryProviderFactory_Statics
{
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Class_MetaDataParams[] = {
		{ "IncludePath", "TrafficGeometryProviderFactory.h" },
		{ "ModuleRelativePath", "Public/TrafficGeometryProviderFactory.h" },
	};
#endif // WITH_METADATA

// ********** Begin Class UTrafficGeometryProviderFactory constinit property declarations **********
// ********** End Class UTrafficGeometryProviderFactory constinit property declarations ************
	static UObject* (*const DependentSingletons[])();
	static constexpr FCppClassTypeInfoStatic StaticCppClassTypeInfo = {
		TCppClassTypeTraits<UTrafficGeometryProviderFactory>::IsAbstract,
	};
	static const UECodeGen_Private::FClassParams ClassParams;
}; // struct Z_Construct_UClass_UTrafficGeometryProviderFactory_Statics
UObject* (*const Z_Construct_UClass_UTrafficGeometryProviderFactory_Statics::DependentSingletons[])() = {
	(UObject* (*)())Z_Construct_UClass_UObject,
	(UObject* (*)())Z_Construct_UPackage__Script_TrafficRuntime,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_UTrafficGeometryProviderFactory_Statics::DependentSingletons) < 16);
const UECodeGen_Private::FClassParams Z_Construct_UClass_UTrafficGeometryProviderFactory_Statics::ClassParams = {
	&UTrafficGeometryProviderFactory::StaticClass,
	nullptr,
	&StaticCppClassTypeInfo,
	DependentSingletons,
	nullptr,
	nullptr,
	nullptr,
	UE_ARRAY_COUNT(DependentSingletons),
	0,
	0,
	0,
	0x001000A0u,
	METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UClass_UTrafficGeometryProviderFactory_Statics::Class_MetaDataParams), Z_Construct_UClass_UTrafficGeometryProviderFactory_Statics::Class_MetaDataParams)
};
void UTrafficGeometryProviderFactory::StaticRegisterNativesUTrafficGeometryProviderFactory()
{
}
UClass* Z_Construct_UClass_UTrafficGeometryProviderFactory()
{
	if (!Z_Registration_Info_UClass_UTrafficGeometryProviderFactory.OuterSingleton)
	{
		UECodeGen_Private::ConstructUClass(Z_Registration_Info_UClass_UTrafficGeometryProviderFactory.OuterSingleton, Z_Construct_UClass_UTrafficGeometryProviderFactory_Statics::ClassParams);
	}
	return Z_Registration_Info_UClass_UTrafficGeometryProviderFactory.OuterSingleton;
}
UTrafficGeometryProviderFactory::UTrafficGeometryProviderFactory(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
DEFINE_VTABLE_PTR_HELPER_CTOR_NS(, UTrafficGeometryProviderFactory);
UTrafficGeometryProviderFactory::~UTrafficGeometryProviderFactory() {}
// ********** End Class UTrafficGeometryProviderFactory ********************************************

// ********** Begin Registration *******************************************************************
struct Z_CompiledInDeferFile_FID_Users_rllax_L_E_O_Plugins_AAAtrafficSystem_Source_TrafficRuntime_Public_TrafficGeometryProviderFactory_h__Script_TrafficRuntime_Statics
{
	static constexpr FClassRegisterCompiledInInfo ClassInfo[] = {
		{ Z_Construct_UClass_UTrafficGeometryProviderFactory, UTrafficGeometryProviderFactory::StaticClass, TEXT("UTrafficGeometryProviderFactory"), &Z_Registration_Info_UClass_UTrafficGeometryProviderFactory, CONSTRUCT_RELOAD_VERSION_INFO(FClassReloadVersionInfo, sizeof(UTrafficGeometryProviderFactory), 977142724U) },
	};
}; // Z_CompiledInDeferFile_FID_Users_rllax_L_E_O_Plugins_AAAtrafficSystem_Source_TrafficRuntime_Public_TrafficGeometryProviderFactory_h__Script_TrafficRuntime_Statics 
static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_rllax_L_E_O_Plugins_AAAtrafficSystem_Source_TrafficRuntime_Public_TrafficGeometryProviderFactory_h__Script_TrafficRuntime_3050016656{
	TEXT("/Script/TrafficRuntime"),
	Z_CompiledInDeferFile_FID_Users_rllax_L_E_O_Plugins_AAAtrafficSystem_Source_TrafficRuntime_Public_TrafficGeometryProviderFactory_h__Script_TrafficRuntime_Statics::ClassInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Users_rllax_L_E_O_Plugins_AAAtrafficSystem_Source_TrafficRuntime_Public_TrafficGeometryProviderFactory_h__Script_TrafficRuntime_Statics::ClassInfo),
	nullptr, 0,
	nullptr, 0,
};
// ********** End Registration *********************************************************************

PRAGMA_ENABLE_DEPRECATION_WARNINGS
