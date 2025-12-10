#include "TrafficAutomationLogger.h"
#include "TrafficRuntimeModule.h"
#include "Misc/DateTime.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"

FString UTrafficAutomationLogger::CurrentLogFilePath;
bool UTrafficAutomationLogger::bLogOpen = false;

void UTrafficAutomationLogger::EnsureDirectoryExists()
{
	const FString BaseDir = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("TrafficAutomation"));
	IFileManager::Get().MakeDirectory(*BaseDir, true);
}

void UTrafficAutomationLogger::BeginTestLog(const FString& TestName)
{
	EnsureDirectoryExists();

	const FDateTime Now = FDateTime::Now();
	const FString Timestamp = Now.ToString(TEXT("%Y%m%d_%H%M%S"));
	const FString BaseDir = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("TrafficAutomation"));
	const FString FileName = FString::Printf(TEXT("Traffic_%s_%s.log"), *TestName, *Timestamp);
	CurrentLogFilePath = FPaths::Combine(BaseDir, FileName);

	if (!FFileHelper::SaveStringToFile(TEXT(""), *CurrentLogFilePath))
	{
		UE_LOG(LogTraffic, Warning, TEXT("Failed to create traffic automation log file: %s"), *CurrentLogFilePath);
		bLogOpen = false;
		return;
	}

	bLogOpen = true;

	FString Header = FString::Printf(
		TEXT("# Traffic Automation Log\n# TestName=%s\n# Timestamp=%s\n\n"),
		*TestName,
		*Now.ToString()
	);
	AppendToFile(Header);

	UE_LOG(LogTraffic, Log, TEXT("Traffic automation log started: %s"), *CurrentLogFilePath);
}

void UTrafficAutomationLogger::AppendToFile(const FString& Text)
{
	if (CurrentLogFilePath.IsEmpty())
	{
		return;
	}

	FString Content = Text;
	if (!Content.EndsWith(TEXT("\n")))
	{
		Content.AppendChar(TEXT('\n'));
	}

	if (!FFileHelper::SaveStringToFile(
			Content,
			*CurrentLogFilePath,
			FFileHelper::EEncodingOptions::AutoDetect,
			&IFileManager::Get(),
			FILEWRITE_Append))
	{
		UE_LOG(LogTraffic, Warning, TEXT("Failed to append to traffic automation log file: %s"), *CurrentLogFilePath);
	}
}

void UTrafficAutomationLogger::LogLine(const FString& Line)
{
	if (!bLogOpen)
	{
		return;
	}
	AppendToFile(Line);
}

void UTrafficAutomationLogger::LogMetric(const FString& Key, const FString& Value)
{
	if (!bLogOpen)
	{
		return;
	}
	const FString Line = FString::Printf(TEXT("%s=%s"), *Key, *Value);
	AppendToFile(Line);
}

void UTrafficAutomationLogger::LogMetricFloat(const FString& Key, float Value, int32 /*Precision*/)
{
	if (!bLogOpen)
	{
		return;
	}
	const FString ValueStr = FString::SanitizeFloat(Value);
	LogMetric(Key, ValueStr);
}

void UTrafficAutomationLogger::EndTestLog()
{
	if (!bLogOpen)
	{
		return;
	}

	AppendToFile(TEXT("\n# EndOfLog\n"));
	UE_LOG(LogTraffic, Log, TEXT("Traffic automation log finished: %s"), *CurrentLogFilePath);
	bLogOpen = false;
}

