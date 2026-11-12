#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "MenuPlayerController.generated.h"

// 메뉴 레벨 전용. 접속 위젯을 띄우고 UI 입력 모드로 전환
UCLASS()
class DEDICATETPS_API AMenuPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AMenuPlayerController();

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<class UUserWidget> ConnectWidgetClass;

	UPROPERTY()
	TObjectPtr<class UUserWidget> ConnectWidget;
};