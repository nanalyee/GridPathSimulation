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
    virtual void NativeConstruct() override;  // BeginPlay ��� NativeConstruct ���

    // UI ������Ʈ �Լ�
    UFUNCTION(BlueprintCallable, Category = "UI")
    void UpdateGridSizeText(int32 Size);
    void AddPathText(FString Path);

private:
    // �ؽ�Ʈ ����� �����ϱ� ���� ����
    UPROPERTY(meta = (BindWidget))
    class UTextBlock* WidgetText;
};
