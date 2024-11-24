// Fill out your copyright notice in the Description page of Project Settings.


#include "GridManager.h"
#include "GridCell.h"
#include "Engine/World.h"
#include "MovingObject.h"

#include "HttpModule.h"  // FHttpModule�� ���� �ʿ�
#include "Http.h"        // IHttpRequest, IHttpResponse�� ���� �ʿ�
#include "Json.h"        // JSON �����͸� ó���ϱ� ���� �ʿ�
#include "JsonUtilities.h" // JSON ����ȭ �� ������ȭ
#include "Blueprint/UserWidget.h"
#include "Misc/ConfigCacheIni.h"

AGridManager::AGridManager()
{
    mGridSize = 10;  // �⺻ �׸��� ũ��
    mCellInterval = 120.0f;  // �� �� ����
    CellBlueprintClass = nullptr;
    MovingObjectBlueprintClass = nullptr;
    UIWidgetInstance = nullptr;
    SizeUrl = "";
    PathUrl = "";
    LogUrl = "";
}

void AGridManager::BeginPlay()
{
    Super::BeginPlay();
    GetUrl();
    InitializeUI();
    FetchInitialSizeData();
}

// DefaultGame.ini���� ������ URL ��������
void AGridManager::GetUrl() 
{
    if (GConfig)
    {
        GConfig->GetString(TEXT("/Script/GridPath.UrlSettings"), TEXT("SizeUrl"), SizeUrl, GGameIni);
        GConfig->GetString(TEXT("/Script/GridPath.UrlSettings"), TEXT("PathUrl"), PathUrl, GGameIni);
        GConfig->GetString(TEXT("/Script/GridPath.UrlSettings"), TEXT("LogUrl"), LogUrl, GGameIni);
        SizeUrl = SizeUrl.TrimQuotes();
        PathUrl = PathUrl.TrimQuotes();
        LogUrl = LogUrl.TrimQuotes();
    }
}


// GetSize() �Լ� ����
void AGridManager::FetchInitialSizeData()
{
    GetSize(SizeUrl);
}


// 1) ������ ����� �Է°�(N)�� �޾ƿ´�.
void AGridManager::GetSize(const FString& Url)
{
    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
    Request->SetURL(Url);
    Request->SetVerb(TEXT("GET")); // GET ��û ����
    Request->OnProcessRequestComplete().BindUObject(this, &AGridManager::HandleGetSize);
    Request->ProcessRequest();
}

// ���� ó��
void AGridManager::HandleGetSize(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
    if (!bWasSuccessful || !Response.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("GET Request failed or invalid response"));
        return;
    }

    FString ResponseContent = Response->GetContentAsString();
    UE_LOG(LogTemp, Log, TEXT("GET Request Success: %s"), *ResponseContent);

    // JSON �Ľ� ó��
    TSharedPtr<FJsonObject> JsonObject = ParseJson(ResponseContent);
    if (!JsonObject.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to parse JSON from response"));
        return;
    }

    if (JsonObject->HasField("size"))
    {
        int32 GridSize = JsonObject->GetIntegerField("size");
        UE_LOG(LogTemp, Log, TEXT("Parsed Grid Size: %d"), GridSize);
        mGridSize = GridSize;
        UpdateUIGridSize(GridSize);
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("JSON does not contain 'size' field"));
    }
}




// 2) Ŭ���̾�Ʈ ���� �� (N x N) �� �׸��带 �����Ѵ�.
void AGridManager::CreateGrid()
{
    if (!CellBlueprintClass) return;
    this->mTargetCells.Empty(); 

    // �迭�� �����ϰ� ����
    int32 GridCount = mGridSize;
    TArray<int32> RandomID;
    for (int32 i = 1; i <= GridCount; ++i)
    {
        RandomID.Add(i);
    }
    for (int32 i = 0; i < GridCount - 1; ++i)
    {
        int32 RandomIndex = FMath::RandRange(i, GridCount - 1);
        int32 Temp = RandomID[i];
        RandomID[i] = RandomID[RandomIndex];
        RandomID[RandomIndex] = Temp;
    }

    // ������(T) ����
    int32 TargetIDIndex = 0; // ������ �ε���
    for (int32 RowIndex = 0; RowIndex < mGridSize; ++RowIndex)
    {
        for (int32 ColIndex = 0; ColIndex < mGridSize; ++ColIndex)
        {
            FVector CellPosition(RowIndex * mCellInterval, ColIndex * mCellInterval, 0.0f);
            FActorSpawnParameters SpawnParams;

            AGridCell* NewCell = GetWorld()->SpawnActor<AGridCell>(CellBlueprintClass, CellPosition, FRotator::ZeroRotator, SpawnParams);

            if (NewCell)
            {
                NewCell->mGridPos = FVector2D(RowIndex+1, ColIndex+1);

                // 3) �׸��� ù �࿡ N ���� ������(T)�� �����Ѵ�.
                if (RowIndex == 0)
                {
                    NewCell->bIsTargetPoint = true;
                    NewCell->mTargetID = RandomID[TargetIDIndex];
                    UE_LOG(LogTemp, Log, TEXT("Target created: ID %d at GridPos (%d, %d)"),
                        RandomID[TargetIDIndex], (int32)NewCell->mGridPos.X, (int32)NewCell->mGridPos.Y);
                    this->mTargetCells.Add(NewCell);
                    TargetIDIndex++; // ���� ������ ID
                }
            }
        }
    }

    StartSpawningMovingObjects();
}

