// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "TrafficSystemEditorSubsystem.h"

PRAGMA_DISABLE_DEPRECATION_WARNINGS
static_assert(!UE_WITH_CONSTINIT_UOBJECT, "This generated code can only be compiled with !UE_WITH_CONSTINIT_OBJECT");
void EmptyLinkFunctionForGeneratedCodeTrafficSystemEditorSubsystem() {}

// ********** Begin Cross Module References ********************************************************
COREUOBJECT_API UScriptStruct* Z_Construct_UScriptStruct_FGuid();
COREUOBJECT_API UScriptStruct* Z_Construct_UScriptStruct_FVector();
EDITORSUBSYSTEM_API UClass* Z_Construct_UClass_UEditorSubsystem();
ENGINE_API UClass* Z_Construct_UClass_AActor_NoRegister();
PROCEDURALMESHCOMPONENT_API UClass* Z_Construct_UClass_UProceduralMeshComponent_NoRegister();
TRAFFICEDITOR_API UClass* Z_Construct_UClass_ALaneCalibrationOverlayActor_NoRegister();
TRAFFICEDITOR_API UClass* Z_Construct_UClass_UTrafficSystemEditorSubsystem();
TRAFFICEDITOR_API UClass* Z_Construct_UClass_UTrafficSystemEditorSubsystem_NoRegister();
TRAFFICEDITOR_API UScriptStruct* Z_Construct_UScriptStruct_FCalibrationSnippet();
UPackage* Z_Construct_UPackage__Script_TrafficEditor();
// ********** End Cross Module References **********************************************************

