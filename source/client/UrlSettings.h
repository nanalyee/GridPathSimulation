// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UrlSettings.generated.h"

UCLASS(Config = Game, DefaultConfig)
class GRIDPATH_API UUrlSettings : public UObject
{
    GENERATED_BODY()

public:
    UUrlSettings();

    UPROPERTY(Config, EditAnywhere, Category = "Http Endpoints")
    FString SizeUrl;

    UPROPERTY(Config, EditAnywhere, Category = "Http Endpoints")
    FString PathUrl;

    UPROPERTY(Config, EditAnywhere, Category = "Http Endpoints")
    FString LogUrl;
};