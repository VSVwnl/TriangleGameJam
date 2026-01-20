#include "CameraTransitionTrigger.h"
#include "Components/BoxComponent.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Character.h"
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
	// 【关键点】转换成 APlatformingCharacter
	APlatformingCharacter* PlayerChar = Cast<APlatformingCharacter>(OtherActor);

	if (PlayerChar)
	{
		APlayerController* PC = Cast<APlayerController>(PlayerChar->GetController());
		if (PC)
		{
			AActor* NewViewTarget = nullptr;

			if (bSwitchToFixedCamera && TargetCameraActor)
			{
				// === 进入 2D ===
				NewViewTarget = TargetCameraActor;

				// 调用你在 PlatformingCharacter 里写的变身函数
				PlayerChar->ToggleSideScrollMode(true);
			}
			else
			{
				// === 回到 3D ===
				NewViewTarget = PlayerChar;

				// 解除变身
				PlayerChar->ToggleSideScrollMode(false);
			}

			if (NewViewTarget)
			{
				PC->SetViewTargetWithBlend(NewViewTarget, BlendTime, VTBlend_Cubic, 0.0f, true);
			}
		}
	}
}