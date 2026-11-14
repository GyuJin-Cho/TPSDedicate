#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "TPSCharacter.generated.h"

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

	// 로컬/서버는 CMC 상태, 시뮬레이티드 프록시는 복제값을 반환
	UFUNCTION(BlueprintPure, Category = "Sprint")
	bool IsSprinting() const;

	// 서버 전용. UTPSCharacterMovementComponent가 매 이동 후 호출
	void SetReplicatedSprinting(bool bNewSprinting);

	UFUNCTION(BlueprintPure, Category = "Weapon")
	class AM4Weapon* GetWeapon() const { return Weapon; }

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void Destroyed() override;

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

	UFUNCTION(Server, Reliable)
	void ServerReload();

	// ---------- Fire ----------
	void FireOnce();
	bool HasAmmo() const;

	// 로컬 전용. 조준점을 랜덤하게 틀고 카메라 셰이크 재생
	void ApplyRecoil();
	void PlayFireMontage();

	UFUNCTION(Server, Reliable)
	void ServerFire(FVector_NetQuantize TraceStart, FVector_NetQuantizeNormal TraceDir);

	// 사격자 본인은 이미 로컬에서 재생했으므로 제외
	UFUNCTION(NetMulticast, Unreliable)
	void MulticastFireEffects(FVector_NetQuantize ImpactPoint, bool bHit);

	// BP에서 총구 화염/사운드/탄흔 처리
	UFUNCTION(BlueprintImplementableEvent, Category = "Fire")
	void OnFireCosmetic(const FVector& MuzzleLocation, const FVector& ImpactPoint, bool bHit);

	FVector GetMuzzleLocation() const;

	// ---------- Health / Death ----------
	UFUNCTION()
	void OnRep_Health(float OldHealth);

	UFUNCTION()
	void OnRep_IsDead();

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

	UPROPERTY(EditDefaultsOnly, Category = "Fire")
	float Damage = 20.f;

	UPROPERTY(EditDefaultsOnly, Category = "Fire")
	float FireRange = 10000.f;

	// 분당 발사 수
	UPROPERTY(EditDefaultsOnly, Category = "Fire")
	float RoundsPerMinute = 600.f;

	UPROPERTY(EditDefaultsOnly, Category = "Fire")
	bool bDrawDebugTrace = false;

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

private:
	float GetFireInterval() const { return 60.f / FMath::Max(RoundsPerMinute, 1.f); }

	FTimerHandle FireTimerHandle;
	double LastLocalFireTime = -1.0;

	// 서버 연사 검증. 평균 연사 속도만 제한하고 서버 틱 때문에 몰려 도착한 RPC는 허용한다
	double NextServerFireTime = 0.0;

	float DefaultArmLength = 300.f;
	float DefaultFieldOfView = 90.f;
};