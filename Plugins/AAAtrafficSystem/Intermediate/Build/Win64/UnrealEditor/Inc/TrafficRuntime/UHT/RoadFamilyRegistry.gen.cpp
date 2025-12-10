// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "RoadFamilyRegistry.h"
#include "TrafficLaneCalibration.h"
#include "TrafficRoadFamilySettings.h"
#include "UObject/SoftObjectPath.h"

PRAGMA_DISABLE_DEPRECATION_WARNINGS
static_assert(!UE_WITH_CONSTINIT_UOBJECT, "This generated code can only be compiled with !UE_WITH_CONSTINIT_OBJECT");
void EmptyLinkFunctionForGeneratedCodeRoadFamilyRegistry() {}

// ********** Begin Cross Module References ********************************************************
COREUOBJECT_API UClass* Z_Construct_UClass_UObject();
COREUOBJECT_API UScriptStruct* Z_Construct_UScriptStruct_FGuid();
COREUOBJECT_API UScriptStruct* Z_Construct_UScriptStruct_FSoftClassPath();
TRAFFICRUNTIME_API UClass* Z_Construct_UClass_URoadFamilyRegistry();
TRAFFICRUNTIME_API UClass* Z_Construct_UClass_URoadFamilyRegistry_NoRegister();
TRAFFICRUNTIME_API UScriptStruct* Z_Construct_UScriptStruct_FRoadFamilyDefinition();
TRAFFICRUNTIME_API UScriptStruct* Z_Construct_UScriptStruct_FRoadFamilyInfo();
TRAFFICRUNTIME_API UScriptStruct* Z_Construct_UScriptStruct_FTrafficLaneFamilyCalibration();
UPackage* Z_Construct_UPackage__Script_TrafficRuntime();
// ********** End Cross Module References **********************************************************

