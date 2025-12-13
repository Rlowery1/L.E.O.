#include "TrafficAutomationLogger.h"
#include "TrafficRuntimeModule.h"
#include "Misc/DateTime.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"

FString UTrafficAutomationLogger::CurrentLogFilePath;
bool UTrafficAutomationLogger::bLogOpen = false;

void FTrafficRunMetrics::AccumulateLateralError(float ErrorCm)
{
	MaxLateralErrorCm = FMath::Max(MaxLateralErrorCm, ErrorCm);
	SumLateralErrorCm += ErrorCm;
	++LateralSamples;
}

void FTrafficRunMetrics::AccumulateHeadingError(float ErrorDeg)
{
	MaxHeadingErrorDeg = FMath::Max(MaxHeadingErrorDeg, ErrorDeg);
	SumHeadingErrorDeg += ErrorDeg;
	++HeadingSamples;
}

void FTrafficRunMetrics::AccumulateHeadway(float HeadwaySeconds)
{
	if (HeadwaySeconds >= 0.f)
	{
		MinHeadwaySeconds = FMath::Min(MinHeadwaySeconds, HeadwaySeconds);
		SumHeadwaySeconds += HeadwaySeconds;
		++HeadwaySamples;
	}
}

void FTrafficRunMetrics::AccumulateAccel(float AccelCmPerSec2)
{
	MaxAccelCmPerSec2 = FMath::Max(MaxAccelCmPerSec2, AccelCmPerSec2);
}

void FTrafficRunMetrics::AccumulateJerk(float JerkCmPerSec3)
{
	MaxJerkCmPerSec3 = FMath::Max(MaxJerkCmPerSec3, JerkCmPerSec3);
}

void FTrafficRunMetrics::Finalize()
{
	// Averages are computed on demand in LogRunMetrics.
}

void UTrafficAutomationLogger::EnsureDirectoryExists()
{
	// Use a stable path under the project directory so logs survive runs that override -userdir (e.g., Gauntlet).
	const FString BaseDir = FPaths::Combine(FPaths::ProjectDir(), TEXT("Saved"), TEXT("TrafficAutomation"));
	IFileManager::Get().MakeDirectory(*BaseDir, true);
}

void UTrafficAutomationLogger::BeginTestLog(const FString& TestName)
{
	EnsureDirectoryExists();

	const FDateTime Now = FDateTime::Now();
	const FString Timestamp = Now.ToString(TEXT("%Y%m%d_%H%M%S"));
	const FString BaseDir = FPaths::Combine(FPaths::ProjectDir(), TEXT("Saved"), TEXT("TrafficAutomation"));
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

void UTrafficAutomationLogger::LogMetricInt(const FString& Key, int32 Value)
{
	if (!bLogOpen)
	{
		return;
	}
	LogMetric(Key, FString::FromInt(Value));
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

void UTrafficAutomationLogger::LogRunMetrics(const FString& TestName, const FTrafficRunMetrics& M)
{
	const float AvgLat = (M.LateralSamples > 0) ? (M.SumLateralErrorCm / M.LateralSamples) : 0.f;
	const float AvgHead = (M.HeadingSamples > 0) ? (M.SumHeadingErrorDeg / M.HeadingSamples) : 0.f;
	const float AvgHeadway = (M.HeadwaySamples > 0) ? (M.SumHeadwaySeconds / M.HeadwaySamples) : 0.f;
	const float MinHeadway = (M.HeadwaySamples > 0) ? M.MinHeadwaySeconds : 0.f;

	UE_LOG(LogTraffic, Display, TEXT("[TrafficMetrics] Test=%s"), *TestName);
	UE_LOG(LogTraffic, Display, TEXT("[TrafficMetrics] VehiclesSpawned=%d VehiclesCompleted=%d VehiclesStuck=%d VehiclesCulled=%d"),
		M.VehiclesSpawned, M.VehiclesCompleted, M.VehiclesStuck, M.VehiclesCulled);
	UE_LOG(LogTraffic, Display, TEXT("[TrafficMetrics] MaxLatErrCm=%.2f AvgLatErrCm=%.2f MaxHeadErrDeg=%.2f AvgHeadErrDeg=%.2f"),
		M.MaxLateralErrorCm, AvgLat, M.MaxHeadingErrorDeg, AvgHead);
	UE_LOG(LogTraffic, Display, TEXT("[TrafficMetrics] MinHeadwaySec=%.2f AvgHeadwaySec=%.2f HeadwaySamples=%d"),
		MinHeadway, AvgHeadway, M.HeadwaySamples);
	UE_LOG(LogTraffic, Display, TEXT("[TrafficMetrics] MaxAccel=%.2f MaxJerk=%.2f EmergencyBrakes=%d HardCollisions=%d GridlockSec=%.2f SimTimeSec=%.2f"),
		M.MaxAccelCmPerSec2, M.MaxJerkCmPerSec3,
		M.NumEmergencyBrakes, M.NumHardCollisions,
		M.GridlockSeconds, M.SimulatedSeconds);
}
