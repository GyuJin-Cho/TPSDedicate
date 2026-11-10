#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ConnectWidget.generated.h"

class UButton;
class UTextBlock;

// 매칭 버튼. 레이아웃은 이 클래스를 부모로 한 위젯 BP에서 구성
UCLASS()
class DEDICATETPS_API UConnectWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UFUNCTION()
	void OnConnectClicked();

	void HandleConnectFailed(const FString& Error);
	void HandleConnectTimeout();

	void SetStatus(const FString& Message);

	// 위젯 BP에서 같은 이름의 위젯을 만들면 자동 연결
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> ConnectButton;

	// 상태/에러 표시용 (없어도 됨)
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> StatusText;

	UPROPERTY(EditAnywhere, Category = "Connect")
	FText ButtonLabel = FText::FromString(TEXT("매칭 시작"));

	// 매칭 서버 주소
	UPROPERTY(EditAnywhere, Category = "Connect")
	FString DefaultAddress = TEXT("127.0.0.1:7777");

	// 서버 응답이 없을 때 실패로 판단하기까지의 시간(초). 메뉴에서의 첫 접속에만 적용
	UPROPERTY(EditAnywhere, Category = "Connect", meta = (ClampMin = "1"))
	float ConnectTimeoutSeconds = 10.f;

private:
	FTimerHandle ConnectTimeoutHandle;
};