// ********** Begin ScriptStruct FRoadFamilyInfo ***************************************************
struct Z_Construct_UScriptStruct_FRoadFamilyInfo_Statics
{
	static inline consteval int32 GetStructSize() { return sizeof(FRoadFamilyInfo); }
	static inline consteval int16 GetStructAlignment() { return alignof(FRoadFamilyInfo); }
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Struct_MetaDataParams[] = {
		{ "BlueprintType", "true" },
		{ "ModuleRelativePath", "Public/RoadFamilyRegistry.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_FamilyId_MetaData[] = {
		{ "Category", "Traffic" },
		{ "ModuleRelativePath", "Public/RoadFamilyRegistry.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_DisplayName_MetaData[] = {
		{ "Category", "Traffic" },
		{ "ModuleRelativePath", "Public/RoadFamilyRegistry.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_RoadClassPath_MetaData[] = {
		{ "Category", "Traffic" },
		{ "ModuleRelativePath", "Public/RoadFamilyRegistry.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_FamilyDefinition_MetaData[] = {
		{ "Category", "Traffic" },
		{ "ModuleRelativePath", "Public/RoadFamilyRegistry.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_CalibrationData_MetaData[] = {
		{ "Category", "Traffic" },
		{ "ModuleRelativePath", "Public/RoadFamilyRegistry.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_bIsCalibrated_MetaData[] = {
		{ "Category", "Traffic" },
		{ "ModuleRelativePath", "Public/RoadFamilyRegistry.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_BackupCalibration_MetaData[] = {
#if !UE_BUILD_SHIPPING
		{ "Comment", "// Last calibration snapshot (backup) for quick restore.\n" },
#endif
		{ "ModuleRelativePath", "Public/RoadFamilyRegistry.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Last calibration snapshot (backup) for quick restore." },
#endif
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_bHasBackupCalibration_MetaData[] = {
		{ "ModuleRelativePath", "Public/RoadFamilyRegistry.h" },
	};
#endif // WITH_METADATA

// ********** Begin ScriptStruct FRoadFamilyInfo constinit property declarations *******************
	static const UECodeGen_Private::FStructPropertyParams NewProp_FamilyId;
	static const UECodeGen_Private::FStrPropertyParams NewProp_DisplayName;
	static const UECodeGen_Private::FStructPropertyParams NewProp_RoadClassPath;
	static const UECodeGen_Private::FStructPropertyParams NewProp_FamilyDefinition;
	static const UECodeGen_Private::FStructPropertyParams NewProp_CalibrationData;
	static void NewProp_bIsCalibrated_SetBit(void* Obj);
	static const UECodeGen_Private::FBoolPropertyParams NewProp_bIsCalibrated;
	static const UECodeGen_Private::FStructPropertyParams NewProp_BackupCalibration;
	static void NewProp_bHasBackupCalibration_SetBit(void* Obj);
	static const UECodeGen_Private::FBoolPropertyParams NewProp_bHasBackupCalibration;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
// ********** End ScriptStruct FRoadFamilyInfo constinit property declarations *********************
	static void* NewStructOps()
	{
		return (UScriptStruct::ICppStructOps*)new UScriptStruct::TCppStructOps<FRoadFamilyInfo>();
	}
	static const UECodeGen_Private::FStructParams StructParams;
}; // struct Z_Construct_UScriptStruct_FRoadFamilyInfo_Statics
static FStructRegistrationInfo Z_Registration_Info_UScriptStruct_FRoadFamilyInfo;
class UScriptStruct* FRoadFamilyInfo::StaticStruct()
{
	if (!Z_Registration_Info_UScriptStruct_FRoadFamilyInfo.OuterSingleton)
	{
		Z_Registration_Info_UScriptStruct_FRoadFamilyInfo.OuterSingleton = GetStaticStruct(Z_Construct_UScriptStruct_FRoadFamilyInfo, (UObject*)Z_Construct_UPackage__Script_TrafficRuntime(), TEXT("RoadFamilyInfo"));
	}
	return Z_Registration_Info_UScriptStruct_FRoadFamilyInfo.OuterSingleton;
	}

// ********** Begin ScriptStruct FRoadFamilyInfo Property Definitions ******************************
const UECodeGen_Private::FStructPropertyParams Z_Construct_UScriptStruct_FRoadFamilyInfo_Statics::NewProp_FamilyId = { "FamilyId", nullptr, (EPropertyFlags)0x0010000000004001, UECodeGen_Private::EPropertyGenFlags::Struct, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(FRoadFamilyInfo, FamilyId), Z_Construct_UScriptStruct_FGuid, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_FamilyId_MetaData), NewProp_FamilyId_MetaData) };
const UECodeGen_Private::FStrPropertyParams Z_Construct_UScriptStruct_FRoadFamilyInfo_Statics::NewProp_DisplayName = { "DisplayName", nullptr, (EPropertyFlags)0x0010000000004001, UECodeGen_Private::EPropertyGenFlags::Str, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(FRoadFamilyInfo, DisplayName), METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_DisplayName_MetaData), NewProp_DisplayName_MetaData) };
const UECodeGen_Private::FStructPropertyParams Z_Construct_UScriptStruct_FRoadFamilyInfo_Statics::NewProp_RoadClassPath = { "RoadClassPath", nullptr, (EPropertyFlags)0x0010000000004001, UECodeGen_Private::EPropertyGenFlags::Struct, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(FRoadFamilyInfo, RoadClassPath), Z_Construct_UScriptStruct_FSoftClassPath, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_RoadClassPath_MetaData), NewProp_RoadClassPath_MetaData) }; // 3467803280
const UECodeGen_Private::FStructPropertyParams Z_Construct_UScriptStruct_FRoadFamilyInfo_Statics::NewProp_FamilyDefinition = { "FamilyDefinition", nullptr, (EPropertyFlags)0x0010000000004001, UECodeGen_Private::EPropertyGenFlags::Struct, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(FRoadFamilyInfo, FamilyDefinition), Z_Construct_UScriptStruct_FRoadFamilyDefinition, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_FamilyDefinition_MetaData), NewProp_FamilyDefinition_MetaData) }; // 3947480440
const UECodeGen_Private::FStructPropertyParams Z_Construct_UScriptStruct_FRoadFamilyInfo_Statics::NewProp_CalibrationData = { "CalibrationData", nullptr, (EPropertyFlags)0x0010000000004001, UECodeGen_Private::EPropertyGenFlags::Struct, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(FRoadFamilyInfo, CalibrationData), Z_Construct_UScriptStruct_FTrafficLaneFamilyCalibration, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_CalibrationData_MetaData), NewProp_CalibrationData_MetaData) }; // 1801600758
void Z_Construct_UScriptStruct_FRoadFamilyInfo_Statics::NewProp_bIsCalibrated_SetBit(void* Obj)
{
	((FRoadFamilyInfo*)Obj)->bIsCalibrated = 1;
}
const UECodeGen_Private::FBoolPropertyParams Z_Construct_UScriptStruct_FRoadFamilyInfo_Statics::NewProp_bIsCalibrated = { "bIsCalibrated", nullptr, (EPropertyFlags)0x0010000000004001, UECodeGen_Private::EPropertyGenFlags::Bool | UECodeGen_Private::EPropertyGenFlags::NativeBool, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, sizeof(bool), sizeof(FRoadFamilyInfo), &Z_Construct_UScriptStruct_FRoadFamilyInfo_Statics::NewProp_bIsCalibrated_SetBit, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_bIsCalibrated_MetaData), NewProp_bIsCalibrated_MetaData) };
const UECodeGen_Private::FStructPropertyParams Z_Construct_UScriptStruct_FRoadFamilyInfo_Statics::NewProp_BackupCalibration = { "BackupCalibration", nullptr, (EPropertyFlags)0x0010000000000000, UECodeGen_Private::EPropertyGenFlags::Struct, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(FRoadFamilyInfo, BackupCalibration), Z_Construct_UScriptStruct_FTrafficLaneFamilyCalibration, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_BackupCalibration_MetaData), NewProp_BackupCalibration_MetaData) }; // 1801600758
void Z_Construct_UScriptStruct_FRoadFamilyInfo_Statics::NewProp_bHasBackupCalibration_SetBit(void* Obj)
{
	((FRoadFamilyInfo*)Obj)->bHasBackupCalibration = 1;
}
const UECodeGen_Private::FBoolPropertyParams Z_Construct_UScriptStruct_FRoadFamilyInfo_Statics::NewProp_bHasBackupCalibration = { "bHasBackupCalibration", nullptr, (EPropertyFlags)0x0010000000000000, UECodeGen_Private::EPropertyGenFlags::Bool | UECodeGen_Private::EPropertyGenFlags::NativeBool, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, sizeof(bool), sizeof(FRoadFamilyInfo), &Z_Construct_UScriptStruct_FRoadFamilyInfo_Statics::NewProp_bHasBackupCalibration_SetBit, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_bHasBackupCalibration_MetaData), NewProp_bHasBackupCalibration_MetaData) };
const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UScriptStruct_FRoadFamilyInfo_Statics::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FRoadFamilyInfo_Statics::NewProp_FamilyId,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FRoadFamilyInfo_Statics::NewProp_DisplayName,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FRoadFamilyInfo_Statics::NewProp_RoadClassPath,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FRoadFamilyInfo_Statics::NewProp_FamilyDefinition,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FRoadFamilyInfo_Statics::NewProp_CalibrationData,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FRoadFamilyInfo_Statics::NewProp_bIsCalibrated,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FRoadFamilyInfo_Statics::NewProp_BackupCalibration,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FRoadFamilyInfo_Statics::NewProp_bHasBackupCalibration,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FRoadFamilyInfo_Statics::PropPointers) < 2048);
// ********** End ScriptStruct FRoadFamilyInfo Property Definitions ********************************
const UECodeGen_Private::FStructParams Z_Construct_UScriptStruct_FRoadFamilyInfo_Statics::StructParams = {
	(UObject* (*)())Z_Construct_UPackage__Script_TrafficRuntime,
	nullptr,
	&NewStructOps,
	"RoadFamilyInfo",
	Z_Construct_UScriptStruct_FRoadFamilyInfo_Statics::PropPointers,
	UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FRoadFamilyInfo_Statics::PropPointers),
	sizeof(FRoadFamilyInfo),
	alignof(FRoadFamilyInfo),
	RF_Public|RF_Transient|RF_MarkAsNative,
	EStructFlags(0x00000201),
	METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FRoadFamilyInfo_Statics::Struct_MetaDataParams), Z_Construct_UScriptStruct_FRoadFamilyInfo_Statics::Struct_MetaDataParams)
};
UScriptStruct* Z_Construct_UScriptStruct_FRoadFamilyInfo()
{
	if (!Z_Registration_Info_UScriptStruct_FRoadFamilyInfo.InnerSingleton)
	{
		UECodeGen_Private::ConstructUScriptStruct(Z_Registration_Info_UScriptStruct_FRoadFamilyInfo.InnerSingleton, Z_Construct_UScriptStruct_FRoadFamilyInfo_Statics::StructParams);
	}
	return CastChecked<UScriptStruct>(Z_Registration_Info_UScriptStruct_FRoadFamilyInfo.InnerSingleton);
}
// ********** End ScriptStruct FRoadFamilyInfo *****************************************************