// �̵�ü ���� ����
void AGridManager::StartSpawningMovingObjects()
{
    InitializeFibonacciSequence();

    // �迭�� �����ϰ� ����
    int32 GridCount = mGridSize;
    for (int32 i = 1; i <= GridCount; ++i)
    {
        mRandomIdMovingObject.Add(i);
    }
    for (int32 i = 0; i < GridCount - 1; ++i)
    {
        int32 RandomIndex = FMath::RandRange(i, GridCount - 1);
        int32 Temp = mRandomIdMovingObject[i];
        mRandomIdMovingObject[i] = mRandomIdMovingObject[RandomIndex];
        mRandomIdMovingObject[RandomIndex] = Temp;
    }

    // �̵�ü ���� ����
    mCurrentSpawnCnt = 0;
    ScheduleNextSpawn();
}

// �Ǻ���ġ ���� �ʱ�ȭ - �̵�ü�� �Ǻ���ġ ���� ����(��) ���� ����
void AGridManager::InitializeFibonacciSequence()
{
    if (mGridSize <= 0)
    {
        UE_LOG(LogTemp, Error, TEXT("Grid size is invalid. Cannot initialize Fibonacci sequence."));
        return;
    }

    mFibonacciSequence.Empty();
    mFibonacciSequence.Add(1);
    mFibonacciSequence.Add(1);

    for (int32 i = 2; i < mGridSize; ++i)
    {
        mFibonacciSequence.Add(mFibonacciSequence[i - 1] + mFibonacciSequence[i - 2]);
    }

    // ����� ���
    for (int32 i = 0; i < mFibonacciSequence.Num(); ++i)
    {
        UE_LOG(LogTemp, Log, TEXT("FibonacciSequence[%d] = %d"), i, mFibonacciSequence[i]);
    }
}

// 4) �׸��� ������ �࿡ N ���� �̵�ü(M)�� �����Ѵ�. (�ó����� ����)
// ���� �̵�ü ���� ����
void AGridManager::ScheduleNextSpawn()
{
    if (mCurrentSpawnCnt >= mGridSize)
    {
        UE_LOG(LogTemp, Log, TEXT("All moving objects have been spawned."));
        GetWorldTimerManager().ClearTimer(mSpawnTimerHandle);
        return;
    }

    int32 MovingObjectID = mRandomIdMovingObject[mCurrentSpawnCnt];
    int32 TargetID = -1; 
    FVector2D TargetPos = FVector2D::ZeroVector; 
    for (AGridCell* Cell : mTargetCells)
    {
        if (Cell && Cell->mTargetID == MovingObjectID)
        {
            TargetID = Cell->mTargetID; // ��ġ�ϴ� TargetID.
            FVector2D matchPos = Cell->mGridPos;
            TargetPos = FVector2D(matchPos.X, matchPos.Y); // ��ġ�ϴ� TargetPos
            break; 
        }
    }
    if (TargetID != -1)
    {
        UE_LOG(LogTemp, Log, TEXT("Moving Object ID %d  Found Target ID %d at GridPos (%d, %d)"), MovingObjectID, TargetID, (int32)TargetPos.X, (int32)TargetPos.Y);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("No matching TargetID found for MovingObjectID %d"), MovingObjectID);
        return;
    }

    // �Ʒ� SetTimer�� ������ Ÿ�̸Ӵ� �񵿱������� �۵�
    // ���� SpawnMovingObject(TargetPos) ���·� ���������
    // ���� �ݹ� �Լ��� ����Ǳ� ���� ���� ������ ���� ������(TargetPos)�� ����Ǹ鼭 ����
    // �⺻ ������ Ÿ��(int)���� �Ű������� �����ϴ� ������� �ذ�
    // �� ������ �� ����(value copy) ������� ó��
    // �� ����� ȣ��� �Լ��� �Ű������� ���� ���� ������ �ѱ�� ����
    // ���� �����Ͱ� ���߿� ����Ǵ��� ȣ��� �Լ��� �ѱ� �Ű������� ���� �����ϰ� ����
    SpawnMovingObject(MovingObjectID, TargetPos.X, TargetPos.Y, mCellInterval);

    // ���� �̵�ü ���� ����
    float Delay = mFibonacciSequence[mCurrentSpawnCnt] * 1.0f;
    mCurrentSpawnCnt++;
    GetWorldTimerManager().SetTimer(
        mSpawnTimerHandle,
        this,
        &AGridManager::ScheduleNextSpawn,
        Delay,
        false
    );
}

