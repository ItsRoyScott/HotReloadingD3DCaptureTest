#pragma once
#include <windows.h>
#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>

namespace UE427 {
    struct FName {
        uint32_t ComparisonIndex;
        uint32_t Number;
    };

    struct FUObjectItem {
        void* Object;
        int32_t Flags;
        int32_t ClusterRootIndex;
        int32_t SerialNumber;
    };

    struct TUObjectArray {
        FUObjectItem** Objects;
        FUObjectItem* PreAllocatedObjects;
        int32_t MaxElements;
        int32_t NumElements;
        int32_t MaxChunks;
        int32_t NumChunks;
    };
}

struct DiscoveredActor {
    uintptr_t address;
    std::string name;
    uint8_t assignedStencilID;
};

class EngineIntrospection {
private:
    uintptr_t m_gObjectsAddr = 0;
    uintptr_t m_fNamePoolAddr = 0;
    uintptr_t m_processEventAddr = 0;
    bool m_initialized = false;
    std::unordered_map<uintptr_t, uint8_t> m_actorToIDMap;
    uint8_t m_nextID = 1;

public:
    bool Initialize();
    std::string GetFNameString(UE427::FName name) const;
    std::vector<DiscoveredActor> ScanAndAssignActorIDs();
    int32_t GetPropertyOffset(uintptr_t uClassAddr, const std::string& propertyName) const;
    bool CallProcessEvent(uintptr_t pUObject, uintptr_t pUFunction, void* pParams) const;
};

extern EngineIntrospection g_EngineIntrospection;