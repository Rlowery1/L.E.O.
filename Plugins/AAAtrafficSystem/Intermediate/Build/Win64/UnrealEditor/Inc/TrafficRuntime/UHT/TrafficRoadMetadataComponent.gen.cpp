// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "TrafficRoadMetadataComponent.h"

PRAGMA_DISABLE_DEPRECATION_WARNINGS
static_assert(!UE_WITH_CONSTINIT_UOBJECT, "This generated code can only be compiled with !UE_WITH_CONSTINIT_OBJECT");
void EmptyLinkFunctionForGeneratedCodeTrafficRoadMetadataComponent() {}

// ********** Begin Cross Module References ********************************************************
COREUOBJECT_API UScriptStruct* Z_Construct_UScriptStruct_FGuid();
ENGINE_API UClass* Z_Construct_UClass_UActorComponent();
TRAFFICRUNTIME_API UClass* Z_Construct_UClass_UTrafficRoadMetadataComponent();
TRAFFICRUNTIME_API UClass* Z_Construct_UClass_UTrafficRoadMetadataComponent_NoRegister();
UPackage* Z_Construct_UPackage__Script_TrafficRuntime();
// ********** End Cross Module References **********************************************************

// ********** Begin Class UTrafficRoadMetadataComponent ********************************************
FClassRegistrationInfo Z_Registration_Info_UClass_UTrafficRoadMetadataComponent;
UClass* UTrafficRoadMetadataComponent::GetPrivateStaticClass()
{
	using TClass = UTrafficRoadMetadataComponent;
	if (!Z_Registration_Info_UClass_UTrafficRoadMetadataComponent.InnerSingleton)
	{
		GetPrivateStaticClassBody(
			TClass::StaticPackage(),
			TEXT("TrafficRoadMetadataComponent"),
			Z_Registration_Info_UClass_UTrafficRoadMetadataComponent.InnerSingleton,
			StaticRegisterNativesUTrafficRoadMetadataComponent,
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
	return Z_Registration_Info_UClass_UTrafficRoadMetadataComponent.InnerSingleton;
}
UClass* Z_Construct_UClass_UTrafficRoadMetadataComponent_NoRegister()
{
	return UTrafficRoadMetadataComponent::GetPrivateStaticClass();
}
struct Z_Construct_UClass_UTrafficRoadMetadataComponent_Statics
{
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Class_MetaDataParams[] = {
		{ "BlueprintSpawnableComponent", "" },
		{ "BlueprintType", "true" },
		{ "ClassGroupNames", "Traffic" },
		{ "IncludePath", "TrafficRoadMetadataComponent.h" },
		{ "IsBlueprintBase", "true" },
		{ "ModuleRelativePath", "Public/TrafficRoadMetadataComponent.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_FamilyName_MetaData[] = {
		{ "Category", "Traffic" },
		{ "ModuleRelativePath", "Public/TrafficRoadMetadataComponent.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_bIncludeInTraffic_MetaData[] = {
		{ "Category", "Traffic" },
		{ "ModuleRelativePath", "Public/TrafficRoadMetadataComponent.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_RoadFamilyId_MetaData[] = {
		{ "Category", "Traffic" },
		{ "ModuleRelativePath", "Public/TrafficRoadMetadataComponent.h" },
	};
#endif // WITH_METADATA

// ********** Begin Class UTrafficRoadMetadataComponent constinit property declarations ************
	static const UECodeGen_Private::FNamePropertyParams NewProp_FamilyName;
	static void NewProp_bIncludeInTraffic_SetBit(void* Obj);
	static const UECodeGen_Private::FBoolPropertyParams NewProp_bIncludeInTraffic;
	static const UECodeGen_Private::FStructPropertyParams NewProp_RoadFamilyId;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
// ********** End Class UTrafficRoadMetadataComponent constinit property declarations **************
	static UObject* (*const DependentSingletons[])();
	static constexpr FCppClassTypeInfoStatic StaticCppClassTypeInfo = {
		TCppClassTypeTraits<UTrafficRoadMetadataComponent>::IsAbstract,
	};
	static const UECodeGen_Private::FClassParams ClassParams;
}; // struct Z_Construct_UClass_UTrafficRoadMetadataComponent_Statics

// ********** Begin Class UTrafficRoadMetadataComponent Property Definitions ***********************
const UECodeGen_Private::FNamePropertyParams Z_Construct_UClass_UTrafficRoadMetadataComponent_Statics::NewProp_FamilyName = { "FamilyName", nullptr, (EPropertyFlags)0x0010000000000001, UECodeGen_Private::EPropertyGenFlags::Name, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(UTrafficRoadMetadataComponent, FamilyName), METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_FamilyName_MetaData), NewProp_FamilyName_MetaData) };
void Z_Construct_UClass_UTrafficRoadMetadataComponent_Statics::NewProp_bIncludeInTraffic_SetBit(void* Obj)
{
	((UTrafficRoadMetadataComponent*)Obj)->bIncludeInTraffic = 1;
}
const UECodeGen_Private::FBoolPropertyParams Z_Construct_UClass_UTrafficRoadMetadataComponent_Statics::NewProp_bIncludeInTraffic = { "bIncludeInTraffic", nullptr, (EPropertyFlags)0x0010000000000001, UECodeGen_Private::EPropertyGenFlags::Bool | UECodeGen_Private::EPropertyGenFlags::NativeBool, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, sizeof(bool), sizeof(UTrafficRoadMetadataComponent), &Z_Construct_UClass_UTrafficRoadMetadataComponent_Statics::NewProp_bIncludeInTraffic_SetBit, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_bIncludeInTraffic_MetaData), NewProp_bIncludeInTraffic_MetaData) };
const UECodeGen_Private::FStructPropertyParams Z_Construct_UClass_UTrafficRoadMetadataComponent_Statics::NewProp_RoadFamilyId = { "RoadFamilyId", nullptr, (EPropertyFlags)0x0010000000000001, UECodeGen_Private::EPropertyGenFlags::Struct, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(UTrafficRoadMetadataComponent, RoadFamilyId), Z_Construct_UScriptStruct_FGuid, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_RoadFamilyId_MetaData), NewProp_RoadFamilyId_MetaData) };
const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UClass_UTrafficRoadMetadataComponent_Statics::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UTrafficRoadMetadataComponent_Statics::NewProp_FamilyName,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UTrafficRoadMetadataComponent_Statics::NewProp_bIncludeInTraffic,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UTrafficRoadMetadataComponent_Statics::NewProp_RoadFamilyId,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_UTrafficRoadMetadataComponent_Statics::PropPointers) < 2048);
// ********** End Class UTrafficRoadMetadataComponent Property Definitions *************************
UObject* (*const Z_Construct_UClass_UTrafficRoadMetadataComponent_Statics::DependentSingletons[])() = {
	(UObject* (*)())Z_Construct_UClass_UActorComponent,
	(UObject* (*)())Z_Construct_UPackage__Script_TrafficRuntime,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_UTrafficRoadMetadataComponent_Statics::DependentSingletons) < 16);
