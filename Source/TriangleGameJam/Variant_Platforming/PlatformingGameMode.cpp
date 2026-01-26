// Copyright Epic Games, Inc. All Rights Reserved.


#include "Variant_Platforming/PlatformingGameMode.h"
#include "Kismet/GameplayStatics.h"
#include "MyJamGameInstance.h"

APlatformingGameMode::APlatformingGameMode()
{
	// stub
}

AActor* APlatformingGameMode::ChoosePlayerStart_Implementation(AController* Player)
{
	// 1. Get the persistent Game Instance
	UMyJamGameInstance* GI = Cast<UMyJamGameInstance>(GetGameInstance());

	// 2. Check if a specific spawn tag was set (from Main Menu or a Portal)
	if (GI && !GI->TargetSpawnTag.IsNone())
	{
		TArray<AActor*> FoundActors;
		// 3. Search for a PlayerStart or Actor with a matching tag
		UGameplayStatics::GetAllActorsWithTag(GetWorld(), GI->TargetSpawnTag, FoundActors);

		if (FoundActors.Num() > 0)
		{
			return FoundActors[0]; // Spawns the player here
		}
	}

	// 4. Default fallback: use the standard Player Start logic
	return Super::ChoosePlayerStart_Implementation(Player);
}