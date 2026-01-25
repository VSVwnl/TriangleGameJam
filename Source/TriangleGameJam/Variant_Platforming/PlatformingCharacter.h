// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Animation/AnimInstance.h"
#include "PlatformingCharacter.generated.h"


class USpringArmComponent;
class UCameraComponent;
class UInputAction;
struct FInputActionValue;
class UAnimMontage;

/**
 * An enhanced Third Person Character with the following functionality:
 * - Platforming game character movement physics
 * - Press and Hold Jump
 * - Double Jump
 * - Wall Jump
 * - Dash
 */

UENUM(BlueprintType)
enum class EPlatformingMovementState : uint8
{
	Normal,
	LedgeGrabbing,
	Climbing
};

UCLASS(abstract)
class APlatformingCharacter : public ACharacter
{
	GENERATED_BODY()

	/** Camera boom positioning the camera behind the character */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	USpringArmComponent* CameraBoom;

	/** Follow camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	UCameraComponent* FollowCamera;

protected:

	/** Jump Input Action */
	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* JumpAction;

	/** Move Input Action */
	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* MoveAction;

	/** Look Input Action */
	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* LookAction;

	/** Mouse Look Input Action */
	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* MouseLookAction;

	/** Dash Input Action */
	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* DashAction;

	/** Sprint Input Action */
	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* SprintAction;

public:

	/** Constructor */
	APlatformingCharacter();

	/** Event Tick*/
	virtual void Tick(float DeltaTime) override;

	/** BeginPlay setup */
	virtual void BeginPlay() override;

protected:

	/** Called for movement input */
	void Move(const FInputActionValue& Value);

	/** Called for looking input */
	void Look(const FInputActionValue& Value);

	/** Called for dash input */
	void Dash();

	/** Called for sprint input */
	void Sprint();

	/** Called for jump pressed to check for advanced multi-jump conditions */
	void MultiJump();

	/** Resets the wall jump input lock */
	void ResetWallJump();

	// Check for mantle opportunity in front of the character
	void CheckForMantle();

	// Initiates a ledge grab at the specified location and normal
	void StartLedgeGrab(const FVector& LedgeLocation, const FRotator& LedgeNormal);

	// Stops the ledge grab and returns to normal movement
	UFUNCTION(BlueprintCallable, Category = "Mantle")
	void StopLedgeGrab();

	// Completes the ledge climb and places the character on top of the ledge
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Mantle")
	void ClimbUpLedge();

public:

	/** Handles move inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category = "Input")
	virtual void DoMove(float Right, float Forward);

	/** Handles look inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category = "Input")
	virtual void DoLook(float Yaw, float Pitch);

	/** Handles dash inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category = "Input")
	virtual void DoDash();

	/** Handles jump pressed inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category = "Input")
	virtual void DoJumpStart();

	/** Handles jump pressed inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category = "Input")
	virtual void DoJumpEnd();

	/** Handles jump pressed inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category = "Input")
	virtual void DoSprint();

	UFUNCTION(BlueprintCallable, Category = "Transition")
	bool GetIs2D();

	UFUNCTION(BlueprintCallable, Category = "Transition")
	void SetIs2D(bool NewIs2D);
protected:

	/** Called from a delegate when the dash montage ends */
	void DashMontageEnded(UAnimMontage* Montage, bool bInterrupted);


public:

	/** Ends the dash state */
	void EndDash();

	/** Stops sprinting */
	void StopSprint();

public:

	/** Returns true if the character has just double jumped */
	UFUNCTION(BlueprintPure, Category = "Platforming")
	bool HasDoubleJumped() const;

	/** Returns true if the character has just wall jumped */
	UFUNCTION(BlueprintPure, Category = "Platforming")
	bool HasWallJumped() const;

public:

	/** EndPlay cleanup */
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** Sets up input action bindings */
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	/** Handle landings to reset dash and advanced jump state */
	virtual void Landed(const FHitResult& Hit) override;

	/** Handle movement mode changes to keep track of coyote time jumps */
	virtual void OnMovementModeChanged(EMovementMode PrevMovementMode, uint8 PreviousCustomMode = 0) override;

protected:

	/** movement state flag bits, packed into a uint8 for memory efficiency */
	uint8 bHasWallJumped : 1;
	uint8 bHasDoubleJumped : 1;
	uint8 bHasDashed : 1;
	uint8 bIsDashing : 1;
	uint8 bIsSprinting : 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Transition")
	uint8 bIs2D : 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	uint8 bIsMantled : 1;

	/** timer for wall jump input reset */
	FTimerHandle WallJumpTimer;

	/** Dash montage ended delegate */
	FOnMontageEnded OnDashMontageEnded;

	/** Distance to trace ahead of the character to look for walls to jump from */
	UPROPERTY(EditAnywhere, Category = "Wall Jump", meta = (ClampMin = 0, ClampMax = 1000, Units = "cm"))
	float WallJumpTraceDistance = 50.0f;

