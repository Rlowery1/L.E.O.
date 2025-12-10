#pragma once

#include "CoreMinimal.h"
#include "TrafficAutomationLogger.generated.h"

UCLASS()
class TRAFFICRUNTIME_API UTrafficAutomationLogger : public UObject
{
	GENERATED_BODY()

public:
	UFUNCTION()
	static void BeginTestLog(const FString& TestName);

	UFUNCTION()
	static void LogLine(const FString& Line);

	UFUNCTION()
	static void LogMetric(const FString& Key, const FString& Value);

	UFUNCTION()
	static void LogMetricFloat(const FString& Key, float Value, int32 Precision = 3);

	UFUNCTION()
	static void EndTestLog();

private:
	static FString CurrentLogFilePath;
	static bool bLogOpen;

	static void EnsureDirectoryExists();
	static void AppendToFile(const FString& Text);
};

