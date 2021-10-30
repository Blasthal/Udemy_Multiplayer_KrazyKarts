// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GoKartMoveComponent.generated.h"


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

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class KRAZYKARTS_API UGoKartMoveComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UGoKartMoveComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	FGoKartMove CreateMove(const float& DeltaTime) const;

	void SimulatedMove(const FGoKartMove& Move);

	const FVector& GetVelocity() const;
	void SetVelocity(const FVector& Velocity);

	void SetThrottle(float Throttle);
	void SetSteeringThrow(float SteeringThrow);
	
private:
	void UpdateLocationFromVelocity(float DeltaTime);

	void ApplyRotation(float DeltaTime, float SteeringThrow);

	FVector GetAirResistance() const;
	FVector GetRollingResistance() const;

private:
	// mass (kg)
	UPROPERTY(EditAnywhere)
	float Mass = 1000;

	// Max Velocity
	UPROPERTY(EditAnywhere)
	float MaxDrivingForce = 10000;
	
	// (m)
	UPROPERTY(EditAnywhere)
	float MinTurningRadius = 10.0f;

	// ãÛãCíÔçRåWêî
	UPROPERTY(EditAnywhere)
	float DragCoefficient = 16.0f;

	// ì]Ç™ÇËíÔçRåWêî
	UPROPERTY(EditAnywhere)
	float RollingResistanceCoefficient = 0.015f;

	FVector Velocity = FVector::ZeroVector;
	float Throttle = 0.0f;
	float SteeringThrow = 0.0f;
};
