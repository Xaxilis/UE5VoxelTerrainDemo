//
//  TerrainData.h
//  UE4VoxelTerrain
//
//  Created by blackw2012 on 19.04.2020..
//

#pragma once

#include "EngineMinimal.h"
#include "VoxelIndex.h"
#include "VoxelData.h"
#include <unordered_map>
#include <mutex>
#include <shared_mutex>
#include <memory>
#include <atomic>
#include <unordered_set>


class TTerrainData {
    
private:


	std::shared_timed_mutex StorageMapMutex;
	std::unordered_map<TVoxelIndex, std::shared_ptr<TVoxelDataInfo>> StorageMap;

	std::shared_timed_mutex SaveIndexSetpMutex;
	std::unordered_set<TVoxelIndex> SaveIndexSet;

    
public:

	void AddSaveIndex(const TVoxelIndex& Index) {
		std::unique_lock<std::shared_timed_mutex> Lock(SaveIndexSetpMutex);
		SaveIndexSet.insert(Index);
	}

	std::unordered_set<TVoxelIndex> PopSaveIndexSet() {
		std::unique_lock<std::shared_timed_mutex> Lock(SaveIndexSetpMutex);
		std::unordered_set<TVoxelIndex> Res = SaveIndexSet;
		SaveIndexSet.clear();
		return Res;
	}

	//=====================================================================================
	// instance objects 
	//=====================================================================================

	std::shared_ptr<TInstanceMeshTypeMap> GetOrCreateInstanceObjectMap(const TVoxelIndex& Index) {
		return GetVoxelDataInfo(Index)->GetOrCreateInstanceObjectMap();
	}

	//=====================================================================================
	// terrain zone mesh 
	//=====================================================================================

	void PutMeshDataToCache(const TVoxelIndex& Index, TMeshDataPtr MeshDataPtr) {
		GetVoxelDataInfo(Index)->PushMeshDataCache(MeshDataPtr);
	}

	//=====================================================================================
	// terrain zone 
	//=====================================================================================

    void AddZone(const TVoxelIndex& Index, UTerrainZoneComponent* ZoneComponent){
		GetVoxelDataInfo(Index)->AddZone(ZoneComponent);
    }
    
    UTerrainZoneComponent* GetZone(const TVoxelIndex& Index){
        return GetVoxelDataInfo(Index)->GetZone();
    }

	void RemoveZone(const TVoxelIndex& Index) {
		auto Ptr = GetVoxelDataInfo(Index);
		Ptr->ResetSpawnFinished();
		Ptr->RemoveZone();
	}
    
	//=====================================================================================
	// terrain voxel data 
	//=====================================================================================
    
	TVoxelDataInfoPtr GetVoxelDataInfo(const TVoxelIndex& Index) {
		std::unique_lock<std::shared_timed_mutex> Lock(StorageMapMutex);
		if (StorageMap.find(Index) != StorageMap.end()) {
			return StorageMap[Index];
		} else {
			TVoxelDataInfoPtr NewVdinfo = std::make_shared<TVoxelDataInfo>();
			StorageMap.insert({ Index, NewVdinfo });
			return NewVdinfo;
		}
    }

	//=====================================================================================
	// clean all 
	//=====================================================================================

    void Clean(){
		// no locking because end play only
		StorageMap.clear();
    }
};

