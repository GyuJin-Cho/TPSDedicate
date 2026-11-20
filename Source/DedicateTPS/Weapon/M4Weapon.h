#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DedicateTPS/Table/TableRows.h"
#include "M4Weapon.generated.h"

// UE5_CPP_Project에서 이주한 무기 (CoreRedirects로 연결)
// - 서버가 스폰해 캐릭터 소켓에 붙이고, 부착 상태는 엔진이 복제한다
// - 판정(히트스캔)과 탄약 차감은 ATPSCharacter::ServerFire가 처리한다
// - 이 클래스는 연출과 데이터만 담당한다
// BP에 저장된 값을 살리기 위해 컴포넌트 이름("M4", "Muzzle", "ShellEject")과
// 프로퍼티 이름은 전 프로젝트와 같게 유지한다
// 예외: 탄약 컴포넌트는 "Armo"에서 "Ammo"로 바꿨다 (BP에 덮어쓴 값이 없어 잃는 값이 없음)
UCLASS()
class DEDICATETPS_API AM4Weapon : public AActor
{
	GENERATED_BODY()

public:
	AM4Weapon();

	// WeaponId로 테이블을 읽어 Stat과 탄창 크기를 채운다 (서버와 클라 각자 읽는다)
	virtual void PostInitializeComponents() override;

	// 데미지, 반동, 탄창 크기. 테이블을 못 읽었으면 전부 0
	const FWeaponStatTableRow& GetStat() const { return Stat; }

	// 총구 화염, 탄피, 총 애니메이션. 데디케이티드 서버에서는 아무것도 하지 않는다
	void PlayFireEffects();

	FVector GetMuzzleLocation() const;

	// 서버 전용. 탄약을 가득 채운다
	void RefillAmmo();

	FORCEINLINE class USkeletalMeshComponent* GetMesh() const { return Mesh; }
	FORCEINLINE class UAmmoComponent* GetAmmo() const { return Ammo; }

private:
	// WeaponTable의 키. 무기 BP마다 지정한다 (M4 = 1)
	// 복제하지 않는다. 클래스 기본값이라 서버와 클라가 같은 값을 갖는다
	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	int32 WeaponId = 1;

	// WeaponId → WeaponTable.StatId → WeaponStatTable 행의 사본
	UPROPERTY(VisibleInstanceOnly, Transient, Category = "Weapon")
	FWeaponStatTableRow Stat;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<class USkeletalMeshComponent> Mesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<class USphereComponent> Muzzle;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<class UParticleSystemComponent> ShellEject;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<class UAmmoComponent> Ammo;

	// 총구 화염
	UPROPERTY(EditAnywhere, Category = "Weapon")
	TObjectPtr<class UParticleSystem> Particle;

	// 전 프로젝트는 생성자에서 경로로 불러왔다. 지금은 BP에서 지정해야 한다 (Gun_Fire)
	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	TObjectPtr<class UAnimationAsset> FireAnimation;
};