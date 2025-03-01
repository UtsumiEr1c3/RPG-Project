// Fill out your copyright notice in the Description page of Project Settings.


#include "Characters/BaseCharacter.h"
#include "Components/BoxComponent.h"
#include "Items/Weapons/Weapon.h"
#include "Components/AttributeComponent.h"
#include "Kismet/GameplayStatics.h"

ABaseCharacter::ABaseCharacter()
{
	// Set this character to call Tick() every frame to update position and state.
	PrimaryActorTick.bCanEverTick = true;

	// Create and assign the attribute component that manages health, stamina, etc.
	Attributes = CreateDefaultSubobject<UAttributeComponent>(TEXT("Attributes"));
}

void ABaseCharacter::BeginPlay()
{
	Super::BeginPlay();

}

// Attack logic would go here in derived classes
void ABaseCharacter::Attack()
{
}

// Death logic would go here in derived classes
void ABaseCharacter::Die()
{
}

// Play attack animation montage will be implemented in derived classes
void ABaseCharacter::PlayAttackMontage()
{
}

// Plays the hit reaction montage when the character is hit. Takes the section name for different reactions.
void ABaseCharacter::PlayHitReactMontage(const FName& SectionName)
{
	// Get the animation instance from the character's mesh
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && HitReactMontage)
	{
		// Play the hit reaction montage and jump to the specified section
		AnimInstance->Montage_Play(HitReactMontage);
		AnimInstance->Montage_JumpToSection(SectionName, HitReactMontage);
	}
}

// React to the direction of the hit, calculating the angle and selecting the appropriate reaction animation.
void ABaseCharacter::DirectionalHitReact(const FVector& ImpactPoint)
{
	const FVector Forward = GetActorForwardVector();
	// Calculate the vector from the character to the impact point, ignoring vertical difference (Z)
	const FVector ImpactLower(ImpactPoint.X, ImpactPoint.Y, GetActorLocation().Z);
	const FVector ToHit = (ImpactLower - GetActorLocation()).GetSafeNormal();

	// Calculate the angle between the character's forward vector and the direction of the hit
	const double CosTheta = FVector::DotProduct(Forward, ToHit);
	double Theta = FMath::Acos(CosTheta);
	Theta = FMath::RadiansToDegrees(Theta); // Convert from radians to degrees

	// Adjust the sign of the angle based on the cross product direction
	const FVector CrossProduct = FVector::CrossProduct(Forward, ToHit);
	if (CrossProduct.Z < 0)
	{
		Theta = -Theta;
	}

	// Determine the appropriate hit react section based on the angle of impact
	FName Section("FromBack");

	// Choose section based on direction of impact
	if (Theta >= -45.f && Theta < 45.f)
	{
		Section = FName("FromFront");
	}
	else if (Theta >= -135.f && Theta < -45.f)
	{
		Section = FName("FromLeft");
	}
	else if (Theta >= 45.f && Theta < 135.f)
	{
		Section = FName("FromRight");
	}

	PlayHitReactMontage(Section);
}

void ABaseCharacter::PlayHitSound(const FVector& ImpactPoint)
{
	if (HitSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, HitSound, ImpactPoint);
	}
}

void ABaseCharacter::SpawnHitParticles(const FVector& ImpactPoint)
{
	if (HitParticles && GetWorld())
	{
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), HitParticles, ImpactPoint);
	}
}

void ABaseCharacter::HandleDamage(float DamageAmount)
{
	if (Attributes)
	{
		Attributes->ReceiveDamage(DamageAmount);
	}
}

// Derived classes should override this with specific attack logic
bool ABaseCharacter::CanAttack()
{
	return false; 
}

bool ABaseCharacter::IsAlive()
{
	return Attributes && Attributes->IsAlive();
}

// Implement in derived class
void ABaseCharacter::AttackEnd()
{
	
}

void ABaseCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	
}

// Set whether the weapon collision is enabled or not.
void ABaseCharacter::SetWeaponCollisionEnabled(ECollisionEnabled::Type CollisionEnabled)
{
	// Check if the weapon is equipped and enable/disable collision on the weapon's box component.
	if (EquippedWeapon && EquippedWeapon->GetWeaponBox())
	{
		EquippedWeapon->GetWeaponBox()->SetCollisionEnabled(CollisionEnabled);
		EquippedWeapon->IgnoreActors.Empty();
	}
}
