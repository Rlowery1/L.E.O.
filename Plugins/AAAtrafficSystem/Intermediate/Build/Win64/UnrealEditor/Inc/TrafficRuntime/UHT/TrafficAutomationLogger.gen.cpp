// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "TrafficAutomationLogger.h"

PRAGMA_DISABLE_DEPRECATION_WARNINGS
static_assert(!UE_WITH_CONSTINIT_UOBJECT, "This generated code can only be compiled with !UE_WITH_CONSTINIT_OBJECT");
void EmptyLinkFunctionForGeneratedCodeTrafficAutomationLogger() {}

// ********** Begin Cross Module References ********************************************************
COREUOBJECT_API UClass* Z_Construct_UClass_UObject();
TRAFFICRUNTIME_API UClass* Z_Construct_UClass_UTrafficAutomationLogger();
TRAFFICRUNTIME_API UClass* Z_Construct_UClass_UTrafficAutomationLogger_NoRegister();
UPackage* Z_Construct_UPackage__Script_TrafficRuntime();
// ********** End Cross Module References **********************************************************

// ********** Begin Class UTrafficAutomationLogger Function BeginTestLog ***************************
struct Z_Construct_UFunction_UTrafficAutomationLogger_BeginTestLog_Statics
{
	struct TrafficAutomationLogger_eventBeginTestLog_Parms
	{
		FString TestName;
	};
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[] = {
		{ "ModuleRelativePath", "Public/TrafficAutomationLogger.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_TestName_MetaData[] = {
		{ "NativeConst", "" },
	};
#endif // WITH_METADATA

// ********** Begin Function BeginTestLog constinit property declarations **************************
	static const UECodeGen_Private::FStrPropertyParams NewProp_TestName;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
// ********** End Function BeginTestLog constinit property declarations ****************************
	static const UECodeGen_Private::FFunctionParams FuncParams;
};

// ********** Begin Function BeginTestLog Property Definitions *************************************
const UECodeGen_Private::FStrPropertyParams Z_Construct_UFunction_UTrafficAutomationLogger_BeginTestLog_Statics::NewProp_TestName = { "TestName", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Str, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(TrafficAutomationLogger_eventBeginTestLog_Parms, TestName), METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_TestName_MetaData), NewProp_TestName_MetaData) };
const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UFunction_UTrafficAutomationLogger_BeginTestLog_Statics::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UTrafficAutomationLogger_BeginTestLog_Statics::NewProp_TestName,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UFunction_UTrafficAutomationLogger_BeginTestLog_Statics::PropPointers) < 2048);
// ********** End Function BeginTestLog Property Definitions ***************************************
const UECodeGen_Private::FFunctionParams Z_Construct_UFunction_UTrafficAutomationLogger_BeginTestLog_Statics::FuncParams = { { (UObject*(*)())Z_Construct_UClass_UTrafficAutomationLogger, nullptr, "BeginTestLog", 	Z_Construct_UFunction_UTrafficAutomationLogger_BeginTestLog_Statics::PropPointers, 
	UE_ARRAY_COUNT(Z_Construct_UFunction_UTrafficAutomationLogger_BeginTestLog_Statics::PropPointers), 
sizeof(Z_Construct_UFunction_UTrafficAutomationLogger_BeginTestLog_Statics::TrafficAutomationLogger_eventBeginTestLog_Parms),
RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x00022401, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UFunction_UTrafficAutomationLogger_BeginTestLog_Statics::Function_MetaDataParams), Z_Construct_UFunction_UTrafficAutomationLogger_BeginTestLog_Statics::Function_MetaDataParams)},  };
static_assert(sizeof(Z_Construct_UFunction_UTrafficAutomationLogger_BeginTestLog_Statics::TrafficAutomationLogger_eventBeginTestLog_Parms) < MAX_uint16);
UFunction* Z_Construct_UFunction_UTrafficAutomationLogger_BeginTestLog()
{
	static UFunction* ReturnFunction = nullptr;
	if (!ReturnFunction)
	{
		UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UFunction_UTrafficAutomationLogger_BeginTestLog_Statics::FuncParams);
	}
	return ReturnFunction;
}
DEFINE_FUNCTION(UTrafficAutomationLogger::execBeginTestLog)
{
	P_GET_PROPERTY(FStrProperty,Z_Param_TestName);
	P_FINISH;
	P_NATIVE_BEGIN;
	UTrafficAutomationLogger::BeginTestLog(Z_Param_TestName);
	P_NATIVE_END;
}
// ********** End Class UTrafficAutomationLogger Function BeginTestLog *****************************