// ********** Begin ScriptStruct FCalibrationSnippet ***********************************************
struct Z_Construct_UScriptStruct_FCalibrationSnippet_Statics
{
	static inline consteval int32 GetStructSize() { return sizeof(FCalibrationSnippet); }
	static inline consteval int16 GetStructAlignment() { return alignof(FCalibrationSnippet); }
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Struct_MetaDataParams[] = {
		{ "ModuleRelativePath", "Public/TrafficSystemEditorSubsystem.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_SnippetPoints_MetaData[] = {
		{ "ModuleRelativePath", "Public/TrafficSystemEditorSubsystem.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_SnippetLength_MetaData[] = {
		{ "ModuleRelativePath", "Public/TrafficSystemEditorSubsystem.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_StartPointIndex_MetaData[] = {
		{ "ModuleRelativePath", "Public/TrafficSystemEditorSubsystem.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_EndPointIndex_MetaData[] = {
		{ "ModuleRelativePath", "Public/TrafficSystemEditorSubsystem.h" },
	};
#endif // WITH_METADATA

// ********** Begin ScriptStruct FCalibrationSnippet constinit property declarations ***************
	static const UECodeGen_Private::FStructPropertyParams NewProp_SnippetPoints_Inner;
	static const UECodeGen_Private::FArrayPropertyParams NewProp_SnippetPoints;
	static const UECodeGen_Private::FFloatPropertyParams NewProp_SnippetLength;
	static const UECodeGen_Private::FIntPropertyParams NewProp_StartPointIndex;
	static const UECodeGen_Private::FIntPropertyParams NewProp_EndPointIndex;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
// ********** End ScriptStruct FCalibrationSnippet constinit property declarations *****************
	static void* NewStructOps()
	{
		return (UScriptStruct::ICppStructOps*)new UScriptStruct::TCppStructOps<FCalibrationSnippet>();
	}
	static const UECodeGen_Private::FStructParams StructParams;
}; // struct Z_Construct_UScriptStruct_FCalibrationSnippet_Statics
static FStructRegistrationInfo Z_Registration_Info_UScriptStruct_FCalibrationSnippet;
class UScriptStruct* FCalibrationSnippet::StaticStruct()
{
	if (!Z_Registration_Info_UScriptStruct_FCalibrationSnippet.OuterSingleton)
	{
		Z_Registration_Info_UScriptStruct_FCalibrationSnippet.OuterSingleton = GetStaticStruct(Z_Construct_UScriptStruct_FCalibrationSnippet, (UObject*)Z_Construct_UPackage__Script_TrafficEditor(), TEXT("CalibrationSnippet"));
	}
	return Z_Registration_Info_UScriptStruct_FCalibrationSnippet.OuterSingleton;
	}

// ********** Begin ScriptStruct FCalibrationSnippet Property Definitions **************************
const UECodeGen_Private::FStructPropertyParams Z_Construct_UScriptStruct_FCalibrationSnippet_Statics::NewProp_SnippetPoints_Inner = { "SnippetPoints", nullptr, (EPropertyFlags)0x0000000000000000, UECodeGen_Private::EPropertyGenFlags::Struct, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, 0, Z_Construct_UScriptStruct_FVector, METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FArrayPropertyParams Z_Construct_UScriptStruct_FCalibrationSnippet_Statics::NewProp_SnippetPoints = { "SnippetPoints", nullptr, (EPropertyFlags)0x0010000000000000, UECodeGen_Private::EPropertyGenFlags::Array, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(FCalibrationSnippet, SnippetPoints), EArrayPropertyFlags::None, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_SnippetPoints_MetaData), NewProp_SnippetPoints_MetaData) };
const UECodeGen_Private::FFloatPropertyParams Z_Construct_UScriptStruct_FCalibrationSnippet_Statics::NewProp_SnippetLength = { "SnippetLength", nullptr, (EPropertyFlags)0x0010000000000000, UECodeGen_Private::EPropertyGenFlags::Float, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(FCalibrationSnippet, SnippetLength), METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_SnippetLength_MetaData), NewProp_SnippetLength_MetaData) };
const UECodeGen_Private::FIntPropertyParams Z_Construct_UScriptStruct_FCalibrationSnippet_Statics::NewProp_StartPointIndex = { "StartPointIndex", nullptr, (EPropertyFlags)0x0010000000000000, UECodeGen_Private::EPropertyGenFlags::Int, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(FCalibrationSnippet, StartPointIndex), METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_StartPointIndex_MetaData), NewProp_StartPointIndex_MetaData) };
const UECodeGen_Private::FIntPropertyParams Z_Construct_UScriptStruct_FCalibrationSnippet_Statics::NewProp_EndPointIndex = { "EndPointIndex", nullptr, (EPropertyFlags)0x0010000000000000, UECodeGen_Private::EPropertyGenFlags::Int, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(FCalibrationSnippet, EndPointIndex), METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_EndPointIndex_MetaData), NewProp_EndPointIndex_MetaData) };
const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UScriptStruct_FCalibrationSnippet_Statics::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FCalibrationSnippet_Statics::NewProp_SnippetPoints_Inner,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FCalibrationSnippet_Statics::NewProp_SnippetPoints,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FCalibrationSnippet_Statics::NewProp_SnippetLength,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FCalibrationSnippet_Statics::NewProp_StartPointIndex,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FCalibrationSnippet_Statics::NewProp_EndPointIndex,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FCalibrationSnippet_Statics::PropPointers) < 2048);
// ********** End ScriptStruct FCalibrationSnippet Property Definitions ****************************
const UECodeGen_Private::FStructParams Z_Construct_UScriptStruct_FCalibrationSnippet_Statics::StructParams = {
	(UObject* (*)())Z_Construct_UPackage__Script_TrafficEditor,
	nullptr,
	&NewStructOps,
	"CalibrationSnippet",
	Z_Construct_UScriptStruct_FCalibrationSnippet_Statics::PropPointers,
	UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FCalibrationSnippet_Statics::PropPointers),
	sizeof(FCalibrationSnippet),
	alignof(FCalibrationSnippet),
	RF_Public|RF_Transient|RF_MarkAsNative,
	EStructFlags(0x00000001),
	METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FCalibrationSnippet_Statics::Struct_MetaDataParams), Z_Construct_UScriptStruct_FCalibrationSnippet_Statics::Struct_MetaDataParams)
};
UScriptStruct* Z_Construct_UScriptStruct_FCalibrationSnippet()
{
	if (!Z_Registration_Info_UScriptStruct_FCalibrationSnippet.InnerSingleton)
	{
		UECodeGen_Private::ConstructUScriptStruct(Z_Registration_Info_UScriptStruct_FCalibrationSnippet.InnerSingleton, Z_Construct_UScriptStruct_FCalibrationSnippet_Statics::StructParams);
	}
	return CastChecked<UScriptStruct>(Z_Registration_Info_UScriptStruct_FCalibrationSnippet.InnerSingleton);
}
// ********** End ScriptStruct FCalibrationSnippet *************************************************

