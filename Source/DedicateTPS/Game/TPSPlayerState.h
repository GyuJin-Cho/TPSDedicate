#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "TPSPlayerState.generated.h"

UCLASS()
class DEDICATETPS_API ATPSPlayerState : public APlayerState
{
	GENERATED_BODY()

public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// 서버에서만 호출
	void AddKill();
	void AddDeath();

	UFUNCTION(BlueprintPure, Category = "Stats")
	int32 GetKills() const { return Kills; }

	UFUNCTION(BlueprintPure, Category = "Stats")
	int32 GetDeaths() const { return Deaths; }

	// 입장 순서로 배정된 스폰 번호. 서버에서만 사용
	int32 GetStartSlot() const { return StartSlot; }
	void SetStartSlot(int32 InStartSlot) { StartSlot = InStartSlot; }

protected:
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Stats")
	int32 Kills = 0;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Stats")
	int32 Deaths = 0;

private:
	int32 StartSlot = INDEX_NONE;
};