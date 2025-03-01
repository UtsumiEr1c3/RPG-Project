// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Characters/BaseCharacter.h"
#include "Characters/CharacterTypes.h"
#include "Enemy.generated.h"

class UHealthBarComponent;
class AAIController;
class UPawnSensingComponent;
class AWeapon;

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

	// Called when the enemy gets hit
	virtual void GetHit_Implementation(const FVector& ImpactPoint) override;

	// Function to handle damage taken by the enemy
	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser) override;

	virtual void Destroyed() override;

protected:
	virtual void BeginPlay() override;
	void StartToPatrol();

	// Death functionality and state changes
	virtual void Die() override;

	// Functions to patrol and attack
	bool InTargetRange(AActor* Target, double Radius);
	void MoveToTarget(AActor* Target);
	AActor* ChoosePatrolTarget();
	virtual void Attack() override;
	virtual void PlayAttackMontage() override;
	virtual bool CanAttack() override;
	virtual void HandleDamage(float DamageAmount) override;

	// Functions to react when the enemy sees a pawn
	UFUNCTION()
	void PawnSeen(APawn* SeenPawn);

	// Targeting and patrol parameters
	UPROPERTY(EditAnywhere)
	double CombatRadius = 1000;

	UPROPERTY(EditAnywhere)
	double AttackRadius = 150;

	// State management for the enemy's behavior
	UPROPERTY(BlueprintReadOnly)
	EEnemyState EnemyState = EEnemyState::EES_Patrolling;

	UPROPERTY(BlueprintReadOnly)
	EDeathPose DeathPose;

private:
	// Components for enemy sensing and health bar display
	UPROPERTY(VisibleAnywhere)
	UPawnSensingComponent* PawnSensing;

	UPROPERTY(VisibleAnywhere)
	UHealthBarComponent* HealthBarWidget;

	// Enemy weapon
	UPROPERTY(EditAnywhere)
	TSubclassOf<AWeapon> WeaponClass;

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

	/* AI Behaviour */
	void HideHealthBar();
	void ShowHealthBar();
	void LoseInterest();
	void StartPatrolling();
	void ChaseTarget();
	bool IsOutsideCombatRadius();
	bool IsOutsideAttackRadius();
	bool IsChasing();
	bool IsAttacking();
	bool IsDead();
	bool IsEngaged();
	void ClearPatrolTime();

	/* Combat */
	void StartAttackTimer();
	void ClearAttackTimer();

	FTimerHandle AttackTimer;

	UPROPERTY(EditAnywhere, Category = "Combat")
	float AttackMin = 0.5f;

	UPROPERTY(EditAnywhere, Category = "Combat")
	float AttackMax = 1.5f;

	UPROPERTY(EditAnywhere, Category = "Combat")
	float PatrollingSpeed = 125.f;

	UPROPERTY(EditAnywhere, Category = "Combat")
	float ChasingSpeed = 300.f;

	// Timer to handle patrol behavior
	FTimerHandle PatrolTimer;
	void PatrolTimerFinished();

	// Check and update patrol/target behavior
	void CheckCombatTarget();
	void CheckPatrolTarget();
};