// ********** Begin Class UTrafficSystemEditorSubsystem Function Editor_BakeCalibrationForActiveFamily 
struct Z_Construct_UFunction_UTrafficSystemEditorSubsystem_Editor_BakeCalibrationForActiveFamily_Statics
{
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[] = {
		{ "CallInEditor", "true" },
		{ "ModuleRelativePath", "Public/TrafficSystemEditorSubsystem.h" },
	};
#endif // WITH_METADATA

// ********** Begin Function Editor_BakeCalibrationForActiveFamily constinit property declarations *
// ********** End Function Editor_BakeCalibrationForActiveFamily constinit property declarations ***
	static const UECodeGen_Private::FFunctionParams FuncParams;
};
const UECodeGen_Private::FFunctionParams Z_Construct_UFunction_UTrafficSystemEditorSubsystem_Editor_BakeCalibrationForActiveFamily_Statics::FuncParams = { { (UObject*(*)())Z_Construct_UClass_UTrafficSystemEditorSubsystem, nullptr, "Editor_BakeCalibrationForActiveFamily", 	nullptr, 
	0, 
0,
RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x00020401, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UFunction_UTrafficSystemEditorSubsystem_Editor_BakeCalibrationForActiveFamily_Statics::Function_MetaDataParams), Z_Construct_UFunction_UTrafficSystemEditorSubsystem_Editor_BakeCalibrationForActiveFamily_Statics::Function_MetaDataParams)},  };
UFunction* Z_Construct_UFunction_UTrafficSystemEditorSubsystem_Editor_BakeCalibrationForActiveFamily()
{
	static UFunction* ReturnFunction = nullptr;
	if (!ReturnFunction)
	{
		UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UFunction_UTrafficSystemEditorSubsystem_Editor_BakeCalibrationForActiveFamily_Statics::FuncParams);
	}
	return ReturnFunction;
}
DEFINE_FUNCTION(UTrafficSystemEditorSubsystem::execEditor_BakeCalibrationForActiveFamily)
{
	P_FINISH;
	P_NATIVE_BEGIN;
	P_THIS->Editor_BakeCalibrationForActiveFamily();
	P_NATIVE_END;
}
// ********** End Class UTrafficSystemEditorSubsystem Function Editor_BakeCalibrationForActiveFamily 

