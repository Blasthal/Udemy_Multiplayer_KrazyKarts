// Fill out your copyright notice in the Description page of Project Settings.


#include "GoKart.h"

#include "DrawDebugHelpers.h"
#include "Components/InputComponent.h"
#include "Engine/Engine.h"
#include "GameFramework/GameStateBase.h"


// Sets default values
AGoKart::AGoKart()
{
 	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	SetReplicates(true);
    AActor::SetReplicateMovement(false);

	MovementComponent = CreateDefaultSubobject<UGoKartMoveComponent>(TEXT("MovementComponent"));
	ensure(MovementComponent);

	MovementReplicator = CreateDefaultSubobject<UGoKartMovementReplicator>(TEXT("MovementReplicator"));
	ensure(MovementReplicator);
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
}

// Called to bind functionality to input
void AGoKart::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAxis("MoveForward", this, &AGoKart::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &AGoKart::MoveRight);
}

void AGoKart::MoveForward(const float Value)
{
	MovementComponent->SetThrottle(Value);
}

void AGoKart::MoveRight(const float Value)
{
	MovementComponent->SetSteeringThrow(Value);
}
