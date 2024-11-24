// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GridCell.generated.h"

UCLASS()
class GRIDPATH_API AGridCell : public AActor
{
    GENERATED_BODY()

public:
    AGridCell();

    // �׸��� ��ġ
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    FVector2D mGridPos;

    // ������ ����
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid Cell")
    bool bIsTargetPoint;

    // ������ ID (�������� ��쿡�� ����)
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Grid Cell")
    int32 mTargetID;
};
