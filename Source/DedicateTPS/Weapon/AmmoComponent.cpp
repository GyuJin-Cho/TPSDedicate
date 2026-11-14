#include "AmmoComponent.h"
#include "Net/UnrealNetwork.h"

UAmmoComponent::UAmmoComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UAmmoComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(UAmmoComponent, Ammo, COND_OwnerOnly);
}

void UAmmoComponent::BeginPlay()
{
	Super::BeginPlay();

	if (GetOwner() && GetOwner()->HasAuthority())
	{
		SetAmmo(MaxAmmo);
	}
}

void UAmmoComponent::SetAmmo(int32 InAmmo)
{
	Ammo = FMath::Clamp(InAmmo, 0, MaxAmmo);
	OnRep_Ammo();
}

bool UAmmoComponent::ConsumeAmmo()
{
	if (Ammo <= 0) return false;

	SetAmmo(Ammo - 1);
	return true;
}

void UAmmoComponent::OnRep_Ammo()
{
	OnAmmoChanged.Broadcast(Ammo, MaxAmmo);
}