// ********** Begin Class UTrafficSystemEditorSubsystem Function Editor_BeginCalibrationForFamily **
struct Z_Construct_UFunction_UTrafficSystemEditorSubsystem_Editor_BeginCalibrationForFamily_Statics
{
	struct TrafficSystemEditorSubsystem_eventEditor_BeginCalibrationForFamily_Parms
	{
		FGuid FamilyId;
	};
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[] = {
		{ "CallInEditor", "true" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "// Calibration session management\n" },
#endif
		{ "ModuleRelativePath", "Public/TrafficSystemEditorSubsystem.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Calibration session management" },
#endif
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_FamilyId_MetaData[] = {
		{ "NativeConst", "" },
	};
#endif // WITH_METADATA

// ********** Begin Function Editor_BeginCalibrationForFamily constinit property declarations ******
	static const UECodeGen_Private::FStructPropertyParams NewProp_FamilyId;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
// ********** End Function Editor_BeginCalibrationForFamily constinit property declarations ********
	static const UECodeGen_Private::FFunctionParams FuncParams;
};

// ********** Begin Function Editor_BeginCalibrationForFamily Property Definitions *****************
const UECodeGen_Private::FStructPropertyParams Z_Construct_UFunction_UTrafficSystemEditorSubsystem_Editor_BeginCalibrationForFamily_Statics::NewProp_FamilyId = { "FamilyId", nullptr, (EPropertyFlags)0x0010000008000182, UECodeGen_Private::EPropertyGenFlags::Struct, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(TrafficSystemEditorSubsystem_eventEditor_BeginCalibrationForFamily_Parms, FamilyId), Z_Construct_UScriptStruct_FGuid, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_FamilyId_MetaData), NewProp_FamilyId_MetaData) };
const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UFunction_UTrafficSystemEditorSubsystem_Editor_BeginCalibrationForFamily_Statics::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UTrafficSystemEditorSubsystem_Editor_BeginCalibrationForFamily_Statics::NewProp_FamilyId,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UFunction_UTrafficSystemEditorSubsystem_Editor_BeginCalibrationForFamily_Statics::PropPointers) < 2048);
// ********** End Function Editor_BeginCalibrationForFamily Property Definitions *******************
const UECodeGen_Private::FFunctionParams Z_Construct_UFunction_UTrafficSystemEditorSubsystem_Editor_BeginCalibrationForFamily_Statics::FuncParams = { { (UObject*(*)())Z_Construct_UClass_UTrafficSystemEditorSubsystem, nullptr, "Editor_BeginCalibrationForFamily", 	Z_Construct_UFunction_UTrafficSystemEditorSubsystem_Editor_BeginCalibrationForFamily_Statics::PropPointers, 
	UE_ARRAY_COUNT(Z_Construct_UFunction_UTrafficSystemEditorSubsystem_Editor_BeginCalibrationForFamily_Statics::PropPointers), 
sizeof(Z_Construct_UFunction_UTrafficSystemEditorSubsystem_Editor_BeginCalibrationForFamily_Statics::TrafficSystemEditorSubsystem_eventEditor_BeginCalibrationForFamily_Parms),
RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x00C20401, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UFunction_UTrafficSystemEditorSubsystem_Editor_BeginCalibrationForFamily_Statics::Function_MetaDataParams), Z_Construct_UFunction_UTrafficSystemEditorSubsystem_Editor_BeginCalibrationForFamily_Statics::Function_MetaDataParams)},  };
static_assert(sizeof(Z_Construct_UFunction_UTrafficSystemEditorSubsystem_Editor_BeginCalibrationForFamily_Statics::TrafficSystemEditorSubsystem_eventEditor_BeginCalibrationForFamily_Parms) < MAX_uint16);
UFunction* Z_Construct_UFunction_UTrafficSystemEditorSubsystem_Editor_BeginCalibrationForFamily()
{
	static UFunction* ReturnFunction = nullptr;
	if (!ReturnFunction)
	{
		UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UFunction_UTrafficSystemEditorSubsystem_Editor_BeginCalibrationForFamily_Statics::FuncParams);
	}
	return ReturnFunction;
}
DEFINE_FUNCTION(UTrafficSystemEditorSubsystem::execEditor_BeginCalibrationForFamily)
{
	P_GET_STRUCT_REF(FGuid,Z_Param_Out_FamilyId);
	P_FINISH;
	P_NATIVE_BEGIN;
	P_THIS->Editor_BeginCalibrationForFamily(Z_Param_Out_FamilyId);
	P_NATIVE_END;
}
// ********** End Class UTrafficSystemEditorSubsystem Function Editor_BeginCalibrationForFamily ****

