// Fill out your copyright notice in the Description page of Project Settings.

#include "LevelController.h"
#include "Objects/BaseObject.h"

#include "DrawDebugHelpers.h"
#include "TerrainController.h"

void ALevelController::SaveMap() {
	SaveLevelJson();
}

void ALevelController::LoadMap() {
	LoadLevelJson();
}

void ALevelController::ContainerToJson(const UContainerComponent* Container, TSharedRef<TJsonWriter<TCHAR>> JsonWriter) {
	JsonWriter->WriteObjectStart();
	JsonWriter->WriteObjectStart("Container");

	FString Name = GetName();
	JsonWriter->WriteValue("Name", Name);

	JsonWriter->WriteArrayStart("Content");
	int SlotId = 0;
	for (const auto& Stack : Container->Content) {
		if (Stack.Amount > 0) {
			if (Stack.ObjectClass) {
				JsonWriter->WriteObjectStart();
				ASandboxObject* SandboxObject = Cast<ASandboxObject>(Stack.ObjectClass->ClassDefaultObject);
				FString ClassName = SandboxObject->GetClass()->GetName();
				JsonWriter->WriteValue("SlotId", SlotId);
				JsonWriter->WriteValue("Class", ClassName);
				JsonWriter->WriteValue("ClassId", SandboxObject->GetSandboxClassId());
				JsonWriter->WriteValue("TypeId", SandboxObject->GetSandboxClassId());
				JsonWriter->WriteValue("Amount", Stack.Amount);

				JsonWriter->WriteObjectEnd();
			}
		}

		SlotId++;
	}
	JsonWriter->WriteArrayEnd();

	JsonWriter->WriteObjectEnd();
	JsonWriter->WriteObjectEnd();
}

void ALevelController::SaveLevelJsonExt(TSharedRef<TJsonWriter<TCHAR>> JsonWriter) {

	if (Environment) {
		float NewTimeOffset = Environment->GetNewTimeOffset();
		JsonWriter->WriteObjectStart("Environment");
		JsonWriter->WriteValue("RealServerTime", NewTimeOffset);
		JsonWriter->WriteObjectEnd();
	}

	JsonWriter->WriteArrayStart("CharacterList");

	TArray<AActor*> FoundActors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ABaseCharacter::StaticClass(), FoundActors);
	for (const auto& Actor : FoundActors) {
		ABaseCharacter* BaseCharacter = Cast<ABaseCharacter>(Actor);
		if (BaseCharacter) {
			if (BaseCharacter->bNoSerialization) {
				continue;
			}

			JsonWriter->WriteObjectStart();
			JsonWriter->WriteObjectStart("Character");

			FString Label = BaseCharacter->GetName();
			JsonWriter->WriteValue("ActorLabel",Label);

			int TypeId = BaseCharacter->SandboxTypeId;
			JsonWriter->WriteValue("TypeId", TypeId);

			int PlayerId = BaseCharacter->SandboxPlayerId;
			JsonWriter->WriteValue("PlayerId", PlayerId);

			JsonWriter->WriteArrayStart("Location");
			FVector Location = BaseCharacter->GetActorLocation();
			JsonWriter->WriteValue(Location.X);
			JsonWriter->WriteValue(Location.Y);
			JsonWriter->WriteValue(Location.Z);
			JsonWriter->WriteArrayEnd();

			JsonWriter->WriteArrayStart("Rotation");
			FRotator Rotation = BaseCharacter->GetActorRotation();
			JsonWriter->WriteValue(Rotation.Pitch);
			JsonWriter->WriteValue(Rotation.Yaw);
			JsonWriter->WriteValue(Rotation.Roll);
			JsonWriter->WriteArrayEnd();

			JsonWriter->WriteArrayStart("Containers");

			TArray<UContainerComponent*> Components;
			BaseCharacter->GetComponents<UContainerComponent>(Components);

			for (UContainerComponent* Container : Components) {
				ContainerToJson(Container, JsonWriter);
			}

			JsonWriter->WriteArrayEnd();

			JsonWriter->WriteObjectEnd();
			JsonWriter->WriteObjectEnd();
		}
	}
	JsonWriter->WriteArrayEnd();

	JsonWriter->WriteArrayStart("MarkerList");
	/*
	for (TActorIterator<AMarker> ActorItr(GetWorld()); ActorItr; ++ActorItr) {
		AMarker* Marker = Cast<AMarker>(*ActorItr);
		if (Marker) {
			FVector Location = Marker->GetActorLocation();

			JsonWriter->WriteObjectStart();
			JsonWriter->WriteObjectStart("Marker");

			int TypeId = 100;
			JsonWriter->WriteValue("TypeId", TypeId);

			JsonWriter->WriteArrayStart("Location");
			JsonWriter->WriteValue(Location.X);
			JsonWriter->WriteValue(Location.Y);
			JsonWriter->WriteValue(Location.Z);
			JsonWriter->WriteArrayEnd();

			JsonWriter->WriteObjectEnd();
			JsonWriter->WriteObjectEnd();
		}
	}
	*/
	JsonWriter->WriteArrayEnd();
}

