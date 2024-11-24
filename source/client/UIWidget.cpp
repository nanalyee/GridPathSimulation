// Fill out your copyright notice in the Description page of Project Settings.

#include "UIWidget.h"
#include "Blueprint/UserWidget.h"
#include "Components/TextBlock.h"


void UUIWidget::NativeConstruct()
{
    Super::NativeConstruct();

    // 위젯의 초기화 로직을 여기에 추가
}


void UUIWidget::UpdateGridSizeText(int32 Size)
{
    if (WidgetText)
    {
        UE_LOG(LogTemp, Log, TEXT("Set Grid Size :%d"), Size);
        WidgetText->SetText(FText::Format(NSLOCTEXT("YourNamespace", "GridSizeFormat", "Set Grid Size : {0}"), FText::AsNumber(Size)));
    }
    else
    {
        UE_LOG(LogTemp, Log, TEXT("Context is null."));

    }
}

void UUIWidget::AddPathText(FString Path)
{
    if (WidgetText)
    {
        // 기존 텍스트 가져오기
        FText CurrentText = WidgetText->GetText();

        // 기존 텍스트와 새로운 Path 문자열을 결합
        FString CombinedText = CurrentText.ToString() + TEXT("\n") + Path;

        // 텍스트 업데이트
        WidgetText->SetText(FText::FromString(CombinedText));

        UE_LOG(LogTemp, Log, TEXT("Added Path Text: %s"), *Path);
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("WidgetText is null. Cannot add Path Text."));
    }
}