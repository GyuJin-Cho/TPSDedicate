#include "TPSCharacter.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "Engine/DamageEvents.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "DrawDebugHelpers.h"
#include "TimerManager.h"
#include "DedicateTPS/DedicateTPS.h"
#include "DedicateTPS/Game/TPSGameMode.h"
#include "DedicateTPS/Weapon/M4Weapon.h"
#include "DedicateTPS/Weapon/ArmoComponent.h"
#include "DedicateTPS/Utill/CHelpers.h"
#include "DedicateTPS/Utill/Utill.h"
#include "TPSCharacterMovementComponent.h"

ATPSCharacter::ATPSCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UTPSCharacterMovementComponent>(ACharacter::CharacterMovementComponentName))
{
	PrimaryActorTick.bCanEverTick = false;

	// Basic 즉 기본 컴포넌드 없을경우 return
	if(!IsValid(GetCapsuleComponent()) || !IsValid(GetCharacterMovement()) || !IsValid(GetMesh()))
	{
		ERROR_LOG(TEXT("ATPSCharacter::ATPSCharacter() Basic Not Valid"));
		return;
	}
	
	GetCapsuleComponent()->InitCapsuleSize(42.f, 94.f);

	// 조준 방향으로 몸을 돌리는 슈터 방식
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = true;
	bUseControllerRotationRoll = false;
	GetCharacterMovement()->bOrientRotationToMovement = false;
	GetCharacterMovement()->JumpZVelocity = 500.f;
	GetCharacterMovement()->MaxWalkSpeed = 400.f;

	USkeletalMesh* MeshAsset = nullptr;
	CHelpers::GetAsset<USkeletalMesh>(&MeshAsset, TEXT("/Script/Engine.SkeletalMesh'/Game/SCK_Casual01/Models/Premade_Characters/MESH_PC_02.MESH_PC_02'"));
	if(!IsValid(MeshAsset))
	{
		ERROR_LOG(TEXT("ATPSCharacter::ATPSCharacter() MeshAsset Not Vaild"));
		return;
	}
	GetMesh()->SetSkeletalMeshAsset(MeshAsset);
	GetMesh()->SetRelativeLocationAndRotation(FVector(0.f, 0.f, -94.f), FRotator(0.f, -90.f, 0.f));
	
	TSubclassOf<UAnimInstance> AnimInstanceClass;
	CHelpers::GetClass<UAnimInstance>(&AnimInstanceClass, "/Game/Player/ABP_IK/ABP_MainPlayer.ABP_MainPlayer_C");
	if(!AnimInstanceClass->IsAsset())
	{
		ERROR_LOG(TEXT("ATPSCharacter::ATPSCharacter() AnimInstanceClass Not Vaild"));
		return;
	}
	
	GetMesh()->SetAnimInstanceClass(AnimInstanceClass);
	CHelpers::CreateComponent(this, &CameraBoom, "CameraBoom", GetCapsuleComponent());
	if (!IsValid(CameraBoom))
	{
		ERROR_LOG(TEXT("ATPSCharacter::ATPSCharacter() CameraBoom Not Vaild"));
		return;
	}
	
	CameraBoom->TargetArmLength = 300.f;
	CameraBoom->SocketOffset = FVector(0.f, 60.f, 60.f);
	CameraBoom->bUsePawnControlRotation = true;

	CHelpers::CreateComponentSocket(this, &FollowCamera, "FollowCamera", USpringArmComponent::SocketName, CameraBoom.Get());
	if (!IsValid(FollowCamera))
	{
		ERROR_LOG(TEXT("ATPSCharacter::ATPSCharacter() FollowCamera Not Vaild"));
		return;
	}
	FollowCamera->bUsePawnControlRotation = false;
}

void ATPSCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ATPSCharacter, Health);
	DOREPLIFETIME(ATPSCharacter, bIsDead);
	DOREPLIFETIME_CONDITION(ATPSCharacter, bReplicatedSprinting, COND_SkipOwner);
	DOREPLIFETIME(ATPSCharacter, Weapon);
}

