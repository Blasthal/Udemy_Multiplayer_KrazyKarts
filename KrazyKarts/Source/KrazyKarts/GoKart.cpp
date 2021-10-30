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

	MovementComponent = CreateDefaultSubobject<UGoKartMoveComponent>(TEXT("MovementComponent"));
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


	ensure(MovementComponent);
	if (!MovementComponent)
	{
		return;
	}


	switch (Role)
	{
		case ENetRole::ROLE_AutonomousProxy:
		{
			const FGoKartMove Move = MovementComponent->CreateMove(DeltaTime);
			UnacknowledgedMoves.Add(Move);
			UE_LOG(LogTemp, Warning, TEXT("Queue length: %d"), UnacknowledgedMoves.Num());

			Server_SendMove(Move);

			MovementComponent->SimulatedMove(Move);
			break;
		}
		case ENetRole::ROLE_Authority:
		{
			if (IsLocallyControlled())
			{
				const FGoKartMove Move = MovementComponent->CreateMove(DeltaTime);
				Server_SendMove(Move);
			}
			break;
		}
		case ENetRole::ROLE_SimulatedProxy:
		{
			MovementComponent->SimulatedMove(ServerState.LastMove);
			break;
		}
		default:
		{
			break;
		}
	}


	// デバッグ文字列表示
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

void AGoKart::MoveForward(float Value)
{
	MovementComponent->SetThrottle(Value);
}

void AGoKart::MoveRight(float Value)
{
	MovementComponent->SetSteeringThrow(Value);
}

void AGoKart::Server_SendMove_Implementation(FGoKartMove Move)
{
	MovementComponent->SimulatedMove(Move);

	ServerState.Transform = GetActorTransform();
	ServerState.LastMove = Move;
	ServerState.Velocity = MovementComponent->GetVelocity();
}

bool AGoKart::Server_SendMove_Validate(FGoKartMove Move)
{
	return true; // TODO: make better
}

void AGoKart::OnRep_ServerState()
{
	UE_LOG(LogTemp, Warning, TEXT("OnRep_ServerState"));

	SetActorTransform(ServerState.Transform);
	MovementComponent->SetVelocity(ServerState.Velocity);


	ClearAcknowledgeMoves(ServerState.LastMove);

	for (const FGoKartMove& Move : UnacknowledgedMoves)
	{
		MovementComponent->SimulatedMove(Move);
	}
}
