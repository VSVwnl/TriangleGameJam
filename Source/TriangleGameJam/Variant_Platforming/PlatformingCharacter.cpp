// Copyright Epic Games, Inc. All Rights Reserved.
#include "PlatformingCharacter.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Camera/CameraComponent.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedInputComponent.h"
#include "TimerManager.h"
#include "Engine/LocalPlayer.h"
#include "DrawDebugHelpers.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Engine/EngineTypes.h"
#include "Kismet/GameplayStatics.h"


APlatformingCharacter::APlatformingCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	// initialize the flags
	bHasWallJumped = false;
	bHasDoubleJumped = false;
	bHasDashed = false;
	bIsDashing = false;

	// bind the attack montage ended delegate
	OnDashMontageEnded.BindUObject(this, &APlatformingCharacter::DashMontageEnded);

	// enable press and hold jump
	JumpMaxHoldTime = 0.4f;

	// set the jump max count to 3 so we can double jump and check for coyote time jumps
	JumpMaxCount = 3;

	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(35.0f, 90.0f);

	// don't rotate the mesh when the controller rotates
	bUseControllerRotationYaw = false;

	// Configure character movement
	GetCharacterMovement()->GravityScale = 2.5f;
	GetCharacterMovement()->MaxAcceleration = 1500.0f;
	GetCharacterMovement()->BrakingFrictionFactor = 1.0f;
	GetCharacterMovement()->bUseSeparateBrakingFriction = true;

	GetCharacterMovement()->GroundFriction = 4.0f;
	GetCharacterMovement()->MaxWalkSpeed = 750.0f;
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.0f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2500.0f;
	GetCharacterMovement()->PerchRadiusThreshold = 15.0f;

	GetCharacterMovement()->JumpZVelocity = 350.0f;
	GetCharacterMovement()->BrakingDecelerationFalling = 750.0f;
	GetCharacterMovement()->AirControl = 1.0f;

	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f);
	GetCharacterMovement()->bOrientRotationToMovement = true;

	GetCharacterMovement()->NavAgentProps.AgentRadius = 42.0;
	GetCharacterMovement()->NavAgentProps.AgentHeight = 192.0;

	// create the camera boom
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);

	CameraBoom->TargetArmLength = 400.0f;
	CameraBoom->bUsePawnControlRotation = true;
	CameraBoom->bEnableCameraLag = true;
	CameraBoom->CameraLagSpeed = 8.0f;
	CameraBoom->bEnableCameraRotationLag = true;
	CameraBoom->CameraRotationLagSpeed = 8.0f;

	// create the orbiting camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;
}

void APlatformingCharacter::Move(const FInputActionValue& Value)
{
	FVector2D MovementVector = Value.Get<FVector2D>();

	// route the input
	DoMove(MovementVector.X, MovementVector.Y);
}

void APlatformingCharacter::Look(const FInputActionValue& Value)
{
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	// route the input
	DoLook(LookAxisVector.X, LookAxisVector.Y);
}

void APlatformingCharacter::DoLook(float Yaw, float Pitch)
{
	if (GetController() != nullptr)
	{
		// add yaw and pitch input to controller
		AddControllerYawInput(Yaw);
		AddControllerPitchInput(Pitch);
	}
}

void APlatformingCharacter::DoDash()
{
	// ignore the input if we've already dashed and have yet to reset
	if (bHasDashed || bIsDashing || bIsMantled)
		return;
	
	UGameplayStatics::PlaySound2D(this, DashSound);

	// raise the dash flags
	bIsDashing = true;
	bHasDashed = true;

	// disable gravity while dashing
	if (GetCharacterMovement())
	{
		GetCharacterMovement()->GravityScale = 0.0f;
		// reset the character velocity so we don't carry momentum into the dash
		GetCharacterMovement()->Velocity = FVector::ZeroVector;
	}

	// play the dash montage
	if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
	{
		// don't restart the montage if it's already playing
		if (AnimInstance->Montage_IsPlaying(DashMontage))
			return;

		UE_LOG(LogTemp, Display, TEXT("Dashed - Gravity disabled"));
		const float MontageLength = AnimInstance->Montage_Play(DashMontage, 1.0f, EMontagePlayReturnType::MontageLength, 0.0f, true);

		// has the montage played successfully?
		if (MontageLength > 0.0f)
		{
			AnimInstance->Montage_SetEndDelegate(OnDashMontageEnded, DashMontage);
		}
	}
}

