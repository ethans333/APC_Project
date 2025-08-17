#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "ChaosWheeledVehicleMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "Net/UnrealNetwork.h"
#include "APCPawn.generated.h"

UCLASS()
class APC_PROJECT_API AAPCPawn : public APawn
{
	GENERATED_BODY()

public:
	AAPCPawn();

protected:
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// === COMPONENTS ===
	USkeletalMeshComponent* VehicleMesh;
	UChaosWheeledVehicleMovementComponent* VehicleMovement;
	USpringArmComponent* SpringArm;
	UCameraComponent* VehicleCamera;
	UCameraComponent* TurretCamera;

	// === PROPERTIES ===
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	float MaxRange = 20000.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	FName MuzzleSocketName = TEXT("gun_jntSocket");

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Health")
	float Health = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effects")
	UNiagaraSystem* BulletImpactEffect;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effects")
	UNiagaraSystem* ExplosionEffect;

	// === REPLICATED VARIABLES ===
	UPROPERTY(ReplicatedUsing = OnRep_CurrentTurretRotation)
	FRotator CurrentTurretRotation;

	UFUNCTION()
	void OnRep_CurrentTurretRotation();

	// === INPUT FUNCTIONS ===
	void MoveForward(float Value);
	void TurnRight(float Value);
	void Break(float Value);
	void LookUp(float Value);
	void LookRight(float Value);
	void ViewTurretCamera();
	void ViewVehicleCamera();
	void ActivateFiring();
	void DeactivateFiring();

	// === COMBAT ===
	UFUNCTION(BlueprintCallable, Server, Reliable)
	void Shoot();

	void ApplyDamage(float DamageAmount);
	void OnHit();

	// === SERVER FUNCTIONS ===
	UFUNCTION(Server, Reliable)
	void ServerLookRight(float Value);

	UFUNCTION(Server, Reliable)
	void ServerLookUp(float Value);

	UFUNCTION(Server, Reliable)
	void ServerActivateFiring();

	UFUNCTION(Server, Reliable)
	void ServerDeactivateFiring();

	// === MULTICAST FUNCTIONS ===
	UFUNCTION(NetMulticast, Reliable)
	void SpawnBulletImpactEffect(const FVector& ImpactLocation);

	UFUNCTION(NetMulticast, Reliable)
	void SpawnExplosionEffect();

	UFUNCTION(NetMulticast, Reliable)
	void MulticastActivateFiring();

	UFUNCTION(NetMulticast, Reliable)
	void MulticastDeactivateFiring();

	UFUNCTION(NetMulticast, Reliable)
	void MulticastSetShowDamaged(bool IsDamaged);

	// === BLUEPRINT EVENTS ===
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
	void SetTurretRotation(float TurretRotation);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
	void SetGunElevation(float GunElevation);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
	void SetActivateFiring(bool ActivateFiring);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
	void SetSkinType(float SkinType);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
	void SetShowDamaged(bool IsDamaged);
};