// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "TrafficVehicleProfile.h"

PRAGMA_DISABLE_DEPRECATION_WARNINGS
static_assert(!UE_WITH_CONSTINIT_UOBJECT, "This generated code can only be compiled with !UE_WITH_CONSTINIT_OBJECT");
void EmptyLinkFunctionForGeneratedCodeTrafficVehicleProfile() {}

// ********** Begin Cross Module References ********************************************************
COREUOBJECT_API UClass* Z_Construct_UClass_UClass_NoRegister();
ENGINE_API UClass* Z_Construct_UClass_APawn_NoRegister();
ENGINE_API UClass* Z_Construct_UClass_UPrimaryDataAsset();
TRAFFICRUNTIME_API UClass* Z_Construct_UClass_UTrafficVehicleProfile();
TRAFFICRUNTIME_API UClass* Z_Construct_UClass_UTrafficVehicleProfile_NoRegister();
UPackage* Z_Construct_UPackage__Script_TrafficRuntime();
// ********** End Cross Module References **********************************************************

// ********** Begin Class UTrafficVehicleProfile ***************************************************
FClassRegistrationInfo Z_Registration_Info_UClass_UTrafficVehicleProfile;
UClass* UTrafficVehicleProfile::GetPrivateStaticClass()
{
	using TClass = UTrafficVehicleProfile;
	if (!Z_Registration_Info_UClass_UTrafficVehicleProfile.InnerSingleton)
	{
		GetPrivateStaticClassBody(
			TClass::StaticPackage(),
			TEXT("TrafficVehicleProfile"),
			Z_Registration_Info_UClass_UTrafficVehicleProfile.InnerSingleton,
			StaticRegisterNativesUTrafficVehicleProfile,
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
	return Z_Registration_Info_UClass_UTrafficVehicleProfile.InnerSingleton;
}
UClass* Z_Construct_UClass_UTrafficVehicleProfile_NoRegister()
{
	return UTrafficVehicleProfile::GetPrivateStaticClass();
}
struct Z_Construct_UClass_UTrafficVehicleProfile_Statics
{
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Class_MetaDataParams[] = {
		{ "BlueprintType", "true" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/**\n * Data-driven vehicle profile describing the visual/Chaos pawn and basic dimensions.\n */" },
#endif
		{ "IncludePath", "TrafficVehicleProfile.h" },
		{ "ModuleRelativePath", "Public/TrafficVehicleProfile.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Data-driven vehicle profile describing the visual/Chaos pawn and basic dimensions." },
#endif
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_VehicleClass_MetaData[] = {
		{ "Category", "TrafficVehicle" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** Visual/Chaos pawn class to use for this profile. */" },
#endif
		{ "ModuleRelativePath", "Public/TrafficVehicleProfile.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Visual/Chaos pawn class to use for this profile." },
#endif
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_CategoryTag_MetaData[] = {
		{ "Category", "TrafficVehicle" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** Optional category tag (Car, Truck, Bus, etc.). */" },
#endif
		{ "ModuleRelativePath", "Public/TrafficVehicleProfile.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Optional category tag (Car, Truck, Bus, etc.)." },
#endif
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_LengthCm_MetaData[] = {
		{ "Category", "TrafficVehicle" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** Approximate physical dimensions (cm). */" },
#endif
		{ "ModuleRelativePath", "Public/TrafficVehicleProfile.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Approximate physical dimensions (cm)." },
#endif
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_WidthCm_MetaData[] = {
		{ "Category", "TrafficVehicle" },
		{ "ModuleRelativePath", "Public/TrafficVehicleProfile.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_HeadwayScale_MetaData[] = {
		{ "Category", "TrafficVehicle" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** Recommended headway multiplier for this profile (for later tuning). */" },
#endif
		{ "ModuleRelativePath", "Public/TrafficVehicleProfile.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Recommended headway multiplier for this profile (for later tuning)." },
#endif
	};
#endif // WITH_METADATA

// ********** Begin Class UTrafficVehicleProfile constinit property declarations *******************
	static const UECodeGen_Private::FSoftClassPropertyParams NewProp_VehicleClass;
	static const UECodeGen_Private::FNamePropertyParams NewProp_CategoryTag;
	static const UECodeGen_Private::FFloatPropertyParams NewProp_LengthCm;
	static const UECodeGen_Private::FFloatPropertyParams NewProp_WidthCm;
	static const UECodeGen_Private::FFloatPropertyParams NewProp_HeadwayScale;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
// ********** End Class UTrafficVehicleProfile constinit property declarations *********************
	static UObject* (*const DependentSingletons[])();
	static constexpr FCppClassTypeInfoStatic StaticCppClassTypeInfo = {
		TCppClassTypeTraits<UTrafficVehicleProfile>::IsAbstract,
	};
	static const UECodeGen_Private::FClassParams ClassParams;
}; // struct Z_Construct_UClass_UTrafficVehicleProfile_Statics

// ********** Begin Class UTrafficVehicleProfile Property Definitions ******************************
const UECodeGen_Private::FSoftClassPropertyParams Z_Construct_UClass_UTrafficVehicleProfile_Statics::NewProp_VehicleClass = { "VehicleClass", nullptr, (EPropertyFlags)0x0014000000000015, UECodeGen_Private::EPropertyGenFlags::SoftClass, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(UTrafficVehicleProfile, VehicleClass), Z_Construct_UClass_APawn_NoRegister, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_VehicleClass_MetaData), NewProp_VehicleClass_MetaData) };
const UECodeGen_Private::FNamePropertyParams Z_Construct_UClass_UTrafficVehicleProfile_Statics::NewProp_CategoryTag = { "CategoryTag", nullptr, (EPropertyFlags)0x0010000000000015, UECodeGen_Private::EPropertyGenFlags::Name, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(UTrafficVehicleProfile, CategoryTag), METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_CategoryTag_MetaData), NewProp_CategoryTag_MetaData) };
const UECodeGen_Private::FFloatPropertyParams Z_Construct_UClass_UTrafficVehicleProfile_Statics::NewProp_LengthCm = { "LengthCm", nullptr, (EPropertyFlags)0x0010000000000015, UECodeGen_Private::EPropertyGenFlags::Float, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(UTrafficVehicleProfile, LengthCm), METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_LengthCm_MetaData), NewProp_LengthCm_MetaData) };
const UECodeGen_Private::FFloatPropertyParams Z_Construct_UClass_UTrafficVehicleProfile_Statics::NewProp_WidthCm = { "WidthCm", nullptr, (EPropertyFlags)0x0010000000000015, UECodeGen_Private::EPropertyGenFlags::Float, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(UTrafficVehicleProfile, WidthCm), METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_WidthCm_MetaData), NewProp_WidthCm_MetaData) };
const UECodeGen_Private::FFloatPropertyParams Z_Construct_UClass_UTrafficVehicleProfile_Statics::NewProp_HeadwayScale = { "HeadwayScale", nullptr, (EPropertyFlags)0x0010000000000015, UECodeGen_Private::EPropertyGenFlags::Float, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(UTrafficVehicleProfile, HeadwayScale), METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_HeadwayScale_MetaData), NewProp_HeadwayScale_MetaData) };
const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UClass_UTrafficVehicleProfile_Statics::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UTrafficVehicleProfile_Statics::NewProp_VehicleClass,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UTrafficVehicleProfile_Statics::NewProp_CategoryTag,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UTrafficVehicleProfile_Statics::NewProp_LengthCm,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UTrafficVehicleProfile_Statics::NewProp_WidthCm,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UTrafficVehicleProfile_Statics::NewProp_HeadwayScale,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_UTrafficVehicleProfile_Statics::PropPointers) < 2048);
// ********** End Class UTrafficVehicleProfile Property Definitions ********************************
UObject* (*const Z_Construct_UClass_UTrafficVehicleProfile_Statics::DependentSingletons[])() = {
	(UObject* (*)())Z_Construct_UClass_UPrimaryDataAsset,
	(UObject* (*)())Z_Construct_UPackage__Script_TrafficRuntime,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_UTrafficVehicleProfile_Statics::DependentSingletons) < 16);
