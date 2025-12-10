// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "TrafficKinematicFollower.h"

PRAGMA_DISABLE_DEPRECATION_WARNINGS
static_assert(!UE_WITH_CONSTINIT_UOBJECT, "This generated code can only be compiled with !UE_WITH_CONSTINIT_OBJECT");
void EmptyLinkFunctionForGeneratedCodeTrafficKinematicFollower() {}

// ********** Begin Cross Module References ********************************************************
COREUOBJECT_API UClass* Z_Construct_UClass_UObject();
TRAFFICRUNTIME_API UClass* Z_Construct_UClass_UTrafficKinematicFollower();
TRAFFICRUNTIME_API UClass* Z_Construct_UClass_UTrafficKinematicFollower_NoRegister();
TRAFFICRUNTIME_API UEnum* Z_Construct_UEnum_TrafficRuntime_EPathFollowTargetType();
TRAFFICRUNTIME_API UScriptStruct* Z_Construct_UScriptStruct_FPathFollowState();
UPackage* Z_Construct_UPackage__Script_TrafficRuntime();
// ********** End Cross Module References **********************************************************

// ********** Begin Enum EPathFollowTargetType *****************************************************
static FEnumRegistrationInfo Z_Registration_Info_UEnum_EPathFollowTargetType;
static UEnum* EPathFollowTargetType_StaticEnum()
{
	if (!Z_Registration_Info_UEnum_EPathFollowTargetType.OuterSingleton)
	{
		Z_Registration_Info_UEnum_EPathFollowTargetType.OuterSingleton = GetStaticEnum(Z_Construct_UEnum_TrafficRuntime_EPathFollowTargetType, (UObject*)Z_Construct_UPackage__Script_TrafficRuntime(), TEXT("EPathFollowTargetType"));
	}
	return Z_Registration_Info_UEnum_EPathFollowTargetType.OuterSingleton;
}
template<> TRAFFICRUNTIME_NON_ATTRIBUTED_API UEnum* StaticEnum<EPathFollowTargetType>()
{
	return EPathFollowTargetType_StaticEnum();
}
struct Z_Construct_UEnum_TrafficRuntime_EPathFollowTargetType_Statics
{
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Enum_MetaDataParams[] = {
		{ "BlueprintType", "true" },
		{ "Lane.Name", "EPathFollowTargetType::Lane" },
		{ "ModuleRelativePath", "Public/TrafficKinematicFollower.h" },
		{ "Movement.Name", "EPathFollowTargetType::Movement" },
		{ "None.Name", "EPathFollowTargetType::None" },
	};
#endif // WITH_METADATA
	static constexpr UECodeGen_Private::FEnumeratorParam Enumerators[] = {
		{ "EPathFollowTargetType::None", (int64)EPathFollowTargetType::None },
		{ "EPathFollowTargetType::Lane", (int64)EPathFollowTargetType::Lane },
		{ "EPathFollowTargetType::Movement", (int64)EPathFollowTargetType::Movement },
	};
	static const UECodeGen_Private::FEnumParams EnumParams;
}; // struct Z_Construct_UEnum_TrafficRuntime_EPathFollowTargetType_Statics 
const UECodeGen_Private::FEnumParams Z_Construct_UEnum_TrafficRuntime_EPathFollowTargetType_Statics::EnumParams = {
	(UObject*(*)())Z_Construct_UPackage__Script_TrafficRuntime,
	nullptr,
	"EPathFollowTargetType",
	"EPathFollowTargetType",
	Z_Construct_UEnum_TrafficRuntime_EPathFollowTargetType_Statics::Enumerators,
	RF_Public|RF_Transient|RF_MarkAsNative,
	UE_ARRAY_COUNT(Z_Construct_UEnum_TrafficRuntime_EPathFollowTargetType_Statics::Enumerators),
	EEnumFlags::None,
	(uint8)UEnum::ECppForm::EnumClass,
	METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UEnum_TrafficRuntime_EPathFollowTargetType_Statics::Enum_MetaDataParams), Z_Construct_UEnum_TrafficRuntime_EPathFollowTargetType_Statics::Enum_MetaDataParams)
};
UEnum* Z_Construct_UEnum_TrafficRuntime_EPathFollowTargetType()
{
	if (!Z_Registration_Info_UEnum_EPathFollowTargetType.InnerSingleton)
	{
		UECodeGen_Private::ConstructUEnum(Z_Registration_Info_UEnum_EPathFollowTargetType.InnerSingleton, Z_Construct_UEnum_TrafficRuntime_EPathFollowTargetType_Statics::EnumParams);
	}
	return Z_Registration_Info_UEnum_EPathFollowTargetType.InnerSingleton;
}
// ********** End Enum EPathFollowTargetType *******************************************************