// ********** Begin Class UTrafficAutomationLogger Function EndTestLog *****************************
struct Z_Construct_UFunction_UTrafficAutomationLogger_EndTestLog_Statics
{
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[] = {
		{ "ModuleRelativePath", "Public/TrafficAutomationLogger.h" },
	};
#endif // WITH_METADATA

// ********** Begin Function EndTestLog constinit property declarations ****************************
// ********** End Function EndTestLog constinit property declarations ******************************
	static const UECodeGen_Private::FFunctionParams FuncParams;
};
const UECodeGen_Private::FFunctionParams Z_Construct_UFunction_UTrafficAutomationLogger_EndTestLog_Statics::FuncParams = { { (UObject*(*)())Z_Construct_UClass_UTrafficAutomationLogger, nullptr, "EndTestLog", 	nullptr, 
	0, 
0,
RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x00022401, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UFunction_UTrafficAutomationLogger_EndTestLog_Statics::Function_MetaDataParams), Z_Construct_UFunction_UTrafficAutomationLogger_EndTestLog_Statics::Function_MetaDataParams)},  };
UFunction* Z_Construct_UFunction_UTrafficAutomationLogger_EndTestLog()
{
	static UFunction* ReturnFunction = nullptr;
	if (!ReturnFunction)
	{
		UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UFunction_UTrafficAutomationLogger_EndTestLog_Statics::FuncParams);
	}
	return ReturnFunction;
}
DEFINE_FUNCTION(UTrafficAutomationLogger::execEndTestLog)
{
	P_FINISH;
	P_NATIVE_BEGIN;
	UTrafficAutomationLogger::EndTestLog();
	P_NATIVE_END;
}
// ********** End Class UTrafficAutomationLogger Function EndTestLog *******************************

// ********** Begin Class UTrafficAutomationLogger Function LogLine ********************************
struct Z_Construct_UFunction_UTrafficAutomationLogger_LogLine_Statics
{
	struct TrafficAutomationLogger_eventLogLine_Parms
	{
		FString Line;
	};
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[] = {
		{ "ModuleRelativePath", "Public/TrafficAutomationLogger.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_Line_MetaData[] = {
		{ "NativeConst", "" },
	};
#endif // WITH_METADATA

// ********** Begin Function LogLine constinit property declarations *******************************
	static const UECodeGen_Private::FStrPropertyParams NewProp_Line;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
// ********** End Function LogLine constinit property declarations *********************************
	static const UECodeGen_Private::FFunctionParams FuncParams;
};

// ********** Begin Function LogLine Property Definitions ******************************************
const UECodeGen_Private::FStrPropertyParams Z_Construct_UFunction_UTrafficAutomationLogger_LogLine_Statics::NewProp_Line = { "Line", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Str, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(TrafficAutomationLogger_eventLogLine_Parms, Line), METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_Line_MetaData), NewProp_Line_MetaData) };
const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UFunction_UTrafficAutomationLogger_LogLine_Statics::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UTrafficAutomationLogger_LogLine_Statics::NewProp_Line,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UFunction_UTrafficAutomationLogger_LogLine_Statics::PropPointers) < 2048);
// ********** End Function LogLine Property Definitions ********************************************
const UECodeGen_Private::FFunctionParams Z_Construct_UFunction_UTrafficAutomationLogger_LogLine_Statics::FuncParams = { { (UObject*(*)())Z_Construct_UClass_UTrafficAutomationLogger, nullptr, "LogLine", 	Z_Construct_UFunction_UTrafficAutomationLogger_LogLine_Statics::PropPointers, 
	UE_ARRAY_COUNT(Z_Construct_UFunction_UTrafficAutomationLogger_LogLine_Statics::PropPointers), 
sizeof(Z_Construct_UFunction_UTrafficAutomationLogger_LogLine_Statics::TrafficAutomationLogger_eventLogLine_Parms),
RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x00022401, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UFunction_UTrafficAutomationLogger_LogLine_Statics::Function_MetaDataParams), Z_Construct_UFunction_UTrafficAutomationLogger_LogLine_Statics::Function_MetaDataParams)},  };
static_assert(sizeof(Z_Construct_UFunction_UTrafficAutomationLogger_LogLine_Statics::TrafficAutomationLogger_eventLogLine_Parms) < MAX_uint16);
UFunction* Z_Construct_UFunction_UTrafficAutomationLogger_LogLine()
{
	static UFunction* ReturnFunction = nullptr;
	if (!ReturnFunction)
	{
		UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UFunction_UTrafficAutomationLogger_LogLine_Statics::FuncParams);
	}
	return ReturnFunction;
}
DEFINE_FUNCTION(UTrafficAutomationLogger::execLogLine)
{
	P_GET_PROPERTY(FStrProperty,Z_Param_Line);
	P_FINISH;
	P_NATIVE_BEGIN;
	UTrafficAutomationLogger::LogLine(Z_Param_Line);
	P_NATIVE_END;
}
// ********** End Class UTrafficAutomationLogger Function LogLine **********************************

