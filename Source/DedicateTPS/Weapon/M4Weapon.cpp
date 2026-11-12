#include "M4Weapon.h"
#include "ArmoComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/SphereComponent.h"
#include "Particles/ParticleSystemComponent.h"
#include "Particles/ParticleSystem.h"
#include "Animation/AnimationAsset.h"
#include "Kismet/GameplayStatics.h"
#include "DedicateTPS/Utill/CHelpers.h"

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
	CHelpers::CreateActorComponent(this, &Armo, "Armo");

	// 사격 트레이스를 막지 않도록
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Muzzle->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	Muzzle->SetRelativeLocation(FVector(0.0f, 55.555556f, 6.896552f));
	Muzzle->SetRelativeScale3D(FVector(0.1f, 0.1f, 0.1f));
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

void AM4Weapon::SetArmo()
{
	if (!HasAuthority()) return;

	Armo->SetArmo(Armo->GetMaxArmo());
}