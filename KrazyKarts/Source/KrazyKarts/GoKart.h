// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
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
	void MoveForward(float Value);

	UFUNCTION(Server, Reliable, WithValidation)
	void Server_MoveForward(float Value);

	void MoveRight(float Value);

	UFUNCTION(Server, Reliable, WithValidation)
	void Server_MoveRight(float Value);


	void UpdateLocationFromVelocity(float DeltaTime);

	void ApplyRotation(float DeltaTime);

	FVector GetAirResistance() const;
	FVector GetRollingResistance() const;

private:
	// mass (kg)
	UPROPERTY(EditAnywhere)
	float Mass = 1000;

	// Max Velocity
	UPROPERTY(EditAnywhere)
	float MaxDrivingForce = 10000;
	
	// (degrees / s)
	UPROPERTY(EditAnywhere)
	float MaxDegreesPerSecond = 90.0f;

	// (m)
	UPROPERTY(EditAnywhere)
	float MinTurningRadius = 10.0f;

	// ãÛãCíÔçRåWêî
	UPROPERTY(EditAnywhere)
	float DragCoefficient = 16.0f;

	// ì]Ç™ÇËíÔçRåWêî
	UPROPERTY(EditAnywhere)
	float RollingResistanceCoefficient = 0.015f;


	UPROPERTY(Replicated)
	FVector ReplicatedLocation = FVector::ZeroVector;

	UPROPERTY(Replicated)
	FRotator ReplicatedRotation = FRotator::ZeroRotator;


	FVector Velocity = FVector::ZeroVector;
	float Throttle = 0.0f;
	float SteeringThrow = 0.0f;
};
