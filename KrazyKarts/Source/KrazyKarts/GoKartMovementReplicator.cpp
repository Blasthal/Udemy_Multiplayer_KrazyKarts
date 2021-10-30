// Fill out your copyright notice in the Description page of Project Settings.


#include "GoKartMovementReplicator.h"

#include "DrawDebugHelpers.h"
#include "UnrealNetwork.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"

// Sets default values for this component's properties
UGoKartMovementReplicator::UGoKartMovementReplicator()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	SetIsReplicated(true);
}


void UGoKartMovementReplicator::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(UGoKartMovementReplicator, ServerState);
}


// Called when the game starts
void UGoKartMovementReplicator::BeginPlay()
{
	Super::BeginPlay();

	MovementComponent = GetOwner()->FindComponentByClass<UGoKartMoveComponent>();
	ensure(MovementComponent);

	
	if (GetOwner()->HasAuthority())
	{
		GetOwner()->NetUpdateFrequency = 1;
	}
}


// Called every frame
void UGoKartMovementReplicator::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	ensure(MovementComponent);
	if (!MovementComponent)
	{
		return;
	}


	const FGoKartMove& Move = MovementComponent->GetLastMove();

	switch (GetOwnerRole())
	{
		case ENetRole::ROLE_AutonomousProxy:
		{
			UnacknowledgedMoves.Add(Move);
			Server_SendMove(Move);
			break;
		}
		case ENetRole::ROLE_Authority:
		{
			APawn* Pawn = CastChecked<APawn>(GetOwner());
			if (Pawn && Pawn->IsLocallyControlled())
			{
				UpdateServerState(Move);
			}
			break;
		}
		case ENetRole::ROLE_SimulatedProxy:
		{
			ClientTick(DeltaTime);
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

		// Role
		DebugString += UEnum::GetValueAsString(GetOwnerRole());
		DebugString += TEXT("\n");

		// Authority
		DebugString += FString::Printf(TEXT("HasAuthority: %s"), GetOwner()->HasAuthority() ? TEXT("true") : TEXT("false"));
		DebugString += TEXT("\n");

		// LoccallyControlled
		if (const APawn* Pawn = CastChecked<APawn>(GetOwner()))
		{
			DebugString += FString::Printf(TEXT("IsLocallyControlled: %s"), Pawn->IsLocallyControlled() ? TEXT("true") : TEXT("false"));
			DebugString += TEXT("\n");
		}

		// Location
		DebugString += GetOwner()->GetActorLocation().ToString();


		DrawDebugString(
			GetWorld(),
			FVector(0.0f, 0.0f, 100.0f),
			DebugString,
			GetOwner(),
			FColor::Yellow,
			DeltaTime / 2,
			true,
			1.0f
		);
	}
}


void UGoKartMovementReplicator::ClearAcknowledgeMoves(const FGoKartMove& LastMove)
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


void UGoKartMovementReplicator::UpdateServerState(const FGoKartMove& Move)
{
	ServerState.Transform = GetOwner()->GetActorTransform();
	ServerState.LastMove = Move;
	ServerState.Velocity = MovementComponent->GetVelocity();
}


void UGoKartMovementReplicator::ClientTick(float DeltaTime)
{
	ClientTimeSinceUpdate += DeltaTime;

	// ごく僅かな時間なら処理しない
	if (ClientTimeBetweenLastUpdates < KINDA_SMALL_NUMBER)
	{
		return;
	}


	// 位置
	const FVector TargetLocation = ServerState.Transform.GetLocation();
	const float LerpRatio = ClientTimeSinceUpdate / ClientTimeBetweenLastUpdates;
	const FVector StartLocation = ClientStartTransform.GetLocation();
	const FVector NewLocation = FMath::Lerp(StartLocation, TargetLocation, LerpRatio);

	GetOwner()->SetActorLocation(NewLocation);


	// 回転
	const FQuat TargetRotation = ServerState.Transform.GetRotation();
	const FQuat StartRotation = ClientStartTransform.GetRotation();
	const FQuat NewRotation = FQuat::Slerp(StartRotation, TargetRotation, LerpRatio);

	GetOwner()->SetActorRotation(NewRotation);


	// デバッグ表示
	{
		FString DebugString;
		DebugString += FString::Printf(TEXT("ClientTimeSinceUpdate: %f"), ClientTimeSinceUpdate);
		DebugString += TEXT("\n");
		DebugString += FString::Printf(TEXT("ClientTimeBetweenLastUpdates: %f"), ClientTimeBetweenLastUpdates);
		DebugString += TEXT("\n");
		DebugString += FString::Printf(TEXT("LerpRatio: %f"), LerpRatio);
		DebugString += TEXT("\n");

		DrawDebugString(
			GetWorld(),
			FVector::DownVector * 100.0f,
			DebugString,
			this->GetOwner(),
			FColor::White,
			DeltaTime / 2,
			true
		);
	}
}


void UGoKartMovementReplicator::Server_SendMove_Implementation(const FGoKartMove& Move)
{
	ensure(MovementComponent);
	if (!MovementComponent)
	{
		return;
	}

	MovementComponent->SimulatedMove(Move);

	UpdateServerState(Move);
}


bool UGoKartMovementReplicator::Server_SendMove_Validate(const FGoKartMove& Move)
{
	return true; // TODO: make better
}


void UGoKartMovementReplicator::OnRep_ServerState()
{
	UE_LOG(LogTemp, Warning, TEXT("OnRep_ServerState"));

	if (GetOwnerRole() == ENetRole::ROLE_AutonomousProxy)
	{
		AutonomousProxy_OnRep_ServerState();
	}
	else if (GetOwnerRole() == ENetRole::ROLE_SimulatedProxy)
	{
		SimulatedProxy_OnRep_ServerState();
	}
}


void UGoKartMovementReplicator::AutonomousProxy_OnRep_ServerState()
{
	ensure(MovementComponent);
	if (!MovementComponent)
	{
		return;
	}

	GetOwner()->SetActorTransform(ServerState.Transform);
	MovementComponent->SetVelocity(ServerState.Velocity);


	ClearAcknowledgeMoves(ServerState.LastMove);

	for (const FGoKartMove& Move : UnacknowledgedMoves)
	{
		MovementComponent->SimulatedMove(Move);
	}
}


void UGoKartMovementReplicator::SimulatedProxy_OnRep_ServerState()
{
	ClientTimeBetweenLastUpdates = ClientTimeSinceUpdate;
	ClientTimeSinceUpdate = 0;
	ClientStartTransform = GetOwner()->GetActorTransform();
}
