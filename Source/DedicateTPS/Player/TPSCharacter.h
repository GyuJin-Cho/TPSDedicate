#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "TPSCharacter.generated.h"

// 피격 부위. 판정은 캡슐이고, 부위는 총알 경로와 본 위치로 정한다
UENUM(BlueprintType)
enum class EHitZone : uint8
{
	Body,
	Head,
	Limb,
};

// 서버 권한(Server Authoritative) TPS 캐릭터
// - 이동: CharacterMovementComponent가 클라 예측 + 서버 보정 처리
// - 조준: 우클릭 동안 견착. 조준 중에만 사격 가능 (서버도 bIsAiming으로 다시 검사)
// - 사격: 클라 -> ServerFire RPC -> 서버 라인트레이스/데미지 -> Multicast 이펙트
//   판정은 카메라 중앙(크로스헤어) 기준. 반동은 사격 후 ControlRotation을 랜덤하게 틀어 다음 탄에 반영
// - 체력/사망: 서버에서만 변경, OnRep으로 클라 반영
UCLASS()
class DEDICATETPS_API ATPSCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	ATPSCharacter(const FObjectInitializer& ObjectInitializer);

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	virtual float TakeDamage(float DamageAmount, const FDamageEvent& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;

	UFUNCTION(BlueprintPure, Category = "Health")
	float GetHealth() const { return Health; }

	UFUNCTION(BlueprintPure, Category = "Health")
	float GetMaxHealth() const { return MaxHealth; }

	UFUNCTION(BlueprintPure, Category = "Health")
	bool IsDead() const { return bIsDead; }

	// 애님BP용 조준 Pitch (-90~90). 원격 캐릭터도 RemoteViewPitch로 자동 복제됨
	UFUNCTION(BlueprintPure, Category = "Aim")
	float GetAimPitch() const;

	UFUNCTION(BlueprintPure, Category = "Aim")
	bool IsAiming() const { return bIsAiming; }

	UFUNCTION(BlueprintPure, Category = "Reload")
	bool IsReloading() const { return bIsReloading; }

	// 로컬/서버는 CMC 상태, 시뮬레이티드 프록시는 복제값을 반환
	UFUNCTION(BlueprintPure, Category = "Sprint")
	bool IsSprinting() const;

	// 서버 전용. UTPSCharacterMovementComponent가 매 이동 후 호출
	void SetReplicatedSprinting(bool bNewSprinting);

	UFUNCTION(BlueprintPure, Category = "Weapon")
	class AM4Weapon* GetWeapon() const { return Weapon; }

	// 소유 클라 전용. 마지막으로 헤드샷을 맞힌 월드 시간 (없으면 음수). HUD가 표시 시간을 계산할 때 쓴다
	double GetLastHeadshotTime() const { return LastHeadshotTime; }

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void Destroyed() override;

	// 기본 구현은 Die 없이 Destroy만 해서 경기가 끝나지 않으므로, 살아 있으면 사망으로 처리한다
	virtual void FellOutOfWorld(const class UDamageType& DmgType) override;

	// ---------- Input ----------
	void OnMoveForward(float InAxis);
	void OnMoveRight(float InAxis);
	void OnHorizontalLook(float InAxis);
	void OnVerticalLook(float InAxis);
	void StartFire();
	void StopFire();
	void StartAim();
	void StopAim();
	void StartSprint();
	void StopSprint();
	void Reload();

	// 로컬 전용. 액션에 묶인 키가 지금 눌려 있는지 (Pressed 이벤트를 놓친 경우 이어서 시작하기 위함)
	bool IsActionHeld(FName ActionName) const;

	// ---------- Aim ----------
	// 로컬에서 바로 적용하고 서버에 알림
	void SetAiming(bool bNewAiming);

	// 사격 RPC와 같은 Reliable 채널이라 ServerFire보다 먼저 도착한다
	UFUNCTION(Server, Reliable)
	void ServerSetAiming(bool bNewAiming);

	// 로컬 전용. 조준 여부에 따라 카메라 거리와 FOV를 보간
	void UpdateAimCamera(float DeltaSeconds);
	void ResetAimCamera();

	// ---------- Weapon ----------
	// 서버 전용
	void SpawnWeapon();
	void AttachWeapon();

	UFUNCTION()
	void OnRep_Weapon();

	// ---------- Reload ----------
	// 소유 클라는 입력 즉시 재생하고 서버에 알린다. 서버는 다시 검사한 뒤 같은 흐름을 탄다
	UFUNCTION(Server, Reliable)
	void ServerReload();

	bool CanReload() const;

	// 소유 클라와 서버 공통. 상태 설정, 몽타주 재생, 타이머 시작 (탄약 채우기는 서버만)
	void StartReload();
	void FinishReload();
	void CancelReload();

	// 서버 전용. ReloadAmmo 노티파이 시점에 호출
	void ReloadAmmo();

	// 몽타주 재생 시간 / ReloadAmmo 노티파이 시간 (RateScale 반영, 몽타주가 없으면 0)
	float GetReloadDuration() const;
	float GetReloadAmmoTime() const;

	UFUNCTION()
	void OnRep_IsReloading();

	// ---------- Fire ----------
	void FireOnce();
	bool HasAmmo() const;

	// 로컬 전용. 조준점을 랜덤하게 틀고 카메라 셰이크 재생
	void ApplyRecoil();
	void PlayFireMontage();

	// 로컬 전용. 탄이 없을 때 사격하면 재생 (서버에 보내지 않으므로 다른 클라에는 보이지 않는다)
	void PlayEmptyFireMontage();

	UFUNCTION(Server, Reliable)
	void ServerFire(FVector_NetQuantize TraceStart, FVector_NetQuantizeNormal TraceDir);

	// 사격자 본인은 이미 로컬에서 재생했으므로 제외
	// HitCharacter는 캐릭터를 맞혔을 때만 채운다 (피 이펙트용)
	UFUNCTION(NetMulticast, Unreliable)
	void MulticastFireEffects(FVector_NetQuantize ImpactPoint, FVector_NetQuantizeNormal ImpactNormal, bool bHit, ATPSCharacter* HitCharacter);

	// 화면이 있는 쪽 전용. 이 캐릭터가 총에 맞은 지점에 피 이펙트 재생
	void PlayBloodEffect(const FVector& Location, const FVector& Normal) const;

	// BP에서 총구 화염/사운드/탄흔 처리
	UFUNCTION(BlueprintImplementableEvent, Category = "Fire")
	void OnFireCosmetic(const FVector& MuzzleLocation, const FVector& ImpactPoint, bool bHit);

	FVector GetMuzzleLocation() const;

	// 스프링암이 카메라 위치를 계산하는 기준점 (카메라 충돌 검사도 여기서 시작한다)
	FVector GetCameraPivot() const;

	// 서버가 허용하는 기준점~TraceStart 최대 거리 (가장 긴 팔 길이 + 소켓 오프셋 + 네트워크 위치 오차)
	float GetMaxTraceStartDistance() const;

	// 카메라(크로스헤어) 방향으로 탄착점을 구한 뒤, 총구에서 그 지점까지 막힌 곳이 있으면 그쪽에 맞은 것으로 처리
	// 클라 연출과 서버 판정이 같은 함수를 쓴다. 반환값은 무언가에 맞았는지, OutImpact는 맞은 지점 또는 사거리 끝
	bool TraceShot(const FVector& TraceStart, const FVector& Dir, FHitResult& OutHit, FVector& OutImpact) const;

	// ---------- Health / Death ----------
	UFUNCTION()
	void OnRep_Health(float OldHealth);

	UFUNCTION()
	void OnRep_IsDead();

	// 서버 전용. 캡슐에 맞은 총알이 실제로 지나간 부위. 머리는 판정 캡슐을 실제로 지나가야 인정하고,
	// 어느 부위에도 닿지 않고 캡슐만 스친 경우는 가장 가까운 몸통/팔다리로 본다
	EHitZone GetHitZone(const FVector& ImpactPoint, const FVector& ShotDir) const;
	float GetHitZoneMultiplier(EHitZone Zone) const;

	// 메시에 해당 본이 없으면 false
	bool GetBoneLocation(FName BoneName, FVector& OutLocation) const;

	// 서버 -> 사격자 클라. 헤드샷 표시용. 기본 배율에서는 머리가 즉사라 드물게 보내므로 Reliable로 둔다
	// (머리 배율을 낮춰도 연사 속도 이하라 부담은 작다)
	UFUNCTION(Client, Reliable)
	void ClientNotifyHeadshot();

	// BP에서 헤드샷 사운드 등 추가 연출 (HUD 문구는 UMainHudWidget이 그린다)
	UFUNCTION(BlueprintImplementableEvent, Category = "Fire")
	void OnHeadshotConfirmed();

	void Die(AController* Killer);
	void HandleDeathCosmetic();

	UFUNCTION(BlueprintImplementableEvent, Category = "Health")
	void OnHealthChanged(float NewHealth, float Delta);

	UFUNCTION(BlueprintImplementableEvent, Category = "Health")
	void OnDeath();

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	TObjectPtr<class USpringArmComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	TObjectPtr<class UCameraComponent> FollowCamera;

	// 스폰할 무기 BP (M4Weapon BP 지정)
	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	TSubclassOf<class AM4Weapon> WeaponClass;

	// 캐릭터 메시에서 무기를 붙일 소켓
	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	FName WeaponAttachSocket = TEXT("WeaponSocket");

	UPROPERTY(ReplicatedUsing = OnRep_Weapon, BlueprintReadOnly, Category = "Weapon")
	TObjectPtr<class AM4Weapon> Weapon;

	// ---------- Aim ----------
	// 조준 시 카메라 거리. 평소 거리는 BeginPlay에서 CameraBoom 값을 읽어 둔다
	UPROPERTY(EditDefaultsOnly, Category = "Aim")
	float AimArmLength = 150.f;

	UPROPERTY(EditDefaultsOnly, Category = "Aim")
	float AimFieldOfView = 65.f;

	UPROPERTY(EditDefaultsOnly, Category = "Aim")
	float AimInterpSpeed = 12.f;

	// ---------- Recoil ----------
	// 한 발마다 조준점이 위로 튀는 각도(도). 음수를 넣으면 아래로도 튄다
	UPROPERTY(EditDefaultsOnly, Category = "Recoil")
	float RecoilPitchMin = 0.2f;

	UPROPERTY(EditDefaultsOnly, Category = "Recoil")
	float RecoilPitchMax = 0.6f;

	// 한 발마다 좌우로 튀는 최대 각도(도). -값 ~ +값에서 랜덤
	UPROPERTY(EditDefaultsOnly, Category = "Recoil")
	float RecoilYawMax = 0.35f;

	// 사격 시 카메라 흔들림 (BP에서 Camera Shake 에셋 지정, 비우면 흔들림 없음)
	// 크게 흔들면 화면 중앙과 판정 방향이 잠깐 어긋나 보이므로 작게 둔다
	UPROPERTY(EditDefaultsOnly, Category = "Recoil")
	TSubclassOf<class UCameraShakeBase> FireCameraShake;

	// 사격 시 캐릭터 몽타주 (ABP의 Slot 노드를 거친다, 비우면 재생 안 함)
	UPROPERTY(EditDefaultsOnly, Category = "Fire")
	TObjectPtr<class UAnimMontage> FireMontage;

	// 탄이 없는데 사격했을 때 몽타주 (빈 격발). 비우면 재생 안 함
	UPROPERTY(EditDefaultsOnly, Category = "Fire")
	TObjectPtr<class UAnimMontage> EmptyFireMontage;

	// 재장전 몽타주. 탄창이 들어가는 프레임에 "Reload Ammo" 노티파이를 넣는다 (없으면 몽타주 끝에서 채움)
	// 비우면 재장전이 즉시 끝난다
	UPROPERTY(EditDefaultsOnly, Category = "Reload")
	TObjectPtr<class UAnimMontage> ReloadMontage;

	// ---------- Blood ----------
	// 총에 맞았을 때 피 이펙트. Niagara와 Cascade 중 지정한 쪽을 재생한다 (둘 다 지정하면 둘 다)
	// 맞은 면의 법선 방향(X축)으로 회전해 스폰하므로, 에셋이 +X로 뿜어지면 맞은 쪽(쏜 사람 방향)으로 튄다
	UPROPERTY(EditDefaultsOnly, Category = "Blood")
	TObjectPtr<class UNiagaraSystem> BloodNiagara;

	UPROPERTY(EditDefaultsOnly, Category = "Blood")
	TObjectPtr<class UParticleSystem> BloodParticle;

	UPROPERTY(EditDefaultsOnly, Category = "Blood")
	FVector BloodScale = FVector(1.f);

	UPROPERTY(EditDefaultsOnly, Category = "Fire")
	float Damage = 20.f;

	UPROPERTY(EditDefaultsOnly, Category = "Fire")
	float FireRange = 10000.f;

	// 분당 발사 수
	UPROPERTY(EditDefaultsOnly, Category = "Fire")
	float RoundsPerMinute = 600.f;

	UPROPERTY(EditDefaultsOnly, Category = "Fire")
	bool bDrawDebugTrace = false;

	// ---------- Hit Zone ----------
	// Damage에 곱한다. 기본값은 머리 100(즉사), 몸통 20, 팔다리 10
	UPROPERTY(EditDefaultsOnly, Category = "Fire|HitZone")
	float HeadDamageMultiplier = 5.f;

	UPROPERTY(EditDefaultsOnly, Category = "Fire|HitZone")
	float LimbDamageMultiplier = 0.5f;

	// 부위별 판정 굵기(반지름, cm). 본 구간을 이 굵기의 캡슐로 본다
	UPROPERTY(EditDefaultsOnly, Category = "Fire|HitZone")
	float HeadRadius = 13.f;

	// head 본(목 위쪽 끝)에서 정수리 방향으로 머리 판정을 늘리는 길이
	UPROPERTY(EditDefaultsOnly, Category = "Fire|HitZone")
	float HeadLength = 15.f;

	UPROPERTY(EditDefaultsOnly, Category = "Fire|HitZone")
	float TorsoRadius = 20.f;

	UPROPERTY(EditDefaultsOnly, Category = "Fire|HitZone")
	float LimbRadius = 9.f;

	UPROPERTY(EditDefaultsOnly, Category = "Health")
	float MaxHealth = 100.f;

	UPROPERTY(ReplicatedUsing = OnRep_Health, BlueprintReadOnly, Category = "Health")
	float Health = 100.f;

	UPROPERTY(ReplicatedUsing = OnRep_IsDead, BlueprintReadOnly, Category = "Health")
	bool bIsDead = false;

	// 소유 클라는 CMC로 직접 알고 있으므로 SkipOwner
	UPROPERTY(Replicated)
	bool bReplicatedSprinting = false;

	// 소유 클라는 입력으로 직접 바꾸므로 SkipOwner. 다른 클라는 애니메이션용으로 받는다
	UPROPERTY(Replicated)
	bool bIsAiming = false;

	// 소유 클라는 직접 예측하므로 SkipOwner. 다른 클라는 OnRep에서 몽타주를 재생한다
	UPROPERTY(ReplicatedUsing = OnRep_IsReloading)
	bool bIsReloading = false;

private:
	float GetFireInterval() const { return 60.f / FMath::Max(RoundsPerMinute, 1.f); }

	FTimerHandle FireTimerHandle;
	FTimerHandle ReloadAmmoTimerHandle;
	FTimerHandle ReloadEndTimerHandle;
	double LastLocalFireTime = -1.0;
	double LastHeadshotTime = -1.0;

	// 서버 연사 검증. 평균 연사 속도만 제한하고 서버 틱 때문에 몰려 도착한 RPC는 허용한다
	double NextServerFireTime = 0.0;

	float DefaultArmLength = 300.f;
	float DefaultFieldOfView = 90.f;
};