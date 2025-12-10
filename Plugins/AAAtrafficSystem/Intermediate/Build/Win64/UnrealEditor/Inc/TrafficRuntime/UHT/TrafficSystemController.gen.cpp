// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "TrafficSystemController.h"

PRAGMA_DISABLE_DEPRECATION_WARNINGS
static_assert(!UE_WITH_CONSTINIT_UOBJECT, "This generated code can only be compiled with !UE_WITH_CONSTINIT_OBJECT");
void EmptyLinkFunctionForGeneratedCodeTrafficSystemController() {}

// ********** Begin Cross Module References ********************************************************
ENGINE_API UClass* Z_Construct_UClass_AActor();
TRAFFICRUNTIME_API UClass* Z_Construct_UClass_ATrafficSystemController();
TRAFFICRUNTIME_API UClass* Z_Construct_UClass_ATrafficSystemController_NoRegister();
TRAFFICRUNTIME_API UClass* Z_Construct_UClass_UTrafficNetworkAsset_NoRegister();
UPackage* Z_Construct_UPackage__Script_TrafficRuntime();
// ********** End Cross Module References **********************************************************

// ********** Begin Class ATrafficSystemController Function Editor_BuildTrafficNetwork *************
struct Z_Construct_UFunction_ATrafficSystemController_Editor_BuildTrafficNetwork_Statics
{
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[] = {
		{ "CallInEditor", "true" },
		{ "Category", "Traffic" },
		{ "ModuleRelativePath", "Public/TrafficSystemController.h" },
	};
#endif // WITH_METADATA

// ********** Begin Function Editor_BuildTrafficNetwork constinit property declarations ************
// ********** End Function Editor_BuildTrafficNetwork constinit property declarations **************
	static const UECodeGen_Private::FFunctionParams FuncParams;
};
const UECodeGen_Private::FFunctionParams Z_Construct_UFunction_ATrafficSystemController_Editor_BuildTrafficNetwork_Statics::FuncParams = { { (UObject*(*)())Z_Construct_UClass_ATrafficSystemController, nullptr, "Editor_BuildTrafficNetwork", 	nullptr, 
	0, 
0,
RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x00020401, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UFunction_ATrafficSystemController_Editor_BuildTrafficNetwork_Statics::Function_MetaDataParams), Z_Construct_UFunction_ATrafficSystemController_Editor_BuildTrafficNetwork_Statics::Function_MetaDataParams)},  };
UFunction* Z_Construct_UFunction_ATrafficSystemController_Editor_BuildTrafficNetwork()
{
	static UFunction* ReturnFunction = nullptr;
	if (!ReturnFunction)
	{
		UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UFunction_ATrafficSystemController_Editor_BuildTrafficNetwork_Statics::FuncParams);
	}
	return ReturnFunction;
}
DEFINE_FUNCTION(ATrafficSystemController::execEditor_BuildTrafficNetwork)
{
	P_FINISH;
	P_NATIVE_BEGIN;
	P_THIS->Editor_BuildTrafficNetwork();
	P_NATIVE_END;
}
// ********** End Class ATrafficSystemController Function Editor_BuildTrafficNetwork ***************

// ********** Begin Class ATrafficSystemController Function Editor_SpawnTestVehicles ***************
struct Z_Construct_UFunction_ATrafficSystemController_Editor_SpawnTestVehicles_Statics
{
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[] = {
		{ "CallInEditor", "true" },
		{ "Category", "Traffic" },
		{ "ModuleRelativePath", "Public/TrafficSystemController.h" },
	};
#endif // WITH_METADATA

// ********** Begin Function Editor_SpawnTestVehicles constinit property declarations **************
// ********** End Function Editor_SpawnTestVehicles constinit property declarations ****************
	static const UECodeGen_Private::FFunctionParams FuncParams;
};
const UECodeGen_Private::FFunctionParams Z_Construct_UFunction_ATrafficSystemController_Editor_SpawnTestVehicles_Statics::FuncParams = { { (UObject*(*)())Z_Construct_UClass_ATrafficSystemController, nullptr, "Editor_SpawnTestVehicles", 	nullptr, 
	0, 
0,
RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x00020401, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UFunction_ATrafficSystemController_Editor_SpawnTestVehicles_Statics::Function_MetaDataParams), Z_Construct_UFunction_ATrafficSystemController_Editor_SpawnTestVehicles_Statics::Function_MetaDataParams)},  };
UFunction* Z_Construct_UFunction_ATrafficSystemController_Editor_SpawnTestVehicles()
{
	static UFunction* ReturnFunction = nullptr;
	if (!ReturnFunction)
	{
		UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UFunction_ATrafficSystemController_Editor_SpawnTestVehicles_Statics::FuncParams);
	}
	return ReturnFunction;
}
DEFINE_FUNCTION(ATrafficSystemController::execEditor_SpawnTestVehicles)
{
	P_FINISH;
	P_NATIVE_BEGIN;
	P_THIS->Editor_SpawnTestVehicles();
	P_NATIVE_END;
}
// ********** End Class ATrafficSystemController Function Editor_SpawnTestVehicles *****************