// ********** Begin ScriptStruct FPathFollowState **************************************************
struct Z_Construct_UScriptStruct_FPathFollowState_Statics
{
	static inline consteval int32 GetStructSize() { return sizeof(FPathFollowState); }
	static inline consteval int16 GetStructAlignment() { return alignof(FPathFollowState); }
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Struct_MetaDataParams[] = {
		{ "BlueprintType", "true" },
		{ "ModuleRelativePath", "Public/TrafficKinematicFollower.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_TargetType_MetaData[] = {
		{ "ModuleRelativePath", "Public/TrafficKinematicFollower.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_TargetId_MetaData[] = {
		{ "ModuleRelativePath", "Public/TrafficKinematicFollower.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_S_MetaData[] = {
		{ "ModuleRelativePath", "Public/TrafficKinematicFollower.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_SpeedCmPerSec_MetaData[] = {
		{ "ModuleRelativePath", "Public/TrafficKinematicFollower.h" },
	};
#endif // WITH_METADATA

// ********** Begin ScriptStruct FPathFollowState constinit property declarations ******************
	static const UECodeGen_Private::FBytePropertyParams NewProp_TargetType_Underlying;
	static const UECodeGen_Private::FEnumPropertyParams NewProp_TargetType;
	static const UECodeGen_Private::FIntPropertyParams NewProp_TargetId;
	static const UECodeGen_Private::FFloatPropertyParams NewProp_S;
	static const UECodeGen_Private::FFloatPropertyParams NewProp_SpeedCmPerSec;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
// ********** End ScriptStruct FPathFollowState constinit property declarations ********************
	static void* NewStructOps()
	{
		return (UScriptStruct::ICppStructOps*)new UScriptStruct::TCppStructOps<FPathFollowState>();
	}
	static const UECodeGen_Private::FStructParams StructParams;
}; // struct Z_Construct_UScriptStruct_FPathFollowState_Statics
static FStructRegistrationInfo Z_Registration_Info_UScriptStruct_FPathFollowState;
class UScriptStruct* FPathFollowState::StaticStruct()
{
	if (!Z_Registration_Info_UScriptStruct_FPathFollowState.OuterSingleton)
	{
		Z_Registration_Info_UScriptStruct_FPathFollowState.OuterSingleton = GetStaticStruct(Z_Construct_UScriptStruct_FPathFollowState, (UObject*)Z_Construct_UPackage__Script_TrafficRuntime(), TEXT("PathFollowState"));
	}
	return Z_Registration_Info_UScriptStruct_FPathFollowState.OuterSingleton;
	}

// ********** Begin ScriptStruct FPathFollowState Property Definitions *****************************
const UECodeGen_Private::FBytePropertyParams Z_Construct_UScriptStruct_FPathFollowState_Statics::NewProp_TargetType_Underlying = { "UnderlyingType", nullptr, (EPropertyFlags)0x0000000000000000, UECodeGen_Private::EPropertyGenFlags::Byte, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, 0, nullptr, METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FEnumPropertyParams Z_Construct_UScriptStruct_FPathFollowState_Statics::NewProp_TargetType = { "TargetType", nullptr, (EPropertyFlags)0x0010000000000000, UECodeGen_Private::EPropertyGenFlags::Enum, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(FPathFollowState, TargetType), Z_Construct_UEnum_TrafficRuntime_EPathFollowTargetType, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_TargetType_MetaData), NewProp_TargetType_MetaData) }; // 1813143010
const UECodeGen_Private::FIntPropertyParams Z_Construct_UScriptStruct_FPathFollowState_Statics::NewProp_TargetId = { "TargetId", nullptr, (EPropertyFlags)0x0010000000000000, UECodeGen_Private::EPropertyGenFlags::Int, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(FPathFollowState, TargetId), METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_TargetId_MetaData), NewProp_TargetId_MetaData) };
const UECodeGen_Private::FFloatPropertyParams Z_Construct_UScriptStruct_FPathFollowState_Statics::NewProp_S = { "S", nullptr, (EPropertyFlags)0x0010000000000000, UECodeGen_Private::EPropertyGenFlags::Float, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(FPathFollowState, S), METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_S_MetaData), NewProp_S_MetaData) };
const UECodeGen_Private::FFloatPropertyParams Z_Construct_UScriptStruct_FPathFollowState_Statics::NewProp_SpeedCmPerSec = { "SpeedCmPerSec", nullptr, (EPropertyFlags)0x0010000000000000, UECodeGen_Private::EPropertyGenFlags::Float, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(FPathFollowState, SpeedCmPerSec), METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_SpeedCmPerSec_MetaData), NewProp_SpeedCmPerSec_MetaData) };
const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UScriptStruct_FPathFollowState_Statics::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FPathFollowState_Statics::NewProp_TargetType_Underlying,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FPathFollowState_Statics::NewProp_TargetType,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FPathFollowState_Statics::NewProp_TargetId,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FPathFollowState_Statics::NewProp_S,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FPathFollowState_Statics::NewProp_SpeedCmPerSec,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FPathFollowState_Statics::PropPointers) < 2048);
// ********** End ScriptStruct FPathFollowState Property Definitions *******************************
const UECodeGen_Private::FStructParams Z_Construct_UScriptStruct_FPathFollowState_Statics::StructParams = {
	(UObject* (*)())Z_Construct_UPackage__Script_TrafficRuntime,
	nullptr,
	&NewStructOps,
	"PathFollowState",
	Z_Construct_UScriptStruct_FPathFollowState_Statics::PropPointers,
	UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FPathFollowState_Statics::PropPointers),
	sizeof(FPathFollowState),
	alignof(FPathFollowState),
	RF_Public|RF_Transient|RF_MarkAsNative,
	EStructFlags(0x00000201),
	METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FPathFollowState_Statics::Struct_MetaDataParams), Z_Construct_UScriptStruct_FPathFollowState_Statics::Struct_MetaDataParams)
};
UScriptStruct* Z_Construct_UScriptStruct_FPathFollowState()
{
	if (!Z_Registration_Info_UScriptStruct_FPathFollowState.InnerSingleton)
	{
		UECodeGen_Private::ConstructUScriptStruct(Z_Registration_Info_UScriptStruct_FPathFollowState.InnerSingleton, Z_Construct_UScriptStruct_FPathFollowState_Statics::StructParams);
	}
	return CastChecked<UScriptStruct>(Z_Registration_Info_UScriptStruct_FPathFollowState.InnerSingleton);
}
// ********** End ScriptStruct FPathFollowState ****************************************************

