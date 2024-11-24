#include "MovingObject.h"
#include "TimerManager.h"
#include "Engine/World.h"
#include "GridManager.h"

#include "HttpModule.h"  // FHttpModule�� ���� �ʿ�
#include "Http.h"        // IHttpRequest, IHttpResponse�� ���� �ʿ�
#include "Json.h"        // JSON �����͸� ó���ϱ� ���� �ʿ�
#include "JsonUtilities.h" // JSON ����ȭ �� ������ȭ

AMovingObject::AMovingObject()
{
    PrimaryActorTick.bCanEverTick = true; // Tick Ȱ��ȭ

    mID = 0;
    mInitialTransform = FVector::ZeroVector;
    mInitialPos = FVector2D::ZeroVector;
    mTargetPos = FVector2D::ZeroVector;
    mMaxSpeed = 100.0f;        // 1m/s (�𸮾� ����: cm/s)
    mAcceleration = 10.0f;     // 0.1m/s��
    mRotationSpeed = 10.0f;    // 10��/s
    mCurrentSpeed = 10.0f;     // �ʱ� �ӵ� ����
    mCurrentPathIndex = 0;
    bIsMoving = false;        // �ʱⰪ: false
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
    mPath.Empty(); // ���� ��� �ʱ�ȭ
    mCurrentPathIndex = 0; // ��� �ʱ�ȭ

    // �� �̵�ü�� �ʱ� ��ġ�� �������� ��ȯ
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
    mStartTime = FDateTime::Now(); // ���� �ð� ���
    bIsMoving = true; // �̵� ����

    if (mPath.Num() == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("Path is empty for Moving Object %d"), mID);
        return;
    }

    mCurrentPosition = FVector(mInitialTransform.X, mInitialTransform.Y, GetActorLocation().Z);
    SetActorLocation(mCurrentPosition); // �ʱ� ��ġ ����
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
            mTargetCell = nullptr; // ������: �ı� �� �����͸� null�� ����
        }
        Destroy(); // �̵�ü ����
        return;
    }

    // ���� ����� ���� ��ǥ
    FVector2D NextCoordinate = mPath[mCurrentPathIndex];
    FVector TargetLocation(NextCoordinate.X, NextCoordinate.Y, mCurrentPosition.Z);
    FVector Direction = (TargetLocation - mCurrentPosition).GetSafeNormal();
    FRotator TargetRotation = Direction.Rotation();

    // �ε巯�� ȸ�� (InterpTo ���)
    FRotator CurrentRotation = GetActorRotation();
    FRotator NewRotation = FMath::RInterpTo(CurrentRotation, TargetRotation, DeltaTime, mRotationSpeed);
    SetActorRotation(NewRotation);

    // ȸ���� �Ϸ�Ǿ����� Ȯ��
    float RotationDifference = FMath::Abs(FMath::UnwindDegrees(NewRotation.Yaw - TargetRotation.Yaw));
    if (RotationDifference < 1.0f) // ȸ�� ���� ����
    {
        // �ӵ� ����
        mCurrentSpeed = FMath::Min(mCurrentSpeed + mAcceleration * DeltaTime, mMaxSpeed);

        // �̵� ���
        FVector NewPosition = mCurrentPosition + Direction * mCurrentSpeed * DeltaTime;
        SetActorLocation(NewPosition);

        // ��ǥ ���� ���� Ȯ��
        float DistanceToTarget = FVector::Dist(NewPosition, TargetLocation);

        if (DistanceToTarget <= 1.0f || DistanceToTarget < mCurrentSpeed * DeltaTime) // �Ÿ� ����
        {
            UE_LOG(LogTemp, Log, TEXT("Moving Object %d reached Point (%f, %f)."), mID, TargetLocation.X, TargetLocation.Y);
            mCurrentPosition = TargetLocation; // ��ġ ����
            SetActorLocation(TargetLocation);
            mCurrentPathIndex++; // ���� ��η� �̵�
        }
        else
        {
            mCurrentPosition = NewPosition; // ���� ��ġ ������Ʈ
        }
    }
}

void AMovingObject::PostLogMessage(const FString& Message)
{
    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
    Request->SetURL(LogUrl);  // ������ �ּҷ� �����ؾ� �մϴ�.
    Request->SetVerb(TEXT("POST"));
    Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));

    // JSON ���� ����
    TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject);
    JsonObject->SetNumberField(TEXT("moving_object_id"), mID);
    JsonObject->SetStringField(TEXT("start_time"), mStartTime.ToString(TEXT("%Y-%m-%d %H:%M:%S")));
    JsonObject->SetStringField(TEXT("end_time"), mEndTime.ToString(TEXT("%Y-%m-%d %H:%M:%S")));

    // RequestContent ����
    FString RequestContent;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&RequestContent);
    FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer);
    Request->SetContentAsString(RequestContent);

    // ���� ó�� �Լ� ���ε�
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

