#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "TPSGameInstance.generated.h"

DECLARE_MULTICAST_DELEGATE_OneParam(FOnConnectFailed, const FString& /*Error*/);

// 접속 실패 사유를 맵 이동(메뉴로 복귀) 이후에도 보관
UCLASS()
class DEDICATETPS_API UTPSGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	virtual void Init() override;
	virtual void Shutdown() override;

	// 마지막 접속 실패 메시지를 꺼내고 비움 (없으면 빈 문자열)
	FString ConsumeLastConnectError();

	// PIE는 접속 실패 시 메뉴 맵을 다시 로드하지 않으므로 메뉴 위젯에 바로 알림
	FOnConnectFailed OnConnectFailed;

private:
	void HandleNetworkFailure(UWorld* World, UNetDriver* NetDriver, ENetworkFailure::Type FailureType, const FString& Error);
	void HandleTravelFailure(UWorld* World, ETravelFailure::Type FailureType, const FString& Error);

	FString LastConnectError;
};