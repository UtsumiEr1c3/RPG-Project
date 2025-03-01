// Fill out your copyright notice in the Description page of Project Settings.


#include "Enemy/Enemy.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Perception/PawnSensingComponent.h"
#include "Components/AttributeComponent.h"
#include "HUD/HealthBarComponent.h"
#include "TimerManager.h"
#include "AIController.h"
#include "Characters/SlashCharacter.h"
#include "Items/Weapons/Weapon.h"
#include "Navigation/PathFollowingComponent.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "RPG_1/DebugMacros.h"

// Sets default values
AEnemy::AEnemy()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	// Set collision properties for the mesh (actor's body) and capsule (hitbox)
	GetMesh()->SetCollisionObjectType(ECollisionChannel::ECC_WorldDynamic);
	GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Visibility, ECollisionResponse::ECR_Block);
	GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);
	GetMesh()->SetGenerateOverlapEvents(true);
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);

	// Initialize the health bar widget and attach it to the root component
	HealthBarWidget = CreateDefaultSubobject<UHealthBarComponent>(TEXT("HealthBar"));
	HealthBarWidget->SetupAttachment(GetRootComponent());

	// Set movement behavior: enemy follows movement direction but doesn't rotate automatically to face the movement direction
	GetCharacterMovement()->bOrientRotationToMovement = true;
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Create and set up the pawn sensing component to detect nearby characters
	PawnSensing = CreateDefaultSubobject<UPawnSensingComponent>(TEXT("Pawn Sensing"));
	PawnSensing->SightRadius = 4000.f;
	PawnSensing->SetPeripheralVisionAngle(45.f);

	EnemyState = EEnemyState::EES_Patrolling;
}

// Called when the game starts or when spawned. Initializes the enemy and begins patrolling.
void AEnemy::BeginPlay()
{
	Super::BeginPlay();

	// Initially hide the health bar when the enemy spawns
	if (HealthBarWidget)
	{
		HealthBarWidget->SetVisibility(false);
	}

	// Get a reference to the enemy's AI controller
	EnemyController = Cast<AAIController>(GetController());

	// Set up a timer to start patrolling shortly after the enemy spawns
	FTimerHandle MoveTimerHandle;
	GetWorldTimerManager().SetTimer(MoveTimerHandle, this, &AEnemy::StartToPatrol, 0.5f, false);

	// Bind the pawn sensing event to the PawnSeen function (detect when the enemy sees a pawn)
	if (PawnSensing)
	{
		PawnSensing->OnSeePawn.AddDynamic(this, &AEnemy::PawnSeen);
	}

	UWorld* World = GetWorld();
	if (World && WeaponClass)
	{
		AWeapon* DefaultWeapon = World->SpawnActor<AWeapon>(WeaponClass);
		DefaultWeapon->Equip(GetMesh(), FName("RightHandSocket"), this, this);
		EquippedWeapon = DefaultWeapon;
	}
}

// Start patrolling by moving the enemy towards a patrol target
void AEnemy::StartToPatrol()
{
	MoveToTarget(PatrolTarget);
}

// Die function: Handles the death of the enemy, including playing the death animation and disabling the character's collision.
void AEnemy::Die()
{
	EnemyState = EEnemyState::EES_Dead;
	PlayDeathMontage();
	ClearAttackTimer();
	HideHealthBar();
	DisableCapsule();
	SetLifeSpan(DeathLifeSpan);
	GetCharacterMovement()->bOrientRotationToMovement = false;
}

// Check if the target is within a specific radius.
bool AEnemy::InTargetRange(AActor* Target, double Radius)
{
	if (Target == nullptr)
	{
		return false;
	}
	// Calculate the distance from the enemy to the target
	const double DistanceToTarget = (Target->GetActorLocation() - GetActorLocation()).Size();
	return DistanceToTarget <= Radius; // Return whether the target is within the specified radius
}

// Move the enemy towards the target actor.
void AEnemy::MoveToTarget(AActor* Target)
{
	if (EnemyController == nullptr || Target == nullptr) return;

	// Create an AI move request with the target actor
	FAIMoveRequest MoveRequest;
	MoveRequest.SetGoalActor(Target);
	MoveRequest.SetAcceptanceRadius(50.f); // Set how close the enemy needs to be to the target before stopping

	// Command the AI controller to move the enemy towards the target
	EnemyController->MoveTo(MoveRequest);
}

// Select a random patrol target from the list of available patrol targets.
AActor* AEnemy::ChoosePatrolTarget()
{
	TArray<AActor*> ValidTargets;
	// Add all patrol targets except the current one to the list of valid targets
	for (AActor* Target : PatrolTargets)
	{
		if (Target != PatrolTarget)
		{
			ValidTargets.AddUnique(Target);
		}
	}

	// Choose a random target from the list of valid patrol targets
	const int32 NumPatrolTargets = ValidTargets.Num();
	if (NumPatrolTargets > 0)
	{
		const int32 TargetSelection = FMath::RandRange(0, NumPatrolTargets - 1);
		return ValidTargets[TargetSelection];
	}
	return nullptr; // Return nullptr if there are no valid patrol targets
}

void AEnemy::Attack()
{
	Super::Attack();

	PlayAttackMontage();
}

bool AEnemy::CanAttack()
{
	bool bCanAttack = !IsOutsideAttackRadius() && 
		!IsAttacking() &&
		!IsDead();
	return bCanAttack;
}

void AEnemy::HandleDamage(float DamageAmount)
{
	Super::HandleDamage(DamageAmount);

	if (Attributes && HealthBarWidget)
	{
		HealthBarWidget->SetHealthPercent(Attributes->GetHealthPercent());
	}
}

