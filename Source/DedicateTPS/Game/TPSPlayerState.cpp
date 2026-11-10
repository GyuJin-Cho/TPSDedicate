#include "TPSPlayerState.h"
#include "Net/UnrealNetwork.h"

void ATPSPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ATPSPlayerState, Kills);
	DOREPLIFETIME(ATPSPlayerState, Deaths);
}

void ATPSPlayerState::AddKill()
{
	if (!HasAuthority()) return;

	++Kills;
	SetScore(GetScore() + 1.f);
}

void ATPSPlayerState::AddDeath()
{
	if (!HasAuthority()) return;

	++Deaths;
}