// ********** Begin Class UTrafficAutomationLogger Function LogMetric ******************************
struct Z_Construct_UFunction_UTrafficAutomationLogger_LogMetric_Statics
{
	struct TrafficAutomationLogger_eventLogMetric_Parms
	{
		FString Key;
		FString Value;
	};
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[] = {
		{ "ModuleRelativePath", "Public/TrafficAutomationLogger.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_Key_MetaData[] = {
		{ "NativeConst", "" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_Value_MetaData[] = {
		{ "NativeConst", "" },
	};
#endif // WITH_METADATA

// ********** Begin Function LogMetric constinit property declarations *****************************
	static const UECodeGen_Private::FStrPropertyParams NewProp_Key;
	static const UECodeGen_Private::FStrPropertyParams NewProp_Value;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
// ********** End Function LogMetric constinit property declarations *******************************
	static const UECodeGen_Private::FFunctionParams FuncParams;
};

// ********** Begin Function LogMetric Property Definitions ****************************************
const UECodeGen_Private::FStrPropertyParams Z_Construct_UFunction_UTrafficAutomationLogger_LogMetric_Statics::NewProp_Key = { "Key", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Str, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(TrafficAutomationLogger_eventLogMetric_Parms, Key), METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_Key_MetaData), NewProp_Key_MetaData) };
const UECodeGen_Private::FStrPropertyParams Z_Construct_UFunction_UTrafficAutomationLogger_LogMetric_Statics::NewProp_Value = { "Value", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Str, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(TrafficAutomationLogger_eventLogMetric_Parms, Value), METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_Value_MetaData), NewProp_Value_MetaData) };
const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UFunction_UTrafficAutomationLogger_LogMetric_Statics::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UTrafficAutomationLogger_LogMetric_Statics::NewProp_Key,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UTrafficAutomationLogger_LogMetric_Statics::NewProp_Value,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UFunction_UTrafficAutomationLogger_LogMetric_Statics::PropPointers) < 2048);
// ********** End Function LogMetric Property Definitions ******************************************
const UECodeGen_Private::FFunctionParams Z_Construct_UFunction_UTrafficAutomationLogger_LogMetric_Statics::FuncParams = { { (UObject*(*)())Z_Construct_UClass_UTrafficAutomationLogger, nullptr, "LogMetric", 	Z_Construct_UFunction_UTrafficAutomationLogger_LogMetric_Statics::PropPointers, 
	UE_ARRAY_COUNT(Z_Construct_UFunction_UTrafficAutomationLogger_LogMetric_Statics::PropPointers), 
sizeof(Z_Construct_UFunction_UTrafficAutomationLogger_LogMetric_Statics::TrafficAutomationLogger_eventLogMetric_Parms),
RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x00022401, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UFunction_UTrafficAutomationLogger_LogMetric_Statics::Function_MetaDataParams), Z_Construct_UFunction_UTrafficAutomationLogger_LogMetric_Statics::Function_MetaDataParams)},  };
static_assert(sizeof(Z_Construct_UFunction_UTrafficAutomationLogger_LogMetric_Statics::TrafficAutomationLogger_eventLogMetric_Parms) < MAX_uint16);
UFunction* Z_Construct_UFunction_UTrafficAutomationLogger_LogMetric()
{
	static UFunction* ReturnFunction = nullptr;
	if (!ReturnFunction)
	{
		UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UFunction_UTrafficAutomationLogger_LogMetric_Statics::FuncParams);
	}
	return ReturnFunction;
}
DEFINE_FUNCTION(UTrafficAutomationLogger::execLogMetric)
{
	P_GET_PROPERTY(FStrProperty,Z_Param_Key);
	P_GET_PROPERTY(FStrProperty,Z_Param_Value);
	P_FINISH;
	P_NATIVE_BEGIN;
	UTrafficAutomationLogger::LogMetric(Z_Param_Key,Z_Param_Value);
	P_NATIVE_END;
}
// ********** End Class UTrafficAutomationLogger Function LogMetric ********************************