const UECodeGen_Private::FClassParams Z_Construct_UClass_UTrafficRoadMetadataComponent_Statics::ClassParams = {
	&UTrafficRoadMetadataComponent::StaticClass,
	"Engine",
	&StaticCppClassTypeInfo,
	DependentSingletons,
	nullptr,
	Z_Construct_UClass_UTrafficRoadMetadataComponent_Statics::PropPointers,
	nullptr,
	UE_ARRAY_COUNT(DependentSingletons),
	0,
	UE_ARRAY_COUNT(Z_Construct_UClass_UTrafficRoadMetadataComponent_Statics::PropPointers),
	0,
	0x00B000A4u,
	METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UClass_UTrafficRoadMetadataComponent_Statics::Class_MetaDataParams), Z_Construct_UClass_UTrafficRoadMetadataComponent_Statics::Class_MetaDataParams)
};
void UTrafficRoadMetadataComponent::StaticRegisterNativesUTrafficRoadMetadataComponent()
{
}
UClass* Z_Construct_UClass_UTrafficRoadMetadataComponent()
{
	if (!Z_Registration_Info_UClass_UTrafficRoadMetadataComponent.OuterSingleton)
	{
		UECodeGen_Private::ConstructUClass(Z_Registration_Info_UClass_UTrafficRoadMetadataComponent.OuterSingleton, Z_Construct_UClass_UTrafficRoadMetadataComponent_Statics::ClassParams);
	}
	return Z_Registration_Info_UClass_UTrafficRoadMetadataComponent.OuterSingleton;
}
DEFINE_VTABLE_PTR_HELPER_CTOR_NS(, UTrafficRoadMetadataComponent);
UTrafficRoadMetadataComponent::~UTrafficRoadMetadataComponent() {}
// ********** End Class UTrafficRoadMetadataComponent **********************************************

// ********** Begin Registration *******************************************************************
struct Z_CompiledInDeferFile_FID_Users_rllax_L_E_O_Plugins_AAAtrafficSystem_Source_TrafficRuntime_Public_TrafficRoadMetadataComponent_h__Script_TrafficRuntime_Statics
{
	static constexpr FClassRegisterCompiledInInfo ClassInfo[] = {
		{ Z_Construct_UClass_UTrafficRoadMetadataComponent, UTrafficRoadMetadataComponent::StaticClass, TEXT("UTrafficRoadMetadataComponent"), &Z_Registration_Info_UClass_UTrafficRoadMetadataComponent, CONSTRUCT_RELOAD_VERSION_INFO(FClassReloadVersionInfo, sizeof(UTrafficRoadMetadataComponent), 3525564232U) },
	};
}; // Z_CompiledInDeferFile_FID_Users_rllax_L_E_O_Plugins_AAAtrafficSystem_Source_TrafficRuntime_Public_TrafficRoadMetadataComponent_h__Script_TrafficRuntime_Statics 
static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_rllax_L_E_O_Plugins_AAAtrafficSystem_Source_TrafficRuntime_Public_TrafficRoadMetadataComponent_h__Script_TrafficRuntime_3778188319{
	TEXT("/Script/TrafficRuntime"),
	Z_CompiledInDeferFile_FID_Users_rllax_L_E_O_Plugins_AAAtrafficSystem_Source_TrafficRuntime_Public_TrafficRoadMetadataComponent_h__Script_TrafficRuntime_Statics::ClassInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Users_rllax_L_E_O_Plugins_AAAtrafficSystem_Source_TrafficRuntime_Public_TrafficRoadMetadataComponent_h__Script_TrafficRuntime_Statics::ClassInfo),
	nullptr, 0,
	nullptr, 0,
};
// ********** End Registration *********************************************************************

PRAGMA_ENABLE_DEPRECATION_WARNINGS