// ********** Begin Class UTrafficSystemEditorSubsystem ********************************************
FClassRegistrationInfo Z_Registration_Info_UClass_UTrafficSystemEditorSubsystem;
UClass* UTrafficSystemEditorSubsystem::GetPrivateStaticClass()
{
	using TClass = UTrafficSystemEditorSubsystem;
	if (!Z_Registration_Info_UClass_UTrafficSystemEditorSubsystem.InnerSingleton)
	{
		GetPrivateStaticClassBody(
			TClass::StaticPackage(),
			TEXT("TrafficSystemEditorSubsystem"),
			Z_Registration_Info_UClass_UTrafficSystemEditorSubsystem.InnerSingleton,
			StaticRegisterNativesUTrafficSystemEditorSubsystem,
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
	return Z_Registration_Info_UClass_UTrafficSystemEditorSubsystem.InnerSingleton;
}
UClass* Z_Construct_UClass_UTrafficSystemEditorSubsystem_NoRegister()
{
	return UTrafficSystemEditorSubsystem::GetPrivateStaticClass();
}
struct Z_Construct_UClass_UTrafficSystemEditorSubsystem_Statics
{
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Class_MetaDataParams[] = {
		{ "IncludePath", "TrafficSystemEditorSubsystem.h" },
		{ "ModuleRelativePath", "Public/TrafficSystemEditorSubsystem.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_CalibrationOverlayActor_MetaData[] = {
		{ "ModuleRelativePath", "Public/TrafficSystemEditorSubsystem.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_ActiveCalibrationRoadActor_MetaData[] = {
		{ "ModuleRelativePath", "Public/TrafficSystemEditorSubsystem.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_ActiveFamilyId_MetaData[] = {
		{ "ModuleRelativePath", "Public/TrafficSystemEditorSubsystem.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_RoadLabRibbonMeshes_MetaData[] = {
		{ "EditInline", "true" },
		{ "ModuleRelativePath", "Public/TrafficSystemEditorSubsystem.h" },
	};
#endif // WITH_METADATA

// ********** Begin Class UTrafficSystemEditorSubsystem constinit property declarations ************
	static const UECodeGen_Private::FWeakObjectPropertyParams NewProp_CalibrationOverlayActor;
	static const UECodeGen_Private::FWeakObjectPropertyParams NewProp_ActiveCalibrationRoadActor;
	static const UECodeGen_Private::FStructPropertyParams NewProp_ActiveFamilyId;
	static const UECodeGen_Private::FObjectPropertyParams NewProp_RoadLabRibbonMeshes_Inner;
	static const UECodeGen_Private::FArrayPropertyParams NewProp_RoadLabRibbonMeshes;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
// ********** End Class UTrafficSystemEditorSubsystem constinit property declarations **************
	static constexpr UE::CodeGen::FClassNativeFunction Funcs[] = {
		{ .NameUTF8 = UTF8TEXT("Editor_BakeCalibrationForActiveFamily"), .Pointer = &UTrafficSystemEditorSubsystem::execEditor_BakeCalibrationForActiveFamily },
		{ .NameUTF8 = UTF8TEXT("Editor_BeginCalibrationForFamily"), .Pointer = &UTrafficSystemEditorSubsystem::execEditor_BeginCalibrationForFamily },
	};
	static UObject* (*const DependentSingletons[])();
	static constexpr FClassFunctionLinkInfo FuncInfo[] = {
		{ &Z_Construct_UFunction_UTrafficSystemEditorSubsystem_Editor_BakeCalibrationForActiveFamily, "Editor_BakeCalibrationForActiveFamily" }, // 1132299146
		{ &Z_Construct_UFunction_UTrafficSystemEditorSubsystem_Editor_BeginCalibrationForFamily, "Editor_BeginCalibrationForFamily" }, // 457308696
	};
	static_assert(UE_ARRAY_COUNT(FuncInfo) < 2048);
	static constexpr FCppClassTypeInfoStatic StaticCppClassTypeInfo = {
		TCppClassTypeTraits<UTrafficSystemEditorSubsystem>::IsAbstract,
	};
	static const UECodeGen_Private::FClassParams ClassParams;
}; // struct Z_Construct_UClass_UTrafficSystemEditorSubsystem_Statics

// ********** Begin Class UTrafficSystemEditorSubsystem Property Definitions ***********************
const UECodeGen_Private::FWeakObjectPropertyParams Z_Construct_UClass_UTrafficSystemEditorSubsystem_Statics::NewProp_CalibrationOverlayActor = { "CalibrationOverlayActor", nullptr, (EPropertyFlags)0x0044000000000000, UECodeGen_Private::EPropertyGenFlags::WeakObject, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(UTrafficSystemEditorSubsystem, CalibrationOverlayActor), Z_Construct_UClass_ALaneCalibrationOverlayActor_NoRegister, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_CalibrationOverlayActor_MetaData), NewProp_CalibrationOverlayActor_MetaData) };
const UECodeGen_Private::FWeakObjectPropertyParams Z_Construct_UClass_UTrafficSystemEditorSubsystem_Statics::NewProp_ActiveCalibrationRoadActor = { "ActiveCalibrationRoadActor", nullptr, (EPropertyFlags)0x0044000000000000, UECodeGen_Private::EPropertyGenFlags::WeakObject, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(UTrafficSystemEditorSubsystem, ActiveCalibrationRoadActor), Z_Construct_UClass_AActor_NoRegister, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_ActiveCalibrationRoadActor_MetaData), NewProp_ActiveCalibrationRoadActor_MetaData) };
const UECodeGen_Private::FStructPropertyParams Z_Construct_UClass_UTrafficSystemEditorSubsystem_Statics::NewProp_ActiveFamilyId = { "ActiveFamilyId", nullptr, (EPropertyFlags)0x0040000000000000, UECodeGen_Private::EPropertyGenFlags::Struct, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(UTrafficSystemEditorSubsystem, ActiveFamilyId), Z_Construct_UScriptStruct_FGuid, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_ActiveFamilyId_MetaData), NewProp_ActiveFamilyId_MetaData) };
const UECodeGen_Private::FObjectPropertyParams Z_Construct_UClass_UTrafficSystemEditorSubsystem_Statics::NewProp_RoadLabRibbonMeshes_Inner = { "RoadLabRibbonMeshes", nullptr, (EPropertyFlags)0x0000000000080008, UECodeGen_Private::EPropertyGenFlags::Object, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, 0, Z_Construct_UClass_UProceduralMeshComponent_NoRegister, METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FArrayPropertyParams Z_Construct_UClass_UTrafficSystemEditorSubsystem_Statics::NewProp_RoadLabRibbonMeshes = { "RoadLabRibbonMeshes", nullptr, (EPropertyFlags)0x0040008000000008, UECodeGen_Private::EPropertyGenFlags::Array, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(UTrafficSystemEditorSubsystem, RoadLabRibbonMeshes), EArrayPropertyFlags::None, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_RoadLabRibbonMeshes_MetaData), NewProp_RoadLabRibbonMeshes_MetaData) };
const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UClass_UTrafficSystemEditorSubsystem_Statics::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UTrafficSystemEditorSubsystem_Statics::NewProp_CalibrationOverlayActor,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UTrafficSystemEditorSubsystem_Statics::NewProp_ActiveCalibrationRoadActor,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UTrafficSystemEditorSubsystem_Statics::NewProp_ActiveFamilyId,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UTrafficSystemEditorSubsystem_Statics::NewProp_RoadLabRibbonMeshes_Inner,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UTrafficSystemEditorSubsystem_Statics::NewProp_RoadLabRibbonMeshes,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_UTrafficSystemEditorSubsystem_Statics::PropPointers) < 2048);
// ********** End Class UTrafficSystemEditorSubsystem Property Definitions *************************
UObject* (*const Z_Construct_UClass_UTrafficSystemEditorSubsystem_Statics::DependentSingletons[])() = {
	(UObject* (*)())Z_Construct_UClass_UEditorSubsystem,
	(UObject* (*)())Z_Construct_UPackage__Script_TrafficEditor,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_UTrafficSystemEditorSubsystem_Statics::DependentSingletons) < 16);
