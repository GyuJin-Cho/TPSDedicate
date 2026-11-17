#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MainHudWidget.generated.h"

// 게임 중 HUD: 체력(좌하단), 탄약(우하단), 크로스헤어(정중앙)
// - 크로스헤어는 NativePaint로 직접 그린다. 사격 판정이 카메라 중앙 기준이라 중앙점이 곧 탄착점이다
// - 체력/탄약은 매 틱 소유 폰에서 읽는다. 리스폰으로 폰이 바뀌어도 다시 바인딩할 필요가 없다
// - BP 없이 C++로 레이아웃을 만들고, BP 자식에서 같은 이름의 위젯을 직접 배치해도 된다
UCLASS()
class DEDICATETPS_API UMainHudWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual int32 NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;

	// ---------- 체력 / 탄약 ----------
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<class UProgressBar> HealthBar;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<class UTextBlock> HealthText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<class UTextBlock> AmmoText;

	// ---------- 크로스헤어 ----------
	// 조준(우클릭) 중에만 표시
	UPROPERTY(EditAnywhere, Category = "Crosshair")
	bool bCrosshairOnlyWhenAiming = true;

	UPROPERTY(EditAnywhere, Category = "Crosshair")
	FLinearColor CrosshairColor = FLinearColor(1.f, 1.f, 1.f, 0.9f);

	// 중앙에서 선이 시작하는 거리 (px)
	UPROPERTY(EditAnywhere, Category = "Crosshair")
	float CrosshairGap = 5.f;

	UPROPERTY(EditAnywhere, Category = "Crosshair")
	float CrosshairLength = 8.f;

	UPROPERTY(EditAnywhere, Category = "Crosshair")
	float CrosshairThickness = 2.f;

	// 중앙 점 크기 (px). 0이면 그리지 않음
	UPROPERTY(EditAnywhere, Category = "Crosshair")
	float CrosshairDotSize = 2.f;

	// ---------- 헤드샷 ----------
	// 서버가 ClientNotifyHeadshot을 보내면 크로스헤어 위에 잠깐 표시한다 (조준 여부와 무관)
	UPROPERTY(EditAnywhere, Category = "Headshot")
	FText HeadshotText = FText::FromString(TEXT("HEADSHOT"));

	UPROPERTY(EditAnywhere, Category = "Headshot")
	FLinearColor HeadshotColor = FLinearColor(1.f, 0.2f, 0.15f, 1.f);

	UPROPERTY(EditAnywhere, Category = "Headshot")
	int32 HeadshotFontSize = 28;

	// 화면 중앙에서 글자 아래쪽까지의 거리 (px)
	UPROPERTY(EditAnywhere, Category = "Headshot")
	float HeadshotOffsetY = 40.f;

	// 전체 표시 시간과, 그중 끝에서 서서히 사라지는 시간 (초)
	UPROPERTY(EditAnywhere, Category = "Headshot")
	float HeadshotDisplayTime = 1.5f;

	UPROPERTY(EditAnywhere, Category = "Headshot")
	float HeadshotFadeTime = 0.5f;

private:
	void BuildDefaultLayout();
	class UTextBlock* MakeText(const TCHAR* Name, int32 FontSize);

	void UpdateHealth(const class ATPSCharacter* Character);
	void UpdateAmmo(const class ATPSCharacter* Character);
	bool ShouldDrawCrosshair() const;

	// 표시 중이면 그리고 true
	bool DrawHeadshotText(const FGeometry& AllottedGeometry, FSlateWindowElementList& OutDrawElements, int32 Layer) const;

	// 값이 바뀔 때만 텍스트를 갱신하기 위한 캐시
	int32 LastHealth = -1;
	int32 LastMaxHealth = -1;
	int32 LastAmmo = -1;
	int32 LastMaxAmmo = -1;
};