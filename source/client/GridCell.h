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

    // 그리드 위치
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    FVector2D mGridPos;

    // 목적지 여부
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid Cell")
    bool bIsTargetPoint;

    // 목적지 ID (목적지인 경우에만 설정)
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Grid Cell")
    int32 mTargetID;
};
