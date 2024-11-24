// Fill out your copyright notice in the Description page of Project Settings.


#include "GridManager.h"
#include "GridCell.h"
#include "Engine/World.h"
#include "MovingObject.h"

#include "HttpModule.h"  // FHttpModule을 위해 필요
#include "Http.h"        // IHttpRequest, IHttpResponse를 위해 필요
#include "Json.h"        // JSON 데이터를 처리하기 위해 필요
#include "JsonUtilities.h" // JSON 직렬화 및 역직렬화
#include "Blueprint/UserWidget.h"
#include "Misc/ConfigCacheIni.h"

AGridManager::AGridManager()
{
    mGridSize = 10;  // 기본 그리드 크기
    mCellInterval = 120.0f;  // 셀 간 간격
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

// DefaultGame.ini에서 저장한 URL 가져오기
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


// GetSize() 함수 실행
void AGridManager::FetchInitialSizeData()
{
    GetSize(SizeUrl);
}


// 1) 서버에 저장된 입력값(N)을 받아온다.
void AGridManager::GetSize(const FString& Url)
{
    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
    Request->SetURL(Url);
    Request->SetVerb(TEXT("GET")); // GET 요청 설정
    Request->OnProcessRequestComplete().BindUObject(this, &AGridManager::HandleGetSize);
    Request->ProcessRequest();
}

// 응답 처리
void AGridManager::HandleGetSize(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
    if (!bWasSuccessful || !Response.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("GET Request failed or invalid response"));
        return;
    }

    FString ResponseContent = Response->GetContentAsString();
    UE_LOG(LogTemp, Log, TEXT("GET Request Success: %s"), *ResponseContent);

    // JSON 파싱 처리
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




// 2) 클라이언트 시작 시 (N x N) 의 그리드를 생성한다.
void AGridManager::CreateGrid()
{
    if (!CellBlueprintClass) return;
    this->mTargetCells.Empty(); 

    // 배열을 랜덤하게 섞음
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

    // 목적지(T) 생성
    int32 TargetIDIndex = 0; // 목적지 인덱스
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

                // 3) 그리드 첫 행에 N 개의 목적지(T)를 생성한다.
                if (RowIndex == 0)
                {
                    NewCell->bIsTargetPoint = true;
                    NewCell->mTargetID = RandomID[TargetIDIndex];
                    UE_LOG(LogTemp, Log, TEXT("Target created: ID %d at GridPos (%d, %d)"),
                        RandomID[TargetIDIndex], (int32)NewCell->mGridPos.X, (int32)NewCell->mGridPos.Y);
                    this->mTargetCells.Add(NewCell);
                    TargetIDIndex++; // 다음 목적지 ID
                }
            }
        }
    }

    StartSpawningMovingObjects();
}

// 이동체 생성 시작
void AGridManager::StartSpawningMovingObjects()
{
    InitializeFibonacciSequence();

    // 배열을 랜덤하게 섞음
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

    // 이동체 스폰 시작
    mCurrentSpawnCnt = 0;
    ScheduleNextSpawn();
}

// 피보나치 수열 초기화 - 이동체는 피보나치 수열 간격(초) 으로 생성
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

    // 디버그 출력
    for (int32 i = 0; i < mFibonacciSequence.Num(); ++i)
    {
        UE_LOG(LogTemp, Log, TEXT("FibonacciSequence[%d] = %d"), i, mFibonacciSequence[i]);
    }
}

