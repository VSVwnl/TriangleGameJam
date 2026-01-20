#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CameraTransitionTrigger.generated.h"

UCLASS()
class ACameraTransitionTrigger : public AActor
{
	GENERATED_BODY()

public:
	ACameraTransitionTrigger();

protected:
	UPROPERTY(VisibleAnywhere)
	class UBoxComponent* TriggerBox;

	// 目标摄像机
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Settings")
	AActor* TargetCameraActor;

	// 勾选=进2D；不勾选=回3D
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Settings")
	bool bSwitchToFixedCamera = true;

	// 切换时间
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Settings")
	float BlendTime = 1.5f;

	UFUNCTION()
	void OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
};