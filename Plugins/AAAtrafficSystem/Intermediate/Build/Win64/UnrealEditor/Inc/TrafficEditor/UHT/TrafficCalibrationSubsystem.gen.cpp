// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "TrafficCalibrationSubsystem.h"

PRAGMA_DISABLE_DEPRECATION_WARNINGS
static_assert(!UE_WITH_CONSTINIT_UOBJECT, "This generated code can only be compiled with !UE_WITH_CONSTINIT_OBJECT");
void EmptyLinkFunctionForGeneratedCodeTrafficCalibrationSubsystem() {}

// ********** Begin Cross Module References ********************************************************
EDITORSUBSYSTEM_API UClass* Z_Construct_UClass_UEditorSubsystem();
TRAFFICEDITOR_API UClass* Z_Construct_UClass_UTrafficCalibrationSubsystem();
TRAFFICEDITOR_API UClass* Z_Construct_UClass_UTrafficCalibrationSubsystem_NoRegister();
UPackage* Z_Construct_UPackage__Script_TrafficEditor();
// ********** End Cross Module References **********************************************************

// ********** Begin Class UTrafficCalibrationSubsystem Function Editor_BuildTrafficNetwork *********
struct Z_Construct_UFunction_UTrafficCalibrationSubsystem_Editor_BuildTrafficNetwork_Statics
{
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[] = {
		{ "CallInEditor", "true" },
		{ "Category", "Traffic" },
		{ "ModuleRelativePath", "Public/TrafficCalibrationSubsystem.h" },
	};
#endif // WITH_METADATA

// ********** Begin Function Editor_BuildTrafficNetwork constinit property declarations ************
// ********** End Function Editor_BuildTrafficNetwork constinit property declarations **************
	static const UECodeGen_Private::FFunctionParams FuncParams;
};
const UECodeGen_Private::FFunctionParams Z_Construct_UFunction_UTrafficCalibrationSubsystem_Editor_BuildTrafficNetwork_Statics::FuncParams = { { (UObject*(*)())Z_Construct_UClass_UTrafficCalibrationSubsystem, nullptr, "Editor_BuildTrafficNetwork", 	nullptr, 
	0, 
0,
RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x00020401, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UFunction_UTrafficCalibrationSubsystem_Editor_BuildTrafficNetwork_Statics::Function_MetaDataParams), Z_Construct_UFunction_UTrafficCalibrationSubsystem_Editor_BuildTrafficNetwork_Statics::Function_MetaDataParams)},  };
UFunction* Z_Construct_UFunction_UTrafficCalibrationSubsystem_Editor_BuildTrafficNetwork()
{
	static UFunction* ReturnFunction = nullptr;
	if (!ReturnFunction)
	{
		UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UFunction_UTrafficCalibrationSubsystem_Editor_BuildTrafficNetwork_Statics::FuncParams);
	}
	return ReturnFunction;
}
DEFINE_FUNCTION(UTrafficCalibrationSubsystem::execEditor_BuildTrafficNetwork)
{
	P_FINISH;
	P_NATIVE_BEGIN;
	P_THIS->Editor_BuildTrafficNetwork();
	P_NATIVE_END;
}
// ********** End Class UTrafficCalibrationSubsystem Function Editor_BuildTrafficNetwork ***********