void ALevelController::LoadLevelJsonExt(TSharedPtr<FJsonObject> JsonParsed) {
	if (Environment) {
		TSharedPtr<FJsonObject> EnvironmentObjectPtr = JsonParsed->GetObjectField(TEXT("Environment"));
		if (EnvironmentObjectPtr) {
			double NewTimeOffset = EnvironmentObjectPtr->GetNumberField("RealServerTime");
			Environment->SetTimeOffset(NewTimeOffset);
		}
	}

	if (MarkerMap) {
		TArray <TSharedPtr<FJsonValue>>MarkerList = JsonParsed->GetArrayField("MarkerList");
		for (int Idx = 0; Idx < MarkerList.Num(); Idx++) {
			TSharedPtr<FJsonObject> MarkerPtr = MarkerList[Idx]->AsObject();
			TSharedPtr<FJsonObject> SandboxObjectPtr = MarkerPtr->GetObjectField(TEXT("Marker"));

			int TypeId = SandboxObjectPtr->GetIntegerField(TEXT("TypeId"));

			FVector Location;
			TArray <TSharedPtr<FJsonValue>> LocationValArray = SandboxObjectPtr->GetArrayField("Location");
			Location.X = LocationValArray[0]->AsNumber();
			Location.Y = LocationValArray[1]->AsNumber();
			Location.Z = LocationValArray[2]->AsNumber();

			//UE_LOG(LogTemp, Warning, TEXT("Location: %f %f %f"), Location.X, Location.Y, Location.Z);

			if (MarkerMap->MarkerTypeMap.Contains(TypeId)) {
				TSubclassOf<AMarker> Marker = MarkerMap->MarkerTypeMap[TypeId];
				FRotator NewRotation;
				GetWorld()->SpawnActor(Marker, &Location, &NewRotation);
			}
		}
	}

	if (CharacterMap) {
		TArray<TSharedPtr<FJsonValue>>MarkerList = JsonParsed->GetArrayField("CharacterList");
		for (int Idx = 0; Idx < MarkerList.Num(); Idx++) {
			TSharedPtr<FJsonObject> MarkerPtr = MarkerList[Idx]->AsObject();
			TSharedPtr<FJsonObject> CharacterPtr = MarkerPtr->GetObjectField(TEXT("Character"));

			FTempCharacterLoadInfo TempCharacterInfo;

			TempCharacterInfo.TypeId = CharacterPtr->GetIntegerField(TEXT("TypeId"));
			TempCharacterInfo.PlayerId = CharacterPtr->GetIntegerField(TEXT("PlayerId"));

			TArray<TSharedPtr<FJsonValue>> LocationValArray = CharacterPtr->GetArrayField("Location");
			TempCharacterInfo.Location.X = LocationValArray[0]->AsNumber();
			TempCharacterInfo.Location.Y = LocationValArray[1]->AsNumber();
			TempCharacterInfo.Location.Z = LocationValArray[2]->AsNumber();

			TArray<TSharedPtr<FJsonValue>> RotationValArray = CharacterPtr->GetArrayField("Rotation");
			TempCharacterInfo.Rotation.Pitch = RotationValArray[0]->AsNumber();
			TempCharacterInfo.Rotation.Yaw = RotationValArray[1]->AsNumber();
			TempCharacterInfo.Rotation.Roll = RotationValArray[2]->AsNumber();

			TArray<TSharedPtr<FJsonValue>>ContainerList = CharacterPtr->GetArrayField("Containers");
			for (int Idx2 = 0; Idx2 < ContainerList.Num(); Idx2++) {
				TSharedPtr<FJsonObject> ContainerPtr = ContainerList[Idx2]->AsObject();
				TSharedPtr<FJsonObject> Container2Ptr = ContainerPtr->GetObjectField(TEXT("Container"));
				FString Name = Container2Ptr->GetStringField("Name"); // TODO handle container name
				//UContainerComponent* Container = BaseCharacter->GetInventory("Inventory");
				TArray<TSharedPtr<FJsonValue>> ContentArray = Container2Ptr->GetArrayField("Content");

				for (int Idx3 = 0; Idx3 < ContentArray.Num(); Idx3++) {
					TSharedPtr<FJsonObject> ContentPtr = ContentArray[Idx3]->AsObject();
					int SlotId = ContentPtr->GetIntegerField("SlotId");
					int ClassId = ContentPtr->GetIntegerField("ClassId");
					int Amount = ContentPtr->GetIntegerField("Amount");

					UE_LOG(LogTemp, Warning, TEXT("SlotId: %d, ClassId: %d, Amount: %d"), SlotId, ClassId, Amount);

					TSubclassOf<ASandboxObject> ASandboxObjectSubclass = GetSandboxObjectByClassId(ClassId);

					FContainerStack Stack;
					Stack.Amount = Amount;
					Stack.ObjectClass = ASandboxObjectSubclass;

					FTempContainerStack TempContainerStack;
					TempContainerStack.Stack = Stack;
					TempContainerStack.SlotId = SlotId;

					TempCharacterInfo.Inventory.Add(TempContainerStack);
					//Container->AddStack(Stack, SlotId);
				}
			}


			if (CharacterMap->CharacterTypeMap.Contains(TempCharacterInfo.TypeId)) {
				TSubclassOf<ABaseCharacter> BaseCharacterSubclass = CharacterMap->CharacterTypeMap[TempCharacterInfo.TypeId];
				if (BaseCharacterSubclass) {
					TempCharacterList.Add(TempCharacterInfo);
				}
			}
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("TempCharacterList: %d"), TempCharacterList.Num());
}

void ALevelController::SpawnTempCharacterList() {
	for (const FTempCharacterLoadInfo& TempCharacterInfo : TempCharacterList) {
		TSubclassOf<ABaseCharacter> BaseCharacterSubclass = CharacterMap->CharacterTypeMap[TempCharacterInfo.TypeId];

		//DrawDebugPoint(GetWorld(), TempCharacterInfo.Location, 5.f, FColor(255, 255, 255, 0), true);

		FVector Pos(TempCharacterInfo.Location.X, TempCharacterInfo.Location.Y, TempCharacterInfo.Location.Z + 90);// ALS spawn issue woraround
		ABaseCharacter* BaseCharacter = (ABaseCharacter*)GetWorld()->SpawnActor(BaseCharacterSubclass, &Pos, &TempCharacterInfo.Rotation);
		BaseCharacter->SandboxPlayerId = TempCharacterInfo.PlayerId;
		UContainerComponent* Container = BaseCharacter->GetInventory("Inventory");
		if (Container) {
			for (const FTempContainerStack& TempContainerStack : TempCharacterInfo.Inventory) {
				Container->AddStack(TempContainerStack.Stack, TempContainerStack.SlotId);
			}
		}
		
	}
}


const TArray<FTempCharacterLoadInfo>& ALevelController::GetTempCharacterList() const {
	return TempCharacterList;
}

ACharacter* ALevelController::SpawnCharacterByTypeId(const int TypeId, const FVector& Location, const FRotator& Rotation) {
	if (CharacterMap) {
		if (CharacterMap->CharacterTypeMap.Contains(TypeId)) {
			TSubclassOf<ACharacter> CharacterSubclass = CharacterMap->CharacterTypeMap[TypeId];
			ACharacter* Character = (ACharacter*)GetWorld()->SpawnActor(CharacterSubclass, &Location, &Rotation);
			return Character;
		}
	}

	return nullptr;
}

ASandboxObject* ALevelController::SpawnSandboxObject(const int ClassId, const FTransform& Transform) {
	ASandboxObject* Obj = Super::SpawnSandboxObject(ClassId, Transform);
	if (Obj && TerrainController) {
		TerrainController->RegisterSandboxObject(Obj);
	}
	return Obj;
}

void ALevelController::SpawnPreparedObjects(const TArray<FSandboxObjectDescriptor>& ObjDescList) {
	if (TerrainController) {
		for (const auto& ObjDesc : ObjDescList) {
			TerrainController->AddToStash(ObjDesc);
		}
	} else {
		Super::SpawnPreparedObjects(ObjDescList);
	}
}

bool ALevelController::RemoveSandboxObject(ASandboxObject* Obj) {
	bool Res = Super::RemoveSandboxObject(Obj);
	if (Res && TerrainController) {
		TerrainController->UnRegisterSandboxObject(Obj);
		UE_LOG(LogTemp, Log, TEXT("UnregisterSandboxObject"));
	}

	return Res;
}

void ALevelController::PrepareObjectForSave(TArray<FSandboxObjectDescriptor>& ObjDescList) {
	Super::PrepareObjectForSave(ObjDescList);
	if (TerrainController) {
		const TMap<TVoxelIndex, FSandboxObjectsByZone>& Map = TerrainController->GetObjectsByZoneMap();
		for (const auto& Elem : Map) {
			const TVoxelIndex& Index = Elem.Key;
			const FSandboxObjectsByZone& ObjectsByZone = Elem.Value;
			for (auto& Itm : ObjectsByZone.Stash) {
				FString ClassName = Itm.Key;
				const FSandboxObjectDescriptor& ObjDesc = Itm.Value;
				ObjDescList.Add(ObjDesc);
			}
		}
	} 
}