// ********** Begin Class URoadFamilyRegistry ******************************************************
FClassRegistrationInfo Z_Registration_Info_UClass_URoadFamilyRegistry;
UClass* URoadFamilyRegistry::GetPrivateStaticClass()
{
	using TClass = URoadFamilyRegistry;
	if (!Z_Registration_Info_UClass_URoadFamilyRegistry.InnerSingleton)
	{
		GetPrivateStaticClassBody(
			TClass::StaticPackage(),
			TEXT("RoadFamilyRegistry"),
			Z_Registration_Info_UClass_URoadFamilyRegistry.InnerSingleton,
			StaticRegisterNativesURoadFamilyRegistry,
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
	return Z_Registration_Info_UClass_URoadFamilyRegistry.InnerSingleton;
}
UClass* Z_Construct_UClass_URoadFamilyRegistry_NoRegister()
{
	return URoadFamilyRegistry::GetPrivateStaticClass();
}
struct Z_Construct_UClass_URoadFamilyRegistry_Statics
{
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Class_MetaDataParams[] = {
#if !UE_BUILD_SHIPPING
		{ "Comment", "/**\n * Maintains a per-road-class registry of discovered road families for editor automation.\n * Families are persisted in editor user settings and keyed by the road actor class.\n */" },
#endif
		{ "IncludePath", "RoadFamilyRegistry.h" },
		{ "ModuleRelativePath", "Public/RoadFamilyRegistry.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Maintains a per-road-class registry of discovered road families for editor automation.\nFamilies are persisted in editor user settings and keyed by the road actor class." },
#endif
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_Families_MetaData[] = {
		{ "ModuleRelativePath", "Public/RoadFamilyRegistry.h" },
	};
#endif // WITH_METADATA

// ********** Begin Class URoadFamilyRegistry constinit property declarations **********************
	static const UECodeGen_Private::FStructPropertyParams NewProp_Families_Inner;
	static const UECodeGen_Private::FArrayPropertyParams NewProp_Families;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
// ********** End Class URoadFamilyRegistry constinit property declarations ************************
	static UObject* (*const DependentSingletons[])();
	static constexpr FCppClassTypeInfoStatic StaticCppClassTypeInfo = {
		TCppClassTypeTraits<URoadFamilyRegistry>::IsAbstract,
	};
	static const UECodeGen_Private::FClassParams ClassParams;
}; // struct Z_Construct_UClass_URoadFamilyRegistry_Statics

// ********** Begin Class URoadFamilyRegistry Property Definitions *********************************
const UECodeGen_Private::FStructPropertyParams Z_Construct_UClass_URoadFamilyRegistry_Statics::NewProp_Families_Inner = { "Families", nullptr, (EPropertyFlags)0x0000000000004000, UECodeGen_Private::EPropertyGenFlags::Struct, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, 0, Z_Construct_UScriptStruct_FRoadFamilyInfo, METADATA_PARAMS(0, nullptr) }; // 994629464
const UECodeGen_Private::FArrayPropertyParams Z_Construct_UClass_URoadFamilyRegistry_Statics::NewProp_Families = { "Families", nullptr, (EPropertyFlags)0x0040000000004000, UECodeGen_Private::EPropertyGenFlags::Array, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(URoadFamilyRegistry, Families), EArrayPropertyFlags::None, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_Families_MetaData), NewProp_Families_MetaData) }; // 994629464
const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UClass_URoadFamilyRegistry_Statics::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_URoadFamilyRegistry_Statics::NewProp_Families_Inner,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_URoadFamilyRegistry_Statics::NewProp_Families,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_URoadFamilyRegistry_Statics::PropPointers) < 2048);
// ********** End Class URoadFamilyRegistry Property Definitions ***********************************
UObject* (*const Z_Construct_UClass_URoadFamilyRegistry_Statics::DependentSingletons[])() = {
	(UObject* (*)())Z_Construct_UClass_UObject,
	(UObject* (*)())Z_Construct_UPackage__Script_TrafficRuntime,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_URoadFamilyRegistry_Statics::DependentSingletons) < 16);