// 4) 그리드 마지막 행에 N 개의 이동체(M)를 생성한다. (시나리오 참고)
// 다음 이동체 생성 예약
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
            TargetID = Cell->mTargetID; // 일치하는 TargetID.
            FVector2D matchPos = Cell->mGridPos;
            TargetPos = FVector2D(matchPos.X, matchPos.Y); // 일치하는 TargetPos
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

    // 아래 SetTimer로 설정된 타이머는 비동기적으로 작동
    // 기존 SpawnMovingObject(TargetPos) 형태로 사용했으나
    // 실제 콜백 함수가 실행되기 전에 복합 데이터 구조 데이터(TargetPos)가 변경되면서 문제
    // 기본 데이터 타입(int)으로 매개변수를 전달하는 방법으로 해결
    // 이 값들은 값 복사(value copy) 방식으로 처리
    // 값 복사는 호출된 함수의 매개변수로 직접 값을 복사해 넘기기 때문
    // 원본 데이터가 나중에 변경되더라도 호출된 함수에 넘긴 매개변수의 값은 안전하게 보존
    SpawnMovingObject(MovingObjectID, TargetPos.X, TargetPos.Y, mCellInterval);

    // 다음 이동체 생성 예약
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

// 이동체 설정 후 생성
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

        // 경로 요청
        PostPath(PathUrl, FString::FromInt(MovingObjectID), NewMovingObject->mInitialPos, NewMovingObject->mTargetPos);
    }
}


// 경로 Post 요청
void AGridManager::PostPath(const FString& Url, const FString& MovingID, const FVector2D& InitialPos, const FVector2D& TargetPos)
{
    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
    Request->SetURL(Url);
    Request->SetVerb(TEXT("POST"));
    Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));

    // JSON 본문 생성
    FString RequestContent = CreateJsonForPath(MovingID, InitialPos, TargetPos);
    Request->SetContentAsString(RequestContent);

    Request->OnProcessRequestComplete().BindUObject(this, &AGridManager::HandlePostPath);
    Request->ProcessRequest();

    UE_LOG(LogTemp, Log, TEXT("Sending POST Request: %s with Content: %s"), *Url, *RequestContent);
}

// 경로 Post 응답
void AGridManager::HandlePostPath(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
    if (!bWasSuccessful || !Response.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("POST Request failed or invalid response"));
        return;
    }

    FString ResponseContent = Response->GetContentAsString();
    UE_LOG(LogTemp, Log, TEXT("POST Request Success: %s"), *ResponseContent);

    // JSON 파싱
    TSharedPtr<FJsonObject> JsonObject = ParseJson(ResponseContent);
    if (!JsonObject.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to parse JSON from ResponseContent"));
        return;
    }
    HandlePathArr(JsonObject);
}

// 경로 업데이트
void AGridManager::HandlePathArr(TSharedPtr<FJsonObject> JsonObject)
{
    // "id" 값 가져오기
    int32 ID = JsonObject->GetIntegerField(TEXT("id"));
    FString ResultString = FString::Printf(TEXT("id: %d, path: "), ID);

    // "path" 배열 처리
    TArray<FVector2D> Path;
    if (const TArray<TSharedPtr<FJsonValue>>* PathArray; JsonObject->TryGetArrayField(TEXT("path"), PathArray))
    {
        TArray<FString> PathStrings; // Path 문자열 저장
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

        // Path 배열을 '-'로 연결
        ResultString += FString::Join(PathStrings, TEXT("-"));
    }
    else
    {
        ResultString += TEXT("[]"); // Path가 비어있을 경우
    }

    UpdateUIPath(ResultString);
    StartMovement(ID, Path);
}

// 이동 시작
void AGridManager::StartMovement(const int32 id, const TArray<FVector2D>& Path)
{
    UE_LOG(LogTemp, Log, TEXT("Start Movement"));
    
    // 이동체ID가 id인 index 구하기
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




// UI 초기 세팅
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

// Grid 크기 업데이트
void AGridManager::UpdateUIGridSize(int32 NewGridSize)
{
    // UI 업데이트
    if (UIWidgetInstance)
    {
        UIWidgetInstance->UpdateGridSizeText(NewGridSize);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("UIWidgetInstance is null. Cannot update UI."));
    }
    CreateGrid(); // 그리드 생성
}

// 경로 업데이트
void AGridManager::UpdateUIPath(FString ResultString)
{
    // UI 업데이트
    if (UIWidgetInstance)
    {
        UIWidgetInstance->AddPathText(ResultString);
    }
}



// JSON 파싱
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

// Json 생성
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