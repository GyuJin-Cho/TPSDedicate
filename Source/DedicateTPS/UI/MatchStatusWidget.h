#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MatchStatusWidget.generated.h"

class UTextBlock;

// 화면 상단에 매칭 상태 표시 ("대기 중 (1/2)", "경기 종료" 등)
// BP 없이 C++만으로 레이아웃을 만들고, BP 자식에서 StatusText를 직접 배치해도 됨
UCLASS()
class DEDICATETPS_API UMatchStatusWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	FString BuildStatusMessage() const;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> StatusText;
};