// �̵�ü ���� �� ����
void AGridManager::SpawnMovingObject(int32 MovingObjectID, int32 TargetPosX, int32 TargetPosY, int32 CellInterval)
{

    if (!MovingObjectBlueprintClass)
    {
        UE_LOG(LogTemp, Error, TEXT("MovingObjectBlueprintClass is not set!"));
        return;
    }

    int initialPosX = mGridSize;
    int32 initialPosY = mCurrentSpawnCnt + 1;
    FVector InitialTransform((initialPosX-1) * CellInterval, (initialPosY-1) * CellInterval, 0.0f);
    FActorSpawnParameters SpawnParams;
    UE_LOG(LogTemp, Log, TEXT("GridSize %d, CellInterval %d, Moving Object %d, InitialPos (%d, %d)"),
        mGridSize, CellInterval, MovingObjectID, initialPosX, initialPosY);

    AMovingObject* NewMovingObject = GetWorld()->SpawnActor<AMovingObject>(
        MovingObjectBlueprintClass,
        InitialTransform,
        FRotator::ZeroRotator,
        SpawnParams
    );

    if (NewMovingObject)
    {
        NewMovingObject->SetID(MovingObjectID);
        NewMovingObject->SetInitialTransform(InitialTransform);
        NewMovingObject->SetInitialPos(FVector2D(initialPosX, initialPosY));
        NewMovingObject->SetTargetPos(FVector2D(TargetPosX, TargetPosY));
        NewMovingObject->SetLogUrl(LogUrl);
        mMovingObjects.Add(NewMovingObject);

        UE_LOG(LogTemp, Log, TEXT("Moving Object %d created at (%d, %d) targeting (%d, %d) real pos (%d, %d)"),
            MovingObjectID, mGridSize, mCurrentSpawnCnt + 1, TargetPosX, TargetPosY, InitialTransform.X, InitialTransform.Y);

        // ��� ��û
        PostPath(PathUrl, FString::FromInt(MovingObjectID), NewMovingObject->mInitialPos, NewMovingObject->mTargetPos);
    }
}


// ��� Post ��û
void AGridManager::PostPath(const FString& Url, const FString& MovingID, const FVector2D& InitialPos, const FVector2D& TargetPos)
{
    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
    Request->SetURL(Url);
    Request->SetVerb(TEXT("POST"));
    Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));

    // JSON ���� ����
    FString RequestContent = CreateJsonForPath(MovingID, InitialPos, TargetPos);
    Request->SetContentAsString(RequestContent);

    Request->OnProcessRequestComplete().BindUObject(this, &AGridManager::HandlePostPath);
    Request->ProcessRequest();

    UE_LOG(LogTemp, Log, TEXT("Sending POST Request: %s with Content: %s"), *Url, *RequestContent);
}

// ��� Post ����
void AGridManager::HandlePostPath(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
    if (!bWasSuccessful || !Response.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("POST Request failed or invalid response"));
        return;
    }

    FString ResponseContent = Response->GetContentAsString();
    UE_LOG(LogTemp, Log, TEXT("POST Request Success: %s"), *ResponseContent);

    // JSON �Ľ�
    TSharedPtr<FJsonObject> JsonObject = ParseJson(ResponseContent);
    if (!JsonObject.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to parse JSON from ResponseContent"));
        return;
    }
    HandlePathArr(JsonObject);
}