void ATPSCharacter::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority())
	{
		Health = MaxHealth;
		SpawnWeapon();
	}
}

void ATPSCharacter::Destroyed()
{
	// 무기는 별도 액터라서 캐릭터와 함께 지워야 한다 (사망 후 LifeSpan 만료 포함)
	if (HasAuthority() && IsValid(Weapon))
	{
		Weapon->Destroy();
	}

	Super::Destroyed();
}

// ---------------------------------------------------------------- Weapon

void ATPSCharacter::SpawnWeapon()
{
	if (!WeaponClass)
	{
		WARNING_LOG(TEXT("[Server] ATPSCharacter::SpawnWeapon() WeaponClass not set on %s"), *GetName());
		return;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	SpawnParams.Instigator = this;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	Weapon = GetWorld()->SpawnActor<AM4Weapon>(WeaponClass, GetActorTransform(), SpawnParams);
	AttachWeapon();
}

void ATPSCharacter::AttachWeapon()
{
	if (!IsValid(Weapon)) return;

	if (!GetMesh()->DoesSocketExist(WeaponAttachSocket))
	{
		WARNING_LOG(TEXT("ATPSCharacter::AttachWeapon() socket '%s' not found on character mesh"), *WeaponAttachSocket.ToString());
	}

	Weapon->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, WeaponAttachSocket);
}

void ATPSCharacter::OnRep_Weapon()
{
	// 부착은 엔진이 복제하지만, 무기와 캐릭터의 도착 순서와 상관없이 맞추기 위해 클라에서도 붙인다
	AttachWeapon();
}

void ATPSCharacter::Reload()
{
	if (bIsDead) return;

	StopFire();
	ServerReload();
}

void ATPSCharacter::ServerReload_Implementation()
{
	if (bIsDead || !IsValid(Weapon)) return;

	Weapon->SetArmo();
}

void ATPSCharacter::NotifyControllerChanged()
{
	Super::NotifyControllerChanged();

	if (APlayerController* PC = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
		{
			if (DefaultMappingContext)
			{
				Subsystem->AddMappingContext(DefaultMappingContext, 0);
			}
		}
	}
}

void ATPSCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	// 축/액션 이름은 Config/DefaultInput.ini 참고
	PlayerInputComponent->BindAxis("MoveForward", this, &ATPSCharacter::OnMoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &ATPSCharacter::OnMoveRight);
	PlayerInputComponent->BindAxis("TurnRate", this, &ATPSCharacter::OnHorizontalLook);
	PlayerInputComponent->BindAxis("LookUpRate", this, &ATPSCharacter::OnVerticalLook);
	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ACharacter::Jump);
	PlayerInputComponent->BindAction("Jump", IE_Released, this, &ACharacter::StopJumping);
	PlayerInputComponent->BindAction("Sprint", IE_Pressed, this, &ATPSCharacter::StartSprint);
	PlayerInputComponent->BindAction("Sprint", IE_Released, this, &ATPSCharacter::StopSprint);
	PlayerInputComponent->BindAction("Reload", IE_Pressed, this, &ATPSCharacter::Reload);

	UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(PlayerInputComponent);
	if (!EIC) return;

	if (FireAction)
	{
		EIC->BindAction(FireAction, ETriggerEvent::Started, this, &ATPSCharacter::StartFire);
		EIC->BindAction(FireAction, ETriggerEvent::Completed, this, &ATPSCharacter::StopFire);
	}
}

void ATPSCharacter::OnMoveForward(float InAxis)
{
	if (bIsDead || !Controller) return;

	const FRotator YawRotation(0.f, Controller->GetControlRotation().Yaw, 0.f);
	AddMovementInput(FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X), InAxis);
}

