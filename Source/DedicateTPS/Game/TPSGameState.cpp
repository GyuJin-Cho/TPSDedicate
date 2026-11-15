#include "TPSGameState.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/PlayerState.h"

void ATPSGameState::SetWinner(const APlayerState* InWinner)
{
	WinnerPlayerId = InWinner ? InWinner->GetPlayerId() : INDEX_NONE;
}

bool ATPSGameState::IsWinner(const APlayerState* PlayerState) const
{
	return HasWinner() && PlayerState && PlayerState->GetPlayerId() == WinnerPlayerId;
}

void ATPSGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ATPSGameState, RequiredPlayers);
	DOREPLIFETIME(ATPSGameState, WinnerPlayerId);
}