const UECodeGen_Private::FClassParams Z_Construct_UClass_UTrafficVehicleProfile_Statics::ClassParams = {
	&UTrafficVehicleProfile::StaticClass,
	nullptr,
	&StaticCppClassTypeInfo,
	DependentSingletons,
	nullptr,
	Z_Construct_UClass_UTrafficVehicleProfile_Statics::PropPointers,
	nullptr,
	UE_ARRAY_COUNT(DependentSingletons),
	0,
	UE_ARRAY_COUNT(Z_Construct_UClass_UTrafficVehicleProfile_Statics::PropPointers),
	0,
	0x001000A0u,
	METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UClass_UTrafficVehicleProfile_Statics::Class_MetaDataParams), Z_Construct_UClass_UTrafficVehicleProfile_Statics::Class_MetaDataParams)
};
void UTrafficVehicleProfile::StaticRegisterNativesUTrafficVehicleProfile()
{
}
UClass* Z_Construct_UClass_UTrafficVehicleProfile()
{
	if (!Z_Registration_Info_UClass_UTrafficVehicleProfile.OuterSingleton)
	{
		UECodeGen_Private::ConstructUClass(Z_Registration_Info_UClass_UTrafficVehicleProfile.OuterSingleton, Z_Construct_UClass_UTrafficVehicleProfile_Statics::ClassParams);
	}
	return Z_Registration_Info_UClass_UTrafficVehicleProfile.OuterSingleton;
}
UTrafficVehicleProfile::UTrafficVehicleProfile(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
DEFINE_VTABLE_PTR_HELPER_CTOR_NS(, UTrafficVehicleProfile);
UTrafficVehicleProfile::~UTrafficVehicleProfile() {}
// ********** End Class UTrafficVehicleProfile *****************************************************

// ********** Begin Registration *******************************************************************
struct Z_CompiledInDeferFile_FID_Users_rllax_L_E_O_Plugins_AAAtrafficSystem_Source_TrafficRuntime_Public_TrafficVehicleProfile_h__Script_TrafficRuntime_Statics
{
	static constexpr FClassRegisterCompiledInInfo ClassInfo[] = {
		{ Z_Construct_UClass_UTrafficVehicleProfile, UTrafficVehicleProfile::StaticClass, TEXT("UTrafficVehicleProfile"), &Z_Registration_Info_UClass_UTrafficVehicleProfile, CONSTRUCT_RELOAD_VERSION_INFO(FClassReloadVersionInfo, sizeof(UTrafficVehicleProfile), 204025939U) },
	};
}; // Z_CompiledInDeferFile_FID_Users_rllax_L_E_O_Plugins_AAAtrafficSystem_Source_TrafficRuntime_Public_TrafficVehicleProfile_h__Script_TrafficRuntime_Statics 
static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_rllax_L_E_O_Plugins_AAAtrafficSystem_Source_TrafficRuntime_Public_TrafficVehicleProfile_h__Script_TrafficRuntime_1407386150{
	TEXT("/Script/TrafficRuntime"),
	Z_CompiledInDeferFile_FID_Users_rllax_L_E_O_Plugins_AAAtrafficSystem_Source_TrafficRuntime_Public_TrafficVehicleProfile_h__Script_TrafficRuntime_Statics::ClassInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Users_rllax_L_E_O_Plugins_AAAtrafficSystem_Source_TrafficRuntime_Public_TrafficVehicleProfile_h__Script_TrafficRuntime_Statics::ClassInfo),
	nullptr, 0,
	nullptr, 0,
};
// ********** End Registration *********************************************************************

PRAGMA_ENABLE_DEPRECATION_WARNINGS
