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

	// 서버 전용. 경기가 끝날 때 GameMode가 EndMatch 직전에 설정 (nullptr = 승자 없음)
	void SetWinner(const class APlayerState* InWinner);

	// 승자가 정해졌는지 (킬 또는 상대 이탈)
	UFUNCTION(BlueprintPure, Category = "Match")
	bool HasWinner() const { return WinnerPlayerId != INDEX_NONE; }

	UFUNCTION(BlueprintPure, Category = "Match")
	bool IsWinner(const class APlayerState* PlayerState) const;

protected:
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Match")
	int32 RequiredPlayers = 2;

	// PlayerState 포인터 대신 PlayerId로 복제한다.
	// 승자가 경기 종료 후 나가면 그 PlayerState가 사라져 포인터는 null이 되고, 패자 화면의 승패 표시가 바뀌기 때문
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Match")
	int32 WinnerPlayerId = INDEX_NONE;
};