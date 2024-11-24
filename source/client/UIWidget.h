// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UIWidget.generated.h"

/**
 * 
 */
UCLASS()
class GRIDPATH_API UUIWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:
    virtual void NativeConstruct() override;  // BeginPlay 대신 NativeConstruct 사용

    // UI 업데이트 함수
    UFUNCTION(BlueprintCallable, Category = "UI")
    void UpdateGridSizeText(int32 Size);
    void AddPathText(FString Path);

private:
    // 텍스트 블록을 참조하기 위한 변수
    UPROPERTY(meta = (BindWidget))
    class UTextBlock* WidgetText;
};
