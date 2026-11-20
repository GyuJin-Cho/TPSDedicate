#include "TPSCharacter.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerInput.h"
#include "Camera/PlayerCameraManager.h"
#include "Camera/CameraShakeBase.h"
#include "Animation/AnimMontage.h"
#include "Engine/DamageEvents.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Particles/ParticleSystem.h"
#include "Net/UnrealNetwork.h"
#include "DrawDebugHelpers.h"
#include "TimerManager.h"
#include "DedicateTPS/DedicateTPS.h"
#include "DedicateTPS/Game/TPSGameMode.h"
#include "DedicateTPS/Weapon/M4Weapon.h"
#include "DedicateTPS/Weapon/AmmoComponent.h"
#include "DedicateTPS/Utill/CHelpers.h"
#include "DedicateTPS/Utill/Utill.h"
#include "TPSCharacterMovementComponent.h"
#include "AnimNotify_ReloadAmmo.h"

ATPSCharacter::ATPSCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UTPSCharacterMovementComponent>(ACharacter::CharacterMovementComponentName))
{
	// 조준 카메라 보간용. 로컬 조종 캐릭터만 실제로 일한다
	PrimaryActorTick.bCanEverTick = true;

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
	DOREPLIFETIME_CONDITION(ATPSCharacter, bIsAiming, COND_SkipOwner);
	DOREPLIFETIME_CONDITION(ATPSCharacter, bIsReloading, COND_SkipOwner);
}

void ATPSCharacter::BeginPlay()
{
	Super::BeginPlay();

	// BP에서 바꾼 값을 평소 상태로 사용
	DefaultArmLength = CameraBoom->TargetArmLength;
	DefaultFieldOfView = FollowCamera->FieldOfView;

	if (HasAuthority())
	{
		Health = MaxHealth;
		SpawnWeapon();

		// 데디 서버는 렌더링을 안 해서 기본값으로는 본이 갱신되지 않고, 무기 소켓에 붙은 총구 위치가 틀어진다
		// 사격 판정(TraceShot)이 총구 위치를 쓰므로 서버에서도 본을 갱신한다
		GetMesh()->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
	}
}

void ATPSCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (IsLocallyControlled())
	{
		UpdateAimCamera(DeltaSeconds);
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

bool ATPSCharacter::IsActionHeld(FName ActionName) const
{
	const APlayerController* PC = Cast<APlayerController>(Controller);
	if (!PC || !PC->PlayerInput) return false;

	for (const FInputActionKeyMapping& Mapping : PC->PlayerInput->GetKeysForAction(ActionName))
	{
		if (PC->IsInputKeyDown(Mapping.Key)) return true;
	}
	return false;
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

// ---------------------------------------------------------------- Reload

void ATPSCharacter::Reload()
{
	if (!CanReload()) return;

	// 견착을 풀고 재장전. ServerSetAiming(false)가 ServerReload보다 먼저 전송된다
	StopAim();
	StartReload();

	if (!HasAuthority())
	{
		ServerReload();
	}
}

void ATPSCharacter::ServerReload_Implementation()
{
	if (!CanReload())
	{
		WARNING_LOG(TEXT("[Server] ServerReload rejected (%s): dead=%d reloading=%d"), *GetName(), bIsDead, bIsReloading);
		return;
	}

	StartReload();
}

bool ATPSCharacter::CanReload() const
{
	if (bIsDead || bIsReloading || !IsValid(Weapon)) return false;

	// 탄창이 가득 차 있으면 하지 않는다 (소유 클라는 복제값 기준)
	const UAmmoComponent* Ammo = Weapon->GetAmmo();
	return Ammo && Ammo->GetAmmo() < Ammo->GetMaxAmmo();
}

void ATPSCharacter::StartReload()
{
	StopFire();
	bIsReloading = true;
	bIsAiming = false;

	if (ReloadMontage && GetNetMode() != NM_DedicatedServer)
	{
		PlayAnimMontage(ReloadMontage);
	}

	const float Duration = GetReloadDuration();

	if (HasAuthority())
	{
		if (ReloadMontage && !ReloadMontage->Notifies.ContainsByPredicate([](const FAnimNotifyEvent& Event) { return Cast<UAnimNotify_ReloadAmmo>(Event.Notify) != nullptr; }))
		{
			WARNING_LOG(TEXT("[Server] %s has no ReloadAmmo notify. Ammo refills at the end"), *GetNameSafe(ReloadMontage));
		}

		const float AmmoTime = GetReloadAmmoTime();
		if (AmmoTime > 0.f)
		{
			GetWorldTimerManager().SetTimer(ReloadAmmoTimerHandle, this, &ATPSCharacter::ReloadAmmo, AmmoTime, false);
		}
		else
		{
			ReloadAmmo();
		}
	}

	// 서버는 소유 클라보다 RPC 지연만큼 늦게 시작하므로 조금 일찍 끝낸다.
	// 같이 끝내면 클라가 재장전 직후 보낸 사격이 서버에서 아직 재장전 중이라 거부된다
	constexpr float ServerEndSlack = 0.15f;
	const float EndTime = HasAuthority() && !IsLocallyControlled()
		? FMath::Max(GetReloadAmmoTime(), Duration - ServerEndSlack)
		: Duration;

	if (EndTime > 0.f)
	{
		GetWorldTimerManager().SetTimer(ReloadEndTimerHandle, this, &ATPSCharacter::FinishReload, EndTime, false);
	}
	else
	{
		FinishReload();
	}
}

void ATPSCharacter::ReloadAmmo()
{
	if (!HasAuthority() || bIsDead || !IsValid(Weapon)) return;

	Weapon->RefillAmmo();
}

void ATPSCharacter::FinishReload()
{
	if (!bIsReloading) return;

	bIsReloading = false;

	// 재장전 중에도 우클릭을 누르고 있었다면 바로 견착 (좌클릭도 누르고 있으면 StartAim이 사격까지 시작)
	if (IsLocallyControlled() && IsActionHeld(TEXT("Aim")))
	{
		StartAim();
	}
}

void ATPSCharacter::CancelReload()
{
	GetWorldTimerManager().ClearTimer(ReloadAmmoTimerHandle);
	GetWorldTimerManager().ClearTimer(ReloadEndTimerHandle);
	bIsReloading = false;
}

float ATPSCharacter::GetReloadDuration() const
{
	if (!ReloadMontage) return 0.f;

	return ReloadMontage->GetPlayLength() / FMath::Max(ReloadMontage->RateScale, KINDA_SMALL_NUMBER);
}

float ATPSCharacter::GetReloadAmmoTime() const
{
	if (!ReloadMontage) return 0.f;

	for (const FAnimNotifyEvent& Event : ReloadMontage->Notifies)
	{
		if (Cast<UAnimNotify_ReloadAmmo>(Event.Notify))
		{
			return Event.GetTriggerTime() / FMath::Max(ReloadMontage->RateScale, KINDA_SMALL_NUMBER);
		}
	}

	// 노티파이가 없으면 몽타주가 끝날 때 채운다
	return GetReloadDuration();
}

void ATPSCharacter::OnRep_IsReloading()
{
	// 다른 클라에 보이는 캐릭터 전용 (소유 클라는 SkipOwner)
	if (bIsReloading && ReloadMontage && !bIsDead)
	{
		PlayAnimMontage(ReloadMontage);
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
	PlayerInputComponent->BindAction("Aim", IE_Pressed, this, &ATPSCharacter::StartAim);
	PlayerInputComponent->BindAction("Aim", IE_Released, this, &ATPSCharacter::StopAim);
	PlayerInputComponent->BindAction("Fire", IE_Pressed, this, &ATPSCharacter::StartFire);
	PlayerInputComponent->BindAction("Fire", IE_Released, this, &ATPSCharacter::StopFire);
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

// ---------------------------------------------------------------- Aim

void ATPSCharacter::StartAim()
{
	if (bIsDead || bIsAiming || bIsReloading) return;

	// 견착 중에는 달리지 않는다
	StopSprint();
	SetAiming(true);

	// 좌클릭을 먼저 누르고 있었다면 견착과 동시에 사격 시작 (ServerSetAiming이 먼저 전송되므로 서버에서도 거부되지 않는다)
	if (IsActionHeld(TEXT("Fire")))
	{
		StartFire();
	}
}

void ATPSCharacter::StopAim()
{
	if (!bIsAiming) return;

	StopFire();
	SetAiming(false);
}

void ATPSCharacter::SetAiming(bool bNewAiming)
{
	bIsAiming = bNewAiming;

	if (!HasAuthority())
	{
		ServerSetAiming(bNewAiming);
	}
}

void ATPSCharacter::ServerSetAiming_Implementation(bool bNewAiming)
{
	if (bIsDead && bNewAiming) return;

	bIsAiming = bNewAiming;
}

void ATPSCharacter::UpdateAimCamera(float DeltaSeconds)
{
	const float TargetArmLength = bIsAiming ? AimArmLength : DefaultArmLength;
	const float TargetFieldOfView = bIsAiming ? AimFieldOfView : DefaultFieldOfView;

	CameraBoom->TargetArmLength = FMath::FInterpTo(CameraBoom->TargetArmLength, TargetArmLength, DeltaSeconds, AimInterpSpeed);
	FollowCamera->SetFieldOfView(FMath::FInterpTo(FollowCamera->FieldOfView, TargetFieldOfView, DeltaSeconds, AimInterpSpeed));
}

void ATPSCharacter::ResetAimCamera()
{
	CameraBoom->TargetArmLength = DefaultArmLength;
	FollowCamera->SetFieldOfView(DefaultFieldOfView);
}

// ---------------------------------------------------------------- Sprint

void ATPSCharacter::StartSprint()
{
	if (bIsDead || bIsAiming) return;

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
	// 견착하지 않으면 좌클릭해도 쏘지 않는다
	if (bIsDead || !bIsAiming || bIsReloading) return;

	FireOnce();
	GetWorldTimerManager().SetTimer(FireTimerHandle, this, &ATPSCharacter::FireOnce, GetFireInterval(), true);
}

void ATPSCharacter::StopFire()
{
	GetWorldTimerManager().ClearTimer(FireTimerHandle);
}

void ATPSCharacter::FireOnce()
{
	if (bIsDead || !bIsAiming || bIsReloading || !IsLocallyControlled())
	{
		StopFire();
		return;
	}

	// 탄약은 복제값 기준. 서버가 다시 검사하므로 지연으로 한두 발 더 연출돼도 판정에는 영향 없음
	// 빈 탄창이면 빈 격발 동작만 한 번 하고 멈춘다 (누를 때마다, 또는 연사 중 탄이 떨어진 순간)
	if (!HasAmmo())
	{
		StopFire();
		if (IsValid(Weapon))
		{
			PlayEmptyFireMontage();
		}
		return;
	}

	const double Now = GetWorld()->GetTimeSeconds();
	if (LastLocalFireTime >= 0.0 && Now - LastLocalFireTime < GetFireInterval() * 0.95) return;
	LastLocalFireTime = Now;

	// 화면 중앙(카메라) 기준으로 조준. 카메라는 ControlRotation을 그대로 바라보므로 크로스헤어 위치가 곧 판정 방향이다
	const FVector TraceStart = FollowCamera->GetComponentLocation();
	const FVector TraceDir = GetControlRotation().Vector();

	// 반응성을 위해 로컬에서 먼저 이펙트 재생 (판정은 서버, 같은 TraceShot을 쓰므로 탄착점이 거의 일치한다)
	FHitResult Hit;
	FVector Impact;
	const bool bHit = TraceShot(TraceStart, TraceDir, Hit, Impact);
	Weapon->PlayFireEffects();
	OnFireCosmetic(GetMuzzleLocation(), Impact, bHit);
	PlayFireMontage();

	// 피도 로컬 판정으로 먼저 보여 준다 (서버 판정과 드물게 다를 수 있다)
	const ATPSCharacter* HitCharacter = bHit ? Cast<ATPSCharacter>(Hit.GetActor()) : nullptr;
	if (IsValid(HitCharacter))
	{
		HitCharacter->PlayBloodEffect(Impact, Hit.ImpactNormal);
	}

	ServerFire(TraceStart, TraceDir);

	// 이번 탄은 이미 보냈으므로 반동은 다음 탄부터 적용된다
	ApplyRecoil();
}

void ATPSCharacter::ApplyRecoil()
{
	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC || !IsValid(Weapon)) return;

	const FWeaponStatTableRow& Stat = Weapon->GetStat();

	// AddControllerPitchInput은 입력 스케일이 곱해지므로 각도를 직접 더한다
	FRotator ControlRotation = PC->GetControlRotation();
	ControlRotation.Pitch += FMath::FRandRange(Stat.RecoilPitchMin, Stat.RecoilPitchMax);
	ControlRotation.Yaw += FMath::FRandRange(-Stat.RecoilYawMax, Stat.RecoilYawMax);
	PC->SetControlRotation(ControlRotation);

	if (FireCameraShake && PC->PlayerCameraManager)
	{
		PC->PlayerCameraManager->StartCameraShake(FireCameraShake);
	}
}

void ATPSCharacter::PlayFireMontage()
{
	if (FireMontage)
	{
		PlayAnimMontage(FireMontage);
	}
}

void ATPSCharacter::PlayEmptyFireMontage()
{
	if (EmptyFireMontage)
	{
		PlayAnimMontage(EmptyFireMontage);
	}
}

bool ATPSCharacter::HasAmmo() const
{
	return IsValid(Weapon) && Weapon->GetAmmo()->GetAmmo() > 0;
}

void ATPSCharacter::ServerFire_Implementation(FVector_NetQuantize TraceStart, FVector_NetQuantizeNormal TraceDir)
{
	// ServerSetAiming이 같은 Reliable 순서로 먼저 도착하므로 정상 클라는 여기서 걸리지 않는다
	if (bIsDead || !bIsAiming || bIsReloading)
	{
		WARNING_LOG(TEXT("[Server] ServerFire rejected (%s): dead=%d aiming=%d reloading=%d"), *GetName(), bIsDead, bIsAiming, bIsReloading);
		return;
	}

	// 연사 속도 검증. 서버 틱(약 21Hz)에 RPC가 몰려 도착해도 FireTimeSlack만큼은 허용하고,
	// 평균 속도는 연사 간격을 넘지 못하게 한다
	constexpr double FireTimeSlack = 0.1;
	const double Now = GetWorld()->GetTimeSeconds();
	if (Now < NextServerFireTime - FireTimeSlack)
	{
		WARNING_LOG(TEXT("[Server] ServerFire rejected (%s): fire rate"), *GetName());
		return;
	}
	NextServerFireTime = FMath::Max(NextServerFireTime, Now - FireTimeSlack) + GetFireInterval();

	// 시작점이 카메라가 있을 수 있는 범위를 벗어나면 조작된 요청으로 간주
	const FVector Pivot = GetCameraPivot();
	const float StartDistance = FVector::Dist(TraceStart, Pivot);
	const float MaxStartDistance = GetMaxTraceStartDistance();
	if (StartDistance > MaxStartDistance)
	{
		WARNING_LOG(TEXT("[Server] ServerFire rejected (%s): trace start too far (%.0f > %.0f)"), *GetName(), StartDistance, MaxStartDistance);
		return;
	}

	const FVector Dir = TraceDir.GetSafeNormal();
	if (Dir.IsNearlyZero()) return;

	// 벽 너머로 옮긴 시작점은 따로 막지 않는다. TraceShot의 총구 -> 탄착점 구간이 실제 총구 위치에서 검사하므로 벽에 막힌다

	// 탄약 검사와 차감은 서버에서만
	if (!IsValid(Weapon) || !Weapon->GetAmmo()->ConsumeAmmo())
	{
		WARNING_LOG(TEXT("[Server] ServerFire rejected (%s): no weapon or no ammo"), *GetName());
		return;
	}

	FHitResult Hit;
	FVector Impact;
	const bool bHit = TraceShot(TraceStart, Dir, Hit, Impact);

	if (bHit && Hit.GetActor())
	{
		UGameplayStatics::ApplyPointDamage(Hit.GetActor(), Weapon->GetStat().Damage, Dir, Hit, GetController(), this, nullptr);
	}

	ATPSCharacter* HitCharacter = bHit ? Cast<ATPSCharacter>(Hit.GetActor()) : nullptr;
	MulticastFireEffects(Impact, Hit.ImpactNormal, bHit, HitCharacter);
}

bool ATPSCharacter::TraceShot(const FVector& TraceStart, const FVector& Dir, FHitResult& OutHit, FVector& OutImpact) const
{
	FCollisionQueryParams Params(SCENE_QUERY_STAT(TPSFire), true, this);
	Params.AddIgnoredActor(Weapon);

	// 1차: 카메라 중앙(크로스헤어) 방향으로 조준 지점 찾기
	// 카메라는 캐릭터 뒤에 있으므로 그대로 쏘면 카메라와 캐릭터 사이에 선 상대(내 뒤)가 맞는다.
	// 같은 조준선 위에서 시작점만 기준점 높이까지 앞으로 옮겨 뒤쪽 구간을 판정에서 뺀다 (방향은 그대로라 크로스헤어와 일치)
	const FVector Pivot = GetCameraPivot();
	const FVector AimStart = TraceStart + Dir * FMath::Max(FVector::DotProduct(Pivot - TraceStart, Dir), 0.f);
	const FVector TraceEnd = TraceStart + Dir * FireRange;
	bool bHit = GetWorld()->LineTraceSingleByChannel(OutHit, AimStart, TraceEnd, COLLISION_WEAPON, Params);
	OutImpact = bHit ? FVector(OutHit.ImpactPoint) : TraceEnd;

	// 2차: 실제 총알은 총구에서 나가므로 기준점 -> 총구 -> 조준 지점 사이가 막혔는지 확인 (코너 피킹, 벽에 붙어 쏘기)
	// 기준점 -> 총구 구간은 총구가 얇은 벽을 뚫고 나간 경우를 잡는다
	const FVector Muzzle = GetMuzzleLocation();
	struct FTraceSegment { FVector Start; FVector End; };
	const FTraceSegment Segments[] = { { Pivot, Muzzle }, { Muzzle, OutImpact } };

	if (bDrawDebugTrace)
	{
		DrawDebugLine(GetWorld(), AimStart, OutImpact, bHit ? FColor::Red : FColor::Green, false, 1.f);
		DrawDebugLine(GetWorld(), Muzzle, OutImpact, FColor::Yellow, false, 1.f);
		DrawDebugSphere(GetWorld(), Muzzle, 5.f, 8, FColor::Yellow, false, 1.f);
	}

	for (const FTraceSegment& Segment : Segments)
	{
		FHitResult BlockHit;
		if (!GetWorld()->LineTraceSingleByChannel(BlockHit, Segment.Start, Segment.End, COLLISION_WEAPON, Params)) continue;

		// 조준한 대상에 먼저 닿은 건 정상 명중 (캡슐을 다른 각도로 들어가면 지점만 조금 다르다)
		if (bHit && BlockHit.GetActor() == OutHit.GetActor()) break;

		OutHit = BlockHit;
		OutImpact = BlockHit.ImpactPoint;
		return true;
	}

	return bHit;
}

FVector ATPSCharacter::GetCameraPivot() const
{
	// USpringArmComponent::UpdateDesiredArmLocation의 ArmOrigin과 같다
	return CameraBoom->GetComponentLocation() + CameraBoom->TargetOffset;
}

float ATPSCharacter::GetMaxTraceStartDistance() const
{
	// 조준 시작 직후에는 카메라가 평소 거리에서 보간 중이므로 두 거리 중 긴 쪽 기준
	// 여유값은 클라와 서버의 캐릭터 위치 차이 (이동 중 지연만큼 어긋난다)
	constexpr float PositionTolerance = 150.f;
	return FMath::Max(DefaultArmLength, AimArmLength) + CameraBoom->SocketOffset.Size() + PositionTolerance;
}

void ATPSCharacter::MulticastFireEffects_Implementation(FVector_NetQuantize ImpactPoint, FVector_NetQuantizeNormal ImpactNormal, bool bHit, ATPSCharacter* HitCharacter)
{
	if (IsLocallyControlled() || GetNetMode() == NM_DedicatedServer) return;

	if (IsValid(Weapon))
	{
		Weapon->PlayFireEffects();
	}
	OnFireCosmetic(GetMuzzleLocation(), ImpactPoint, bHit);
	PlayFireMontage();

	if (IsValid(HitCharacter))
	{
		HitCharacter->PlayBloodEffect(ImpactPoint, ImpactNormal);
	}
}

void ATPSCharacter::PlayBloodEffect(const FVector& Location, const FVector& Normal) const
{
	if (GetNetMode() == NM_DedicatedServer) return;

	const FRotator Rotation = Normal.IsNearlyZero() ? FRotator::ZeroRotator : Normal.Rotation();

	if (BloodNiagara)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, BloodNiagara, Location, Rotation, BloodScale);
	}
	if (BloodParticle)
	{
		UGameplayStatics::SpawnEmitterAtLocation(this, BloodParticle, Location, Rotation, BloodScale);
	}
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

	// 총알(포인트 데미지)은 맞은 부위에 따라 배율 적용
	bool bHeadshot = false;
	if (DamageEvent.IsOfType(FPointDamageEvent::ClassID))
	{
		const FPointDamageEvent& PointEvent = static_cast<const FPointDamageEvent&>(DamageEvent);
		// 실제로 맞은 트레이스의 방향을 쓴다. TraceShot의 2차 검사(기준점 -> 총구 -> 탄착점)에서 맞으면 카메라 방향과 다르다
		const FVector TraceDelta = PointEvent.HitInfo.TraceEnd - PointEvent.HitInfo.TraceStart;
		const FVector ShotDir = TraceDelta.IsNearlyZero() ? FVector(PointEvent.ShotDirection) : TraceDelta;
		const EHitZone Zone = GetHitZone(PointEvent.HitInfo.ImpactPoint, ShotDir);
		DamageAmount *= GetHitZoneMultiplier(Zone);
		bHeadshot = Zone == EHitZone::Head;
		INFO_LOG(TEXT("[Server] %s hit by %s: zone=%s damage=%.1f"), *GetName(), *GetNameSafe(DamageCauser), *UEnum::GetValueAsString(Zone), DamageAmount);
	}

	const float Applied = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
	if (Applied <= 0.f) return 0.f;

	// 헤드샷은 쏜 사람에게만 알린다 (ServerFire가 DamageCauser로 사격자 캐릭터를 넘긴다)
	ATPSCharacter* Shooter = Cast<ATPSCharacter>(DamageCauser);
	if (bHeadshot && IsValid(Shooter))
	{
		Shooter->ClientNotifyHeadshot();
	}

	const float OldHealth = Health;
	Health = FMath::Clamp(Health - Applied, 0.f, MaxHealth);
	OnRep_Health(OldHealth);

	if (Health <= 0.f)
	{
		Die(EventInstigator);
	}

	return Applied;
}

void ATPSCharacter::ClientNotifyHeadshot_Implementation()
{
	LastHeadshotTime = GetWorld()->GetTimeSeconds();
	OnHeadshotConfirmed();
}

EHitZone ATPSCharacter::GetHitZone(const FVector& ImpactPoint, const FVector& ShotDir) const
{
	const FVector Dir = ShotDir.GetSafeNormal();
	if (Dir.IsNearlyZero()) return EHitZone::Body;

	// 부위별 판정 캡슐 (본 구간 A-B + 반지름)
	struct FZoneCapsule { FVector A; FVector B; float Radius; EHitZone Zone; };
	TArray<FZoneCapsule, TInlineAllocator<16>> ZoneCapsules;

	// 머리: head 본에서 목 -> 머리 방향으로 HeadLength만큼
	FVector Neck, Head;
	if (GetBoneLocation(TEXT("neck_01"), Neck) && GetBoneLocation(TEXT("head"), Head))
	{
		ZoneCapsules.Add({ Head, Head + (Head - Neck).GetSafeNormal() * HeadLength, HeadRadius, EHitZone::Head });
	}

	// UE4 마네킹 스켈레톤 본 이름 (MESH_PC_xx 공통)
	struct FBoneSegment
	{
		const TCHAR* From;
		const TCHAR* To;
		EHitZone Zone;
	};
	
	static const FBoneSegment Segments[] =
	{
		{ TEXT("pelvis"),     TEXT("spine_03"),   EHitZone::Body },
		{ TEXT("spine_03"),   TEXT("neck_01"),    EHitZone::Body },
		{ TEXT("upperarm_l"), TEXT("lowerarm_l"), EHitZone::Limb },
		{ TEXT("lowerarm_l"), TEXT("hand_l"),     EHitZone::Limb },
		{ TEXT("upperarm_r"), TEXT("lowerarm_r"), EHitZone::Limb },
		{ TEXT("lowerarm_r"), TEXT("hand_r"),     EHitZone::Limb },
		{ TEXT("thigh_l"),    TEXT("calf_l"),     EHitZone::Limb },
		{ TEXT("calf_l"),     TEXT("foot_l"),     EHitZone::Limb },
		{ TEXT("foot_l"),     TEXT("ball_l"),     EHitZone::Limb },
		{ TEXT("thigh_r"),    TEXT("calf_r"),     EHitZone::Limb },
		{ TEXT("calf_r"),     TEXT("foot_r"),     EHitZone::Limb },
		{ TEXT("foot_r"),     TEXT("ball_r"),     EHitZone::Limb },
	};

	for (const FBoneSegment& Segment : Segments)
	{
		FVector A, B;
		if (!GetBoneLocation(Segment.From, A) || !GetBoneLocation(Segment.To, B)) continue;
		ZoneCapsules.Add({ A, B, Segment.Zone == EHitZone::Body ? TorsoRadius : LimbRadius, Segment.Zone });
	}

	// 총알 경로: 캡슐 명중 지점보다 캡슐 반지름만큼 앞에서 시작해 캡슐을 완전히 빠져나갈 때까지
	// 조준 자세의 손/팔뚝은 캡슐 밖으로 나와 있으므로, 명중 지점부터 시작하면 얼굴 앞을 가린 팔을 건너뛴다
	const UCapsuleComponent* Capsule = GetCapsuleComponent();
	const float LeadLength = Capsule->GetScaledCapsuleRadius();
	const float PassLength = 2.f * (Capsule->GetScaledCapsuleHalfHeight() + Capsule->GetScaledCapsuleRadius());
	const FVector RayStart = ImpactPoint - Dir * LeadLength;
	const FVector RayEnd = ImpactPoint + Dir * PassLength;

	bool bPassedAnyZone = false;
	EHitZone FirstZone = EHitZone::Body;		// 경로가 실제로 지나간 부위 중 가장 앞쪽 (팔이 머리를 가리면 팔)
	float FirstAlongRay = TNumericLimits<float>::Max();
	EHitZone NearestZone = EHitZone::Body;		// 아무 부위도 안 지나갔을 때 쓸 가장 가까운 몸통/팔다리
	float NearestGap = TNumericLimits<float>::Max();

	for (const FZoneCapsule& ZoneCapsule : ZoneCapsules)
	{
		FVector OnRay, OnBone;
		FMath::SegmentDistToSegmentSafe(RayStart, RayEnd, ZoneCapsule.A, ZoneCapsule.B, OnRay, OnBone);
		const float Gap = FVector::Dist(OnRay, OnBone) - ZoneCapsule.Radius;

		if (Gap <= 0.f)
		{
			const float AlongRay = FVector::DotProduct(OnRay - RayStart, Dir);
			if (AlongRay < FirstAlongRay)
			{
				bPassedAnyZone = true;
				FirstAlongRay = AlongRay;
				FirstZone = ZoneCapsule.Zone;
			}
		}
		else if (ZoneCapsule.Zone != EHitZone::Head && Gap < NearestGap)
		{
			NearestGap = Gap;
			NearestZone = ZoneCapsule.Zone;
		}
	}

	return bPassedAnyZone ? FirstZone : NearestZone;
}

bool ATPSCharacter::GetBoneLocation(FName BoneName, FVector& OutLocation) const
{
	const USkeletalMeshComponent* MeshComp = GetMesh();
	if (!IsValid(MeshComp) || MeshComp->GetBoneIndex(BoneName) == INDEX_NONE) return false;

	OutLocation = MeshComp->GetSocketLocation(BoneName);
	return true;
}

float ATPSCharacter::GetHitZoneMultiplier(EHitZone Zone) const
{
	switch (Zone)
	{
	case EHitZone::Head: return HeadDamageMultiplier;
	case EHitZone::Limb: return LimbDamageMultiplier;
	default:             return 1.f;
	}
}

void ATPSCharacter::OnRep_Health(float OldHealth)
{
	OnHealthChanged(Health, Health - OldHealth);
}

void ATPSCharacter::FellOutOfWorld(const UDamageType& DmgType)
{
	// 킬러 없는 사망 -> GameMode가 남은 사람을 승자로 정한다
	// CMC는 MOVE_None에서도 매 틱 KillZ를 검사해 사망 후에도 계속 불린다. 여기서 Destroy하면 클라가 사망 연출(OnRep_IsDead)을
	// 받기 전에 사라질 수 있으므로, 서버에서는 Destroy하지 않고 Die의 LifeSpan에 맡긴다
	if (HasAuthority())
	{
		if (!bIsDead)
		{
			INFO_LOG(TEXT("[Server] %s fell out of world"), *GetName());
			Die(nullptr);
		}
		return;
	}

	Super::FellOutOfWorld(DmgType);
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
	CancelReload();
	StopSprint();

	// 서버와 각 클라에서 직접 끈다 (사망 후에는 컨트롤러가 떨어져 Tick 보간이 멈추므로 카메라도 바로 되돌린다)
	bIsAiming = false;
	ResetAimCamera();

	GetCharacterMovement()->StopMovementImmediately();
	GetCharacterMovement()->DisableMovement();
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// 래그돌은 화면이 있는 쪽에서만
	if (GetNetMode() != NM_DedicatedServer)
	{
		GetMesh()->SetCollisionProfileName(TEXT("Ragdoll"));
		GetMesh()->SetSimulatePhysics(true);
	}

	// 서버는 메시가 기본 프로필(CharacterMesh) 그대로라 PhysicsAsset이 있으면 시체가 총알을 막는다. 래그돌 프로필도 Weapon 채널을 Block하므로 양쪽 모두 끈다
	GetMesh()->SetCollisionResponseToChannel(COLLISION_WEAPON, ECR_Ignore);

	OnDeath();
}