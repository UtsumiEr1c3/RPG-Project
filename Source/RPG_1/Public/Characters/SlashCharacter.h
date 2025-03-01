// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BaseCharacter.h"
#include "CharacterTypes.h"
#include "SlashCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UGroomComponent;
class AItem;
class UAnimMontage;

// A class for a character that can equip and use a weapon, as well as perform actions like moving, attacking, and equipping.
UCLASS()
class RPG_1_API ASlashCharacter : public ABaseCharacter
{
	GENERATED_BODY()

public:
	// Constructor to set default values for this character's properties
	ASlashCharacter();

	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Setup input bindings for the player
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

protected:
	// Initialization when the game starts or the character is spawned
	virtual void BeginPlay() override;

	// Callbacks for player input (movement, attack, etc.)
	void MoveForward(float Value);
	void MoveRight(float Value);
	void Turn(float Value);
	void Lookup(float Value);
	void EKeyPressed(); // Called when the Equip key is pressed
	virtual void Attack() override;

	// Ends the attack animation
	virtual void AttackEnd() override;

	// Check if the character can attack
	virtual bool CanAttack() override;

	// Functions for equipping and disarming the weapon
	void PlayEquipMontage(const FName& SectionName);
	bool CanDisarm();
	bool CanArm();

	// Functions for arming and disarming the character
	UFUNCTION(BlueprintCallable)
	void Disarm();

	UFUNCTION(BlueprintCallable)
	void Arm();

	UFUNCTION(BlueprintCallable)
	void FinishEquipping();

private:
	// State management for the character
	ECharacterState CharacterState = ECharacterState::ECS_Unequipped;

	// Action state of the character
	UPROPERTY(BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	EActionState ActionState = EActionState::EAS_Unoccupied;

	// Components for the camera, hair, and equipped items
	UPROPERTY(VisibleAnywhere)
	USpringArmComponent* CameraBoom;

	UPROPERTY(VisibleAnywhere)
	UCameraComponent* ViewCamera;

	UPROPERTY(VisibleAnywhere, Category = "Hair")
	UGroomComponent* Hair;

	UPROPERTY(VisibleAnywhere, Category = "Hair")
	UGroomComponent* Eyebrows;

	// Item the character is currently overlapping
	UPROPERTY(VisibleInstanceOnly)
	AItem* OverlappingItem;

	// Montage for equipping the weapon
	UPROPERTY(EditDefaultsOnly, Category = "Montages")
	UAnimMontage* EquipMontage;

public:
	// Functions to access private members
	FORCEINLINE void SetOverlappingItem(AItem* Item) { OverlappingItem = Item; }
	FORCEINLINE ECharacterState GetCharacterState() const { return CharacterState; }
};
