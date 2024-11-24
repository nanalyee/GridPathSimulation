#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Http.h"
#include "UIWidget.h" // UIWidget 헤더 포함
#include "GridManager.generated.h"

UCLASS()
class GRIDPATH_API AGridManager : public AActor
{
    GENERATED_BODY()

public:
    AGridManager();

protected:
    virtual void BeginPlay() override;

public:
    // 위치
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Transform")
    FTransform ActorTransform;

    // 그리드 크기
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid Settings")
    int32 mGridSize;

    // 셀 간 간격
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid Settings")
    float mCellInterval;

    // 생성할 셀의 클래스
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid Settings")
    TSubclassOf<class AGridCell> CellBlueprintClass;

    // 이동체(M)를 나타낼 클래스 선언
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid Settings")
    TSubclassOf<class AMovingObject> MovingObjectBlueprintClass;

    // UIWidgetInstance 참조 변수
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI Management")
    UUIWidget* UIWidgetInstance;

    // URL
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "URL")
    FString SizeUrl;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "URL")
    FString PathUrl;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "URL")
    FString LogUrl;

    

private:
    TArray<int32> mFibonacciSequence; // 피보나치 수열 저장
    TArray<int32> mRandomIdMovingObject; // 이동체 랜덤 ID 설정
    TArray<AMovingObject*> mMovingObjects; // 생성된 이동체 목록
    TArray<AGridCell*> mTargetCells; // 목적지 셀 목록
    FTimerHandle mSpawnTimerHandle;   // 타이머 핸들
    int32 mCurrentSpawnCnt; // 현재 스폰 중인 mMovingObjects 인덱스
    
    // config
    void GetUrl();

    // 그리드 생성
    void GetSize(const FString& Url);
    void HandleGetSize(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);
    void CreateGrid(); 
    void FetchInitialSizeData();

    // 이동체 및 목적지
    void StartSpawningMovingObjects();
    void InitializeFibonacciSequence();
    void ScheduleNextSpawn();
    void SpawnMovingObject(int32 MovingObjectID, int32 TargetPosX, int32 TargetPosY, int32 CellInterval);

    // 이동 및 경로
    void PostPath(const FString& Url, const FString& MovingID, const FVector2D& InitialPos, const FVector2D& TargetPos);
    void HandlePostPath(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);
    void HandlePathArr(TSharedPtr<FJsonObject> JsonObject);
    void StartMovement(const int32 id, const TArray<FVector2D>& Path); 

    // UI 세팅
    void InitializeUI();
    void UpdateUIGridSize(int32 NewGridSize);
    void UpdateUIPath(FString ResultString);

    // Json
    TSharedPtr<FJsonObject> ParseJson(const FString& JsonString); 
    FString CreateJsonForPath(const FString& MovingID, const FVector2D& InitialPos, const FVector2D& TargetPos); 
};