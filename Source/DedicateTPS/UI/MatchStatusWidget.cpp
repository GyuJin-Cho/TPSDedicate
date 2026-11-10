#include "MatchStatusWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"
#include "GameFramework/GameMode.h"
#include "DedicateTPS/Game/TPSGameState.h"

void UMatchStatusWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	// BP에서 레이아웃을 만들지 않았으면 C++로 생성
	if (StatusText || !WidgetTree || WidgetTree->RootWidget) return;

	UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("Root"));
	WidgetTree->RootWidget = Root;

	StatusText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("StatusText"));
	StatusText->SetJustification(ETextJustify::Center);
	StatusText->SetShadowOffset(FVector2D(1.f, 1.f));
	StatusText->SetShadowColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, 0.8f));

	FSlateFontInfo Font = StatusText->GetFont();
	Font.Size = 28;
	StatusText->SetFont(Font);

	// 화면 상단 중앙
	UCanvasPanelSlot* CanvasSlot = Root->AddChildToCanvas(StatusText);
	CanvasSlot->SetAnchors(FAnchors(0.5f, 0.15f));
	CanvasSlot->SetAlignment(FVector2D(0.5f, 0.5f));
	CanvasSlot->SetAutoSize(true);
}

void UMatchStatusWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (!StatusText) return;

	const FString Message = BuildStatusMessage();
	StatusText->SetText(FText::FromString(Message));
	StatusText->SetVisibility(Message.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
}

FString UMatchStatusWidget::BuildStatusMessage() const
{
	const UWorld* World = GetWorld();
	const ATPSGameState* GS = World ? World->GetGameState<ATPSGameState>() : nullptr;
	if (!GS) return TEXT("서버 접속 중...");

	const FName State = GS->GetMatchState();

	if (State == MatchState::EnteringMap || State == MatchState::WaitingToStart)
	{
		return FString::Printf(TEXT("상대를 기다리는 중... (%d/%d)"), GS->GetCurrentPlayers(), GS->GetRequiredPlayers());
	}

	if (State == MatchState::WaitingPostMatch)
	{
		return TEXT("경기 종료 - 잠시 후 다시 매칭합니다");
	}

	// InProgress 등은 표시 안 함
	return FString();
}