// ********** Begin Class UTrafficCalibrationSubsystem Function Editor_BuildTrafficNetworkToAsset **
struct Z_Construct_UFunction_UTrafficCalibrationSubsystem_Editor_BuildTrafficNetworkToAsset_Statics
{
	struct TrafficCalibrationSubsystem_eventEditor_BuildTrafficNetworkToAsset_Parms
	{
		FString AssetPath;
	};
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[] = {
		{ "CallInEditor", "true" },
		{ "Category", "Traffic" },
		{ "ModuleRelativePath", "Public/TrafficCalibrationSubsystem.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_AssetPath_MetaData[] = {
		{ "NativeConst", "" },
	};
#endif // WITH_METADATA

// ********** Begin Function Editor_BuildTrafficNetworkToAsset constinit property declarations *****
	static const UECodeGen_Private::FStrPropertyParams NewProp_AssetPath;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
// ********** End Function Editor_BuildTrafficNetworkToAsset constinit property declarations *******
	static const UECodeGen_Private::FFunctionParams FuncParams;
};

// ********** Begin Function Editor_BuildTrafficNetworkToAsset Property Definitions ****************
const UECodeGen_Private::FStrPropertyParams Z_Construct_UFunction_UTrafficCalibrationSubsystem_Editor_BuildTrafficNetworkToAsset_Statics::NewProp_AssetPath = { "AssetPath", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Str, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(TrafficCalibrationSubsystem_eventEditor_BuildTrafficNetworkToAsset_Parms, AssetPath), METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_AssetPath_MetaData), NewProp_AssetPath_MetaData) };
const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UFunction_UTrafficCalibrationSubsystem_Editor_BuildTrafficNetworkToAsset_Statics::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UTrafficCalibrationSubsystem_Editor_BuildTrafficNetworkToAsset_Statics::NewProp_AssetPath,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UFunction_UTrafficCalibrationSubsystem_Editor_BuildTrafficNetworkToAsset_Statics::PropPointers) < 2048);
// ********** End Function Editor_BuildTrafficNetworkToAsset Property Definitions ******************
const UECodeGen_Private::FFunctionParams Z_Construct_UFunction_UTrafficCalibrationSubsystem_Editor_BuildTrafficNetworkToAsset_Statics::FuncParams = { { (UObject*(*)())Z_Construct_UClass_UTrafficCalibrationSubsystem, nullptr, "Editor_BuildTrafficNetworkToAsset", 	Z_Construct_UFunction_UTrafficCalibrationSubsystem_Editor_BuildTrafficNetworkToAsset_Statics::PropPointers, 
	UE_ARRAY_COUNT(Z_Construct_UFunction_UTrafficCalibrationSubsystem_Editor_BuildTrafficNetworkToAsset_Statics::PropPointers), 
sizeof(Z_Construct_UFunction_UTrafficCalibrationSubsystem_Editor_BuildTrafficNetworkToAsset_Statics::TrafficCalibrationSubsystem_eventEditor_BuildTrafficNetworkToAsset_Parms),
RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x00020401, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UFunction_UTrafficCalibrationSubsystem_Editor_BuildTrafficNetworkToAsset_Statics::Function_MetaDataParams), Z_Construct_UFunction_UTrafficCalibrationSubsystem_Editor_BuildTrafficNetworkToAsset_Statics::Function_MetaDataParams)},  };
static_assert(sizeof(Z_Construct_UFunction_UTrafficCalibrationSubsystem_Editor_BuildTrafficNetworkToAsset_Statics::TrafficCalibrationSubsystem_eventEditor_BuildTrafficNetworkToAsset_Parms) < MAX_uint16);
UFunction* Z_Construct_UFunction_UTrafficCalibrationSubsystem_Editor_BuildTrafficNetworkToAsset()
{
	static UFunction* ReturnFunction = nullptr;
	if (!ReturnFunction)
	{
		UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UFunction_UTrafficCalibrationSubsystem_Editor_BuildTrafficNetworkToAsset_Statics::FuncParams);
	}
	return ReturnFunction;
}
DEFINE_FUNCTION(UTrafficCalibrationSubsystem::execEditor_BuildTrafficNetworkToAsset)
{
	P_GET_PROPERTY(FStrProperty,Z_Param_AssetPath);
	P_FINISH;
	P_NATIVE_BEGIN;
	P_THIS->Editor_BuildTrafficNetworkToAsset(Z_Param_AssetPath);
	P_NATIVE_END;
}
// ********** End Class UTrafficCalibrationSubsystem Function Editor_BuildTrafficNetworkToAsset ****

