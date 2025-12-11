// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
PRAGMA_DISABLE_DEPRECATION_WARNINGS
void EmptyLinkFunctionForGeneratedCodeTrafficRuntime_init() {}
static_assert(!UE_WITH_CONSTINIT_UOBJECT, "This generated code can only be compiled with !UE_WITH_CONSTINIT_OBJECT");	static FPackageRegistrationInfo Z_Registration_Info_UPackage__Script_TrafficRuntime;
	FORCENOINLINE UPackage* Z_Construct_UPackage__Script_TrafficRuntime()
	{
		if (!Z_Registration_Info_UPackage__Script_TrafficRuntime.OuterSingleton)
		{
		static const UECodeGen_Private::FPackageParams PackageParams = {
			"/Script/TrafficRuntime",
			nullptr,
			0,
			PKG_CompiledIn | 0x00000000,
			0x2CCDF88B,
			0x5B205B73,
			METADATA_PARAMS(0, nullptr)
		};
		UECodeGen_Private::ConstructUPackage(Z_Registration_Info_UPackage__Script_TrafficRuntime.OuterSingleton, PackageParams);
	}
	return Z_Registration_Info_UPackage__Script_TrafficRuntime.OuterSingleton;
}
static FRegisterCompiledInInfo Z_CompiledInDeferPackage_UPackage__Script_TrafficRuntime(Z_Construct_UPackage__Script_TrafficRuntime, TEXT("/Script/TrafficRuntime"), Z_Registration_Info_UPackage__Script_TrafficRuntime, CONSTRUCT_RELOAD_VERSION_INFO(FPackageReloadVersionInfo, 0x2CCDF88B, 0x5B205B73));
PRAGMA_ENABLE_DEPRECATION_WARNINGS
