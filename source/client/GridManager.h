#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Http.h"
#include "UIWidget.h" // UIWidget ��� ����
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
    // ��ġ
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Transform")
    FTransform ActorTransform;

    // �׸��� ũ��
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid Settings")
    int32 mGridSize;

    // �� �� ����
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid Settings")
    float mCellInterval;

    // ������ ���� Ŭ����
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid Settings")
    TSubclassOf<class AGridCell> CellBlueprintClass;

    // �̵�ü(M)�� ��Ÿ�� Ŭ���� ����
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid Settings")
    TSubclassOf<class AMovingObject> MovingObjectBlueprintClass;

    // UIWidgetInstance ���� ����
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
    TArray<int32> mFibonacciSequence; // �Ǻ���ġ ���� ����
    TArray<int32> mRandomIdMovingObject; // �̵�ü ���� ID ����
    TArray<AMovingObject*> mMovingObjects; // ������ �̵�ü ���
    TArray<AGridCell*> mTargetCells; // ������ �� ���
    FTimerHandle mSpawnTimerHandle;   // Ÿ�̸� �ڵ�
    int32 mCurrentSpawnCnt; // ���� ���� ���� mMovingObjects �ε���
    
    // config
    void GetUrl();

    // �׸��� ����
    void GetSize(const FString& Url);
    void HandleGetSize(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);
    void CreateGrid(); 
    void FetchInitialSizeData();

    // �̵�ü �� ������
    void StartSpawningMovingObjects();
    void InitializeFibonacciSequence();
    void ScheduleNextSpawn();
    void SpawnMovingObject(int32 MovingObjectID, int32 TargetPosX, int32 TargetPosY, int32 CellInterval);

    // �̵� �� ���
    void PostPath(const FString& Url, const FString& MovingID, const FVector2D& InitialPos, const FVector2D& TargetPos);
    void HandlePostPath(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);
    void HandlePathArr(TSharedPtr<FJsonObject> JsonObject);
    void StartMovement(const int32 id, const TArray<FVector2D>& Path); 

    // UI ����
    void InitializeUI();
    void UpdateUIGridSize(int32 NewGridSize);
    void UpdateUIPath(FString ResultString);

    // Json
    TSharedPtr<FJsonObject> ParseJson(const FString& JsonString); 
    FString CreateJsonForPath(const FString& MovingID, const FVector2D& InitialPos, const FVector2D& TargetPos); 
};