void APlatformingCharacter::Sprint()
{
	DoSprint();
}



void APlatformingCharacter::Dash()
{
	// route the input
	DoDash();
}

/** Check for mantle opportunity in front of the character */
void APlatformingCharacter::CheckForMantle()
{
	// prevent immediate re-mantle after release
	if (GetWorld() && (GetWorld()->GetTimeSeconds() - LastLedgeReleaseTime) < LedgeRegrabDelay)
	{
		return;
	}

	FVector StartLocation = GetActorLocation() + (GetActorForwardVector() * 45.0f) + FVector(0, 0, 100.f);
	FVector EndLocation = GetActorLocation() + (GetActorForwardVector() * 45.f) + FVector(0, 0, 50.f);
	FVector BoxHalfSize(25.f, 5.f, 1.f);
	FRotator BoxOrientation = GetActorRotation(); // Use actor rotation

	FHitResult WallHit;
	TArray<AActor*> IgnoreActors;
	IgnoreActors.Add(this);

	//  Box Trace By Channel
	bool bWallHit = UKismetSystemLibrary::BoxTraceSingle(
		GetWorld(),             // World Context
		StartLocation,          // Start
		EndLocation,            // End
		BoxHalfSize,            // HalfSize
		BoxOrientation,         // Orientation
		UEngineTypes::ConvertToTraceType(ECC_Visibility), // TraceChannel
		false,                  // bTraceComplex
		IgnoreActors,         // ActorsToIgnore
		EDrawDebugTrace::None, // DrawDebugType (shows debug box for a set duration)
		WallHit,              // OutHit
		true                    // bIgnoreSelf
	);

	if (bWallHit && WallHit.Distance > 0.f && WallHit.GetActor()->ActorHasTag("CanMantle"))
	{
		// Grab Alignment and Rotation

		FHitResult AlignmentHit;
		FVector StartLocationAlignment = GetActorLocation() + (GetActorForwardVector() * 5.0f);
		StartLocationAlignment = FVector(StartLocationAlignment.X, StartLocationAlignment.Y, WallHit.ImpactPoint.Z);
		FVector EndLocationAlignment = WallHit.Location + (GetActorForwardVector() * 5.0f);
		FVector BoxHalfSizeAlignment(5.f, 5.f, 5.f);

		bool bAlignmentHit = UKismetSystemLibrary::BoxTraceSingle(
			GetWorld(),             // World Context
			StartLocationAlignment,          // Start
			EndLocationAlignment,            // End
			BoxHalfSizeAlignment,            // HalfSize
			FRotator(0.f, 0.f, 0.f), // Orientation
			UEngineTypes::ConvertToTraceType(ECC_Visibility),
			false,                  // bTraceComplex
			IgnoreActors,         // ActorsToIgnore
			EDrawDebugTrace::None, // DrawDebugType (shows debug box for a set duration)
			AlignmentHit,              // OutHit
			true                    // bIgnoreSelf
		);

		if (bAlignmentHit)
		{
			// Align With Ledge
			UE_LOG(LogTemp, Display, TEXT("Ledge detected, starting mantle"));
			bIsMantled = true;

			if (AActor* HitActor = AlignmentHit.GetActor())
			{
				// store the component we hit so we can attach to it and follow its movement
				MantledComponent = AlignmentHit.GetComponent();
				MantledActor = HitActor;
				
				FVector Origin;
				FVector BoxExtent;
				HitActor->GetActorBounds(false, Origin, BoxExtent);
				float const GrabHeight = (AlignmentHit.Location.Z) - 55.f; // Adjust for character half height
				float const XValueOffset = AlignmentHit.Location.X - (GetActorForwardVector() * 22.f).X;
				float const YValueOffset = AlignmentHit.Location.Y - (GetActorForwardVector() * 22.f).Y;
				FVector FinalGrabLocation = FVector(XValueOffset, YValueOffset, GrabHeight);
				FRotator FinalGrabOrientation = (AlignmentHit.Normal * -1.0f).Rotation();

				// Use a helper to start the ledge grab (snaps location and disables movement cleanly)
				StartLedgeGrab(FinalGrabLocation, FinalGrabOrientation);
			}
		}
	}
}

