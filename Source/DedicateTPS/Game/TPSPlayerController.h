#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "TPSPlayerController.generated.h"

class UUserWidget;

// 게임 맵용 컨트롤러. 로컬 플레이어에게 매칭 상태 위젯을 띄움
UCLASS()
class DEDICATETPS_API ATPSPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	ATPSPlayerController();

protected:
	virtual void BeginPlay() override;

	// 기본은 C++로만 구성된 UMatchStatusWidget. BP 위젯으로 교체 가능
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UUserWidget> MatchStatusWidgetClass;

	UPROPERTY()
	TObjectPtr<UUserWidget> MatchStatusWidget;
};