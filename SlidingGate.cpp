#include "SlidingGate.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/BoxComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "Engine/StaticMesh.h"


ASlidingGate::ASlidingGate()
{
	PrimaryActorTick.bCanEverTick = true;

	// Create the root component.
	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	// Create the frame.
	Frame = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Frame"));
	Frame->SetupAttachment(SceneRoot);

	// Load the frame mesh from the specified path.
	static ConstructorHelpers::FObjectFinder<UStaticMesh> FrameMesh(
		TEXT("/Game/SciFiDoors/Meshes/SM_Frame.SM_Frame")
	);

	if (FrameMesh.Succeeded())
	{
		Frame->SetStaticMesh(FrameMesh.Object);
	}

	// Create the left door panel and attach it to the root.
	LeftDoor = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LeftDoor"));
	LeftDoor->SetupAttachment(SceneRoot);
	LeftDoor->SetMobility(EComponentMobility::Movable);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> LeftDoorMesh(TEXT("/Game/SciFiDoors/Meshes/SM_DoorLeft.SM_DoorLeft"));

	if (LeftDoorMesh.Succeeded())
	{
		LeftDoor->SetStaticMesh(LeftDoorMesh.Object);
	}

	// Create the right door panel and attach it to the root.
	RightDoor = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RightDoor"));
	RightDoor->SetupAttachment(SceneRoot);
	RightDoor->SetMobility(EComponentMobility::Movable);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> RightDoorMesh(TEXT("/Game/SciFiDoors/Meshes/SM_DoorRight.SM_DoorRight"));

	if (RightDoorMesh.Succeeded())
	{
		RightDoor->SetStaticMesh(RightDoorMesh.Object);
	}

	// Create the player detection area.
	TriggerBox = CreateDefaultSubobject<UBoxComponent>(TEXT("TriggerBox"));
	TriggerBox->SetupAttachment(SceneRoot);
	TriggerBox->SetBoxExtent(DetectionSize * 0.5f);
	TriggerBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	TriggerBox->SetCollisionResponseToAllChannels(ECR_Ignore);
	TriggerBox->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	TriggerBox->SetGenerateOverlapEvents(true);

}

void ASlidingGate::BeginPlay()
{
	Super::BeginPlay();

	// Store the initial positions of both door panels.
	LeftDoorClosedLocation = LeftDoor->GetRelativeLocation();
	RightDoorClosedLocation = RightDoor->GetRelativeLocation();
}

void ASlidingGate::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	const float Step = DeltaTime / FMath::Max(OpenDuration, 0.1f);

	if (bShouldOpen)
	{
		OpenAmount += Step;
	}
	else
	{
		OpenAmount -= Step;
	}

	OpenAmount = FMath::Clamp(OpenAmount, 0.f, 1.f);


	FVector LeftDoorOpenPosition = LeftDoorClosedLocation + FVector(0.f, -SlideDistance, 0.f);
	FVector RightDoorOpenPosition = RightDoorClosedLocation + FVector(0.f, SlideDistance, 0.f);

	LeftDoor->SetRelativeLocation(
		FMath::Lerp(LeftDoorClosedLocation, LeftDoorOpenPosition, OpenAmount)
	);

	RightDoor->SetRelativeLocation(
		FMath::Lerp(RightDoorClosedLocation, RightDoorOpenPosition, OpenAmount)
	);

	// Get the pawn controlled by the first local player.
	APawn* Player = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);

	const bool bPlayerInside =
		Player != nullptr && TriggerBox->IsOverlappingActor(Player);

	if (bPlayerInside)
	{
		// Cancel closing when the player is inside.
		GetWorldTimerManager().ClearTimer(CloseTimerHandle);
		OpenDoor();
	}
	else if (bShouldOpen && !GetWorldTimerManager().IsTimerActive(CloseTimerHandle))
	{
		if (CloseDelay <= 0.f)
		{
			CloseDoor();
		}
		else
		{
			GetWorldTimerManager().SetTimer(
				CloseTimerHandle,
				this,
				&ASlidingGate::CloseDoor,
				CloseDelay,
				false
			);
		}
	}

}

void ASlidingGate::OpenDoor()
{
	// Ignore repeated requests to open.
	if (bShouldOpen)
	{
		return;
	}

	bShouldOpen = true;

	if (OpenGateSound)
	{
		UGameplayStatics::PlaySoundAtLocation(
			this, OpenGateSound, GetActorLocation()
		);
	}
}

void ASlidingGate::CloseDoor()
{

	// Ignore repeated requests to close.
	if (!bShouldOpen)
	{
		return;
	}

	bShouldOpen = false;

	if (CloseGateSound)
	{
		UGameplayStatics::PlaySoundAtLocation(
			this, CloseGateSound, GetActorLocation()
		);
	}
}

void ASlidingGate::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	TriggerBox->SetBoxExtent(DetectionSize * 0.5f);
}