#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameState.h"
#include "TPSGameState.generated.h"

// 매칭 상태를 클라에 전달 (MatchState, PlayerArray는 AGameState가 이미 복제)
UCLASS()
class DEDICATETPS_API ATPSGameState : public AGameState
{
	GENERATED_BODY()

public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	void SetRequiredPlayers(int32 InRequiredPlayers) { RequiredPlayers = InRequiredPlayers; }

	UFUNCTION(BlueprintPure, Category = "Match")
	int32 GetRequiredPlayers() const { return RequiredPlayers; }

	UFUNCTION(BlueprintPure, Category = "Match")
	int32 GetCurrentPlayers() const { return PlayerArray.Num(); }

protected:
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Match")
	int32 RequiredPlayers = 2;
};