// Fill out your copyright notice in the Description page of Project Settings.


#include "GoKartMovementReplicator.h"

#include "DrawDebugHelpers.h"
#include "UnrealNetwork.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/GameStateBase.h"
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


	// 補間
	const FHermiteCubicSpline Spline = CreateSpline();
	const float LerpRatio = ClientTimeSinceUpdate / ClientTimeBetweenLastUpdates;

	// 位置
	InterpolateLocation(Spline, LerpRatio);
	// 速度
	InterpolateVelocity(Spline, LerpRatio);
	// 回転
	InterpolateRotation(LerpRatio);


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


FHermiteCubicSpline UGoKartMovementReplicator::CreateSpline() const
{
	FHermiteCubicSpline Spline;
	Spline.TargetLocation = ServerState.Transform.GetLocation();
	Spline.StartLocation = ClientStartTransform.GetLocation();
	Spline.StartDerivative = ClientStartVelocity * VelocityToDerivative();
	Spline.TargetDerivative = ServerState.Velocity * VelocityToDerivative();

	return Spline;
}


float UGoKartMovementReplicator::VelocityToDerivative() const
{
	// 100.0fは (m/s) -> (cm/s) への変換
	const float VelocityToDerivative = ClientTimeBetweenLastUpdates * 100.0f;
	return VelocityToDerivative;
}


void UGoKartMovementReplicator::InterpolateLocation(const FHermiteCubicSpline& Spline, float LerpRatio) const
{
	const FVector NewLocation = Spline.InterpolateLocation(LerpRatio);

	ensure(MeshOffsetRoot);
	if (MeshOffsetRoot)
	{
		MeshOffsetRoot->SetWorldLocation(NewLocation);
	}
}


void UGoKartMovementReplicator::InterpolateVelocity(const FHermiteCubicSpline& Spline, float LerpRatio) const
{
	const FVector NewDerivative = Spline.InterpolateDerivative(LerpRatio);
	const FVector NewVelocity = NewDerivative / VelocityToDerivative();

	ensure(MovementComponent);
	if (MovementComponent)
	{
		MovementComponent->SetVelocity(NewVelocity);
	}
}


void UGoKartMovementReplicator::InterpolateRotation(float LerpRatio) const
{
	const FQuat TargetRotation = ServerState.Transform.GetRotation();
	const FQuat StartRotation = ClientStartTransform.GetRotation();
	const FQuat NewRotation = FQuat::Slerp(StartRotation, TargetRotation, LerpRatio);

	ensure(MeshOffsetRoot);
	if (MeshOffsetRoot)
	{
		MeshOffsetRoot->SetWorldRotation(NewRotation);
	}
}


void UGoKartMovementReplicator::Server_SendMove_Implementation(const FGoKartMove& Move)
{
	ensure(MovementComponent);
	if (!MovementComponent)
	{
		return;
	}

	ClientSimulatedTime += Move.DeltaTime;

	MovementComponent->SimulatedMove(Move);

	UpdateServerState(Move);
}


bool UGoKartMovementReplicator::Server_SendMove_Validate(const FGoKartMove& Move)
{
	const float ProcessedTime = ClientSimulatedTime - Move.DeltaTime;
	const bool bClientNotRunningAhead = (ProcessedTime < GetWorld()->GetGameState()->GetServerWorldTimeSeconds());
	if (!bClientNotRunningAhead)
	{
		UE_LOG(LogTemp, Error, TEXT("Client is running too fast."));
		return false;
	}

	if (!Move.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("Receive invalid move."));
		return false;
	}

	return true;
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

	ensure(MovementComponent);
	if (MovementComponent)
	{
		ClientStartVelocity = MovementComponent->GetVelocity();
	}

	ensure(MeshOffsetRoot);
	if (MeshOffsetRoot)
	{
		ClientStartTransform.SetLocation(MeshOffsetRoot->GetComponentLocation());
		ClientStartTransform.SetRotation(MeshOffsetRoot->GetComponentQuat());
	}

	// コリジョンの位置だけ直入
	GetOwner()->SetActorTransform(ServerState.Transform);
}


void UGoKartMovementReplicator::SetMeshOffsetRoot(USceneComponent* Root)
{
	MeshOffsetRoot = Root;
}