void APlatformingCharacter::StartLedgeGrab(const FVector& LedgeLocation, const FRotator& LedgeNormal)
{
	// snap exactly to the ledge (preserve world transform on attach)
	SetActorLocation(LedgeLocation, false, nullptr, ETeleportType::TeleportPhysics);
	SetActorRotation(LedgeNormal);

	// stop any velocity and fully disable movement so character stays locked in place
	GetCharacterMovement()->StopMovementImmediately();
	GetCharacterMovement()->SetMovementMode(EMovementMode::MOVE_None);

	// record mantle start time and reset forward-hold
	MantleStartTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
	MantleForwardHoldStartTime = 0.0f;
	bForwardHoldActive = false;

	// If we have a valid component that we hit, attach to it so the character follows moving platforms
	if (MantledComponent && MantledComponent->IsRegistered())
	{
		// Keep world transform so we remain in the exact snapped location, but become a child of the platform.
		AttachToComponent(MantledComponent, FAttachmentTransformRules::KeepWorldTransform);
	}

	if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
	{
		if (AnimInstance->Montage_IsPlaying(LedgeGrabMontage))
			return;

		UE_LOG(LogTemp, Display, TEXT("Mantling"));
		const float MontageLength = AnimInstance->Montage_Play(LedgeGrabMontage, 1.0f, EMontagePlayReturnType::MontageLength, 0.0f, true);
	}
}

void APlatformingCharacter::StopLedgeGrab()
{
	// Detach from any mantled component first so we don't remain parented to a moved/destroyed actor
	if (MantledComponent)
	{
		DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
		MantledComponent = nullptr;
	}

	if (MantledActor)
	{
		MantledActor = nullptr;
		MantleWorldLocation = FVector(0.f, 0.f, 0.f);
	}

	if (!GetCharacterMovement()->IsFalling())
	{
		if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
		{
			if (AnimInstance->Montage_IsPlaying(LedgeGrabMontage))
			{
				const float BlendOutTime = 0.15f;
				AnimInstance->Montage_Stop(BlendOutTime, LedgeGrabMontage);
			}
		}

		GetCharacterMovement()->SetMovementMode(EMovementMode::MOVE_Walking);
		bIsMantled = false;

		// block mantle checks for a short time so the player can step away
		LastLedgeReleaseTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;

		// reset forward-hold state
		MantleForwardHoldStartTime = 0.0f;
		bForwardHoldActive = false;
	}
}

