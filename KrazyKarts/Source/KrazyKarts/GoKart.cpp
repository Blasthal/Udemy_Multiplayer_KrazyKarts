// Fill out your copyright notice in the Description page of Project Settings.


#include "GoKart.h"

#include "DrawDebugHelpers.h"
#include "UnrealNetwork.h"
#include "Components/InputComponent.h"
#include "Engine/Engine.h"


// Sets default values
AGoKart::AGoKart()
{
 	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	bReplicates = true;
}

void AGoKart::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(AGoKart, ReplicatedTransform);
}

// Called when the game starts or when spawned
void AGoKart::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority())
	{
		NetUpdateFrequency = 1;
	}
}

// Called every frame
void AGoKart::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);


	FVector Force = GetActorForwardVector() * MaxDrivingForce * Throttle;

	// 空気抵抗
	Force += GetAirResistance();
	// 転がり抵抗
	Force += GetRollingResistance();


	const FVector Acceleration = Force / Mass;
	

	Velocity += Acceleration * DeltaTime;

	if (GEngine)
	{
		FString DebugString;
		DebugString += FString::Printf(TEXT("Velocity: %s"), *Velocity.ToString());
		DebugString += TEXT("\n");
		DebugString += FString::Printf(TEXT("Speed: %f"), Velocity.Size());
		GEngine->AddOnScreenDebugMessage(INDEX_NONE, DeltaTime, FColor::Green, DebugString);
	}

	UpdateLocationFromVelocity(DeltaTime);

	ApplyRotation(DeltaTime);


	// トランスフォーム同期元情報
	if (HasAuthority())
	{
		ReplicatedTransform = GetActorTransform();
	}


	// デバッグ文字列表示
	{
		FString DebugString;
		DebugString += UEnum::GetValueAsString(GetLocalRole());
		DebugString += TEXT("\n");
		DebugString += FString::Printf(TEXT("HasAuthority: %s"), HasAuthority() ? TEXT("true") : TEXT("false"));
		DebugString += TEXT("\n");
		DebugString += GetActorLocation().ToString();

		DrawDebugString(
			GetWorld(),
			FVector(0.0f, 0.0f, 100.0f),
			DebugString,
			this,
			FColor::Yellow,
			DeltaTime / 2,
			true,
			1.0f
		);
	}
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
	Server_MoveForward(Value);
}

void AGoKart::Server_MoveForward_Implementation(float Value)
{
	Throttle = Value;
}

void AGoKart::MoveRight(float Value)
{
	SteeringThrow = Value;
	Server_MoveRight(Value);
}

bool AGoKart::Server_MoveForward_Validate(float Value)
{
	return (FMath::Abs(Value) <= 1);
}

void AGoKart::Server_MoveRight_Implementation(float Value)
{
	SteeringThrow = Value;
}

bool AGoKart::Server_MoveRight_Validate(float Value)
{
	return (FMath::Abs(Value) <= 1);
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
	const float DeltaLocation = FVector::DotProduct(GetActorForwardVector(), Velocity) * DeltaTime;
	const float RotationAngle = DeltaLocation / MinTurningRadius * SteeringThrow;
	const FQuat RotationDelta(GetActorUpVector(), RotationAngle);

	AddActorWorldRotation(RotationDelta);

	Velocity = RotationDelta.RotateVector(Velocity);
}

FVector AGoKart::GetAirResistance() const
{
	return -(Velocity.GetSafeNormal() * Velocity.SizeSquared() * DragCoefficient);
}

FVector AGoKart::GetRollingResistance() const
{
	const float AccelerationDueToGravity = -GetWorld()->GetGravityZ() / 100;
	const float NormalForce = Mass * AccelerationDueToGravity;
	const FVector Result = -(Velocity.GetSafeNormal() * NormalForce * RollingResistanceCoefficient);
	
	return Result;
}

void AGoKart::OnRep_ReplicatedTransform()
{
	UE_LOG(LogTemp, Warning, TEXT("OnRep_ReplicatedTransform"));

	SetActorTransform(ReplicatedTransform);
}
