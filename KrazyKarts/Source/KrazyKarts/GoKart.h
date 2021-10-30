// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "GoKartMoveComponent.h"
#include "GoKartMovementReplicator.h"
#include "GoKart.generated.h"

UCLASS()
class KRAZYKARTS_API AGoKart : public APawn
{
	GENERATED_BODY()

public:
	// Sets default values for this pawn's properties
	AGoKart();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;


public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;


private:
	void MoveForward(const float Value);
	void MoveRight(const float Value);
	

public: // private
	UPROPERTY(VisibleAnywhere)
	UGoKartMoveComponent* MovementComponent = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	UGoKartMovementReplicator* MovementReplicator = nullptr;
};