	/** Radius of the wall jump sphere trace check */
	UPROPERTY(EditAnywhere, Category = "Wall Jump", meta = (ClampMin = 0, ClampMax = 100, Units = "cm"))
	float WallJumpTraceRadius = 50.0f;

	/** Impulse to apply away from the wall when wall jumping */
	UPROPERTY(EditAnywhere, Category = "Wall Jump", meta = (ClampMin = 0, ClampMax = 10000, Units = "cm/s"))
	float WallJumpBounceImpulse = 800.0f;

	/** Vertical impulse to apply when wall jumping */
	UPROPERTY(EditAnywhere, Category = "Wall Jump", meta = (ClampMin = 0, ClampMax = 10000, Units = "cm/s"))
	float WallJumpVerticalImpulse = 900.0f;

	/** Time to ignore jump inputs after a wall jump */
	UPROPERTY(EditAnywhere, Category = "Wall Jump", meta = (ClampMin = 0, ClampMax = 5, Units = "s"))
	float DelayBetweenWallJumps = 0.1f;

	/** AnimMontage to use for the Dash action */
	UPROPERTY(EditAnywhere, Category = "Dash")
	UAnimMontage* DashMontage;

	/** AnimMontage to use for the Ledge Grab action */
	UPROPERTY(EditAnywhere, Category = "Mantle")
	UAnimMontage* LedgeGrabMontage;

	/** AnimMontage to use for the Ledge Grab action */
	UPROPERTY(EditAnywhere, Category = "Mantle")
	UAnimMontage* ClimbLedgeMontage;

	/** Last recorded time when this character started falling */
	float LastFallTime = 0.0f;

	/** Max amount of time that can pass since we started falling when we allow a regular jump */
	UPROPERTY(EditAnywhere, Category = "Coyote Time", meta = (ClampMin = 0, ClampMax = 5, Units = "s"))
	float MaxCoyoteTime = 0.16f;

	// Default movement speed (set in BeginPlay)
	float DefaultMaxWalkSpeed;

	// Sprint speed modifier or value
	UPROPERTY(EditDefaultsOnly, Category = "Movement")
	float SprintSpeed = 800.0f;

	/** Seconds to wait after releasing a ledge before allowing mantle checks again */
	UPROPERTY(EditAnywhere, Category = "Ledge")
	float LedgeRegrabDelay = 0.25f;

	/** World time when the ledge was last released (used to block immediate regrab) */
	float LastLedgeReleaseTime = -1000.0f;

	// Component we are currently mantled to (so we follow moving platforms)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mantling")
	UPrimitiveComponent* MantledComponent = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mantling")
	AActor* MantledActor = nullptr;
	
	// Relative location to maintain while mantled
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mantling")
	FVector MantleWorldLocation;

public:
	/** Returns CameraBoom subobject **/
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }

	/** Returns FollowCamera subobject **/
	FORCEINLINE class UCameraComponent* GetFollowCamera() const { return FollowCamera; }

	// =====================================================================
	// [Game Jam Additions] - 2D/3D Mode Switching
	// =====================================================================
public:
	/** 标记当前是否处于 2D 模式 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Game Jam")
	bool bIsSideScrollMode = false;

	/** 切换 2D/3D 模式 (锁定轴向) */
	UFUNCTION(BlueprintCallable, Category = "Game Jam")
	void ToggleSideScrollMode(bool bEnable);
	// =====================================================================

private:

	// Mantle variables to manage forward-hold to climb
	float MantleStartTime = 0.0f;
	float MantleForwardHoldStartTime = 0.0f;
	bool bForwardHoldActive = false;
	float ClimbUpHoldThreshold = 0.4f; // seconds required holding forward to climb (tweakable)

	protected:
		// === Health System ===
		UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health")
		int32 MaxHealth = 3;

		UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Health")
		int32 CurrentHealth;
	
	    //This will allow your C++ code to tell the Blueprint exactly
		UFUNCTION(BlueprintImplementableEvent, Category = "Health")
		void OnHealthUpdate(int32 NewHealth);
	
	
		UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health")
		FVector ManualRespawnLocation;

		// === ReStart System ===
		// Record the initial position (PlayerStart)
		FVector InitialSpawnLocation;

		// Record the last checkpoint position
		FVector LastCheckpointLocation;

		// Record the respawn rotation
		FRotator RespawnRotation;

public:
	// === For outside ===

	// Player Get Hurt (For Spikes)
	UFUNCTION(BlueprintCallable, Category = "Health")
	void TakeDamage();

	// Update the Checkpoint (For Checkpoint)
	UFUNCTION(BlueprintCallable, Category = "Checkpoint")
	void UpdateCheckpoint(FVector NewLocation);

	// Add this to your Protected section
	UFUNCTION(BlueprintImplementableEvent, Category = "Health")
	void OnPlayerDied();

	// Add this to your Public section so BP can call it
	UFUNCTION(BlueprintCallable, Category = "Health")
	void FinalizeRespawn();
};