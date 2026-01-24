#include "CameraTransitionTrigger.h"
#include "Components/BoxComponent.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"
#include "TimerManager.h"

// [新增] 引用你的 GameInstance
#include "MyJamGameInstance.h" 

ACameraTransitionTrigger::ACameraTransitionTrigger()
{
	TriggerBox = CreateDefaultSubobject<UBoxComponent>(TEXT("TriggerBox"));
	RootComponent = TriggerBox;
	TriggerBox->SetCollisionProfileName(TEXT("Trigger"));
	TriggerBox->OnComponentBeginOverlap.AddDynamic(this, &ACameraTransitionTrigger::OnOverlapBegin);
}

void ACameraTransitionTrigger::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);

	if (PC && !LevelToLoad.IsNone() && OtherActor == PC->GetPawn())
	{
		// === [核心修改] 存入记忆 ===
		// 获取 GameInstance
		UMyJamGameInstance* GI = Cast<UMyJamGameInstance>(GetGameInstance());
		if (GI && !SaveReturnTag.IsNone())
		{
			// 把 "Spawn_Book1" 这种标签存下来
			GI->TargetSpawnTag = SaveReturnTag;
		}
		// =========================

		if (APawn* Pawn = PC->GetPawn())
		{
			Pawn->DisableInput(PC);
		}

		PC->PlayerCameraManager->StartCameraFade(0.0f, 1.0f, FadeDuration, FLinearColor::Black, false, true);

		GetWorld()->GetTimerManager().SetTimer(
			TransitionTimerHandle,
			this,
			&ACameraTransitionTrigger::PerformLevelTransition,
			FadeDuration,
			false
		);
	}
}

void ACameraTransitionTrigger::PerformLevelTransition()
{
	UGameplayStatics::OpenLevel(this, LevelToLoad);
}