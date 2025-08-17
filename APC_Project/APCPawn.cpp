#include "APCPawn.h"

AAPCPawn::AAPCPawn()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
}

void AAPCPawn::BeginPlay()
{
	Super::BeginPlay();

	VehicleMovement = FindComponentByClass<UChaosWheeledVehicleMovementComponent>();
	SpringArm = FindComponentByClass<USpringArmComponent>();
	VehicleMesh = FindComponentByClass<USkeletalMeshComponent>();
	VehicleCamera = Cast<UCameraComponent>(GetDefaultSubobjectByName(TEXT("VehicleCamera")));
	TurretCamera = Cast<UCameraComponent>(GetDefaultSubobjectByName(TEXT("TurretCamera")));

	SetSkinType(1.f);
}

void AAPCPawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AAPCPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAxis("MoveForward", this, &AAPCPawn::MoveForward);
	PlayerInputComponent->BindAxis("LookRight", this, &AAPCPawn::LookRight);
	PlayerInputComponent->BindAxis("LookUp", this, &AAPCPawn::LookUp);
	PlayerInputComponent->BindAxis("TurnRight", this, &AAPCPawn::TurnRight);
	PlayerInputComponent->BindAxis("Break", this, &AAPCPawn::Break);

	PlayerInputComponent->BindAction("ViewTurretCamera", IE_Pressed, this, &AAPCPawn::ViewTurretCamera);
	PlayerInputComponent->BindAction("ViewTurretCamera", IE_Released, this, &AAPCPawn::ViewVehicleCamera);
	PlayerInputComponent->BindAction("Shoot", IE_Pressed, this, &AAPCPawn::ActivateFiring);
	PlayerInputComponent->BindAction("Shoot", IE_Released, this, &AAPCPawn::DeactivateFiring);
}

void AAPCPawn::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{

	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AAPCPawn, CurrentTurretRotation);
}

void AAPCPawn::MoveForward(float Value)
{
	if (VehicleMovement)
	{
		VehicleMovement->SetThrottleInput(Value);
	}
}

void AAPCPawn::LookRight(float Value)
{
	if (!SpringArm || FMath::Abs(Value) <= KINDA_SMALL_NUMBER) return;

	FRotator NewRotation = SpringArm->GetRelativeRotation();
	NewRotation.Yaw += Value;
	SpringArm->SetRelativeRotation(NewRotation);
	SetTurretRotation(NewRotation.Yaw);

	if (HasAuthority())
	{
		CurrentTurretRotation = NewRotation;
	}
	else
	{
		ServerLookRight(Value);
	}
}

void AAPCPawn::LookUp(float Value)
{
	if (!SpringArm || FMath::Abs(Value) <= KINDA_SMALL_NUMBER) return;

	FRotator NewRotation = SpringArm->GetRelativeRotation();
	NewRotation.Pitch += Value;
	NewRotation.Pitch = FMath::Clamp(NewRotation.Pitch, -10.0f, 20.0f);
	SpringArm->SetRelativeRotation(NewRotation);
	SetGunElevation(NewRotation.Pitch);

	if (HasAuthority())
	{
		CurrentTurretRotation = NewRotation;
	}
	else
	{
		ServerLookRight(Value);
	}
}

void AAPCPawn::ServerLookRight_Implementation(float Value)
{
	LookRight(Value);
}

void AAPCPawn::ServerLookUp_Implementation(float Value)
{
	LookUp(Value);
}

void AAPCPawn::TurnRight(float Value)
{
	if (VehicleMovement)
	{
		VehicleMovement->SetSteeringInput(Value);
	}
}

void AAPCPawn::SpawnBulletImpactEffect_Implementation(const FVector& ImpactLocation)
{
	UNiagaraComponent* ShootEffectComponent = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
		GetWorld(),
		BulletImpactEffect,
		ImpactLocation
	);
}

void AAPCPawn::SpawnExplosionEffect_Implementation()
{
	UNiagaraComponent* ExplosionEffectComponent = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
		GetWorld(),
		ExplosionEffect,
		GetActorLocation()
	);
}

void AAPCPawn::Break(float Value)
{
	if (VehicleMovement)
	{
		VehicleMovement->SetBrakeInput(Value);
	}
}

void AAPCPawn::ViewTurretCamera()
{
	TurretCamera->SetActive(true);
	VehicleCamera->SetActive(false);
}

void AAPCPawn::ViewVehicleCamera()
{
	VehicleCamera->SetActive(true);
	TurretCamera->SetActive(false);
}


void AAPCPawn::ActivateFiring()
{
	if (HasAuthority())
	{
		MulticastActivateFiring();
	}
	else
	{
		ServerActivateFiring();
	}
}

void AAPCPawn::DeactivateFiring()
{
	if (HasAuthority())
	{
		MulticastDeactivateFiring();
	}
	else
	{
		ServerDeactivateFiring();
	}
}

void AAPCPawn::ServerActivateFiring_Implementation()
{
	MulticastActivateFiring();
}

void AAPCPawn::ServerDeactivateFiring_Implementation()
{
	MulticastDeactivateFiring();
}

void AAPCPawn::MulticastActivateFiring_Implementation()
{
	SetActivateFiring(true);
}

void AAPCPawn::MulticastDeactivateFiring_Implementation()
{
	SetActivateFiring(false);
}

void AAPCPawn::OnHit()
{
	ApplyDamage(10.f);
}

void AAPCPawn::Shoot_Implementation()
{
	FVector MuzzleLocation = VehicleMesh->GetSocketLocation(MuzzleSocketName);
	FVector MuzzleDirection = VehicleMesh->GetSocketRotation(MuzzleSocketName).Vector();
	FVector EndLocation = MuzzleLocation + (MuzzleDirection * MaxRange);

	FHitResult Hit;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);

	if (GetWorld()->LineTraceSingleByChannel(Hit, MuzzleLocation, EndLocation, ECC_Visibility, Params))
	{
		EndLocation = Hit.ImpactPoint;

		if (AAPCPawn* HitAPC = Cast<AAPCPawn>(Hit.GetActor()))
		{
			HitAPC->OnHit();
		}
	}

	SpawnBulletImpactEffect(EndLocation);
}

void AAPCPawn::OnRep_CurrentTurretRotation()
{
	// Only apply to non-controlling clients to prevent conflicts
	if (!IsLocallyControlled() && SpringArm)
	{
		SpringArm->SetRelativeRotation(CurrentTurretRotation);
		SetTurretRotation(CurrentTurretRotation.Yaw);
		SetGunElevation(CurrentTurretRotation.Pitch);
	}
}

void AAPCPawn::ApplyDamage(float DamageAmount)
{
	if (HasAuthority())
	{
		Health -= DamageAmount;

		if (Health <= 0.f)
		{
			MulticastSetShowDamaged(true);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("APC Health: %f"), Health);
		}
	}
}

void AAPCPawn::MulticastSetShowDamaged_Implementation(bool IsDamaged)
{
	SetShowDamaged(IsDamaged);
}
