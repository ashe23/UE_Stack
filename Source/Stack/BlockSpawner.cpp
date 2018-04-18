// Fill out your copyright notice in the Description page of Project Settings.

#include "BlockSpawner.h"
#include "Tile.h"

#include "Components/ArrowComponent.h"
#include "Runtime/Engine/Classes/GameFramework/SpringArmComponent.h"
#include "Runtime/Engine/Classes/Camera/CameraComponent.h"
#include "Runtime/Engine/Classes/Components/InputComponent.h"
#include "Runtime/Engine/Public/Rendering/PositionVertexBuffer.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Engine/Engine.h"


// Sets default values
ABlockSpawner::ABlockSpawner()
{
 	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	ZOffset = 10.0f;
	SpawnScale = FVector{ 1.f };

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));

	// Setting default spawn points (Arrow components)
	RightSpawnPoint = CreateDefaultSubobject<UArrowComponent>(TEXT("RightSpawnPoint"));
	RightSpawnPoint->SetRelativeLocation(FVector{0, -500.0f, 0});
	RightSpawnPoint->SetRelativeRotation(FRotator{0, 90.0f, 0 });

	LeftSpawnPoint = CreateDefaultSubobject<UArrowComponent>(TEXT("LeftSpawnPoint"));
	LeftSpawnPoint->SetRelativeLocation(FVector{500.0f, 0, 0});
	LeftSpawnPoint->SetRelativeRotation(FRotator{0, 180.0f, 0.0f});

	RightSpawnPoint->SetupAttachment(RootComponent);
	LeftSpawnPoint->SetupAttachment(RootComponent);

	RightSpawnPoint->bHiddenInGame = false;
	LeftSpawnPoint->bHiddenInGame = false;

	OurCameraSpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraSpringArm"));
	OurCameraSpringArm->SetupAttachment(RootComponent);
	OurCameraSpringArm->TargetArmLength = 400.f;
	OurCameraSpringArm->bEnableCameraLag = true;
	OurCameraSpringArm->CameraLagSpeed = 3.0f;
	OurCameraSpringArm->bDoCollisionTest = false;
	OurCameraSpringArm->SetRelativeRotation(FRotator{-30.0f, -45.0f, 0.0f});

	OurCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("GameCamera"));
	OurCamera->SetupAttachment(OurCameraSpringArm, USpringArmComponent::SocketName);

	//Take control of the default Player
	AutoPossessPlayer = EAutoReceiveInput::Player0;
}

// Called when the game starts or when spawned
void ABlockSpawner::BeginPlay()
{
	Super::BeginPlay();

	// Spawning One Tile for Prev
	auto World = GetWorld();

	if (!World)
	{
		UE_LOG(LogTemp, Error, TEXT("Cant find World!"));
		return;
	}

	FTransform FirstTileTransform{ FVector{0, 0, -10.0f} };
	PreviousTile = World->SpawnActor<ATile>(ATile::StaticClass(), FirstTileTransform);
	PreviousTile->Speed = 0;


	SpawnTile();
	
}

void ABlockSpawner::SetTileCallback()
{	
	// Moving our Pawn Root Component Up By Zoffset
	AddActorWorldOffset(FVector{ 0, 0, ZOffset });

	// Calculate Tile Scale
	SetCurrentTileLocation();
	SetCurrentTileScale();

	PreviousTile = CurrentTile;
	CurrentTile->Speed = 0;

	UpdateArrowLocations();

	// Spawning New Tile with old scales
	SpawnTile();
}

void ABlockSpawner::CalcTilesIntersection()
{
	

	
}

// Called every frame
void ABlockSpawner::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

// Called to bind functionality to input
void ABlockSpawner::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAction("SetTile", IE_Pressed, this, &ABlockSpawner::SetTileCallback);
}

void ABlockSpawner::SpawnTile()
{		
	auto World = GetWorld();

	if (!World)
	{
		UE_LOG(LogTemp, Error, TEXT("Cant find World!"));
		return;
	}

	// should spawn based on previuos tile scale and location
	if (bIsRightTurn)
	{
		CurrentTile = World->SpawnActor<ATile>(ATile::StaticClass(), RightSpawnPoint->GetComponentTransform());
		if (CurrentTile)
		{
			CurrentTile->SetActorScale3D(SpawnScale);
			CurrentTile->MoveDirection = FVector{ 0, 1.0f, 0 };
			CurrentTile->StartPosition = RightSpawnPoint->GetComponentTransform().GetLocation();
			CurrentTile->EndPosition = CurrentTile->StartPosition + CurrentTile->MoveDirection * CurrentTile->ReverseDistance;
			CurrentTile->CurrentDestLocation = CurrentTile->EndPosition;
		}
	}
	else
	{
		CurrentTile = World->SpawnActor<ATile>(ATile::StaticClass(), LeftSpawnPoint->GetComponentTransform());
		if (CurrentTile)
		{
			CurrentTile->SetActorScale3D(SpawnScale);
			CurrentTile->MoveDirection = FVector{ -1.0f, 0, 0 };
			CurrentTile->StartPosition = LeftSpawnPoint->GetComponentTransform().GetLocation();
			CurrentTile->EndPosition = CurrentTile->StartPosition + CurrentTile->MoveDirection * CurrentTile->ReverseDistance;
			CurrentTile->CurrentDestLocation = CurrentTile->EndPosition;
		}
	}

	bIsRightTurn = !bIsRightTurn;
}

void ABlockSpawner::SetCurrentTileLocation()
{
	FVector NewCenter;

	if (bIsRightTurn)
	{
		NewCenter.X = CurrentTile->GetActorLocation().X;
		NewCenter.Y = (CurrentTile->GetActorLocation().Y - PreviousTile->GetActorLocation().Y) / 2;
	}
	else
	{
		NewCenter.X = (PreviousTile->GetActorLocation().X - CurrentTile->GetActorLocation().X) / 2;
		NewCenter.Y = CurrentTile->GetActorLocation().Y;

	}
	NewCenter.Z = CurrentTile->GetActorLocation().Z;

	CurrentTile->SetActorLocation(NewCenter);
}

void ABlockSpawner::SetCurrentTileScale()
{
	FVector NewScale;
	if (bIsRightTurn)
	{
		NewScale.X = PreviousTile->GetActorScale3D().X;
		NewScale.Y = 1 - FMath::Abs(((CurrentTile->GetActorLocation().Y - PreviousTile->GetActorLocation().Y) / 100));
	}
	else
	{
		NewScale.X = 1 - FMath::Abs(((PreviousTile->GetActorLocation().X - CurrentTile->GetActorLocation().X) / 100));
		NewScale.Y = PreviousTile->GetActorScale3D().Y;
	}
	NewScale.Z = PreviousTile->GetActorScale3D().Z;

	SpawnScale.X = NewScale.Y;
	SpawnScale.Y = NewScale.X;
	SpawnScale.Z = NewScale.Z;

	CurrentTile->SetActorScale3D(NewScale);

}

void ABlockSpawner::UpdateArrowLocations()
{
	if (bIsRightTurn)
	{
		FVector NewLoc = RightSpawnPoint->GetComponentLocation();
		NewLoc.X = PreviousTile->GetActorLocation().X;

		RightSpawnPoint->SetWorldLocation(NewLoc);
	}
	else
	{
		FVector NewLoc = LeftSpawnPoint->GetComponentLocation();
		NewLoc.Y = PreviousTile->GetActorLocation().Y;

		LeftSpawnPoint->SetWorldLocation(NewLoc);
	}
}

