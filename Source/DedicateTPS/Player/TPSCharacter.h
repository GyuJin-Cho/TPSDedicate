#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "TPSCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UInputMappingContext;
class UInputAction;

// 서버 권한(Server Authoritative) TPS 캐릭터
// - 이동: CharacterMovementComponent가 클라 예측 + 서버 보정 처리
// - 사격: 클라 -> ServerFire RPC -> 서버 라인트레이스/데미지 -> Multicast 이펙트
// - 체력/사망: 서버에서만 변경, OnRep으로 클라 반영
UCLASS()
class DEDICATETPS_API ATPSCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	ATPSCharacter();

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

protected:
	virtual void BeginPlay() override;
	virtual void NotifyControllerChanged() override;

	// ---------- Input ----------
	void OnMoveForward(float InAxis);
	void OnMoveRight(float InAxis);
	void OnHorizontalLook(float InAxis);
	void OnVerticalLook(float InAxis);
	void StartFire();
	void StopFire();

	// ---------- Fire ----------
	void FireOnce();

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
	TObjectPtr<USpringArmComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	TObjectPtr<UCameraComponent> FollowCamera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
	TObjectPtr<USkeletalMeshComponent> WeaponMesh;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	FName WeaponAttachSocket = TEXT("hand_rSocket");

	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	FName MuzzleSocket = TEXT("Muzzle");

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputMappingContext> DefaultMappingContext;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> FireAction;

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

private:
	float GetFireInterval() const { return 60.f / FMath::Max(RoundsPerMinute, 1.f); }

	FTimerHandle FireTimerHandle;
	double LastLocalFireTime = -1.0;
	double LastServerFireTime = -1.0;
};