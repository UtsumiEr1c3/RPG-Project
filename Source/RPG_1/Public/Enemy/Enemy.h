// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Characters/BaseCharacter.h"
#include "Characters/CharacterTypes.h"
#include "Enemy.generated.h"

class UHealthBarComponent;
class AAIController;
class UPawnSensingComponent;

// Enemy character class with AI and combat functionality
UCLASS()
class RPG_1_API AEnemy : public ABaseCharacter
{
	GENERATED_BODY()

public:
	// Constructor to set default values
	AEnemy();

	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Set up input bindings for the enemy (if needed)
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	// Called when the enemy gets hit
	virtual void GetHit_Implementation(const FVector& ImpactPoint) override;

	// Function to handle damage taken by the enemy
	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser) override;

protected:
	// Initialization when the game starts or when the enemy is spawned
	virtual void BeginPlay() override;
	void StartPatrolling();

	// Death functionality and state changes
	virtual void Die() override;

	// Functions to manage patrol and combat targeting
	bool InTargetRange(AActor* Target, double Radius);
	void MoveToTarget(AActor* Target);
	AActor* ChoosePatrolTarget();

	// Functions to react when the enemy sees a pawn
	UFUNCTION()
	void PawnSeen(APawn* SeenPawn);


	// Components for enemy sensing and health bar display
	UPROPERTY(VisibleAnywhere)
	UPawnSensingComponent* PawnSensing;

	UPROPERTY(VisibleAnywhere)
	UHealthBarComponent* HealthBarWidget;

	// Targeting and patrol parameters
	UPROPERTY(EditAnywhere)
	double CombatRadius = 1000;

	UPROPERTY(EditAnywhere)
	double AttackRadius = 150;

	// State management for the enemy's behavior
	EEnemyState EnemyState = EEnemyState::EES_Patrolling;

	UPROPERTY(BlueprintReadOnly)
	EDeathPose DeathPose = EDeathPose::EDP_Alive;

private:
	// Enemy's combat target and AI controller
	UPROPERTY()
	AActor* CombatTarget;

	// AI controller and patrol targets
	UPROPERTY()
	AAIController* EnemyController;
	UPROPERTY(EditInstanceOnly, Category = "AI Navigation")
	AActor* PatrolTarget;

	UPROPERTY(EditInstanceOnly, Category = "AI Navigation")
	TArray<AActor*> PatrolTargets;

	// Patrol radius and wait time parameters
	UPROPERTY(EditAnywhere)
	double PatrolRadius = 200.f;

	UPROPERTY(EditAnywhere, Category = "AI Navigation")
	float WaitMin = 5.f;

	UPROPERTY(EditAnywhere, Category = "AI Navigation")
	float WaitMax = 10.f;

	// Timer to handle patrol behavior
	FTimerHandle PatrolTimer;
	void PatrolTimerFinished();

	// Check and update patrol/target behavior
	void CheckCombatTarget();
	void CheckPatrolTarget();
};