// ��� ������Ʈ
void AGridManager::HandlePathArr(TSharedPtr<FJsonObject> JsonObject)
{
    // "id" �� ��������
    int32 ID = JsonObject->GetIntegerField(TEXT("id"));
    FString ResultString = FString::Printf(TEXT("id: %d, path: "), ID);

    // "path" �迭 ó��
    TArray<FVector2D> Path;
    if (const TArray<TSharedPtr<FJsonValue>>* PathArray; JsonObject->TryGetArrayField(TEXT("path"), PathArray))
    {
        TArray<FString> PathStrings; // Path ���ڿ� ����
        for (const TSharedPtr<FJsonValue>& Value : *PathArray)
        {
            if (const TArray<TSharedPtr<FJsonValue>>* CoordinateArray; Value->TryGetArray(CoordinateArray) && CoordinateArray->Num() == 2)
            {
                float X = static_cast<float>((*CoordinateArray)[0]->AsNumber());
                float Y = static_cast<float>((*CoordinateArray)[1]->AsNumber());

                Path.Add(FVector2D(X, Y));
                PathStrings.Add(FString::Printf(TEXT("[%d,%d]"), static_cast<int32>(X), static_cast<int32>(Y)));
            }
        }

        // Path �迭�� '-'�� ����
        ResultString += FString::Join(PathStrings, TEXT("-"));
    }
    else
    {
        ResultString += TEXT("[]"); // Path�� ������� ���
    }

    UpdateUIPath(ResultString);
    StartMovement(ID, Path);
}

// �̵� ����
void AGridManager::StartMovement(const int32 id, const TArray<FVector2D>& Path)
{
    UE_LOG(LogTemp, Log, TEXT("Start Movement"));
    
    // �̵�üID�� id�� index ���ϱ�
    int32 index = -1;
    for (int32 i = 0; i < mRandomIdMovingObject.Num(); ++i) {
        if (mRandomIdMovingObject[i] == id) {
            index = i;
            break;
        }
    }
    if (index == -1) 
    {
        UE_LOG(LogTemp, Error, TEXT("Invalid ID index: %d"), id);
        return;
    }

    AMovingObject* TargetObject = mMovingObjects[index];
    if (TargetObject)
    {
        TargetObject->SetPath(Path, mCellInterval);
        TargetObject->StartMoving();
        UE_LOG(LogTemp, Log, TEXT("Accessed MovingObject with ID: %d"), id);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("MovingObject at ID is null."));
    }
}




// UI �ʱ� ����
void AGridManager::InitializeUI()
{
    if (UIWidgetInstance)
    {
        UIWidgetInstance->AddToViewport();
        UE_LOG(LogTemp, Log, TEXT("UIWidgetInstance successfully added to viewport."));
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("UIWidgetInstance is null. Check if it's properly assigned in the editor."));
    }
}

// Grid ũ�� ������Ʈ
void AGridManager::UpdateUIGridSize(int32 NewGridSize)
{
    // UI ������Ʈ
    if (UIWidgetInstance)
    {
        UIWidgetInstance->UpdateGridSizeText(NewGridSize);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("UIWidgetInstance is null. Cannot update UI."));
    }
    CreateGrid(); // �׸��� ����
}

// ��� ������Ʈ
void AGridManager::UpdateUIPath(FString ResultString)
{
    // UI ������Ʈ
    if (UIWidgetInstance)
    {
        UIWidgetInstance->AddPathText(ResultString);
    }
}



// JSON �Ľ�
TSharedPtr<FJsonObject> AGridManager::ParseJson(const FString& JsonString)
{
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);

    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("JSON parsing failed for string: %s"), *JsonString);
    }

    return JsonObject;
}

// Json ����
FString AGridManager::CreateJsonForPath(const FString& MovingID, const FVector2D& InitialPos, const FVector2D& TargetPos)
{
    TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject);
    JsonObject->SetStringField(TEXT("id"), MovingID);
    JsonObject->SetNumberField(TEXT("InitialPosX"), InitialPos.X);
    JsonObject->SetNumberField(TEXT("InitialPosY"), InitialPos.Y);
    JsonObject->SetNumberField(TEXT("TargetPosX"), TargetPos.X);
    JsonObject->SetNumberField(TEXT("TargetPosY"), TargetPos.Y);

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer);

    return OutputString;
}