// ********** Begin Class UTrafficAutomationLogger Function LogMetricFloat *************************
struct Z_Construct_UFunction_UTrafficAutomationLogger_LogMetricFloat_Statics
{
	struct TrafficAutomationLogger_eventLogMetricFloat_Parms
	{
		FString Key;
		float Value;
		int32 Precision;
	};
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[] = {
		{ "ModuleRelativePath", "Public/TrafficAutomationLogger.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_Key_MetaData[] = {
		{ "NativeConst", "" },
	};
#endif // WITH_METADATA

// ********** Begin Function LogMetricFloat constinit property declarations ************************
	static const UECodeGen_Private::FStrPropertyParams NewProp_Key;
	static const UECodeGen_Private::FFloatPropertyParams NewProp_Value;
	static const UECodeGen_Private::FIntPropertyParams NewProp_Precision;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
// ********** End Function LogMetricFloat constinit property declarations **************************
	static const UECodeGen_Private::FFunctionParams FuncParams;
};

// ********** Begin Function LogMetricFloat Property Definitions ***********************************
const UECodeGen_Private::FStrPropertyParams Z_Construct_UFunction_UTrafficAutomationLogger_LogMetricFloat_Statics::NewProp_Key = { "Key", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Str, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(TrafficAutomationLogger_eventLogMetricFloat_Parms, Key), METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_Key_MetaData), NewProp_Key_MetaData) };
const UECodeGen_Private::FFloatPropertyParams Z_Construct_UFunction_UTrafficAutomationLogger_LogMetricFloat_Statics::NewProp_Value = { "Value", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Float, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(TrafficAutomationLogger_eventLogMetricFloat_Parms, Value), METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FIntPropertyParams Z_Construct_UFunction_UTrafficAutomationLogger_LogMetricFloat_Statics::NewProp_Precision = { "Precision", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Int, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(TrafficAutomationLogger_eventLogMetricFloat_Parms, Precision), METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UFunction_UTrafficAutomationLogger_LogMetricFloat_Statics::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UTrafficAutomationLogger_LogMetricFloat_Statics::NewProp_Key,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UTrafficAutomationLogger_LogMetricFloat_Statics::NewProp_Value,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UTrafficAutomationLogger_LogMetricFloat_Statics::NewProp_Precision,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UFunction_UTrafficAutomationLogger_LogMetricFloat_Statics::PropPointers) < 2048);
// ********** End Function LogMetricFloat Property Definitions *************************************
const UECodeGen_Private::FFunctionParams Z_Construct_UFunction_UTrafficAutomationLogger_LogMetricFloat_Statics::FuncParams = { { (UObject*(*)())Z_Construct_UClass_UTrafficAutomationLogger, nullptr, "LogMetricFloat", 	Z_Construct_UFunction_UTrafficAutomationLogger_LogMetricFloat_Statics::PropPointers, 
	UE_ARRAY_COUNT(Z_Construct_UFunction_UTrafficAutomationLogger_LogMetricFloat_Statics::PropPointers), 
sizeof(Z_Construct_UFunction_UTrafficAutomationLogger_LogMetricFloat_Statics::TrafficAutomationLogger_eventLogMetricFloat_Parms),
RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x00022401, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UFunction_UTrafficAutomationLogger_LogMetricFloat_Statics::Function_MetaDataParams), Z_Construct_UFunction_UTrafficAutomationLogger_LogMetricFloat_Statics::Function_MetaDataParams)},  };
static_assert(sizeof(Z_Construct_UFunction_UTrafficAutomationLogger_LogMetricFloat_Statics::TrafficAutomationLogger_eventLogMetricFloat_Parms) < MAX_uint16);
UFunction* Z_Construct_UFunction_UTrafficAutomationLogger_LogMetricFloat()
{
	static UFunction* ReturnFunction = nullptr;
	if (!ReturnFunction)
	{
		UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UFunction_UTrafficAutomationLogger_LogMetricFloat_Statics::FuncParams);
	}
	return ReturnFunction;
}
DEFINE_FUNCTION(UTrafficAutomationLogger::execLogMetricFloat)
{
	P_GET_PROPERTY(FStrProperty,Z_Param_Key);
	P_GET_PROPERTY(FFloatProperty,Z_Param_Value);
	P_GET_PROPERTY(FIntProperty,Z_Param_Precision);
	P_FINISH;
	P_NATIVE_BEGIN;
	UTrafficAutomationLogger::LogMetricFloat(Z_Param_Key,Z_Param_Value,Z_Param_Precision);
	P_NATIVE_END;
}
// ********** End Class UTrafficAutomationLogger Function LogMetricFloat ***************************

