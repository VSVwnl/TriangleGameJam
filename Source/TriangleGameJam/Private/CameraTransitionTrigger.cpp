#include "CameraTransitionTrigger.h"
#include "Components/BoxComponent.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Character.h"

// ?????????????????????????
#include "PlatformingCharacter.h" 

ACameraTransitionTrigger::ACameraTransitionTrigger()
{
	TriggerBox = CreateDefaultSubobject<UBoxComponent>(TEXT("TriggerBox"));
	RootComponent = TriggerBox;
	TriggerBox->SetCollisionProfileName(TEXT("Trigger"));

	TriggerBox->OnComponentBeginOverlap.AddDynamic(this, &ACameraTransitionTrigger::OnOverlapBegin);
}

void ACameraTransitionTrigger::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	// ???????? APlatformingCharacter
	APlatformingCharacter* PlayerChar = Cast<APlatformingCharacter>(OtherActor);

	if (PlayerChar)
	{
		APlayerController* PC = Cast<APlayerController>(PlayerChar->GetController());
		if (PC)
		{
			AActor* NewViewTarget = nullptr;

			if (bSwitchToFixedCamera && TargetCameraActor)
			{
				// === ?? 2D ===
				NewViewTarget = TargetCameraActor;

				// ???? PlatformingCharacter ???????
				PlayerChar->ToggleSideScrollMode(true);
			}
			else
			{
				// === ?? 3D ===
				NewViewTarget = PlayerChar;

				// ????
				PlayerChar->ToggleSideScrollMode(false);
			}

			if (NewViewTarget)
			{
				PC->SetViewTargetWithBlend(NewViewTarget, BlendTime, VTBlend_Cubic, 0.0f, true);
			}
		}
	}
}