const UECodeGen_Private::FClassParams Z_Construct_UClass_URoadFamilyRegistry_Statics::ClassParams = {
	&URoadFamilyRegistry::StaticClass,
	"EditorPerProjectUserSettings",
	&StaticCppClassTypeInfo,
	DependentSingletons,
	nullptr,
	Z_Construct_UClass_URoadFamilyRegistry_Statics::PropPointers,
	nullptr,
	UE_ARRAY_COUNT(DependentSingletons),
	0,
	UE_ARRAY_COUNT(Z_Construct_UClass_URoadFamilyRegistry_Statics::PropPointers),
	0,
	0x001000A4u,
	METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UClass_URoadFamilyRegistry_Statics::Class_MetaDataParams), Z_Construct_UClass_URoadFamilyRegistry_Statics::Class_MetaDataParams)
};
void URoadFamilyRegistry::StaticRegisterNativesURoadFamilyRegistry()
{
}
UClass* Z_Construct_UClass_URoadFamilyRegistry()
{
	if (!Z_Registration_Info_UClass_URoadFamilyRegistry.OuterSingleton)
	{
		UECodeGen_Private::ConstructUClass(Z_Registration_Info_UClass_URoadFamilyRegistry.OuterSingleton, Z_Construct_UClass_URoadFamilyRegistry_Statics::ClassParams);
	}
	return Z_Registration_Info_UClass_URoadFamilyRegistry.OuterSingleton;
}
URoadFamilyRegistry::URoadFamilyRegistry(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
DEFINE_VTABLE_PTR_HELPER_CTOR_NS(, URoadFamilyRegistry);
URoadFamilyRegistry::~URoadFamilyRegistry() {}
// ********** End Class URoadFamilyRegistry ********************************************************

// ********** Begin Registration *******************************************************************
struct Z_CompiledInDeferFile_FID_Users_rllax_L_E_O_Plugins_AAAtrafficSystem_Source_TrafficRuntime_Public_RoadFamilyRegistry_h__Script_TrafficRuntime_Statics
{
	static constexpr FStructRegisterCompiledInInfo ScriptStructInfo[] = {
		{ FRoadFamilyInfo::StaticStruct, Z_Construct_UScriptStruct_FRoadFamilyInfo_Statics::NewStructOps, TEXT("RoadFamilyInfo"),&Z_Registration_Info_UScriptStruct_FRoadFamilyInfo, CONSTRUCT_RELOAD_VERSION_INFO(FStructReloadVersionInfo, sizeof(FRoadFamilyInfo), 994629464U) },
	};
	static constexpr FClassRegisterCompiledInInfo ClassInfo[] = {
		{ Z_Construct_UClass_URoadFamilyRegistry, URoadFamilyRegistry::StaticClass, TEXT("URoadFamilyRegistry"), &Z_Registration_Info_UClass_URoadFamilyRegistry, CONSTRUCT_RELOAD_VERSION_INFO(FClassReloadVersionInfo, sizeof(URoadFamilyRegistry), 776711177U) },
	};
}; // Z_CompiledInDeferFile_FID_Users_rllax_L_E_O_Plugins_AAAtrafficSystem_Source_TrafficRuntime_Public_RoadFamilyRegistry_h__Script_TrafficRuntime_Statics 
static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_rllax_L_E_O_Plugins_AAAtrafficSystem_Source_TrafficRuntime_Public_RoadFamilyRegistry_h__Script_TrafficRuntime_1802167768{
	TEXT("/Script/TrafficRuntime"),
	Z_CompiledInDeferFile_FID_Users_rllax_L_E_O_Plugins_AAAtrafficSystem_Source_TrafficRuntime_Public_RoadFamilyRegistry_h__Script_TrafficRuntime_Statics::ClassInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Users_rllax_L_E_O_Plugins_AAAtrafficSystem_Source_TrafficRuntime_Public_RoadFamilyRegistry_h__Script_TrafficRuntime_Statics::ClassInfo),
	Z_CompiledInDeferFile_FID_Users_rllax_L_E_O_Plugins_AAAtrafficSystem_Source_TrafficRuntime_Public_RoadFamilyRegistry_h__Script_TrafficRuntime_Statics::ScriptStructInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Users_rllax_L_E_O_Plugins_AAAtrafficSystem_Source_TrafficRuntime_Public_RoadFamilyRegistry_h__Script_TrafficRuntime_Statics::ScriptStructInfo),
	nullptr, 0,
};
// ********** End Registration *********************************************************************

PRAGMA_ENABLE_DEPRECATION_WARNINGS
