// Fill out your copyright notice in the Description page of Project Settings.


#include "GoKartMoveComponent.h"

#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/GameStateBase.h"

// Sets default values for this component's properties
UGoKartMoveComponent::UGoKartMoveComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}


// Called when the game starts
void UGoKartMoveComponent::BeginPlay()
{
	Super::BeginPlay();

	// ...
	
}


// Called every frame
void UGoKartMoveComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);


	// 操作してるアクターの場合のみ処理する
	if (const APawn* Pawn = CastChecked<APawn>(GetOwner()))
	{
		if (Pawn->IsLocallyControlled())
		{
			LastMove = CreateMove(DeltaTime);
			SimulatedMove(LastMove);
		}
	}
}

FGoKartMove UGoKartMoveComponent::CreateMove(const float& DeltaTime) const
{
	FGoKartMove Move;
	Move.Throttle = Throttle;
	Move.SteeringThrow = SteeringThrow;
	Move.DeltaTime = DeltaTime;
	Move.Time = GetWorld()->GetGameState()->GetServerWorldTimeSeconds();

	return Move;
}

void UGoKartMoveComponent::SimulatedMove(const FGoKartMove& Move)
{
	FVector Force = GetOwner()->GetActorForwardVector() * MaxDrivingForce * Move.Throttle;

	// 空気抵抗
	Force += GetAirResistance();
	// 転がり抵抗
	Force += GetRollingResistance();


	const FVector Acceleration = Force / Mass;
	

	Velocity += Acceleration * Move.DeltaTime;

	APawn* Pawn = CastChecked<APawn>(GetOwner());
	if (GEngine &&
		Pawn && Pawn->IsLocallyControlled()
		)
	{
		FString DebugString;
		DebugString += FString::Printf(TEXT("Velocity: %s"), *Velocity.ToString());
		DebugString += TEXT("\n");
		DebugString += FString::Printf(TEXT("Speed: %f"), Velocity.Size());
		GEngine->AddOnScreenDebugMessage(INDEX_NONE, Move.DeltaTime, FColor::Green, DebugString);
	}

	UpdateLocationFromVelocity(Move.DeltaTime);

	ApplyRotation(Move.DeltaTime, Move.SteeringThrow);

}

const FVector& UGoKartMoveComponent::GetVelocity() const
{
	return Velocity;
}

void UGoKartMoveComponent::SetVelocity(const FVector& Velocity)
{
	this->Velocity = Velocity;
}

void UGoKartMoveComponent::SetThrottle(float Throttle)
{
	this->Throttle = Throttle;
}

void UGoKartMoveComponent::SetSteeringThrow(float SteeringThrow)
{
	this->SteeringThrow = SteeringThrow;
}


const FGoKartMove& UGoKartMoveComponent::GetLastMove() const
{
	return LastMove;
}

void UGoKartMoveComponent::UpdateLocationFromVelocity(float DeltaTime)
{
	const FVector Translation = Velocity * 100 * DeltaTime;

	FHitResult HitResult;
	GetOwner()->AddActorWorldOffset(Translation, true, &HitResult);
	if (HitResult.IsValidBlockingHit())
	{
		Velocity = FVector::ZeroVector;
	}
}

void UGoKartMoveComponent::ApplyRotation(float DeltaTime, float SteeringThrow)
{
	const float DeltaLocation = FVector::DotProduct(GetOwner()->GetActorForwardVector(), Velocity) * DeltaTime;
	const float RotationAngle = DeltaLocation / MinTurningRadius * SteeringThrow;
	const FQuat RotationDelta(GetOwner()->GetActorUpVector(), RotationAngle);

	GetOwner()->AddActorWorldRotation(RotationDelta);

	Velocity = RotationDelta.RotateVector(Velocity);
}

FVector UGoKartMoveComponent::GetAirResistance() const
{
	return -(Velocity.GetSafeNormal() * Velocity.SizeSquared() * DragCoefficient);
}

FVector UGoKartMoveComponent::GetRollingResistance() const
{
	const float AccelerationDueToGravity = -GetWorld()->GetGravityZ() / 100;
	const float NormalForce = Mass * AccelerationDueToGravity;
	const FVector Result = -(Velocity.GetSafeNormal() * NormalForce * RollingResistanceCoefficient);
	
	return Result;
}
