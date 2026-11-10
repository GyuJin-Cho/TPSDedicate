#include "ConnectWidget.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Blueprint/WidgetTree.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "DedicateTPS/Game/TPSGameInstance.h"
#include "DedicateTPS/Utill/Utill.h"

void UConnectWidget::NativeConstruct()
{
	Super::NativeConstruct();

	ConnectButton->OnClicked.AddUniqueDynamic(this, &UConnectWidget::OnConnectClicked);

	// 버튼 안의 TextBlock에 글자 설정, 없으면 새로 만들어 넣음
	UTextBlock* Label = Cast<UTextBlock>(ConnectButton->GetContent());
	if (!Label && !ConnectButton->GetContent() && WidgetTree)
	{
		Label = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ConnectButtonLabel"));
		Label->SetJustification(ETextJustify::Center);
		ConnectButton->SetContent(Label);
	}
	if (Label)
	{
		Label->SetText(ButtonLabel);
	}

	// 접속 실패로 메뉴에 돌아온 경우 사유 표시
	FString Error;
	if (UTPSGameInstance* GI = GetGameInstance<UTPSGameInstance>())
	{
		Error = GI->ConsumeLastConnectError();
		GI->OnConnectFailed.AddUObject(this, &UConnectWidget::HandleConnectFailed);
	}
	SetStatus(Error.IsEmpty() ? FString() : FString::Printf(TEXT("서버 연결 실패 : %s"), *Error));
}

void UConnectWidget::NativeDestruct()
{
	if (UTPSGameInstance* GI = GetGameInstance<UTPSGameInstance>())
	{
		GI->OnConnectFailed.RemoveAll(this);
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ConnectTimeoutHandle);
	}

	Super::NativeDestruct();
}

void UConnectWidget::OnConnectClicked()
{
	const FString Address = DefaultAddress.TrimStartAndEnd();
	if (Address.IsEmpty())
	{
		ERROR_LOG(TEXT("[Client] DefaultAddress is empty"));
		return;
	}

	APlayerController* PC = GetOwningPlayer();
	if (!PC) return;

	// 이전 시도의 실패 사유가 나중에 메뉴로 돌아왔을 때 표시되지 않도록 비움
	if (UTPSGameInstance* GI = GetGameInstance<UTPSGameInstance>())
	{
		GI->ConsumeLastConnectError();
	}

	INFO_LOG(TEXT("[Client] Connect to %s"), *Address);
	SetStatus(TEXT("매칭 중..."));
	ConnectButton->SetIsEnabled(false);

	// 입력 모드는 맵 이동 후에도 유지되므로 게임용으로 되돌리고 이동
	PC->SetInputMode(FInputModeGameOnly());
	PC->SetShowMouseCursor(false);

	// 서버가 현재 맵을 알려주면 클라가 그 맵을 로드하고 접속
	PC->ClientTravel(Address, TRAVEL_Absolute);

	// 서버가 응답하면 맵 이동으로 이 위젯과 함께 타이머도 사라짐
	GetWorld()->GetTimerManager().SetTimer(ConnectTimeoutHandle, this, &UConnectWidget::HandleConnectTimeout, ConnectTimeoutSeconds, false);
}

void UConnectWidget::HandleConnectTimeout()
{
	WARNING_LOG(TEXT("[Client] Connect timed out (%.0f sec)"), ConnectTimeoutSeconds);

	GEngine->CancelPending(GetWorld());
	HandleConnectFailed(TEXT("서버 응답 없음"));
}

void UConnectWidget::HandleConnectFailed(const FString& Error)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ConnectTimeoutHandle);
	}

	SetStatus(FString::Printf(TEXT("서버 연결 실패 : %s"), *Error));
	ConnectButton->SetIsEnabled(true);

	APlayerController* PC = GetOwningPlayer();
	if (!PC) return;

	FInputModeUIOnly InputMode;
	InputMode.SetWidgetToFocus(TakeWidget());
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	PC->SetInputMode(InputMode);
	PC->SetShowMouseCursor(true);
}

void UConnectWidget::SetStatus(const FString& Message)
{
	if (StatusText)
	{
		StatusText->SetText(FText::FromString(Message));
	}
}