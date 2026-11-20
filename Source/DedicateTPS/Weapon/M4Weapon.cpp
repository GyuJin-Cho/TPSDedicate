#include "M4Weapon.h"
#include "AmmoComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/SphereComponent.h"
#include "Particles/ParticleSystemComponent.h"
#include "Particles/ParticleSystem.h"
#include "Animation/AnimationAsset.h"
#include "Kismet/GameplayStatics.h"
#include "DedicateTPS/Table/TableSubsystem.h"
#include "DedicateTPS/Utill/CHelpers.h"
#include "DedicateTPS/Utill/Utill.h"

AM4Weapon::AM4Weapon()
{
	PrimaryActorTick.bCanEverTick = false;

	bReplicates = true;
	// 캐릭터에 붙어 다니므로 이동은 부착 복제로 충분하다
	SetReplicatingMovement(false);
	// 소유 캐릭터가 보이면 무기도 보이게
	bNetUseOwnerRelevancy = true;

	CHelpers::CreateComponent(this, &Mesh, "M4");
	CHelpers::CreateComponent(this, &Muzzle, "Muzzle", Mesh.Get());
	CHelpers::CreateComponent(this, &ShellEject, "ShellEject", Mesh.Get());
	CHelpers::CreateActorComponent(this, &Ammo, "Ammo");

	// 사격 트레이스를 막지 않도록
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Muzzle->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	Muzzle->SetRelativeLocation(FVector(0.0f, 55.555556f, 6.896552f));
	Muzzle->SetRelativeScale3D(FVector(0.1f, 0.1f, 0.1f));
}

void AM4Weapon::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	// 에디터 미리보기 등 게임 월드가 아니면 테이블 서브시스템이 없다
	const UWorld* World = GetWorld();
	if (!World || !World->IsGameWorld()) return;

	const UTableSubsystem* Tables = UTableSubsystem::Get(this);
	const FWeaponTableRow* WeaponRow = Tables ? Tables->Find<FWeaponTableRow>(WeaponId) : nullptr;
	const FWeaponStatTableRow* StatRow = WeaponRow ? Tables->Find<FWeaponStatTableRow>(WeaponRow->StatId) : nullptr;
	if (StatRow == nullptr)
	{
		ERROR_LOG(TEXT("AM4Weapon: no stat for WeaponId %d (%s). Damage/recoil are 0"), WeaponId, *GetName());
		return;
	}

	Stat = *StatRow;

	INFO_LOG(TEXT("[%s] AM4Weapon: WeaponId %d (%s) -> StatId %d, Damage %.1f, MaxAmmo %d"),
		HasAuthority() ? TEXT("Server") : TEXT("Client"), WeaponId, *WeaponRow->WeaponName, Stat.StatId, Stat.Damage, Stat.MaxAmmo);

	// 컴포넌트 BeginPlay(서버가 탄을 가득 채움)보다 먼저 정해야 한다
	Ammo->SetMaxAmmo(Stat.MaxAmmo);
}

void AM4Weapon::PlayFireEffects()
{
	if (GetNetMode() == NM_DedicatedServer) return;

	ShellEject->Activate(true);

	if (Particle)
	{
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), Particle, Muzzle->GetComponentLocation(), FRotator::ZeroRotator, FVector(1.0f), true);
	}

	if (FireAnimation)
	{
		Mesh->PlayAnimation(FireAnimation, false);
	}
}

FVector AM4Weapon::GetMuzzleLocation() const
{
	return Muzzle->GetComponentLocation();
}

void AM4Weapon::RefillAmmo()
{
	if (!HasAuthority()) return;

	Ammo->SetAmmo(Ammo->GetMaxAmmo());
}