// ********** Begin Class UTrafficCalibrationSubsystem *********************************************
FClassRegistrationInfo Z_Registration_Info_UClass_UTrafficCalibrationSubsystem;
UClass* UTrafficCalibrationSubsystem::GetPrivateStaticClass()
{
	using TClass = UTrafficCalibrationSubsystem;
	if (!Z_Registration_Info_UClass_UTrafficCalibrationSubsystem.InnerSingleton)
	{
		GetPrivateStaticClassBody(
			TClass::StaticPackage(),
			TEXT("TrafficCalibrationSubsystem"),
			Z_Registration_Info_UClass_UTrafficCalibrationSubsystem.InnerSingleton,
			StaticRegisterNativesUTrafficCalibrationSubsystem,
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
	return Z_Registration_Info_UClass_UTrafficCalibrationSubsystem.InnerSingleton;
}
UClass* Z_Construct_UClass_UTrafficCalibrationSubsystem_NoRegister()
{
	return UTrafficCalibrationSubsystem::GetPrivateStaticClass();
}
struct Z_Construct_UClass_UTrafficCalibrationSubsystem_Statics
{
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Class_MetaDataParams[] = {
		{ "IncludePath", "TrafficCalibrationSubsystem.h" },
		{ "ModuleRelativePath", "Public/TrafficCalibrationSubsystem.h" },
	};
#endif // WITH_METADATA

// ********** Begin Class UTrafficCalibrationSubsystem constinit property declarations *************
// ********** End Class UTrafficCalibrationSubsystem constinit property declarations ***************
	static constexpr UE::CodeGen::FClassNativeFunction Funcs[] = {
		{ .NameUTF8 = UTF8TEXT("Editor_BuildTrafficNetwork"), .Pointer = &UTrafficCalibrationSubsystem::execEditor_BuildTrafficNetwork },
		{ .NameUTF8 = UTF8TEXT("Editor_BuildTrafficNetworkToAsset"), .Pointer = &UTrafficCalibrationSubsystem::execEditor_BuildTrafficNetworkToAsset },
	};
	static UObject* (*const DependentSingletons[])();
	static constexpr FClassFunctionLinkInfo FuncInfo[] = {
		{ &Z_Construct_UFunction_UTrafficCalibrationSubsystem_Editor_BuildTrafficNetwork, "Editor_BuildTrafficNetwork" }, // 2199275046
		{ &Z_Construct_UFunction_UTrafficCalibrationSubsystem_Editor_BuildTrafficNetworkToAsset, "Editor_BuildTrafficNetworkToAsset" }, // 3876837210
	};
	static_assert(UE_ARRAY_COUNT(FuncInfo) < 2048);
	static constexpr FCppClassTypeInfoStatic StaticCppClassTypeInfo = {
		TCppClassTypeTraits<UTrafficCalibrationSubsystem>::IsAbstract,
	};
	static const UECodeGen_Private::FClassParams ClassParams;
}; // struct Z_Construct_UClass_UTrafficCalibrationSubsystem_Statics
UObject* (*const Z_Construct_UClass_UTrafficCalibrationSubsystem_Statics::DependentSingletons[])() = {
	(UObject* (*)())Z_Construct_UClass_UEditorSubsystem,
	(UObject* (*)())Z_Construct_UPackage__Script_TrafficEditor,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_UTrafficCalibrationSubsystem_Statics::DependentSingletons) < 16);
const UECodeGen_Private::FClassParams Z_Construct_UClass_UTrafficCalibrationSubsystem_Statics::ClassParams = {
	&UTrafficCalibrationSubsystem::StaticClass,
	nullptr,
	&StaticCppClassTypeInfo,
	DependentSingletons,
	FuncInfo,
	nullptr,
	nullptr,
	UE_ARRAY_COUNT(DependentSingletons),
	UE_ARRAY_COUNT(FuncInfo),
	0,
	0,
	0x001000A0u,
	METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UClass_UTrafficCalibrationSubsystem_Statics::Class_MetaDataParams), Z_Construct_UClass_UTrafficCalibrationSubsystem_Statics::Class_MetaDataParams)
};
void UTrafficCalibrationSubsystem::StaticRegisterNativesUTrafficCalibrationSubsystem()
{
	UClass* Class = UTrafficCalibrationSubsystem::StaticClass();
	FNativeFunctionRegistrar::RegisterFunctions(Class, MakeConstArrayView(Z_Construct_UClass_UTrafficCalibrationSubsystem_Statics::Funcs));
}
UClass* Z_Construct_UClass_UTrafficCalibrationSubsystem()
{
	if (!Z_Registration_Info_UClass_UTrafficCalibrationSubsystem.OuterSingleton)
	{
		UECodeGen_Private::ConstructUClass(Z_Registration_Info_UClass_UTrafficCalibrationSubsystem.OuterSingleton, Z_Construct_UClass_UTrafficCalibrationSubsystem_Statics::ClassParams);
	}
	return Z_Registration_Info_UClass_UTrafficCalibrationSubsystem.OuterSingleton;
}
UTrafficCalibrationSubsystem::UTrafficCalibrationSubsystem() {}
DEFINE_VTABLE_PTR_HELPER_CTOR_NS(, UTrafficCalibrationSubsystem);
UTrafficCalibrationSubsystem::~UTrafficCalibrationSubsystem() {}
// ********** End Class UTrafficCalibrationSubsystem ***********************************************

// ********** Begin Registration *******************************************************************
struct Z_CompiledInDeferFile_FID_Users_rllax_L_E_O_Plugins_AAAtrafficSystem_Source_TrafficEditor_Public_TrafficCalibrationSubsystem_h__Script_TrafficEditor_Statics
{
	static constexpr FClassRegisterCompiledInInfo ClassInfo[] = {
		{ Z_Construct_UClass_UTrafficCalibrationSubsystem, UTrafficCalibrationSubsystem::StaticClass, TEXT("UTrafficCalibrationSubsystem"), &Z_Registration_Info_UClass_UTrafficCalibrationSubsystem, CONSTRUCT_RELOAD_VERSION_INFO(FClassReloadVersionInfo, sizeof(UTrafficCalibrationSubsystem), 2098289083U) },
	};
}; // Z_CompiledInDeferFile_FID_Users_rllax_L_E_O_Plugins_AAAtrafficSystem_Source_TrafficEditor_Public_TrafficCalibrationSubsystem_h__Script_TrafficEditor_Statics 
static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_rllax_L_E_O_Plugins_AAAtrafficSystem_Source_TrafficEditor_Public_TrafficCalibrationSubsystem_h__Script_TrafficEditor_1261096556{
	TEXT("/Script/TrafficEditor"),
	Z_CompiledInDeferFile_FID_Users_rllax_L_E_O_Plugins_AAAtrafficSystem_Source_TrafficEditor_Public_TrafficCalibrationSubsystem_h__Script_TrafficEditor_Statics::ClassInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Users_rllax_L_E_O_Plugins_AAAtrafficSystem_Source_TrafficEditor_Public_TrafficCalibrationSubsystem_h__Script_TrafficEditor_Statics::ClassInfo),
	nullptr, 0,
	nullptr, 0,
};
// ********** End Registration *********************************************************************

PRAGMA_ENABLE_DEPRECATION_WARNINGS