const UECodeGen_Private::FClassParams Z_Construct_UClass_UTrafficSystemEditorSubsystem_Statics::ClassParams = {
	&UTrafficSystemEditorSubsystem::StaticClass,
	nullptr,
	&StaticCppClassTypeInfo,
	DependentSingletons,
	FuncInfo,
	Z_Construct_UClass_UTrafficSystemEditorSubsystem_Statics::PropPointers,
	nullptr,
	UE_ARRAY_COUNT(DependentSingletons),
	UE_ARRAY_COUNT(FuncInfo),
	UE_ARRAY_COUNT(Z_Construct_UClass_UTrafficSystemEditorSubsystem_Statics::PropPointers),
	0,
	0x009000A0u,
	METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UClass_UTrafficSystemEditorSubsystem_Statics::Class_MetaDataParams), Z_Construct_UClass_UTrafficSystemEditorSubsystem_Statics::Class_MetaDataParams)
};
void UTrafficSystemEditorSubsystem::StaticRegisterNativesUTrafficSystemEditorSubsystem()
{
	UClass* Class = UTrafficSystemEditorSubsystem::StaticClass();
	FNativeFunctionRegistrar::RegisterFunctions(Class, MakeConstArrayView(Z_Construct_UClass_UTrafficSystemEditorSubsystem_Statics::Funcs));
}
UClass* Z_Construct_UClass_UTrafficSystemEditorSubsystem()
{
	if (!Z_Registration_Info_UClass_UTrafficSystemEditorSubsystem.OuterSingleton)
	{
		UECodeGen_Private::ConstructUClass(Z_Registration_Info_UClass_UTrafficSystemEditorSubsystem.OuterSingleton, Z_Construct_UClass_UTrafficSystemEditorSubsystem_Statics::ClassParams);
	}
	return Z_Registration_Info_UClass_UTrafficSystemEditorSubsystem.OuterSingleton;
}
UTrafficSystemEditorSubsystem::UTrafficSystemEditorSubsystem() {}
DEFINE_VTABLE_PTR_HELPER_CTOR_NS(, UTrafficSystemEditorSubsystem);
UTrafficSystemEditorSubsystem::~UTrafficSystemEditorSubsystem() {}
// ********** End Class UTrafficSystemEditorSubsystem **********************************************