// ********** Begin Class UTrafficAutomationLogger Function LogMetricInt ***************************
struct Z_Construct_UFunction_UTrafficAutomationLogger_LogMetricInt_Statics
{
	struct TrafficAutomationLogger_eventLogMetricInt_Parms
	{
		FString Key;
		int32 Value;
	};
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[] = {
		{ "ModuleRelativePath", "Public/TrafficAutomationLogger.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_Key_MetaData[] = {
		{ "NativeConst", "" },
	};
#endif // WITH_METADATA

// ********** Begin Function LogMetricInt constinit property declarations **************************
	static const UECodeGen_Private::FStrPropertyParams NewProp_Key;
	static const UECodeGen_Private::FIntPropertyParams NewProp_Value;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
// ********** End Function LogMetricInt constinit property declarations ****************************
	static const UECodeGen_Private::FFunctionParams FuncParams;
};

// ********** Begin Function LogMetricInt Property Definitions *************************************
const UECodeGen_Private::FStrPropertyParams Z_Construct_UFunction_UTrafficAutomationLogger_LogMetricInt_Statics::NewProp_Key = { "Key", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Str, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(TrafficAutomationLogger_eventLogMetricInt_Parms, Key), METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_Key_MetaData), NewProp_Key_MetaData) };
const UECodeGen_Private::FIntPropertyParams Z_Construct_UFunction_UTrafficAutomationLogger_LogMetricInt_Statics::NewProp_Value = { "Value", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Int, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(TrafficAutomationLogger_eventLogMetricInt_Parms, Value), METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UFunction_UTrafficAutomationLogger_LogMetricInt_Statics::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UTrafficAutomationLogger_LogMetricInt_Statics::NewProp_Key,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UTrafficAutomationLogger_LogMetricInt_Statics::NewProp_Value,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UFunction_UTrafficAutomationLogger_LogMetricInt_Statics::PropPointers) < 2048);
// ********** End Function LogMetricInt Property Definitions ***************************************
const UECodeGen_Private::FFunctionParams Z_Construct_UFunction_UTrafficAutomationLogger_LogMetricInt_Statics::FuncParams = { { (UObject*(*)())Z_Construct_UClass_UTrafficAutomationLogger, nullptr, "LogMetricInt", 	Z_Construct_UFunction_UTrafficAutomationLogger_LogMetricInt_Statics::PropPointers, 
	UE_ARRAY_COUNT(Z_Construct_UFunction_UTrafficAutomationLogger_LogMetricInt_Statics::PropPointers), 
sizeof(Z_Construct_UFunction_UTrafficAutomationLogger_LogMetricInt_Statics::TrafficAutomationLogger_eventLogMetricInt_Parms),
RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x00022401, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UFunction_UTrafficAutomationLogger_LogMetricInt_Statics::Function_MetaDataParams), Z_Construct_UFunction_UTrafficAutomationLogger_LogMetricInt_Statics::Function_MetaDataParams)},  };
static_assert(sizeof(Z_Construct_UFunction_UTrafficAutomationLogger_LogMetricInt_Statics::TrafficAutomationLogger_eventLogMetricInt_Parms) < MAX_uint16);
UFunction* Z_Construct_UFunction_UTrafficAutomationLogger_LogMetricInt()
{
	static UFunction* ReturnFunction = nullptr;
	if (!ReturnFunction)
	{
		UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UFunction_UTrafficAutomationLogger_LogMetricInt_Statics::FuncParams);
	}
	return ReturnFunction;
}
DEFINE_FUNCTION(UTrafficAutomationLogger::execLogMetricInt)
{
	P_GET_PROPERTY(FStrProperty,Z_Param_Key);
	P_GET_PROPERTY(FIntProperty,Z_Param_Value);
	P_FINISH;
	P_NATIVE_BEGIN;
	UTrafficAutomationLogger::LogMetricInt(Z_Param_Key,Z_Param_Value);
	P_NATIVE_END;
}
// ********** End Class UTrafficAutomationLogger Function LogMetricInt *****************************

