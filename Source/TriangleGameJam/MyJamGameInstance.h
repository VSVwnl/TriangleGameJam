#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "MyJamGameInstance.generated.h"

UCLASS()
class TRIANGLEGAMEJAM_API UMyJamGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	// 用来存储“我应该在哪里出生”的标签
	// 比如 "Spawn_Book1", "Spawn_Book2"
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Level Flow")
	FName TargetSpawnTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Transition")
	uint8 bIsCharacter2D : 1;
};