// ********** Begin Registration *******************************************************************
struct Z_CompiledInDeferFile_FID_Users_rllax_L_E_O_Plugins_AAAtrafficSystem_Source_TrafficEditor_Public_TrafficSystemEditorSubsystem_h__Script_TrafficEditor_Statics
{
	static constexpr FStructRegisterCompiledInInfo ScriptStructInfo[] = {
		{ FCalibrationSnippet::StaticStruct, Z_Construct_UScriptStruct_FCalibrationSnippet_Statics::NewStructOps, TEXT("CalibrationSnippet"),&Z_Registration_Info_UScriptStruct_FCalibrationSnippet, CONSTRUCT_RELOAD_VERSION_INFO(FStructReloadVersionInfo, sizeof(FCalibrationSnippet), 3668550008U) },
	};
	static constexpr FClassRegisterCompiledInInfo ClassInfo[] = {
		{ Z_Construct_UClass_UTrafficSystemEditorSubsystem, UTrafficSystemEditorSubsystem::StaticClass, TEXT("UTrafficSystemEditorSubsystem"), &Z_Registration_Info_UClass_UTrafficSystemEditorSubsystem, CONSTRUCT_RELOAD_VERSION_INFO(FClassReloadVersionInfo, sizeof(UTrafficSystemEditorSubsystem), 2253121419U) },
	};
}; // Z_CompiledInDeferFile_FID_Users_rllax_L_E_O_Plugins_AAAtrafficSystem_Source_TrafficEditor_Public_TrafficSystemEditorSubsystem_h__Script_TrafficEditor_Statics 
static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_rllax_L_E_O_Plugins_AAAtrafficSystem_Source_TrafficEditor_Public_TrafficSystemEditorSubsystem_h__Script_TrafficEditor_2626391231{
	TEXT("/Script/TrafficEditor"),
	Z_CompiledInDeferFile_FID_Users_rllax_L_E_O_Plugins_AAAtrafficSystem_Source_TrafficEditor_Public_TrafficSystemEditorSubsystem_h__Script_TrafficEditor_Statics::ClassInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Users_rllax_L_E_O_Plugins_AAAtrafficSystem_Source_TrafficEditor_Public_TrafficSystemEditorSubsystem_h__Script_TrafficEditor_Statics::ClassInfo),
	Z_CompiledInDeferFile_FID_Users_rllax_L_E_O_Plugins_AAAtrafficSystem_Source_TrafficEditor_Public_TrafficSystemEditorSubsystem_h__Script_TrafficEditor_Statics::ScriptStructInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Users_rllax_L_E_O_Plugins_AAAtrafficSystem_Source_TrafficEditor_Public_TrafficSystemEditorSubsystem_h__Script_TrafficEditor_Statics::ScriptStructInfo),
	nullptr, 0,
};
// ********** End Registration *********************************************************************

PRAGMA_ENABLE_DEPRECATION_WARNINGS