// ********** Begin Class UTrafficAutomationLogger *************************************************
FClassRegistrationInfo Z_Registration_Info_UClass_UTrafficAutomationLogger;
UClass* UTrafficAutomationLogger::GetPrivateStaticClass()
{
	using TClass = UTrafficAutomationLogger;
	if (!Z_Registration_Info_UClass_UTrafficAutomationLogger.InnerSingleton)
	{
		GetPrivateStaticClassBody(
			TClass::StaticPackage(),
			TEXT("TrafficAutomationLogger"),
			Z_Registration_Info_UClass_UTrafficAutomationLogger.InnerSingleton,
			StaticRegisterNativesUTrafficAutomationLogger,
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
	return Z_Registration_Info_UClass_UTrafficAutomationLogger.InnerSingleton;
}
UClass* Z_Construct_UClass_UTrafficAutomationLogger_NoRegister()
{
	return UTrafficAutomationLogger::GetPrivateStaticClass();
}
struct Z_Construct_UClass_UTrafficAutomationLogger_Statics
{
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Class_MetaDataParams[] = {
		{ "IncludePath", "TrafficAutomationLogger.h" },
		{ "ModuleRelativePath", "Public/TrafficAutomationLogger.h" },
	};
#endif // WITH_METADATA

// ********** Begin Class UTrafficAutomationLogger constinit property declarations *****************
// ********** End Class UTrafficAutomationLogger constinit property declarations *******************
	static constexpr UE::CodeGen::FClassNativeFunction Funcs[] = {
		{ .NameUTF8 = UTF8TEXT("BeginTestLog"), .Pointer = &UTrafficAutomationLogger::execBeginTestLog },
		{ .NameUTF8 = UTF8TEXT("EndTestLog"), .Pointer = &UTrafficAutomationLogger::execEndTestLog },
		{ .NameUTF8 = UTF8TEXT("LogLine"), .Pointer = &UTrafficAutomationLogger::execLogLine },
		{ .NameUTF8 = UTF8TEXT("LogMetric"), .Pointer = &UTrafficAutomationLogger::execLogMetric },
		{ .NameUTF8 = UTF8TEXT("LogMetricFloat"), .Pointer = &UTrafficAutomationLogger::execLogMetricFloat },
		{ .NameUTF8 = UTF8TEXT("LogMetricInt"), .Pointer = &UTrafficAutomationLogger::execLogMetricInt },
	};
	static UObject* (*const DependentSingletons[])();
	static constexpr FClassFunctionLinkInfo FuncInfo[] = {
		{ &Z_Construct_UFunction_UTrafficAutomationLogger_BeginTestLog, "BeginTestLog" }, // 1444651963
		{ &Z_Construct_UFunction_UTrafficAutomationLogger_EndTestLog, "EndTestLog" }, // 3067949162
		{ &Z_Construct_UFunction_UTrafficAutomationLogger_LogLine, "LogLine" }, // 1442477498
		{ &Z_Construct_UFunction_UTrafficAutomationLogger_LogMetric, "LogMetric" }, // 888515116
		{ &Z_Construct_UFunction_UTrafficAutomationLogger_LogMetricFloat, "LogMetricFloat" }, // 1310947185
		{ &Z_Construct_UFunction_UTrafficAutomationLogger_LogMetricInt, "LogMetricInt" }, // 2054581900
	};
	static_assert(UE_ARRAY_COUNT(FuncInfo) < 2048);
	static constexpr FCppClassTypeInfoStatic StaticCppClassTypeInfo = {
		TCppClassTypeTraits<UTrafficAutomationLogger>::IsAbstract,
	};
	static const UECodeGen_Private::FClassParams ClassParams;
}; // struct Z_Construct_UClass_UTrafficAutomationLogger_Statics
UObject* (*const Z_Construct_UClass_UTrafficAutomationLogger_Statics::DependentSingletons[])() = {
	(UObject* (*)())Z_Construct_UClass_UObject,
	(UObject* (*)())Z_Construct_UPackage__Script_TrafficRuntime,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_UTrafficAutomationLogger_Statics::DependentSingletons) < 16);
const UECodeGen_Private::FClassParams Z_Construct_UClass_UTrafficAutomationLogger_Statics::ClassParams = {
	&UTrafficAutomationLogger::StaticClass,
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
	METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UClass_UTrafficAutomationLogger_Statics::Class_MetaDataParams), Z_Construct_UClass_UTrafficAutomationLogger_Statics::Class_MetaDataParams)
};
void UTrafficAutomationLogger::StaticRegisterNativesUTrafficAutomationLogger()
{
	UClass* Class = UTrafficAutomationLogger::StaticClass();
	FNativeFunctionRegistrar::RegisterFunctions(Class, MakeConstArrayView(Z_Construct_UClass_UTrafficAutomationLogger_Statics::Funcs));
}
UClass* Z_Construct_UClass_UTrafficAutomationLogger()
{
	if (!Z_Registration_Info_UClass_UTrafficAutomationLogger.OuterSingleton)
	{
		UECodeGen_Private::ConstructUClass(Z_Registration_Info_UClass_UTrafficAutomationLogger.OuterSingleton, Z_Construct_UClass_UTrafficAutomationLogger_Statics::ClassParams);
	}
	return Z_Registration_Info_UClass_UTrafficAutomationLogger.OuterSingleton;
}
UTrafficAutomationLogger::UTrafficAutomationLogger(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
DEFINE_VTABLE_PTR_HELPER_CTOR_NS(, UTrafficAutomationLogger);
UTrafficAutomationLogger::~UTrafficAutomationLogger() {}
// ********** End Class UTrafficAutomationLogger ***************************************************

// ********** Begin Registration *******************************************************************
struct Z_CompiledInDeferFile_FID_Users_rllax_L_E_O_Plugins_AAAtrafficSystem_Source_TrafficRuntime_Public_TrafficAutomationLogger_h__Script_TrafficRuntime_Statics
{
	static constexpr FClassRegisterCompiledInInfo ClassInfo[] = {
		{ Z_Construct_UClass_UTrafficAutomationLogger, UTrafficAutomationLogger::StaticClass, TEXT("UTrafficAutomationLogger"), &Z_Registration_Info_UClass_UTrafficAutomationLogger, CONSTRUCT_RELOAD_VERSION_INFO(FClassReloadVersionInfo, sizeof(UTrafficAutomationLogger), 406823165U) },
	};
}; // Z_CompiledInDeferFile_FID_Users_rllax_L_E_O_Plugins_AAAtrafficSystem_Source_TrafficRuntime_Public_TrafficAutomationLogger_h__Script_TrafficRuntime_Statics 
static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_rllax_L_E_O_Plugins_AAAtrafficSystem_Source_TrafficRuntime_Public_TrafficAutomationLogger_h__Script_TrafficRuntime_1448756982{
	TEXT("/Script/TrafficRuntime"),
	Z_CompiledInDeferFile_FID_Users_rllax_L_E_O_Plugins_AAAtrafficSystem_Source_TrafficRuntime_Public_TrafficAutomationLogger_h__Script_TrafficRuntime_Statics::ClassInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Users_rllax_L_E_O_Plugins_AAAtrafficSystem_Source_TrafficRuntime_Public_TrafficAutomationLogger_h__Script_TrafficRuntime_Statics::ClassInfo),
	nullptr, 0,
	nullptr, 0,
};
// ********** End Registration *********************************************************************

PRAGMA_ENABLE_DEPRECATION_WARNINGS
