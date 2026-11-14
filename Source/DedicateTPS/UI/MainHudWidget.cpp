#include "MainHudWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/VerticalBox.h"
#include "Components/SizeBox.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"
#include "DedicateTPS/Player/TPSCharacter.h"
#include "DedicateTPS/Weapon/M4Weapon.h"
#include "DedicateTPS/Weapon/AmmoComponent.h"

void UMainHudWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	// BP에서 레이아웃을 만들지 않았으면 C++로 생성
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		BuildDefaultLayout();
	}

	SetVisibility(ESlateVisibility::HitTestInvisible);
}

void UMainHudWidget::BuildDefaultLayout()
{
	UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("Root"));
	WidgetTree->RootWidget = Root;

	// 좌하단: 체력 숫자 + 바
	UVerticalBox* HealthBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("HealthBox"));
	UCanvasPanelSlot* HealthSlot = Root->AddChildToCanvas(HealthBox);
	HealthSlot->SetAnchors(FAnchors(0.f, 1.f));
	HealthSlot->SetAlignment(FVector2D(0.f, 1.f));
	HealthSlot->SetPosition(FVector2D(40.f, -40.f));
	HealthSlot->SetAutoSize(true);

	HealthText = MakeText(TEXT("HealthText"), 20);
	HealthBox->AddChildToVerticalBox(HealthText);

	USizeBox* HealthBarSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("HealthBarSize"));
	HealthBarSize->SetWidthOverride(240.f);
	HealthBarSize->SetHeightOverride(12.f);
	HealthBox->AddChildToVerticalBox(HealthBarSize);

	HealthBar = WidgetTree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass(), TEXT("HealthBar"));
	HealthBar->SetFillColorAndOpacity(FLinearColor(0.2f, 0.85f, 0.3f));
	HealthBarSize->AddChild(HealthBar);

	// 우하단: 탄약
	AmmoText = MakeText(TEXT("AmmoText"), 32);
	AmmoText->SetJustification(ETextJustify::Right);
	UCanvasPanelSlot* AmmoSlot = Root->AddChildToCanvas(AmmoText);
	AmmoSlot->SetAnchors(FAnchors(1.f, 1.f));
	AmmoSlot->SetAlignment(FVector2D(1.f, 1.f));
	AmmoSlot->SetPosition(FVector2D(-40.f, -40.f));
	AmmoSlot->SetAutoSize(true);
}

UTextBlock* UMainHudWidget::MakeText(const TCHAR* Name, int32 FontSize)
{
	UTextBlock* Text = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), Name);
	Text->SetShadowOffset(FVector2D(1.f, 1.f));
	Text->SetShadowColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, 0.8f));

	FSlateFontInfo Font = Text->GetFont();
	Font.Size = FontSize;
	Text->SetFont(Font);
	return Text;
}

void UMainHudWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	const ATPSCharacter* Character = GetOwningPlayerPawn<ATPSCharacter>();
	UpdateHealth(Character);
	UpdateAmmo(Character);
}

void UMainHudWidget::UpdateHealth(const ATPSCharacter* Character)
{
	// 사망 후에는 컨트롤러가 떨어져 폰이 없어지므로 리스폰 전까지 숨긴다
	const bool bVisible = Character && !Character->IsDead();
	const ESlateVisibility HealthVisibility = bVisible ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed;
	if (HealthText) HealthText->SetVisibility(HealthVisibility);
	if (HealthBar) HealthBar->SetVisibility(HealthVisibility);
	if (!bVisible) return;

	const int32 Health = FMath::CeilToInt(Character->GetHealth());
	const int32 MaxHealth = FMath::CeilToInt(Character->GetMaxHealth());
	if (Health == LastHealth && MaxHealth == LastMaxHealth) return;
	LastHealth = Health;
	LastMaxHealth = MaxHealth;

	if (HealthText)
	{
		HealthText->SetText(FText::FromString(FString::Printf(TEXT("HP %d / %d"), Health, MaxHealth)));
	}
	if (HealthBar)
	{
		HealthBar->SetPercent(MaxHealth > 0 ? static_cast<float>(Health) / MaxHealth : 0.f);
	}
}

void UMainHudWidget::UpdateAmmo(const ATPSCharacter* Character)
{
	// 무기는 별도 액터라 캐릭터보다 늦게 복제될 수 있다
	const AM4Weapon* Weapon = Character && !Character->IsDead() ? Character->GetWeapon() : nullptr;
	const UAmmoComponent* AmmoComponent = IsValid(Weapon) ? Weapon->GetAmmo() : nullptr;

	if (AmmoText)
	{
		AmmoText->SetVisibility(AmmoComponent ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
	if (!AmmoComponent) return;

	const int32 Ammo = AmmoComponent->GetAmmo();
	const int32 MaxAmmo = AmmoComponent->GetMaxAmmo();
	if (Ammo == LastAmmo && MaxAmmo == LastMaxAmmo) return;
	LastAmmo = Ammo;
	LastMaxAmmo = MaxAmmo;

	if (AmmoText)
	{
		AmmoText->SetText(FText::FromString(FString::Printf(TEXT("%d / %d"), Ammo, MaxAmmo)));
	}
}

bool UMainHudWidget::ShouldDrawCrosshair() const
{
	const ATPSCharacter* Character = GetOwningPlayerPawn<ATPSCharacter>();
	if (!Character || Character->IsDead()) return false;

	return !bCrosshairOnlyWhenAiming || Character->IsAiming();
}

int32 UMainHudWidget::NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
	FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	const int32 MaxLayer = Super::NativePaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);
	if (!ShouldDrawCrosshair()) return MaxLayer;

	const int32 Layer = MaxLayer + 1;
	const FVector2f Center = FVector2f(AllottedGeometry.GetLocalSize()) * 0.5f;

	auto DrawLine = [&](const FVector2f& Direction)
	{
		TArray<FVector2f> Points;
		Points.Add(Center + Direction * CrosshairGap);
		Points.Add(Center + Direction * (CrosshairGap + CrosshairLength));
		FSlateDrawElement::MakeLines(OutDrawElements, Layer, AllottedGeometry.ToPaintGeometry(), MoveTemp(Points), ESlateDrawEffect::None, CrosshairColor, true, CrosshairThickness);
	};

	DrawLine(FVector2f(0.f, -1.f));
	DrawLine(FVector2f(0.f, 1.f));
	DrawLine(FVector2f(-1.f, 0.f));
	DrawLine(FVector2f(1.f, 0.f));

	if (CrosshairDotSize > 0.f)
	{
		const FVector2f DotExtent(CrosshairDotSize, CrosshairDotSize);
		FSlateDrawElement::MakeBox(OutDrawElements, Layer,
			AllottedGeometry.ToPaintGeometry(DotExtent, FSlateLayoutTransform(Center - DotExtent * 0.5f)),
			FCoreStyle::Get().GetBrush("WhiteBrush"), ESlateDrawEffect::None, CrosshairColor);
	}

	return Layer;
}