// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "GoKart.generated.h"

USTRUCT()
struct FGoKartMove
{
	GENERATED_BODY()

	UPROPERTY()
	float Throttle;

	UPROPERTY()
	float SteeringThrow;

	UPROPERTY()
	float DeltaTime;
	
	UPROPERTY()
	float Time;
};

USTRUCT()
struct FGoKartState
{
	GENERATED_BODY()

	UPROPERTY()
	FTransform Transform;

	UPROPERTY()
	FVector Velocity;

	UPROPERTY()
	FGoKartMove LastMove;
};

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
	void SimulatedMove(const FGoKartMove& Move);

	FGoKartMove CreateMove(const float& DeltaTime) const;
	void ClearAcknowledgeMoves(const FGoKartMove& LastMove);

	void MoveForward(float Value);
	void MoveRight(float Value);

	UFUNCTION(Server, Reliable, WithValidation)
	void Server_SendMove(FGoKartMove Move);


	void UpdateLocationFromVelocity(float DeltaTime);

	void ApplyRotation(float DeltaTime, float SteeringThrow);

	FVector GetAirResistance() const;
	FVector GetRollingResistance() const;


	UFUNCTION()
	void OnRep_ServerState();


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


	UPROPERTY(ReplicatedUsing = OnRep_ServerState)
	FGoKartState ServerState;

	
	FVector Velocity = FVector::ZeroVector;
	float Throttle = 0.0f;
	float SteeringThrow = 0.0f;

	TArray<FGoKartMove> UnacknowledgedMoves;
};