// ********** Begin Class UTrafficKinematicFollower ************************************************
FClassRegistrationInfo Z_Registration_Info_UClass_UTrafficKinematicFollower;
UClass* UTrafficKinematicFollower::GetPrivateStaticClass()
{
	using TClass = UTrafficKinematicFollower;
	if (!Z_Registration_Info_UClass_UTrafficKinematicFollower.InnerSingleton)
	{
		GetPrivateStaticClassBody(
			TClass::StaticPackage(),
			TEXT("TrafficKinematicFollower"),
			Z_Registration_Info_UClass_UTrafficKinematicFollower.InnerSingleton,
			StaticRegisterNativesUTrafficKinematicFollower,
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
	return Z_Registration_Info_UClass_UTrafficKinematicFollower.InnerSingleton;
}
UClass* Z_Construct_UClass_UTrafficKinematicFollower_NoRegister()
{
	return UTrafficKinematicFollower::GetPrivateStaticClass();
}
struct Z_Construct_UClass_UTrafficKinematicFollower_Statics
{
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Class_MetaDataParams[] = {
		{ "IncludePath", "TrafficKinematicFollower.h" },
		{ "ModuleRelativePath", "Public/TrafficKinematicFollower.h" },
	};
#endif // WITH_METADATA

// ********** Begin Class UTrafficKinematicFollower constinit property declarations ****************
// ********** End Class UTrafficKinematicFollower constinit property declarations ******************
	static UObject* (*const DependentSingletons[])();
	static constexpr FCppClassTypeInfoStatic StaticCppClassTypeInfo = {
		TCppClassTypeTraits<UTrafficKinematicFollower>::IsAbstract,
	};
	static const UECodeGen_Private::FClassParams ClassParams;
}; // struct Z_Construct_UClass_UTrafficKinematicFollower_Statics
UObject* (*const Z_Construct_UClass_UTrafficKinematicFollower_Statics::DependentSingletons[])() = {
	(UObject* (*)())Z_Construct_UClass_UObject,
	(UObject* (*)())Z_Construct_UPackage__Script_TrafficRuntime,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_UTrafficKinematicFollower_Statics::DependentSingletons) < 16);
const UECodeGen_Private::FClassParams Z_Construct_UClass_UTrafficKinematicFollower_Statics::ClassParams = {
	&UTrafficKinematicFollower::StaticClass,
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
	METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UClass_UTrafficKinematicFollower_Statics::Class_MetaDataParams), Z_Construct_UClass_UTrafficKinematicFollower_Statics::Class_MetaDataParams)
};
void UTrafficKinematicFollower::StaticRegisterNativesUTrafficKinematicFollower()
{
}
UClass* Z_Construct_UClass_UTrafficKinematicFollower()
{
	if (!Z_Registration_Info_UClass_UTrafficKinematicFollower.OuterSingleton)
	{
		UECodeGen_Private::ConstructUClass(Z_Registration_Info_UClass_UTrafficKinematicFollower.OuterSingleton, Z_Construct_UClass_UTrafficKinematicFollower_Statics::ClassParams);
	}
	return Z_Registration_Info_UClass_UTrafficKinematicFollower.OuterSingleton;
}
UTrafficKinematicFollower::UTrafficKinematicFollower(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
DEFINE_VTABLE_PTR_HELPER_CTOR_NS(, UTrafficKinematicFollower);
UTrafficKinematicFollower::~UTrafficKinematicFollower() {}
// ********** End Class UTrafficKinematicFollower **************************************************

// ********** Begin Registration *******************************************************************
struct Z_CompiledInDeferFile_FID_Users_rllax_L_E_O_Plugins_AAAtrafficSystem_Source_TrafficRuntime_Public_TrafficKinematicFollower_h__Script_TrafficRuntime_Statics
{
	static constexpr FEnumRegisterCompiledInInfo EnumInfo[] = {
		{ EPathFollowTargetType_StaticEnum, TEXT("EPathFollowTargetType"), &Z_Registration_Info_UEnum_EPathFollowTargetType, CONSTRUCT_RELOAD_VERSION_INFO(FEnumReloadVersionInfo, 1813143010U) },
	};
	static constexpr FStructRegisterCompiledInInfo ScriptStructInfo[] = {
		{ FPathFollowState::StaticStruct, Z_Construct_UScriptStruct_FPathFollowState_Statics::NewStructOps, TEXT("PathFollowState"),&Z_Registration_Info_UScriptStruct_FPathFollowState, CONSTRUCT_RELOAD_VERSION_INFO(FStructReloadVersionInfo, sizeof(FPathFollowState), 131686988U) },
	};
	static constexpr FClassRegisterCompiledInInfo ClassInfo[] = {
		{ Z_Construct_UClass_UTrafficKinematicFollower, UTrafficKinematicFollower::StaticClass, TEXT("UTrafficKinematicFollower"), &Z_Registration_Info_UClass_UTrafficKinematicFollower, CONSTRUCT_RELOAD_VERSION_INFO(FClassReloadVersionInfo, sizeof(UTrafficKinematicFollower), 360835028U) },
	};
}; // Z_CompiledInDeferFile_FID_Users_rllax_L_E_O_Plugins_AAAtrafficSystem_Source_TrafficRuntime_Public_TrafficKinematicFollower_h__Script_TrafficRuntime_Statics 
static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_rllax_L_E_O_Plugins_AAAtrafficSystem_Source_TrafficRuntime_Public_TrafficKinematicFollower_h__Script_TrafficRuntime_2579306333{
	TEXT("/Script/TrafficRuntime"),
	Z_CompiledInDeferFile_FID_Users_rllax_L_E_O_Plugins_AAAtrafficSystem_Source_TrafficRuntime_Public_TrafficKinematicFollower_h__Script_TrafficRuntime_Statics::ClassInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Users_rllax_L_E_O_Plugins_AAAtrafficSystem_Source_TrafficRuntime_Public_TrafficKinematicFollower_h__Script_TrafficRuntime_Statics::ClassInfo),
	Z_CompiledInDeferFile_FID_Users_rllax_L_E_O_Plugins_AAAtrafficSystem_Source_TrafficRuntime_Public_TrafficKinematicFollower_h__Script_TrafficRuntime_Statics::ScriptStructInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Users_rllax_L_E_O_Plugins_AAAtrafficSystem_Source_TrafficRuntime_Public_TrafficKinematicFollower_h__Script_TrafficRuntime_Statics::ScriptStructInfo),
	Z_CompiledInDeferFile_FID_Users_rllax_L_E_O_Plugins_AAAtrafficSystem_Source_TrafficRuntime_Public_TrafficKinematicFollower_h__Script_TrafficRuntime_Statics::EnumInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Users_rllax_L_E_O_Plugins_AAAtrafficSystem_Source_TrafficRuntime_Public_TrafficKinematicFollower_h__Script_TrafficRuntime_Statics::EnumInfo),
};
// ********** End Registration *********************************************************************

PRAGMA_ENABLE_DEPRECATION_WARNINGS
