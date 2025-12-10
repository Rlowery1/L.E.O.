// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
PRAGMA_DISABLE_DEPRECATION_WARNINGS
void EmptyLinkFunctionForGeneratedCodeTrafficEditor_init() {}
static_assert(!UE_WITH_CONSTINIT_UOBJECT, "This generated code can only be compiled with !UE_WITH_CONSTINIT_OBJECT");	static FPackageRegistrationInfo Z_Registration_Info_UPackage__Script_TrafficEditor;
	FORCENOINLINE UPackage* Z_Construct_UPackage__Script_TrafficEditor()
	{
		if (!Z_Registration_Info_UPackage__Script_TrafficEditor.OuterSingleton)
		{
		static const UECodeGen_Private::FPackageParams PackageParams = {
			"/Script/TrafficEditor",
			nullptr,
			0,
			PKG_CompiledIn | 0x00000040,
			0x41D4FB21,
			0x522E2AB8,
			METADATA_PARAMS(0, nullptr)
		};
		UECodeGen_Private::ConstructUPackage(Z_Registration_Info_UPackage__Script_TrafficEditor.OuterSingleton, PackageParams);
	}
	return Z_Registration_Info_UPackage__Script_TrafficEditor.OuterSingleton;
}
static FRegisterCompiledInInfo Z_CompiledInDeferPackage_UPackage__Script_TrafficEditor(Z_Construct_UPackage__Script_TrafficEditor, TEXT("/Script/TrafficEditor"), Z_Registration_Info_UPackage__Script_TrafficEditor, CONSTRUCT_RELOAD_VERSION_INFO(FPackageReloadVersionInfo, 0x41D4FB21, 0x522E2AB8));
PRAGMA_ENABLE_DEPRECATION_WARNINGS
