#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "TPSPlayerController.generated.h"

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
	TSubclassOf<class UUserWidget> MatchStatusWidgetClass;

	UPROPERTY()
	TObjectPtr<class UUserWidget> MatchStatusWidget;

	// 체력/탄약/크로스헤어. 생성자에서 MainHudWidget_BP(부모 UMainHudWidget)를 경로로 불러온다
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<class UUserWidget> MainHudWidgetClass;

	UPROPERTY()
	TObjectPtr<class UUserWidget> MainHudWidget;
};