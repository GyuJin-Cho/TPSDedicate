#include "TPSPlayerController.h"
#include "Blueprint/UserWidget.h"
#include "DedicateTPS/UI/MatchStatusWidget.h"

ATPSPlayerController::ATPSPlayerController()
{
	MatchStatusWidgetClass = UMatchStatusWidget::StaticClass();
}

void ATPSPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (!IsLocalController() || !MatchStatusWidgetClass) return;

	MatchStatusWidget = CreateWidget<UUserWidget>(this, MatchStatusWidgetClass);
	if (MatchStatusWidget)
	{
		MatchStatusWidget->AddToViewport();
	}
}