// ********** Begin Class ATrafficSystemController *************************************************
FClassRegistrationInfo Z_Registration_Info_UClass_ATrafficSystemController;
UClass* ATrafficSystemController::GetPrivateStaticClass()
{
	using TClass = ATrafficSystemController;
	if (!Z_Registration_Info_UClass_ATrafficSystemController.InnerSingleton)
	{
		GetPrivateStaticClassBody(
			TClass::StaticPackage(),
			TEXT("TrafficSystemController"),
			Z_Registration_Info_UClass_ATrafficSystemController.InnerSingleton,
			StaticRegisterNativesATrafficSystemController,
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
	return Z_Registration_Info_UClass_ATrafficSystemController.InnerSingleton;
}
UClass* Z_Construct_UClass_ATrafficSystemController_NoRegister()
{
	return ATrafficSystemController::GetPrivateStaticClass();
}
struct Z_Construct_UClass_ATrafficSystemController_Statics
{
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Class_MetaDataParams[] = {
		{ "IncludePath", "TrafficSystemController.h" },
		{ "ModuleRelativePath", "Public/TrafficSystemController.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_BuiltNetworkAsset_MetaData[] = {
		{ "ModuleRelativePath", "Public/TrafficSystemController.h" },
	};
#endif // WITH_METADATA

// ********** Begin Class ATrafficSystemController constinit property declarations *****************
	static const UECodeGen_Private::FObjectPropertyParams NewProp_BuiltNetworkAsset;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
// ********** End Class ATrafficSystemController constinit property declarations *******************
	static constexpr UE::CodeGen::FClassNativeFunction Funcs[] = {
		{ .NameUTF8 = UTF8TEXT("Editor_BuildTrafficNetwork"), .Pointer = &ATrafficSystemController::execEditor_BuildTrafficNetwork },
		{ .NameUTF8 = UTF8TEXT("Editor_SpawnTestVehicles"), .Pointer = &ATrafficSystemController::execEditor_SpawnTestVehicles },
	};
	static UObject* (*const DependentSingletons[])();
	static constexpr FClassFunctionLinkInfo FuncInfo[] = {
		{ &Z_Construct_UFunction_ATrafficSystemController_Editor_BuildTrafficNetwork, "Editor_BuildTrafficNetwork" }, // 2828629664
		{ &Z_Construct_UFunction_ATrafficSystemController_Editor_SpawnTestVehicles, "Editor_SpawnTestVehicles" }, // 3648432327
	};
	static_assert(UE_ARRAY_COUNT(FuncInfo) < 2048);
	static constexpr FCppClassTypeInfoStatic StaticCppClassTypeInfo = {
		TCppClassTypeTraits<ATrafficSystemController>::IsAbstract,
	};
	static const UECodeGen_Private::FClassParams ClassParams;
}; // struct Z_Construct_UClass_ATrafficSystemController_Statics

// ********** Begin Class ATrafficSystemController Property Definitions ****************************
const UECodeGen_Private::FObjectPropertyParams Z_Construct_UClass_ATrafficSystemController_Statics::NewProp_BuiltNetworkAsset = { "BuiltNetworkAsset", nullptr, (EPropertyFlags)0x0020080000000000, UECodeGen_Private::EPropertyGenFlags::Object, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(ATrafficSystemController, BuiltNetworkAsset), Z_Construct_UClass_UTrafficNetworkAsset_NoRegister, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_BuiltNetworkAsset_MetaData), NewProp_BuiltNetworkAsset_MetaData) };
const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UClass_ATrafficSystemController_Statics::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_ATrafficSystemController_Statics::NewProp_BuiltNetworkAsset,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_ATrafficSystemController_Statics::PropPointers) < 2048);
// ********** End Class ATrafficSystemController Property Definitions ******************************
UObject* (*const Z_Construct_UClass_ATrafficSystemController_Statics::DependentSingletons[])() = {
	(UObject* (*)())Z_Construct_UClass_AActor,
	(UObject* (*)())Z_Construct_UPackage__Script_TrafficRuntime,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_ATrafficSystemController_Statics::DependentSingletons) < 16);
