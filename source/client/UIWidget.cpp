// Fill out your copyright notice in the Description page of Project Settings.

#include "UIWidget.h"
#include "Blueprint/UserWidget.h"
#include "Components/TextBlock.h"


void UUIWidget::NativeConstruct()
{
    Super::NativeConstruct();

    // ������ �ʱ�ȭ ������ ���⿡ �߰�
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
        // ���� �ؽ�Ʈ ��������
        FText CurrentText = WidgetText->GetText();

        // ���� �ؽ�Ʈ�� ���ο� Path ���ڿ��� ����
        FString CombinedText = CurrentText.ToString() + TEXT("\n") + Path;

        // �ؽ�Ʈ ������Ʈ
        WidgetText->SetText(FText::FromString(CombinedText));

        UE_LOG(LogTemp, Log, TEXT("Added Path Text: %s"), *Path);
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("WidgetText is null. Cannot add Path Text."));
    }
}