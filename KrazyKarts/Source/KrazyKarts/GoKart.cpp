// Fill out your copyright notice in the Description page of Project Settings.


#include "GoKart.h"

#include "DrawDebugHelpers.h"
#include "UnrealNetwork.h"
#include "Components/InputComponent.h"
#include "Engine/Engine.h"
#include "GameFramework/GameStateBase.h"


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

    DOREPLIFETIME(AGoKart, ServerState);
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

FGoKartMove AGoKart::CreateMove(const float& DeltaTime) const
{
	FGoKartMove Move;
	Move.Throttle = Throttle;
	Move.SteeringThrow = SteeringThrow;
	Move.DeltaTime = DeltaTime;
	Move.Time = GetWorld()->GetGameState()->GetServerWorldTimeSeconds();

	return Move;
}

void AGoKart::ClearAcknowledgeMoves(const FGoKartMove& LastMove)
{
	TArray<FGoKartMove> NewMoves;

	for (const FGoKartMove& Move : UnacknowledgedMoves)
	{
		if (Move.Time > LastMove.Time)
		{
			NewMoves.Add(Move);
		}
	}

	UnacknowledgedMoves = NewMoves;
}

// Called every frame
void AGoKart::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);


	switch (Role)
	{
		case ENetRole::ROLE_AutonomousProxy:
		{
			const FGoKartMove Move = CreateMove(DeltaTime);
			UnacknowledgedMoves.Add(Move);
			UE_LOG(LogTemp, Warning, TEXT("Queue length: %d"), UnacknowledgedMoves.Num());

			Server_SendMove(Move);

			SimulatedMove(Move);
			break;
		}
		case ENetRole::ROLE_Authority:
		{
			if (IsLocallyControlled())
			{
				const FGoKartMove Move = CreateMove(DeltaTime);
				Server_SendMove(Move);
			}
			break;
		}
		case ENetRole::ROLE_SimulatedProxy:
		{
			SimulatedMove(ServerState.LastMove);
			break;
		}
		default:
		{
			break;
		}
	}


	// ƒfƒoƒbƒO•¶Žš—ñ•\Ž¦
	{
		FString DebugString;
		DebugString += UEnum::GetValueAsString(GetLocalRole());
		DebugString += TEXT("\n");
		DebugString += FString::Printf(TEXT("HasAuthority: %s"), HasAuthority() ? TEXT("true") : TEXT("false"));
		DebugString += TEXT("\n");
		DebugString += FString::Printf(TEXT("IsLocallyControlled: %s"), IsLocallyControlled() ? TEXT("true") : TEXT("false"));
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

void AGoKart::SimulatedMove(const FGoKartMove& Move)
{
	FVector Force = GetActorForwardVector() * MaxDrivingForce * Move.Throttle;

	// ‹ó‹C’ïR
	Force += GetAirResistance();
	// “]‚ª‚è’ïR
	Force += GetRollingResistance();


	const FVector Acceleration = Force / Mass;
	

	Velocity += Acceleration * Move.DeltaTime;

	if (GEngine
		&& IsLocallyControlled()
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

void AGoKart::MoveForward(float Value)
{
	Throttle = Value;
}

void AGoKart::MoveRight(float Value)
{
	SteeringThrow = Value;
}

void AGoKart::Server_SendMove_Implementation(FGoKartMove Move)
{
	SimulatedMove(Move);

	ServerState.Transform = GetActorTransform();
	ServerState.LastMove = Move;
	ServerState.Velocity = Velocity;
}

bool AGoKart::Server_SendMove_Validate(FGoKartMove Move)
{
	return true; // TODO: make better
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

void AGoKart::ApplyRotation(float DeltaTime, float SteeringThrow)
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

void AGoKart::OnRep_ServerState()
{
	UE_LOG(LogTemp, Warning, TEXT("OnRep_ServerState"));

	SetActorTransform(ServerState.Transform);
	Velocity = ServerState.Velocity;


	ClearAcknowledgeMoves(ServerState.LastMove);

	for (const FGoKartMove& Move : UnacknowledgedMoves)
	{
		SimulatedMove(Move);
	}
}
