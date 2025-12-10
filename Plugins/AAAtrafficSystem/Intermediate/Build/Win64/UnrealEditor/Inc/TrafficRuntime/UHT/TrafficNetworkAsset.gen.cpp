// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "TrafficNetworkAsset.h"
#include "TrafficRoadTypes.h"

PRAGMA_DISABLE_DEPRECATION_WARNINGS
static_assert(!UE_WITH_CONSTINIT_UOBJECT, "This generated code can only be compiled with !UE_WITH_CONSTINIT_OBJECT");
void EmptyLinkFunctionForGeneratedCodeTrafficNetworkAsset() {}

// ********** Begin Cross Module References ********************************************************
ENGINE_API UClass* Z_Construct_UClass_UDataAsset();
TRAFFICRUNTIME_API UClass* Z_Construct_UClass_UTrafficNetworkAsset();
TRAFFICRUNTIME_API UClass* Z_Construct_UClass_UTrafficNetworkAsset_NoRegister();
TRAFFICRUNTIME_API UScriptStruct* Z_Construct_UScriptStruct_FTrafficNetwork();
UPackage* Z_Construct_UPackage__Script_TrafficRuntime();
// ********** End Cross Module References **********************************************************

// ********** Begin Class UTrafficNetworkAsset *****************************************************
FClassRegistrationInfo Z_Registration_Info_UClass_UTrafficNetworkAsset;
UClass* UTrafficNetworkAsset::GetPrivateStaticClass()
{
	using TClass = UTrafficNetworkAsset;
	if (!Z_Registration_Info_UClass_UTrafficNetworkAsset.InnerSingleton)
	{
		GetPrivateStaticClassBody(
			TClass::StaticPackage(),
			TEXT("TrafficNetworkAsset"),
			Z_Registration_Info_UClass_UTrafficNetworkAsset.InnerSingleton,
			StaticRegisterNativesUTrafficNetworkAsset,
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
	return Z_Registration_Info_UClass_UTrafficNetworkAsset.InnerSingleton;
}
UClass* Z_Construct_UClass_UTrafficNetworkAsset_NoRegister()
{
	return UTrafficNetworkAsset::GetPrivateStaticClass();
}
struct Z_Construct_UClass_UTrafficNetworkAsset_Statics
{
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Class_MetaDataParams[] = {
		{ "BlueprintType", "true" },
		{ "IncludePath", "TrafficNetworkAsset.h" },
		{ "ModuleRelativePath", "Public/TrafficNetworkAsset.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_Network_MetaData[] = {
		{ "Category", "Traffic" },
		{ "ModuleRelativePath", "Public/TrafficNetworkAsset.h" },
	};
#endif // WITH_METADATA

// ********** Begin Class UTrafficNetworkAsset constinit property declarations *********************
	static const UECodeGen_Private::FStructPropertyParams NewProp_Network;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
// ********** End Class UTrafficNetworkAsset constinit property declarations ***********************
	static UObject* (*const DependentSingletons[])();
	static constexpr FCppClassTypeInfoStatic StaticCppClassTypeInfo = {
		TCppClassTypeTraits<UTrafficNetworkAsset>::IsAbstract,
	};
	static const UECodeGen_Private::FClassParams ClassParams;
}; // struct Z_Construct_UClass_UTrafficNetworkAsset_Statics

// ********** Begin Class UTrafficNetworkAsset Property Definitions ********************************
const UECodeGen_Private::FStructPropertyParams Z_Construct_UClass_UTrafficNetworkAsset_Statics::NewProp_Network = { "Network", nullptr, (EPropertyFlags)0x0010000000000015, UECodeGen_Private::EPropertyGenFlags::Struct, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(UTrafficNetworkAsset, Network), Z_Construct_UScriptStruct_FTrafficNetwork, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_Network_MetaData), NewProp_Network_MetaData) }; // 1665217710
const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UClass_UTrafficNetworkAsset_Statics::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UTrafficNetworkAsset_Statics::NewProp_Network,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_UTrafficNetworkAsset_Statics::PropPointers) < 2048);
// ********** End Class UTrafficNetworkAsset Property Definitions **********************************
UObject* (*const Z_Construct_UClass_UTrafficNetworkAsset_Statics::DependentSingletons[])() = {
	(UObject* (*)())Z_Construct_UClass_UDataAsset,
	(UObject* (*)())Z_Construct_UPackage__Script_TrafficRuntime,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_UTrafficNetworkAsset_Statics::DependentSingletons) < 16);
