// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "CityBLDRoadGeometryProvider.h"

PRAGMA_DISABLE_DEPRECATION_WARNINGS
static_assert(!UE_WITH_CONSTINIT_UOBJECT, "This generated code can only be compiled with !UE_WITH_CONSTINIT_OBJECT");
void EmptyLinkFunctionForGeneratedCodeCityBLDRoadGeometryProvider() {}

// ********** Begin Cross Module References ********************************************************
COREUOBJECT_API UClass* Z_Construct_UClass_UObject();
TRAFFICRUNTIME_API UClass* Z_Construct_UClass_UCityBLDRoadGeometryProvider();
TRAFFICRUNTIME_API UClass* Z_Construct_UClass_UCityBLDRoadGeometryProvider_NoRegister();
UPackage* Z_Construct_UPackage__Script_TrafficRuntime();
// ********** End Cross Module References **********************************************************

// ********** Begin Class UCityBLDRoadGeometryProvider *********************************************
FClassRegistrationInfo Z_Registration_Info_UClass_UCityBLDRoadGeometryProvider;
UClass* UCityBLDRoadGeometryProvider::GetPrivateStaticClass()
{
	using TClass = UCityBLDRoadGeometryProvider;
	if (!Z_Registration_Info_UClass_UCityBLDRoadGeometryProvider.InnerSingleton)
	{
		GetPrivateStaticClassBody(
			TClass::StaticPackage(),
			TEXT("CityBLDRoadGeometryProvider"),
			Z_Registration_Info_UClass_UCityBLDRoadGeometryProvider.InnerSingleton,
			StaticRegisterNativesUCityBLDRoadGeometryProvider,
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
	return Z_Registration_Info_UClass_UCityBLDRoadGeometryProvider.InnerSingleton;
}
UClass* Z_Construct_UClass_UCityBLDRoadGeometryProvider_NoRegister()
{
	return UCityBLDRoadGeometryProvider::GetPrivateStaticClass();
}
struct Z_Construct_UClass_UCityBLDRoadGeometryProvider_Statics
{
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Class_MetaDataParams[] = {
		{ "IncludePath", "CityBLDRoadGeometryProvider.h" },
		{ "ModuleRelativePath", "Public/CityBLDRoadGeometryProvider.h" },
	};
#endif // WITH_METADATA

// ********** Begin Class UCityBLDRoadGeometryProvider constinit property declarations *************
// ********** End Class UCityBLDRoadGeometryProvider constinit property declarations ***************
	static UObject* (*const DependentSingletons[])();
	static constexpr FCppClassTypeInfoStatic StaticCppClassTypeInfo = {
		TCppClassTypeTraits<UCityBLDRoadGeometryProvider>::IsAbstract,
	};
	static const UECodeGen_Private::FClassParams ClassParams;
}; // struct Z_Construct_UClass_UCityBLDRoadGeometryProvider_Statics
UObject* (*const Z_Construct_UClass_UCityBLDRoadGeometryProvider_Statics::DependentSingletons[])() = {
	(UObject* (*)())Z_Construct_UClass_UObject,
	(UObject* (*)())Z_Construct_UPackage__Script_TrafficRuntime,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_UCityBLDRoadGeometryProvider_Statics::DependentSingletons) < 16);
const UECodeGen_Private::FClassParams Z_Construct_UClass_UCityBLDRoadGeometryProvider_Statics::ClassParams = {
	&UCityBLDRoadGeometryProvider::StaticClass,
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
	METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UClass_UCityBLDRoadGeometryProvider_Statics::Class_MetaDataParams), Z_Construct_UClass_UCityBLDRoadGeometryProvider_Statics::Class_MetaDataParams)
};
void UCityBLDRoadGeometryProvider::StaticRegisterNativesUCityBLDRoadGeometryProvider()
{
}
UClass* Z_Construct_UClass_UCityBLDRoadGeometryProvider()
{
	if (!Z_Registration_Info_UClass_UCityBLDRoadGeometryProvider.OuterSingleton)
	{
		UECodeGen_Private::ConstructUClass(Z_Registration_Info_UClass_UCityBLDRoadGeometryProvider.OuterSingleton, Z_Construct_UClass_UCityBLDRoadGeometryProvider_Statics::ClassParams);
	}
	return Z_Registration_Info_UClass_UCityBLDRoadGeometryProvider.OuterSingleton;
}
UCityBLDRoadGeometryProvider::UCityBLDRoadGeometryProvider(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
DEFINE_VTABLE_PTR_HELPER_CTOR_NS(, UCityBLDRoadGeometryProvider);
UCityBLDRoadGeometryProvider::~UCityBLDRoadGeometryProvider() {}
// ********** End Class UCityBLDRoadGeometryProvider ***********************************************

// ********** Begin Registration *******************************************************************
struct Z_CompiledInDeferFile_FID_Users_rllax_L_E_O_Plugins_AAAtrafficSystem_Source_TrafficRuntime_Public_CityBLDRoadGeometryProvider_h__Script_TrafficRuntime_Statics
{
	static constexpr FClassRegisterCompiledInInfo ClassInfo[] = {
		{ Z_Construct_UClass_UCityBLDRoadGeometryProvider, UCityBLDRoadGeometryProvider::StaticClass, TEXT("UCityBLDRoadGeometryProvider"), &Z_Registration_Info_UClass_UCityBLDRoadGeometryProvider, CONSTRUCT_RELOAD_VERSION_INFO(FClassReloadVersionInfo, sizeof(UCityBLDRoadGeometryProvider), 354597245U) },
	};
}; // Z_CompiledInDeferFile_FID_Users_rllax_L_E_O_Plugins_AAAtrafficSystem_Source_TrafficRuntime_Public_CityBLDRoadGeometryProvider_h__Script_TrafficRuntime_Statics 
static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_rllax_L_E_O_Plugins_AAAtrafficSystem_Source_TrafficRuntime_Public_CityBLDRoadGeometryProvider_h__Script_TrafficRuntime_4291890777{
	TEXT("/Script/TrafficRuntime"),
	Z_CompiledInDeferFile_FID_Users_rllax_L_E_O_Plugins_AAAtrafficSystem_Source_TrafficRuntime_Public_CityBLDRoadGeometryProvider_h__Script_TrafficRuntime_Statics::ClassInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Users_rllax_L_E_O_Plugins_AAAtrafficSystem_Source_TrafficRuntime_Public_CityBLDRoadGeometryProvider_h__Script_TrafficRuntime_Statics::ClassInfo),
	nullptr, 0,
	nullptr, 0,
};
// ********** End Registration *********************************************************************

PRAGMA_ENABLE_DEPRECATION_WARNINGS
