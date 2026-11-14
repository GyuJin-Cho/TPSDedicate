#include "TPSPlayerController.h"
#include "Blueprint/UserWidget.h"
#include "DedicateTPS/UI/MatchStatusWidget.h"
#include "DedicateTPS/UI/MainHudWidget.h"
#include "DedicateTPS/Utill/CHelpers.h"

ATPSPlayerController::ATPSPlayerController()
{
	MatchStatusWidgetClass = UMatchStatusWidget::StaticClass();

	// 컨트롤러는 BP 없이 C++ 클래스를 그대로 쓰므로 HUD BP는 여기서 경로로 지정한다
	CHelpers::GetClass<UUserWidget>(&MainHudWidgetClass, "/Game/Player/Widget/MainHudWidget_BP.MainHudWidget_BP_C");
}

void ATPSPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (!IsLocalController()) return;

	if (MatchStatusWidgetClass)
	{
		MatchStatusWidget = CreateWidget<UUserWidget>(this, MatchStatusWidgetClass);
		if (MatchStatusWidget)
		{
			MatchStatusWidget->AddToViewport();
		}
	}

	if (MainHudWidgetClass)
	{
		MainHudWidget = CreateWidget<UUserWidget>(this, MainHudWidgetClass);
		if (MainHudWidget)
		{
			MainHudWidget->AddToViewport();
		}
	}
}