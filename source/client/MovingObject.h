// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MovingObject.generated.h"

UCLASS()
class GRIDPATH_API AMovingObject : public AActor
{
    GENERATED_BODY()

public:
    // 생성자
    AMovingObject();

protected:
    virtual void BeginPlay() override;

public:
    virtual void Tick(float DeltaTime) override; // Tick 활성화

    // 이동체 정보
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Moving Object")
    int32 mID;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Moving Object")
    float mMaxSpeed;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Moving Object")
    float mAcceleration;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Moving Object")
    float mRotationSpeed;

    // 이동체 실시간 정보
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Moving Object")
    FVector mCurrentPosition;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Moving Object")
    float mCurrentSpeed;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Moving Object")
    int32 mCurrentPathIndex;

    // 좌표 및 경로
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Moving Object")
    FVector mInitialTransform; // 초기 실제 좌표
    
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Moving Object")
    FVector2D mInitialPos; // 초기 그리드 좌표
    
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Moving Object")
    FVector2D mTargetPos; // 최종 목표 그리드 좌표
    
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Moving Object")
    TArray<FVector2D> mPath; // 경로 배열

    // 좌표 및 경로
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "URL")
    FString LogUrl;

    // ID 및 초기 위치/목표 좌표 설정 함수
    void SetID(int32 NewID);
    void SetInitialTransform(FVector InitialTransform);
    void SetInitialPos(FVector2D NewInitialPos);
    void SetTargetPos(FVector2D NewTargetPos);
    void SetPath(const TArray<FVector2D>& NewPath, int32 CellInterval);
    void SetLogUrl(FString Url);

    // 이동 시작 함수
    void StartMoving();

private:
    bool bIsMoving; // 이동 중 여부
    FDateTime mStartTime; // 시작시간
    FDateTime mEndTime; // 종료시간
    FTimerHandle mMovementTimerHandle;

    void TickMove(float DeltaTime);  
    void PostLogMessage(const FString& Message);
};