const UECodeGen_Private::FClassParams Z_Construct_UClass_ATrafficSystemController_Statics::ClassParams = {
	&ATrafficSystemController::StaticClass,
	"Engine",
	&StaticCppClassTypeInfo,
	DependentSingletons,
	FuncInfo,
	Z_Construct_UClass_ATrafficSystemController_Statics::PropPointers,
	nullptr,
	UE_ARRAY_COUNT(DependentSingletons),
	UE_ARRAY_COUNT(FuncInfo),
	UE_ARRAY_COUNT(Z_Construct_UClass_ATrafficSystemController_Statics::PropPointers),
	0,
	0x009000A4u,
	METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UClass_ATrafficSystemController_Statics::Class_MetaDataParams), Z_Construct_UClass_ATrafficSystemController_Statics::Class_MetaDataParams)
};
void ATrafficSystemController::StaticRegisterNativesATrafficSystemController()
{
	UClass* Class = ATrafficSystemController::StaticClass();
	FNativeFunctionRegistrar::RegisterFunctions(Class, MakeConstArrayView(Z_Construct_UClass_ATrafficSystemController_Statics::Funcs));
}
UClass* Z_Construct_UClass_ATrafficSystemController()
{
	if (!Z_Registration_Info_UClass_ATrafficSystemController.OuterSingleton)
	{
		UECodeGen_Private::ConstructUClass(Z_Registration_Info_UClass_ATrafficSystemController.OuterSingleton, Z_Construct_UClass_ATrafficSystemController_Statics::ClassParams);
	}
	return Z_Registration_Info_UClass_ATrafficSystemController.OuterSingleton;
}
DEFINE_VTABLE_PTR_HELPER_CTOR_NS(, ATrafficSystemController);
ATrafficSystemController::~ATrafficSystemController() {}
// ********** End Class ATrafficSystemController ***************************************************

// ********** Begin Registration *******************************************************************
struct Z_CompiledInDeferFile_FID_Users_rllax_L_E_O_Plugins_AAAtrafficSystem_Source_TrafficRuntime_Public_TrafficSystemController_h__Script_TrafficRuntime_Statics
{
	static constexpr FClassRegisterCompiledInInfo ClassInfo[] = {
		{ Z_Construct_UClass_ATrafficSystemController, ATrafficSystemController::StaticClass, TEXT("ATrafficSystemController"), &Z_Registration_Info_UClass_ATrafficSystemController, CONSTRUCT_RELOAD_VERSION_INFO(FClassReloadVersionInfo, sizeof(ATrafficSystemController), 1561709681U) },
	};
}; // Z_CompiledInDeferFile_FID_Users_rllax_L_E_O_Plugins_AAAtrafficSystem_Source_TrafficRuntime_Public_TrafficSystemController_h__Script_TrafficRuntime_Statics 
static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_rllax_L_E_O_Plugins_AAAtrafficSystem_Source_TrafficRuntime_Public_TrafficSystemController_h__Script_TrafficRuntime_987332009{
	TEXT("/Script/TrafficRuntime"),
	Z_CompiledInDeferFile_FID_Users_rllax_L_E_O_Plugins_AAAtrafficSystem_Source_TrafficRuntime_Public_TrafficSystemController_h__Script_TrafficRuntime_Statics::ClassInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Users_rllax_L_E_O_Plugins_AAAtrafficSystem_Source_TrafficRuntime_Public_TrafficSystemController_h__Script_TrafficRuntime_Statics::ClassInfo),
	nullptr, 0,
	nullptr, 0,
};
// ********** End Registration *********************************************************************

PRAGMA_ENABLE_DEPRECATION_WARNINGS