const UECodeGen_Private::FClassParams Z_Construct_UClass_UTrafficNetworkAsset_Statics::ClassParams = {
	&UTrafficNetworkAsset::StaticClass,
	nullptr,
	&StaticCppClassTypeInfo,
	DependentSingletons,
	nullptr,
	Z_Construct_UClass_UTrafficNetworkAsset_Statics::PropPointers,
	nullptr,
	UE_ARRAY_COUNT(DependentSingletons),
	0,
	UE_ARRAY_COUNT(Z_Construct_UClass_UTrafficNetworkAsset_Statics::PropPointers),
	0,
	0x001000A0u,
	METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UClass_UTrafficNetworkAsset_Statics::Class_MetaDataParams), Z_Construct_UClass_UTrafficNetworkAsset_Statics::Class_MetaDataParams)
};
void UTrafficNetworkAsset::StaticRegisterNativesUTrafficNetworkAsset()
{
}
UClass* Z_Construct_UClass_UTrafficNetworkAsset()
{
	if (!Z_Registration_Info_UClass_UTrafficNetworkAsset.OuterSingleton)
	{
		UECodeGen_Private::ConstructUClass(Z_Registration_Info_UClass_UTrafficNetworkAsset.OuterSingleton, Z_Construct_UClass_UTrafficNetworkAsset_Statics::ClassParams);
	}
	return Z_Registration_Info_UClass_UTrafficNetworkAsset.OuterSingleton;
}
UTrafficNetworkAsset::UTrafficNetworkAsset(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
DEFINE_VTABLE_PTR_HELPER_CTOR_NS(, UTrafficNetworkAsset);
UTrafficNetworkAsset::~UTrafficNetworkAsset() {}
// ********** End Class UTrafficNetworkAsset *******************************************************

// ********** Begin Registration *******************************************************************
struct Z_CompiledInDeferFile_FID_Users_rllax_L_E_O_Plugins_AAAtrafficSystem_Source_TrafficRuntime_Public_TrafficNetworkAsset_h__Script_TrafficRuntime_Statics
{
	static constexpr FClassRegisterCompiledInInfo ClassInfo[] = {
		{ Z_Construct_UClass_UTrafficNetworkAsset, UTrafficNetworkAsset::StaticClass, TEXT("UTrafficNetworkAsset"), &Z_Registration_Info_UClass_UTrafficNetworkAsset, CONSTRUCT_RELOAD_VERSION_INFO(FClassReloadVersionInfo, sizeof(UTrafficNetworkAsset), 998562107U) },
	};
}; // Z_CompiledInDeferFile_FID_Users_rllax_L_E_O_Plugins_AAAtrafficSystem_Source_TrafficRuntime_Public_TrafficNetworkAsset_h__Script_TrafficRuntime_Statics 
static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_rllax_L_E_O_Plugins_AAAtrafficSystem_Source_TrafficRuntime_Public_TrafficNetworkAsset_h__Script_TrafficRuntime_4285895732{
	TEXT("/Script/TrafficRuntime"),
	Z_CompiledInDeferFile_FID_Users_rllax_L_E_O_Plugins_AAAtrafficSystem_Source_TrafficRuntime_Public_TrafficNetworkAsset_h__Script_TrafficRuntime_Statics::ClassInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Users_rllax_L_E_O_Plugins_AAAtrafficSystem_Source_TrafficRuntime_Public_TrafficNetworkAsset_h__Script_TrafficRuntime_Statics::ClassInfo),
	nullptr, 0,
	nullptr, 0,
};
// ********** End Registration *********************************************************************

PRAGMA_ENABLE_DEPRECATION_WARNINGS