int32 AEnemy::PlayDeathMontage()
{
	const int32 Selection = Super::PlayDeathMontage();
	TEnumAsByte<EDeathPose> Pose(Selection);
	if (Pose < EDeathPose::EDP_MAX)
	{
		DeathPose = Pose;
	}
	return Selection;
}

// Handle the event when the enemy sees a pawn (potential target).
void AEnemy::PawnSeen(APawn* SeenPawn)
{
	const bool bShouldChaseTarget =
		EnemyState != EEnemyState::EES_Dead &&
		EnemyState != EEnemyState::EES_Chasing &&
		EnemyState < EEnemyState::EES_Attacking &&
		SeenPawn->ActorHasTag(FName("SlashCharacter"));
	if (bShouldChaseTarget)
	{
		CombatTarget = SeenPawn;
		ClearPatrolTime();
		ChaseTarget();
	}
}

void AEnemy::HideHealthBar()
{
	if (HealthBarWidget)
	{
		HealthBarWidget->SetVisibility(false);
	}
}

void AEnemy::ShowHealthBar()
{
	if (HealthBarWidget)
	{
		HealthBarWidget->SetVisibility(true);
	}
}

void AEnemy::LoseInterest()
{
	CombatTarget = nullptr;
	HideHealthBar();
}

void AEnemy::StartPatrolling()
{
	EnemyState = EEnemyState::EES_Patrolling;
	GetCharacterMovement()->MaxWalkSpeed = PatrollingSpeed;
	MoveToTarget(PatrolTarget);
}

void AEnemy::ChaseTarget()
{
	EnemyState = EEnemyState::EES_Chasing;
	GetCharacterMovement()->MaxWalkSpeed = ChasingSpeed;
	MoveToTarget(CombatTarget);
}

bool AEnemy::IsOutsideCombatRadius()
{
	return !InTargetRange(CombatTarget, CombatRadius);
}

bool AEnemy::IsOutsideAttackRadius()
{
	return !InTargetRange(CombatTarget, AttackRadius);
}

bool AEnemy::IsChasing()
{
	return EnemyState == EEnemyState::EES_Chasing;
}

bool AEnemy::IsAttacking()
{
	return EnemyState == EEnemyState::EES_Attacking;
}

bool AEnemy::IsDead()
{
	return EnemyState == EEnemyState::EES_Dead;
}

bool AEnemy::IsEngaged()
{
	return EnemyState == EEnemyState::EES_Engaged;
}

void AEnemy::ClearPatrolTime()
{
	GetWorldTimerManager().ClearTimer(PatrolTimer);
}

void AEnemy::StartAttackTimer()
{
	EnemyState = EEnemyState::EES_Attacking;
	const float AttackTime = FMath::RandRange(AttackMin, AttackMax);
	GetWorldTimerManager().SetTimer(AttackTimer, this, &AEnemy::Attack, AttackTime);
}

void AEnemy::ClearAttackTimer()
{
	GetWorldTimerManager().ClearTimer(AttackTimer);
}

// 
void AEnemy::PatrolTimerFinished()
{
	MoveToTarget(PatrolTarget);
}

// Function to check the enemy's combat target and adjust behavior accordingly.
void AEnemy::CheckCombatTarget()
{
	if (IsOutsideCombatRadius())
	{
		ClearAttackTimer();
		LoseInterest();
		if (!IsEngaged())
		{
			StartPatrolling();
		}
	}
	else if (IsOutsideAttackRadius() && !IsChasing())
	{
		ClearAttackTimer();
		if (!IsEngaged())
		{
			ChaseTarget();
		}
	}
	else if (CanAttack())
	{
		StartAttackTimer();
	}
}

// Function to check the enemy's patrol target and update the patrol behavior.
void AEnemy::CheckPatrolTarget()
{
	// If the patrol target is within range, choose a new patrol target and wait for a random time before moving
	if (InTargetRange(PatrolTarget, PatrolRadius))
	{
		PatrolTarget = ChoosePatrolTarget();
		const float WaitTime = FMath::RandRange(WaitMin, WaitMax);
		GetWorldTimerManager().SetTimer(PatrolTimer, this, &AEnemy::PatrolTimerFinished, WaitTime);
	}
}

// Called every frame
void AEnemy::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (IsDead())
	{
		return;
	}

	// If the enemy is in a combat state, check the combat target behavior
	if (EnemyState > EEnemyState::EES_Patrolling)
	{
		CheckCombatTarget();
	}
	// If the enemy is patrolling, check the patrol target behavior
	else
	{
		CheckPatrolTarget();
	}
}

// Handles getting hit by an impact point. Plays hit reactions and updates health.
void AEnemy::GetHit_Implementation(const FVector& ImpactPoint)
{
	ShowHealthBar();

	if (IsAlive())
	{
		DirectionalHitReact(ImpactPoint);
	}
	else
	{
		Die();
	}

	// Play sound and spawn particle effects for the hit
	PlayHitSound(ImpactPoint);
	SpawnHitParticles(ImpactPoint);
}

// Handle damage taken by the enemy, adjusting health and possibly changing state.
float AEnemy::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	// Apply damage to the enemy's attributes and update health bar
	HandleDamage(DamageAmount);

	// Set the combat target to the damage instigator and start chasing the target
	CombatTarget = EventInstigator->GetPawn();
	ChaseTarget();

	// Return the damage amount for potential further processing
	return DamageAmount;
}

void AEnemy::Destroyed()
{
	if (EquippedWeapon)
	{
		EquippedWeapon->Destroy();
	}
}