void ATPSCharacter::OnMoveRight(float InAxis)
{
	if (bIsDead || !Controller) return;

	const FRotator YawRotation(0.f, Controller->GetControlRotation().Yaw, 0.f);
	AddMovementInput(FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y), InAxis);
}

void ATPSCharacter::OnHorizontalLook(float InAxis)
{
	AddControllerYawInput(InAxis);
}

void ATPSCharacter::OnVerticalLook(float InAxis)
{
	AddControllerPitchInput(InAxis);
}

float ATPSCharacter::GetAimPitch() const
{
	return FRotator::NormalizeAxis(GetBaseAimRotation().Pitch);
}

// ---------------------------------------------------------------- Sprint

void ATPSCharacter::StartSprint()
{
	if (bIsDead) return;

	if (UTPSCharacterMovementComponent* CMC = Cast<UTPSCharacterMovementComponent>(GetCharacterMovement()))
	{
		CMC->bWantsToSprint = true;
	}
}

void ATPSCharacter::StopSprint()
{
	if (UTPSCharacterMovementComponent* CMC = Cast<UTPSCharacterMovementComponent>(GetCharacterMovement()))
	{
		CMC->bWantsToSprint = false;
	}
}

bool ATPSCharacter::IsSprinting() const
{
	if (GetLocalRole() == ROLE_SimulatedProxy)
	{
		return bReplicatedSprinting;
	}

	const UTPSCharacterMovementComponent* CMC = Cast<UTPSCharacterMovementComponent>(GetCharacterMovement());
	return CMC && CMC->IsSprinting();
}

void ATPSCharacter::SetReplicatedSprinting(bool bNewSprinting)
{
	bReplicatedSprinting = bNewSprinting;
}

// ---------------------------------------------------------------- Fire

void ATPSCharacter::StartFire()
{
	if (bIsDead) return;

	FireOnce();
	GetWorldTimerManager().SetTimer(FireTimerHandle, this, &ATPSCharacter::FireOnce, GetFireInterval(), true);
}

void ATPSCharacter::StopFire()
{
	GetWorldTimerManager().ClearTimer(FireTimerHandle);
}

void ATPSCharacter::FireOnce()
{
	if (bIsDead || !IsLocallyControlled())
	{
		StopFire();
		return;
	}

	// 탄약은 복제값 기준. 서버가 다시 검사하므로 지연으로 한두 발 더 연출돼도 판정에는 영향 없음
	if (!HasArmo())
	{
		StopFire();
		return;
	}

	const double Now = GetWorld()->GetTimeSeconds();
	if (LastLocalFireTime >= 0.0 && Now - LastLocalFireTime < GetFireInterval() * 0.95) return;
	LastLocalFireTime = Now;

	// 화면 중앙(카메라) 기준으로 조준
	const FVector TraceStart = FollowCamera->GetComponentLocation();
	const FVector TraceDir = GetControlRotation().Vector();

	// 반응성을 위해 로컬에서 먼저 이펙트 재생 (판정은 서버)
	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(TPSFireLocal), true, this);
	Params.AddIgnoredActor(Weapon);
	const FVector TraceEnd = TraceStart + TraceDir * FireRange;
	const bool bHit = GetWorld()->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, COLLISION_WEAPON, Params);
	Weapon->PlayFireEffects();
	OnFireCosmetic(GetMuzzleLocation(), bHit ? Hit.ImpactPoint : TraceEnd, bHit);

	ServerFire(TraceStart, TraceDir);
}

bool ATPSCharacter::HasArmo() const
{
	return IsValid(Weapon) && Weapon->GetArmo()->GetArmo() > 0;
}

