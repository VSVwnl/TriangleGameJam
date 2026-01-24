#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CameraTransitionTrigger.generated.h"


UCLASS()
class TRIANGLEGAMEJAM_API ACameraTransitionTrigger : public AActor
{
	GENERATED_BODY()

public:
	ACameraTransitionTrigger();

protected:
	UPROPERTY(VisibleAnywhere)
	class UBoxComponent* TriggerBox;

	// === [现有] 要去哪个关卡 ===
	// 比如: "Lvl_Forest", "Lvl_Cave"
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Transition Settings")
	FName LevelToLoad;

	// === [新增] 回来时的路标 ===
	// 如果这是去程 Trigger，填入这扇门对应的出生点名字 (例如 "Spawn_Book1")
	// 如果这是回程 Trigger (在2D关底)，留空即可
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Transition Settings")
	FName SaveReturnTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Transition Settings")
	float FadeDuration = 1.0f;

	UFUNCTION()
	void OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

private:
	FTimerHandle TransitionTimerHandle;
	void PerformLevelTransition();
};