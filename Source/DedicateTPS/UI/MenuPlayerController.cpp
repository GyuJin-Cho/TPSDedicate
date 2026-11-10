#include "MenuPlayerController.h"
#include "Blueprint/UserWidget.h"
#include "DedicateTPS/Utill/Utill.h"
#include "UObject/ConstructorHelpers.h"

AMenuPlayerController::AMenuPlayerController()
{
	// 기본 접속 위젯. BP에서 다른 위젯으로 덮어쓸 수 있음
	static ConstructorHelpers::FClassFinder<UUserWidget> ConnectWidgetBP(TEXT("/Game/Menu/BP/Widget/ConnectWidget_BP"));
	if (ConnectWidgetBP.Succeeded())
	{
		ConnectWidgetClass = ConnectWidgetBP.Class;
	}
}

void AMenuPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (!IsLocalController()) return;

	if (!ConnectWidgetClass)
	{
		FString MSG = TEXT("[Menu] ConnectWidgetClass is not set");
		WARNING_MSG(TEXT("%s"), *MSG);
		return;
	}

	ConnectWidget = CreateWidget<UUserWidget>(this, ConnectWidgetClass);
	if (!ConnectWidget) return;

	ConnectWidget->AddToViewport();

	FInputModeUIOnly InputMode;
	InputMode.SetWidgetToFocus(ConnectWidget->TakeWidget());
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	SetInputMode(InputMode);
	SetShowMouseCursor(true);
}