void ATPSCharacter::ServerFire_Implementation(FVector_NetQuantize TraceStart, FVector_NetQuantizeNormal TraceDir)
{
	if (bIsDead) return;

	// 연사 속도 검증 (네트워크 지터 감안해 약간 여유)
	const double Now = GetWorld()->GetTimeSeconds();
	if (LastServerFireTime >= 0.0 && Now - LastServerFireTime < GetFireInterval() * 0.8) return;
	LastServerFireTime = Now;

	// 시작점이 캐릭터에서 너무 멀면 조작된 요청으로 간주
	if (FVector::DistSquared(TraceStart, GetActorLocation()) > FMath::Square(600.f)) return;

	const FVector Dir = TraceDir.GetSafeNormal();
	if (Dir.IsNearlyZero()) return;

	// 탄약 검사와 차감은 서버에서만
	if (!IsValid(Weapon) || !Weapon->GetArmo()->ConsumeArmo()) return;

	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(TPSFireServer), true, this);
	Params.AddIgnoredActor(Weapon);
	const FVector TraceEnd = TraceStart + Dir * FireRange;
	const bool bHit = GetWorld()->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, COLLISION_WEAPON, Params);

	if (bHit && Hit.GetActor())
	{
		UGameplayStatics::ApplyPointDamage(Hit.GetActor(), Damage, Dir, Hit, GetController(), this, nullptr);
	}

	if (bDrawDebugTrace)
	{
		DrawDebugLine(GetWorld(), TraceStart, bHit ? Hit.ImpactPoint : TraceEnd, bHit ? FColor::Red : FColor::Green, false, 1.f);
	}

	MulticastFireEffects(bHit ? FVector(Hit.ImpactPoint) : TraceEnd, bHit);
}

void ATPSCharacter::MulticastFireEffects_Implementation(FVector_NetQuantize ImpactPoint, bool bHit)
{
	if (IsLocallyControlled() || GetNetMode() == NM_DedicatedServer) return;

	if (IsValid(Weapon))
	{
		Weapon->PlayFireEffects();
	}
	OnFireCosmetic(GetMuzzleLocation(), ImpactPoint, bHit);
}

FVector ATPSCharacter::GetMuzzleLocation() const
{
	if (IsValid(Weapon))
	{
		return Weapon->GetMuzzleLocation();
	}

	return GetActorLocation() + GetActorForwardVector() * 50.f + FVector(0.f, 0.f, 50.f);
}

// ---------------------------------------------------------------- Health / Death

float ATPSCharacter::TakeDamage(float DamageAmount, const FDamageEvent& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	if (!HasAuthority() || bIsDead) return 0.f;

	const float Applied = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
	if (Applied <= 0.f) return 0.f;

	const float OldHealth = Health;
	Health = FMath::Clamp(Health - Applied, 0.f, MaxHealth);
	OnRep_Health(OldHealth);

	if (Health <= 0.f)
	{
		Die(EventInstigator);
	}

	return Applied;
}

void ATPSCharacter::OnRep_Health(float OldHealth)
{
	OnHealthChanged(Health, Health - OldHealth);
}

void ATPSCharacter::Die(AController* Killer)
{
	bIsDead = true;
	OnRep_IsDead();

	AController* VictimController = GetController();
	if (ATPSGameMode* GM = GetWorld()->GetAuthGameMode<ATPSGameMode>())
	{
		GM->OnPlayerKilled(VictimController, Killer);
	}

	DetachFromControllerPendingDestroy();
	SetLifeSpan(10.f);
}

void ATPSCharacter::OnRep_IsDead()
{
	if (bIsDead)
	{
		HandleDeathCosmetic();
	}
}

void ATPSCharacter::HandleDeathCosmetic()
{
	StopFire();
	StopSprint();

	GetCharacterMovement()->StopMovementImmediately();
	GetCharacterMovement()->DisableMovement();
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// 래그돌은 화면이 있는 쪽에서만
	if (GetNetMode() != NM_DedicatedServer)
	{
		GetMesh()->SetCollisionProfileName(TEXT("Ragdoll"));
		GetMesh()->SetSimulatePhysics(true);
	}

	OnDeath();
}