#include "ArmoComponent.h"
#include "Net/UnrealNetwork.h"

UArmoComponent::UArmoComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UArmoComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(UArmoComponent, Armo, COND_OwnerOnly);
}

void UArmoComponent::BeginPlay()
{
	Super::BeginPlay();

	if (GetOwner() && GetOwner()->HasAuthority())
	{
		SetArmo(MaxArmo);
	}
}

void UArmoComponent::SetArmo(int32 InArmo)
{
	Armo = FMath::Clamp(InArmo, 0, MaxArmo);
	OnRep_Armo();
}

bool UArmoComponent::ConsumeArmo()
{
	if (Armo <= 0) return false;

	SetArmo(Armo - 1);
	return true;
}

void UArmoComponent::OnRep_Armo()
{
	OnArmoChanged.Broadcast(Armo, MaxArmo);
}