void APlatformingCharacter::MultiJump()
{
	// are we already in the air?
	if (GetCharacterMovement()->IsFalling())
	{

		// have we already wall jumped?
		if (!bHasWallJumped)
		{
			// run a sphere sweep to check if we're in front of a wall
			FHitResult OutHit;

			const FVector TraceStart = GetActorLocation();
			const FVector TraceEnd = TraceStart + (GetActorForwardVector() * WallJumpTraceDistance);
			const FCollisionShape TraceShape = FCollisionShape::MakeSphere(WallJumpTraceRadius);

			FCollisionQueryParams QueryParams;
			QueryParams.AddIgnoredActor(this);

			if (GetWorld()->SweepSingleByChannel(OutHit, TraceStart, TraceEnd, FQuat(), ECollisionChannel::ECC_Visibility, TraceShape, QueryParams))
			{

				// rotate the character to face away from the wall, so we're correctly oriented for the next wall jump
				FRotator WallOrientation = OutHit.ImpactNormal.ToOrientationRotator();
				WallOrientation.Pitch = 0.0f;
				WallOrientation.Roll = 0.0f;

				SetActorRotation(WallOrientation);

				// apply a launch impulse to the character to perform the actual wall jump
				const FVector WallJumpImpulse = (OutHit.ImpactNormal * WallJumpBounceImpulse) + (FVector::UpVector * WallJumpVerticalImpulse);

				LaunchCharacter(WallJumpImpulse, true, true);

				// raise the wall jump flag to prevent an immediate second wall jump
				bHasWallJumped = true;

				GetWorld()->GetTimerManager().SetTimer(WallJumpTimer, this, &APlatformingCharacter::ResetWallJump, DelayBetweenWallJumps, false);

				UE_LOG(LogTemp, Display, TEXT("Wall Jump"));
			}

			// no wall jump, try a double jump next
			// test for double jump only if we haven't already tested for wall jump
			if (!bHasWallJumped)
			{
				// are we still within coyote time frames?
				if (GetWorld()->GetTimeSeconds() - LastFallTime < MaxCoyoteTime)
				{
					UE_LOG(LogTemp, Display, TEXT("Coyote Jump"));

					// use the built-in CMC functionality to do the jump
					Jump();
					// no coyote time jump
				}
				else {

					// The movement component handles double jump but we still need to manage the flag for animation
					if (!bHasDoubleJumped)
					{
						// raise the double jump flag
						bHasDoubleJumped = true;

						// let the CMC handle jump
						Jump();
					}
				}
				UE_LOG(LogTemp, Display, TEXT("Double Jump"));
			}
		}
	}
	else
	{
		// we're grounded so just do a regular jump
		Jump();
	}
}

void APlatformingCharacter::ResetWallJump()
{
	// reset the wall jump input lock
	bHasWallJumped = false;
}

// =========================================================================================
// [Game Jam Modification] - Updated DoMove to handle 2D Side Scrolling
// =========================================================================================
void APlatformingCharacter::DoMove(float Right, float Forward)
{
	if (GetController() != nullptr)
	{
		// Momentarily disable movement inputs if we've just wall jumped
		if (!bHasWallJumped)
		{
			// ====================================================================
			// 1. Handle Mantling/Ledge Grab Logic
			//    (Check this first to allow 2D inputs to trigger climbing)
			// ====================================================================
			if (bIsMantled)
			{
				// Define the input used for climbing
				float ClimbInput = Forward; // Default 3D behavior: Forward (W) climbs

				// [Game Jam] In 2D Mode, map Right/Left input (A/D) to "Climb Up"
				if (bIsSideScrollMode)
				{
					// If pressing A or D, treat it as pressing W to climb up
					if (FMath::Abs(Right) > 0.1f)
					{
						ClimbInput = 1.0f;
					}
					// If pressing S (Backwards), treat it as dropping down
					else if (Forward < -0.1f)
					{
						ClimbInput = -1.0f;
					}
					else
					{
						ClimbInput = 0.0f;
					}
				}

				// --- Existing Team Logic (Modified to use ClimbInput) ---

				// Negative input: Let player drop/unmantle
				if (ClimbInput < 0)
				{
					// Cancel any forward-hold timing
					MantleForwardHoldStartTime = 0.0f;
					bForwardHoldActive = false;
					StopLedgeGrab();
				}
				// Positive input: Climb up
				else if (ClimbInput > 0)
				{
					// Start or continue the forward-hold timer
					const float Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
					if (!bForwardHoldActive)
					{
						bForwardHoldActive = true;
						MantleForwardHoldStartTime = Now;
					}
					else
					{
						if ((Now - MantleForwardHoldStartTime) >= ClimbUpHoldThreshold)
						{
							// Held input long enough -> Climb up
							ClimbUpLedge();

							// Reset forward-hold after climbing to avoid repeat calls
							bForwardHoldActive = false;
							MantleForwardHoldStartTime = 0.0f;
						}
					}
				}
				else
				{
					// No input: Reset hold timer
					MantleForwardHoldStartTime = 0.0f;
					bForwardHoldActive = false;
				}

				// If mantling, do not process standard movement
				return;
			}

			// ====================================================================
			// 2. Handle Movement (If not mantling)
			// ====================================================================

			if (bIsSideScrollMode)
			{
				// [Game Jam] 2D Mode Logic
				// Move along world Y-axis (RightVector). Ignore Forward input (W/S).
				AddMovementInput(FVector::RightVector, Right);
			}
			else
			{
				// [Original] 3D Mode Logic (Camera Relative)
				const FRotator Rotation = GetController()->GetControlRotation();
				const FRotator YawRotation(0, Rotation.Yaw, 0);

				// Get forward vector
				const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);

				// Get right vector 
				const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

				// Add movement 
				AddMovementInput(ForwardDirection, Forward);
				AddMovementInput(RightDirection, Right);
			}
		}
	}
}
void APlatformingCharacter::DoSprint()
{
	// Change the character's movement speed when the key is pressed (held)
	GetCharacterMovement()->MaxWalkSpeed = SprintSpeed;
	UE_LOG(LogTemp, Warning, TEXT("Sprinting Start $: %f"), SprintSpeed);
}

