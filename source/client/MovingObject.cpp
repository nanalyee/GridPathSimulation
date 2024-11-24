#include "MovingObject.h"
#include "TimerManager.h"
#include "Engine/World.h"
#include "GridManager.h"

#include "HttpModule.h"  // FHttpModule을 위해 필요
#include "Http.h"        // IHttpRequest, IHttpResponse를 위해 필요
#include "Json.h"        // JSON 데이터를 처리하기 위해 필요
#include "JsonUtilities.h" // JSON 직렬화 및 역직렬화

AMovingObject::AMovingObject()
{
    PrimaryActorTick.bCanEverTick = true; // Tick 활성화

    mID = 0;
    mInitialTransform = FVector::ZeroVector;
    mInitialPos = FVector2D::ZeroVector;
    mTargetPos = FVector2D::ZeroVector;
    mMaxSpeed = 100.0f;        // 1m/s (언리얼 단위: cm/s)
    mAcceleration = 10.0f;     // 0.1m/s²
    mRotationSpeed = 10.0f;    // 10도/s
    mCurrentSpeed = 10.0f;     // 초기 속도 설정
    mCurrentPathIndex = 0;
    bIsMoving = false;        // 초기값: false
    mStartTime = FDateTime::Now();
    mEndTime = FDateTime::Now();
    LogUrl = "";
}

void AMovingObject::BeginPlay()
{
    Super::BeginPlay();
    mCurrentPosition = FVector(mInitialTransform.X, mInitialTransform.Y, GetActorLocation().Z);
}

void AMovingObject::SetID(int32 NewID)
{
    mID = NewID;
}

void AMovingObject::SetInitialTransform(FVector NewInitialTransform)
{
    mInitialTransform = NewInitialTransform;
}

void AMovingObject::SetInitialPos(FVector2D NewInitialPos)
{
    mInitialPos = NewInitialPos;
}

void AMovingObject::SetTargetPos(FVector2D NewTargetPos)
{
    mTargetPos = NewTargetPos;
}

void AMovingObject::SetPath(const TArray<FVector2D>& NewPath, int32 CellInterval)
{
    mPath.Empty(); // 기존 경로 초기화
    mCurrentPathIndex = 0; // 경로 초기화

    // 각 이동체의 초기 위치를 기준으로 변환
    for (const FVector2D& GridCoordinate : NewPath)
    {
        FVector2D WorldCoordinate;
        WorldCoordinate.X = mInitialTransform.X + ((GridCoordinate.X - mInitialPos.X) * CellInterval);
        WorldCoordinate.Y = mInitialTransform.Y + ((GridCoordinate.Y - mInitialPos.Y) * CellInterval);

        mPath.Add(WorldCoordinate);
    }
}

void AMovingObject::SetLogUrl(FString Url)
{
    LogUrl = Url;
}

void AMovingObject::SetTargetCell(AGridCell* TargetCell)
{
    mTargetCell = TargetCell;
}

void AMovingObject::StartMoving()
{
    mStartTime = FDateTime::Now(); // 시작 시간 기록
    bIsMoving = true; // 이동 시작

    if (mPath.Num() == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("Path is empty for Moving Object %d"), mID);
        return;
    }

    mCurrentPosition = FVector(mInitialTransform.X, mInitialTransform.Y, GetActorLocation().Z);
    SetActorLocation(mCurrentPosition); // 초기 위치 설정
}

void AMovingObject::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (bIsMoving)
    {
        TickMove(DeltaTime);
    }
}

void AMovingObject::TickMove(float DeltaTime)
{
    if (mCurrentPathIndex >= mPath.Num())
    {
        UE_LOG(LogTemp, Log, TEXT("Moving Object %d reached final target."), mID);
        mEndTime = FDateTime::Now();
        FString LogMessage = FString::Printf(TEXT("Moving Object %d started at %s reached the final target at %s"), mID, *mStartTime.ToString(), *mEndTime.ToString());
        PostLogMessage(LogMessage);
        bIsMoving = false;

        if (mTargetCell) {
            mTargetCell->Destroy();
            mTargetCell = nullptr; // 선택적: 파괴 후 포인터를 null로 설정
        }
        Destroy(); // 이동체 제거
        return;
    }

    // 현재 경로의 다음 좌표
    FVector2D NextCoordinate = mPath[mCurrentPathIndex];
    FVector TargetLocation(NextCoordinate.X, NextCoordinate.Y, mCurrentPosition.Z);
    FVector Direction = (TargetLocation - mCurrentPosition).GetSafeNormal();
    FRotator TargetRotation = Direction.Rotation();

    // 부드러운 회전 (InterpTo 사용)
    FRotator CurrentRotation = GetActorRotation();
    FRotator NewRotation = FMath::RInterpTo(CurrentRotation, TargetRotation, DeltaTime, mRotationSpeed);
    SetActorRotation(NewRotation);

    // 회전이 완료되었는지 확인
    float RotationDifference = FMath::Abs(FMath::UnwindDegrees(NewRotation.Yaw - TargetRotation.Yaw));
    if (RotationDifference < 1.0f) // 회전 오차 범위
    {
        // 속도 증가
        mCurrentSpeed = FMath::Min(mCurrentSpeed + mAcceleration * DeltaTime, mMaxSpeed);

        // 이동 계산
        FVector NewPosition = mCurrentPosition + Direction * mCurrentSpeed * DeltaTime;
        SetActorLocation(NewPosition);

        // 목표 도달 여부 확인
        float DistanceToTarget = FVector::Dist(NewPosition, TargetLocation);

        if (DistanceToTarget <= 1.0f || DistanceToTarget < mCurrentSpeed * DeltaTime) // 거리 조건
        {
            UE_LOG(LogTemp, Log, TEXT("Moving Object %d reached Point (%f, %f)."), mID, TargetLocation.X, TargetLocation.Y);
            mCurrentPosition = TargetLocation; // 위치 보정
            SetActorLocation(TargetLocation);
            mCurrentPathIndex++; // 다음 경로로 이동
        }
        else
        {
            mCurrentPosition = NewPosition; // 현재 위치 업데이트
        }
    }
}

void AMovingObject::PostLogMessage(const FString& Message)
{
    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
    Request->SetURL(LogUrl);  // 서버의 주소로 변경해야 합니다.
    Request->SetVerb(TEXT("POST"));
    Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));

    // JSON 본문 구성
    TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject);
    JsonObject->SetNumberField(TEXT("moving_object_id"), mID);
    JsonObject->SetStringField(TEXT("start_time"), mStartTime.ToString(TEXT("%Y-%m-%d %H:%M:%S")));
    JsonObject->SetStringField(TEXT("end_time"), mEndTime.ToString(TEXT("%Y-%m-%d %H:%M:%S")));

    // RequestContent 구성
    FString RequestContent;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&RequestContent);
    FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer);
    Request->SetContentAsString(RequestContent);

    // 응답 처리 함수 바인딩
    Request->OnProcessRequestComplete().BindLambda([](FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
        {
            if (bWasSuccessful && Response.IsValid())
            {
                UE_LOG(LogTemp, Log, TEXT("Log message was successfully sent: %s"), *Response->GetContentAsString());
            }
            else
            {
                UE_LOG(LogTemp, Error, TEXT("Failed to send log message"));
            }
        });
    Request->ProcessRequest();
}

