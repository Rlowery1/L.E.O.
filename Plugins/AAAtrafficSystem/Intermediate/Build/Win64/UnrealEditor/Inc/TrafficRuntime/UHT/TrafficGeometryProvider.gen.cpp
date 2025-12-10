// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "TrafficGeometryProvider.h"

PRAGMA_DISABLE_DEPRECATION_WARNINGS
static_assert(!UE_WITH_CONSTINIT_UOBJECT, "This generated code can only be compiled with !UE_WITH_CONSTINIT_OBJECT");
void EmptyLinkFunctionForGeneratedCodeTrafficGeometryProvider() {}

// ********** Begin Cross Module References ********************************************************
COREUOBJECT_API UClass* Z_Construct_UClass_UObject();
TRAFFICRUNTIME_API UClass* Z_Construct_UClass_UGenericSplineRoadGeometryProvider();
TRAFFICRUNTIME_API UClass* Z_Construct_UClass_UGenericSplineRoadGeometryProvider_NoRegister();
UPackage* Z_Construct_UPackage__Script_TrafficRuntime();
// ********** End Cross Module References **********************************************************

// ********** Begin Class UGenericSplineRoadGeometryProvider ***************************************
FClassRegistrationInfo Z_Registration_Info_UClass_UGenericSplineRoadGeometryProvider;
UClass* UGenericSplineRoadGeometryProvider::GetPrivateStaticClass()
{
	using TClass = UGenericSplineRoadGeometryProvider;
	if (!Z_Registration_Info_UClass_UGenericSplineRoadGeometryProvider.InnerSingleton)
	{
		GetPrivateStaticClassBody(
			TClass::StaticPackage(),
			TEXT("GenericSplineRoadGeometryProvider"),
			Z_Registration_Info_UClass_UGenericSplineRoadGeometryProvider.InnerSingleton,
			StaticRegisterNativesUGenericSplineRoadGeometryProvider,
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
	return Z_Registration_Info_UClass_UGenericSplineRoadGeometryProvider.InnerSingleton;
}
UClass* Z_Construct_UClass_UGenericSplineRoadGeometryProvider_NoRegister()
{
	return UGenericSplineRoadGeometryProvider::GetPrivateStaticClass();
}
struct Z_Construct_UClass_UGenericSplineRoadGeometryProvider_Statics
{
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Class_MetaDataParams[] = {
		{ "BlueprintType", "true" },
		{ "IncludePath", "TrafficGeometryProvider.h" },
		{ "ModuleRelativePath", "Public/TrafficGeometryProvider.h" },
	};
#endif // WITH_METADATA

// ********** Begin Class UGenericSplineRoadGeometryProvider constinit property declarations *******
// ********** End Class UGenericSplineRoadGeometryProvider constinit property declarations *********
	static UObject* (*const DependentSingletons[])();
	static constexpr FCppClassTypeInfoStatic StaticCppClassTypeInfo = {
		TCppClassTypeTraits<UGenericSplineRoadGeometryProvider>::IsAbstract,
	};
	static const UECodeGen_Private::FClassParams ClassParams;
}; // struct Z_Construct_UClass_UGenericSplineRoadGeometryProvider_Statics
UObject* (*const Z_Construct_UClass_UGenericSplineRoadGeometryProvider_Statics::DependentSingletons[])() = {
	(UObject* (*)())Z_Construct_UClass_UObject,
	(UObject* (*)())Z_Construct_UPackage__Script_TrafficRuntime,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_UGenericSplineRoadGeometryProvider_Statics::DependentSingletons) < 16);
const UECodeGen_Private::FClassParams Z_Construct_UClass_UGenericSplineRoadGeometryProvider_Statics::ClassParams = {
	&UGenericSplineRoadGeometryProvider::StaticClass,
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
	METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UClass_UGenericSplineRoadGeometryProvider_Statics::Class_MetaDataParams), Z_Construct_UClass_UGenericSplineRoadGeometryProvider_Statics::Class_MetaDataParams)
};
void UGenericSplineRoadGeometryProvider::StaticRegisterNativesUGenericSplineRoadGeometryProvider()
{
}
UClass* Z_Construct_UClass_UGenericSplineRoadGeometryProvider()
{
	if (!Z_Registration_Info_UClass_UGenericSplineRoadGeometryProvider.OuterSingleton)
	{
		UECodeGen_Private::ConstructUClass(Z_Registration_Info_UClass_UGenericSplineRoadGeometryProvider.OuterSingleton, Z_Construct_UClass_UGenericSplineRoadGeometryProvider_Statics::ClassParams);
	}
	return Z_Registration_Info_UClass_UGenericSplineRoadGeometryProvider.OuterSingleton;
}
UGenericSplineRoadGeometryProvider::UGenericSplineRoadGeometryProvider(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
DEFINE_VTABLE_PTR_HELPER_CTOR_NS(, UGenericSplineRoadGeometryProvider);
UGenericSplineRoadGeometryProvider::~UGenericSplineRoadGeometryProvider() {}
// ********** End Class UGenericSplineRoadGeometryProvider *****************************************

// ********** Begin Registration *******************************************************************
struct Z_CompiledInDeferFile_FID_Users_rllax_L_E_O_Plugins_AAAtrafficSystem_Source_TrafficRuntime_Public_TrafficGeometryProvider_h__Script_TrafficRuntime_Statics
{
	static constexpr FClassRegisterCompiledInInfo ClassInfo[] = {
		{ Z_Construct_UClass_UGenericSplineRoadGeometryProvider, UGenericSplineRoadGeometryProvider::StaticClass, TEXT("UGenericSplineRoadGeometryProvider"), &Z_Registration_Info_UClass_UGenericSplineRoadGeometryProvider, CONSTRUCT_RELOAD_VERSION_INFO(FClassReloadVersionInfo, sizeof(UGenericSplineRoadGeometryProvider), 580276544U) },
	};
}; // Z_CompiledInDeferFile_FID_Users_rllax_L_E_O_Plugins_AAAtrafficSystem_Source_TrafficRuntime_Public_TrafficGeometryProvider_h__Script_TrafficRuntime_Statics 
static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_rllax_L_E_O_Plugins_AAAtrafficSystem_Source_TrafficRuntime_Public_TrafficGeometryProvider_h__Script_TrafficRuntime_2038530082{
	TEXT("/Script/TrafficRuntime"),
	Z_CompiledInDeferFile_FID_Users_rllax_L_E_O_Plugins_AAAtrafficSystem_Source_TrafficRuntime_Public_TrafficGeometryProvider_h__Script_TrafficRuntime_Statics::ClassInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Users_rllax_L_E_O_Plugins_AAAtrafficSystem_Source_TrafficRuntime_Public_TrafficGeometryProvider_h__Script_TrafficRuntime_Statics::ClassInfo),
	nullptr, 0,
	nullptr, 0,
};
// ********** End Registration *********************************************************************

PRAGMA_ENABLE_DEPRECATION_WARNINGS
