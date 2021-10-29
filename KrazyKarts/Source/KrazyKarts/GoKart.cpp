// Fill out your copyright notice in the Description page of Project Settings.


#include "GoKart.h"

#include "Components/InputComponent.h"
#include "Engine/Engine.h"

// Sets default values
AGoKart::AGoKart()
{
 	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void AGoKart::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AGoKart::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	const FVector Force = GetActorForwardVector() * MaxDrivingForce * Throttle;
	const FVector Acceleration = Force / Mass;

	Velocity += Acceleration * DeltaTime;
	if (GEngine)
	{
		const FString DebugString = FString::Printf(TEXT("Velocity=%s"), *Velocity.ToString());
		GEngine->AddOnScreenDebugMessage(INDEX_NONE, 0.0f, FColor::Green, DebugString);
	}

	UpdateLocationFromVelocity(DeltaTime);

	ApplyRotation(DeltaTime);
}

// Called to bind functionality to input
void AGoKart::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAxis("MoveForward", this, &AGoKart::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &AGoKart::MoveRight);
}

void AGoKart::MoveForward(float Value)
{
	Throttle = Value;
}

void AGoKart::MoveRight(float Value)
{
	SteeringThrow = Value;
}

void AGoKart::UpdateLocationFromVelocity(float DeltaTime)
{
	const FVector Translation = Velocity * 100 * DeltaTime;

	FHitResult HitResult;
	AddActorWorldOffset(Translation, true, &HitResult);
	if (HitResult.IsValidBlockingHit())
	{
		Velocity = FVector::ZeroVector;
	}
}

void AGoKart::ApplyRotation(float DeltaTime)
{
	const float RotationAngle = MaxDegreesPerSecond * DeltaTime * SteeringThrow;
	const FQuat RotationDelta(GetActorUpVector(), FMath::DegreesToRadians(RotationAngle));

	AddActorWorldRotation(RotationDelta);

	Velocity = RotationDelta.RotateVector(Velocity);
}
