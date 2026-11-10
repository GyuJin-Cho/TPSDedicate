#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "TPSGameMode.generated.h"

// 서버에만 존재하는 게임 규칙 (매칭 대기, 접속 관리, 킬 처리, 리스폰)
// 흐름 : WaitingToStart (인원 대기) -> InProgress (경기) -> 인원 부족 시 맵 재시작 -> WaitingToStart
UCLASS()
class DEDICATETPS_API ATPSGameMode : public AGameMode
{
	GENERATED_BODY()

public:
	ATPSGameMode();

	virtual void InitGameState() override;
	virtual void PreLogin(const FString& Options, const FString& Address, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage) override;
	virtual APlayerController* Login(UPlayer* NewPlayer, ENetRole InRemoteRole, const FString& Portal, const FString& Options, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage) override;
	virtual void PostLogin(APlayerController* NewPlayer) override;
	virtual void Logout(AController* Exiting) override;
	virtual AActor* ChoosePlayerStart_Implementation(AController* Player) override;
	virtual bool ShouldSpawnAtStartSpot(AController* Player) override;

	// 캐릭터가 죽었을 때 서버에서 호출
	void OnPlayerKilled(AController* Victim, AController* Killer);

protected:
	virtual bool ReadyToStartMatch_Implementation() override;
	virtual void HandleMatchHasStarted() override;
	virtual void HandleMatchHasEnded() override;

	void RespawnPlayer(TWeakObjectPtr<AController> Controller);

	void AssignStartSlot(APlayerController* NewPlayer);

	// 지금 입장할 수 없는 사유 (입장 가능하면 빈 문자열)
	FString GetJoinRejectReason() const;

	// 이 인원이 모이면 경기 시작
	UPROPERTY(EditDefaultsOnly, Category = "Rules")
	int32 RequiredPlayers = 2;

	UPROPERTY(EditDefaultsOnly, Category = "Rules")
	float RespawnDelay = 3.f;

	// 경기 종료 후 맵을 다시 불러와 다음 매칭을 받기까지의 시간
	UPROPERTY(EditDefaultsOnly, Category = "Rules")
	float RestartDelay = 5.f;

	// 입장 순서별로 사용할 PlayerStart의 Player Start Tag
	UPROPERTY(EditDefaultsOnly, Category = "Rules")
	TArray<FName> PlayerStartTags = { FName(TEXT("PlayerStart")), FName(TEXT("PlayerStart2")) };

private:
	FTimerHandle RestartTimerHandle;
};
