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

void APlatformingCharacter::Sprint()
{
	DoSprint();
}



void APlatformingCharacter::Dash()
{
	// route the input
	DoDash();
}

void APlatformingCharacter::MultiJump()
{
	// ignore jumps while dashing
	//if(bIsDashing)
		//return;

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
				} else {
		
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

void APlatformingCharacter::DoMove(float Right, float Forward)
{
	if (GetController() != nullptr)
	{
		// momentarily disable movement inputs if we've just wall jumped
		if (!bHasWallJumped)
		{
			// find out which way is forward
			const FRotator Rotation = GetController()->GetControlRotation();
			const FRotator YawRotation(0, Rotation.Yaw, 0);

			// get forward vector
			const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);

			// get right vector 
			const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

			// add movement 
			AddMovementInput(ForwardDirection, Forward);
			AddMovementInput(RightDirection, Right);
		}
	}
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
	if (bHasDashed || bIsDashing)
		return;
	
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

void APlatformingCharacter::DoSprint()
{
	// Change the character's movement speed when the key is pressed (held)
	GetCharacterMovement()->MaxWalkSpeed = SprintSpeed;
	UE_LOG(LogTemp, Warning, TEXT("Sprinting Start $: %f"), SprintSpeed);
}

void APlatformingCharacter::DoJumpStart()
{
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

	// Store the default speed
	DefaultMaxWalkSpeed = GetCharacterMovement()->MaxWalkSpeed;
}



void APlatformingCharacter::StopSprint()
{
	// Revert to default speed when the key is released
	GetCharacterMovement()->MaxWalkSpeed = DefaultMaxWalkSpeed;
	UE_LOG(LogTemp, Display, TEXT("Sprinting Start $: %f"), DefaultMaxWalkSpeed);
}