void APlatformingCharacter::DoJumpStart()
{
	if (bIsMantled)
	{
		// End the ledge grab and allow movement
		StopLedgeGrab();
	}

	// handle special jump cases
	MultiJump();
}

void APlatformingCharacter::DoJumpEnd()
{
	// stop jumping
	StopJumping();
}

void APlatformingCharacter::DashMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	// always end the dash when the montage is done, whether interrupted or completed
	EndDash();
}

void APlatformingCharacter::EndDash()
{
	// restore gravity
	GetCharacterMovement()->GravityScale = 2.5f;

	// reset the dashing flag
	bIsDashing = false;

	// if we're currently on the ground, allow dashing again
	if (GetCharacterMovement() && !GetCharacterMovement()->IsFalling())
	{
		bHasDashed = false;
	}
}

bool APlatformingCharacter::HasDoubleJumped() const
{
	return bHasDoubleJumped;
}

bool APlatformingCharacter::HasWallJumped() const
{
	return bHasWallJumped;
}

void APlatformingCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	// clear the wall jump reset timer
	GetWorld()->GetTimerManager().ClearTimer(WallJumpTimer);
}

void APlatformingCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{

		// Jumping
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &APlatformingCharacter::DoJumpStart);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &APlatformingCharacter::DoJumpEnd);

		// Moving
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &APlatformingCharacter::Move);
		EnhancedInputComponent->BindAction(MouseLookAction, ETriggerEvent::Triggered, this, &APlatformingCharacter::Look);

		// Sprinting
		EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Started, this, &APlatformingCharacter::Sprint);
		EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Completed, this, &APlatformingCharacter::StopSprint);

		// Looking
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &APlatformingCharacter::Look);

		// Dashing
		EnhancedInputComponent->BindAction(DashAction, ETriggerEvent::Triggered, this, &APlatformingCharacter::Dash);
	}
}

void APlatformingCharacter::Landed(const FHitResult& Hit)
{
	Super::Landed(Hit);

	// reset the double jump and dash flags
	bHasDoubleJumped = false;
	bHasDashed = false;

	UE_LOG(LogTemp, Display, TEXT("Landed - Double Jump and Dash reset"));

}

void APlatformingCharacter::OnMovementModeChanged(EMovementMode PrevMovementMode, uint8 PreviousCustomMode /*= 0*/)
{
	Super::OnMovementModeChanged(PrevMovementMode, PreviousCustomMode);

	// are we falling?
	if (GetCharacterMovement()->MovementMode == EMovementMode::MOVE_Falling)
	{
		// save the game time when we started falling, so we can check it later for coyote time jumps
		LastFallTime = GetWorld()->GetTimeSeconds();
	}
}

void APlatformingCharacter::BeginPlay()
{
	Super::BeginPlay();

	// 1. Full Health at Start
	CurrentHealth = MaxHealth;

	// 2. Register the Location and Rotation as the Respawn Point
	InitialSpawnLocation = GetActorLocation();
	RespawnRotation = GetActorRotation();

	// 3. Set Default Checkpoint is the Initial Spawn Point
	LastCheckpointLocation = InitialSpawnLocation;

	// Store the default speed
	DefaultMaxWalkSpeed = GetCharacterMovement()->MaxWalkSpeed;
}

