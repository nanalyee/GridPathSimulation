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
    // ������
    AMovingObject();

protected:
    virtual void BeginPlay() override;

public:
    virtual void Tick(float DeltaTime) override; // Tick Ȱ��ȭ

    // �̵�ü ����
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Moving Object")
    int32 mID;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Moving Object")
    float mMaxSpeed;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Moving Object")
    float mAcceleration;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Moving Object")
    float mRotationSpeed;

    // �̵�ü �ǽð� ����
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Moving Object")
    FVector mCurrentPosition;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Moving Object")
    float mCurrentSpeed;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Moving Object")
    int32 mCurrentPathIndex;

    // ��ǥ �� ���
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Moving Object")
    FVector mInitialTransform; // �ʱ� ���� ��ǥ
    
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Moving Object")
    FVector2D mInitialPos; // �ʱ� �׸��� ��ǥ
    
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Moving Object")
    FVector2D mTargetPos; // ���� ��ǥ �׸��� ��ǥ
    
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Moving Object")
    TArray<FVector2D> mPath; // ��� �迭

    // ��ǥ �� ���
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "URL")
    FString LogUrl;

    // ID �� �ʱ� ��ġ/��ǥ ��ǥ ���� �Լ�
    void SetID(int32 NewID);
    void SetInitialTransform(FVector InitialTransform);
    void SetInitialPos(FVector2D NewInitialPos);
    void SetTargetPos(FVector2D NewTargetPos);
    void SetPath(const TArray<FVector2D>& NewPath, int32 CellInterval);
    void SetLogUrl(FString Url);

    // �̵� ���� �Լ�
    void StartMoving();

private:
    bool bIsMoving; // �̵� �� ����
    FDateTime mStartTime; // ���۽ð�
    FDateTime mEndTime; // ����ð�
    FTimerHandle mMovementTimerHandle;

    void TickMove(float DeltaTime);  
    void PostLogMessage(const FString& Message);
};