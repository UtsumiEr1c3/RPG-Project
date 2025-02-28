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
	GetWorldTimerManager().SetTimer(MoveTimerHandle, this, &AEnemy::StartPatrolling, 0.5f, false);

	// Bind the pawn sensing event to the PawnSeen function (detect when the enemy sees a pawn)
	if (PawnSensing)
	{
		PawnSensing->OnSeePawn.AddDynamic(this, &AEnemy::PawnSeen);
	}
}

// Start patrolling by moving the enemy towards a patrol target
void AEnemy::StartPatrolling()
{
	MoveToTarget(PatrolTarget);
}

// Die function: Handles the death of the enemy, including playing the death animation and disabling the character's collision.
void AEnemy::Die()
{
	// Play the death animation montage
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && DeathMontage)
	{
		AnimInstance->Montage_Play(DeathMontage);

		// Randomly select a death animation section
		const int32 Selection = FMath::RandRange(0, 2);
		FName SectionName = FName();
		switch (Selection)
		{
		case 0:
			SectionName = FName("Death1");
			DeathPose = EDeathPose::EDP_Death1;
			break;
		case 1:
			SectionName = FName("Death2");
			DeathPose = EDeathPose::EDP_Death2;
			break;
		case 2:
			SectionName = FName("Death3");
			DeathPose = EDeathPose::EDP_Death3;
			break;
		default:
			break;
		}

		// Hide the health bar when the enemy dies
		if (HealthBarWidget)
		{
			HealthBarWidget->SetVisibility(false);
		}

		// Jump to the selected death animation section
		AnimInstance->Montage_JumpToSection(SectionName, DeathMontage);
	}

	// Disable collision and set a lifespan for the enemy after death (this will destroy the enemy after 10 seconds)
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SetLifeSpan(10.f);
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
	MoveRequest.SetAcceptanceRadius(15.f); // Set how close the enemy needs to be to the target before stopping

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

// Handle the event when the enemy sees a pawn (potential target).
void AEnemy::PawnSeen(APawn* SeenPawn)
{
	// If the enemy is already chasing, do nothing
	if (EnemyState == EEnemyState::EES_Chasing)
	{
		return;
	}

	// If the enemy sees a SlashCharacter, start chasing them
	if (SeenPawn->ActorHasTag(FName("SlashCharacter")))
	{
		// Stop patrolling and clear the patrol timer
		GetWorldTimerManager().ClearTimer(PatrolTimer);

		// Set the character's movement speed to chasing speed
		GetCharacterMovement()->MaxWalkSpeed = 300.f;

		// Set the combat target to the seen pawn (the player)
		CombatTarget = SeenPawn;

		// Start chasing the target if the enemy is not already attacking
		if (EnemyState != EEnemyState::EES_Attacking)
		{
			EnemyState = EEnemyState::EES_Chasing;
			MoveToTarget(CombatTarget);
		}
	}
}

// 
void AEnemy::PatrolTimerFinished()
{
	MoveToTarget(PatrolTarget);
}

// Function to check the enemy's combat target and adjust behavior accordingly.
void AEnemy::CheckCombatTarget()
{
	// If the combat target is out of range, stop chasing and resume patrolling
	if (!InTargetRange(CombatTarget, CombatRadius))
	{
		CombatTarget = nullptr;
		if (HealthBarWidget)
		{
			HealthBarWidget->SetVisibility(false);
		}
		EnemyState = EEnemyState::EES_Patrolling;
		GetCharacterMovement()->MaxWalkSpeed = 125.f;
		MoveToTarget(PatrolTarget);
	}
	// If the combat target is out of attack range, continue chasing
	else if (!InTargetRange(CombatTarget, AttackRadius) && EnemyState != EEnemyState::EES_Chasing)
	{
		EnemyState = EEnemyState::EES_Chasing;
		GetCharacterMovement()->MaxWalkSpeed = 300.f;
		MoveToTarget(CombatTarget);
	}
	// If the combat target is within attack range, stop chasing and attack
	else if (InTargetRange(CombatTarget, AttackRadius) && EnemyState != EEnemyState::EES_Attacking)
	{
		EnemyState = EEnemyState::EES_Attacking;
		// TODO: Implement attack logic (e.g., play attack montage)
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

// Setup player input components for the enemy (if needed). Currently, no input is handled for the enemy.
void AEnemy::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	// No input binding needed for AI-controlled enemy
}

// Handles getting hit by an impact point. Plays hit reactions and updates health.
void AEnemy::GetHit_Implementation(const FVector& ImpactPoint)
{
	// Make health bar visible when the enemy is hit
	HealthBarWidget->SetVisibility(true);

	// If the enemy is alive, play a directional hit reaction
	if (Attributes && Attributes->IsAlive())
	{
		DirectionalHitReact(ImpactPoint);
	}
	else
	{
		// If the enemy is dead, trigger the death sequence
		Die();
	}

	// Play sound and spawn particle effects for the hit
	if (HitSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, HitSound, ImpactPoint);
	}
	if (HitParticles && GetWorld())
	{
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), HitParticles, ImpactPoint);
	}
}

// Handle damage taken by the enemy, adjusting health and possibly changing state.
float AEnemy::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	// Apply damage to the enemy's attributes and update health bar
	if (Attributes && HealthBarWidget)
	{
		Attributes->ReceiveDamage(DamageAmount);
		HealthBarWidget->SetHealthPercent(Attributes->GetHealthPercent());
	}

	// Set the combat target to the damage instigator and start chasing the target
	CombatTarget = EventInstigator->GetPawn();
	EnemyState = EEnemyState::EES_Chasing;
	GetCharacterMovement()->MaxWalkSpeed = 300.f;
	MoveToTarget(CombatTarget);

	// Return the damage amount for potential further processing
	return DamageAmount;
}