void APlatformingCharacter::UpdateCheckpoint(FVector NewLocation)
{
	LastCheckpointLocation = NewLocation;
	
	UE_LOG(LogTemp, Log, TEXT("Checkpoint Saved!"));
}

void APlatformingCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Check for mantle opportunity each frame
	if (GetCharacterMovement()->MovementMode == EMovementMode::MOVE_Falling && !bIsMantled)
	{
		// Only check for mantle when falling
		CheckForMantle();
	}

	if (bIsMantled)
	{
		if (MantledActor != nullptr)
		{
			MantleWorldLocation = MantledActor->GetActorLocation();
		}
	}
}

void APlatformingCharacter::StopSprint()
{
	// Revert to default speed when the key is released
	GetCharacterMovement()->MaxWalkSpeed = DefaultMaxWalkSpeed;
	UE_LOG(LogTemp, Display, TEXT("Sprinting Start $: %f"), DefaultMaxWalkSpeed);
}

// =========================================================================================
// [Game Jam Modification] - Function to switch modes (Locked X-Axis)
// =========================================================================================
void APlatformingCharacter::ToggleSideScrollMode(bool bEnable)
{
	bIsSideScrollMode = bEnable;

	UCharacterMovementComponent* MoveComp = GetCharacterMovement();
	if (!MoveComp) return;

	if (bEnable) // === Enter 2D Mode ===
	{
		// 1. Lock X-Axis (Depth), Allow Movement on Y (Left/Right) and Z (Up/Down)
		//    Normal = (1, 0, 0)
		MoveComp->SetPlaneConstraintNormal(FVector(1.0f, 0.0f, 0.0f));
		MoveComp->bConstrainToPlane = true;

		// 2. Orient Rotation to Movement (Crucial for Mantling!)
		//    This ensures when you move Right (+Y), the character faces Right.
		//    The teammate's CheckForMantle() uses GetActorForwardVector(), so this makes it work!
		MoveComp->bOrientRotationToMovement = true;

		// 3. Disable Controller Rotation (Don't let mouse turn the character)
		bUseControllerRotationYaw = false;
	}
	else // === Return to 3D Mode ===
	{
		// Un-constrain movement
		MoveComp->bConstrainToPlane = false;

		// Restore any other 3D settings if needed (usually defaults are fine)
	}
}

bool APlatformingCharacter::GetIs2D()
{
	return bIs2D;
}

void APlatformingCharacter::SetIs2D(bool bNewIs2D)
{
	bIs2D = bNewIs2D;
}

//Hurting the character
void APlatformingCharacter::TakeDamage()
{
	CurrentHealth--;
	OnHealthUpdate(CurrentHealth);

	APlayerController* PC = Cast<APlayerController>(GetController());
    
	if (CurrentHealth > 0)
	{
		// Standard respawn
		SetActorLocation(LastCheckpointLocation);
	}
	else
	{
		if (PC && PC->PlayerCameraManager)
		{
			// 1. Fade to Black
			PC->PlayerCameraManager->StartCameraFade(0.f, 1.f, 4.f, FLinearColor::Black, true, true);

			// 2. Delay the teleport so the player doesn't see it
			FTimerHandle RespawnTimer;
			GetWorldTimerManager().SetTimer(RespawnTimer, [this, PC]()
			{
				// Teleport Logic
				SetActorLocation(InitialSpawnLocation);
				SetActorRotation(RespawnRotation);
				CurrentHealth = MaxHealth;
				OnHealthUpdate(CurrentHealth);

				// 3. Fade back in
				if (PC && PC->PlayerCameraManager)
				{
					PC->PlayerCameraManager->StartCameraFade(1.f, 0.f, 4.2f, FLinearColor::Black, true, false);
				}
			}, 0.5f, false);
		}
	}
}

//Health == 0
void APlatformingCharacter::FinalizeRespawn()
{
	// This logic is moved from the old TakeDamage 'else' block
	SetActorLocation(InitialSpawnLocation);
	SetActorRotation(RespawnRotation);

	CurrentHealth = MaxHealth;
	LastCheckpointLocation = InitialSpawnLocation;

	// Reset hearts in the UI
	OnHealthUpdate(CurrentHealth);
}