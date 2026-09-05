#include "UnrealEngine.h"
#include "Scanner.h"
#include <iostream>

EngineIntrospection g_EngineIntrospection;

bool EngineIntrospection::Initialize() {
    if (m_initialized) return true;

    HMODULE hGame = GetModuleHandle(nullptr);

    // 1. GUObjectArray RIP-relative pattern scan
    uintptr_t gObjectsInst = FindPattern(hGame, "\x48\x8B\x05\x00\x00\x00\x00\x48\x8B\x0C\xC8", "xxx????xxxx");
    if (gObjectsInst) {
        m_gObjectsAddr = ResolveRIP(gObjectsInst, 3, 7);
        std::cout << "[Reflection] Resolved GUObjectArray: 0x" << std::hex << m_gObjectsAddr << std::dec << std::endl;
    }

    // 2. FNamePool RIP-relative pattern scan
    uintptr_t fNamePoolInst = FindPattern(hGame, "\x48\x8D\x0D\x00\x00\x00\x00\xE8\x00\x00\x00\x00\x4C\x8B\xC0", "xxx????x????xxx");
    if (fNamePoolInst) {
        m_fNamePoolAddr = ResolveRIP(fNamePoolInst, 3, 7);
        std::cout << "[Reflection] Resolved FNamePool: 0x" << std::hex << m_fNamePoolAddr << std::dec << std::endl;
    }

    // 3. ProcessEvent signature scan
    m_processEventAddr = FindPattern(hGame, "\x40\x55\x56\x57\x41\x54\x41\x55\x41\x56\x41\x57\x48\x81\xEC", "xxxxxxxxxxxxxxx");
    if (m_processEventAddr) {
        std::cout << "[Reflection] Resolved ProcessEvent: 0x" << std::hex << m_processEventAddr << std::dec << std::endl;
    }

    m_initialized = (m_gObjectsAddr != 0 && m_fNamePoolAddr != 0);
    return m_initialized;
}

std::string EngineIntrospection::GetFNameString(UE427::FName name) const {
    if (!m_fNamePoolAddr || name.ComparisonIndex == 0) return "None";

    uint32_t blockIndex = name.ComparisonIndex >> 16;
    uint16_t offset = static_cast<uint16_t>(name.ComparisonIndex & 0xFFFF);

    uintptr_t blocksAddr = m_fNamePoolAddr + 0x10;
    uintptr_t blockPtr = *reinterpret_cast<uintptr_t*>(blocksAddr + (blockIndex * sizeof(uintptr_t)));
    if (!blockPtr) return "None";

    uintptr_t entryAddr = blockPtr + (offset * 0x02);
    uint16_t header = *reinterpret_cast<uint16_t*>(entryAddr);
    uint32_t len = header >> 6;

    if (len == 0 || len > 256) return "None";

    char nameBuf[256] = { 0 };
    memcpy(nameBuf, reinterpret_cast<const void*>(entryAddr + 2), len);

    std::string result(nameBuf);
    if (name.Number > 0) {
        result += "_" + std::to_string(name.Number - 1);
    }
    return result;
}

std::vector<DiscoveredActor> EngineIntrospection::ScanAndAssignActorIDs() {
    std::vector<DiscoveredActor> results;
    if (!m_initialized) return results;

    auto* pGObjects = reinterpret_cast<UE427::TUObjectArray*>(m_gObjectsAddr);
    if (!pGObjects || !pGObjects->Objects) return results;

    for (int32_t i = 0; i < pGObjects->NumElements; ++i) {
        int32_t chunkIndex = i / 65536;
        int32_t withinChunkIndex = i % 65536;

        if (chunkIndex >= pGObjects->NumChunks) break;
        UE427::FUObjectItem* chunk = pGObjects->Objects[chunkIndex];
        if (!chunk) continue;

        uintptr_t pObject = reinterpret_cast<uintptr_t>(chunk[withinChunkIndex].Object);
        if (!pObject) continue;

        UE427::FName objectName = *reinterpret_cast<UE427::FName*>(pObject + 0x18);
        std::string strName = GetFNameString(objectName);

        // Filter for active gameplay targets in room scenes
        if (strName.find("Baby") != std::string::npos ||
            strName.find("Bottle") != std::string::npos ||
            strName.find("Chair") != std::string::npos) {

            if (m_actorToIDMap.find(pObject) == m_actorToIDMap.end()) {
                m_actorToIDMap[pObject] = m_nextID++;
                if (m_nextID == 0) m_nextID = 1;
            }

            results.push_back({ pObject, strName, m_actorToIDMap[pObject] });
        }
    }
    return results;
}

int32_t EngineIntrospection::GetPropertyOffset(uintptr_t uClassAddr, const std::string& propertyName) const {
    if (!uClassAddr) return -1;

    uintptr_t propertyLink = *reinterpret_cast<uintptr_t*>(uClassAddr + 0x50);
    while (propertyLink) {
        UE427::FName propFName = *reinterpret_cast<UE427::FName*>(propertyLink + 0x28);
        std::string propNameStr = GetFNameString(propFName);

        if (propNameStr == propertyName) {
            return *reinterpret_cast<int32_t*>(propertyLink + 0x4C);
        }
        propertyLink = *reinterpret_cast<uintptr_t*>(propertyLink + 0x20);
    }
    return -1;
}

bool EngineIntrospection::CallProcessEvent(uintptr_t pUObject, uintptr_t pUFunction, void* pParams) const {
    if (!m_processEventAddr || !pUObject || !pUFunction) return false;

    typedef void(__fastcall* PFN_ProcessEvent)(void* Object, void* Function, void* Params);
    auto ProcessEventFunc = reinterpret_cast<PFN_ProcessEvent>(m_processEventAddr);

    ProcessEventFunc(reinterpret_cast<void*>(pUObject), reinterpret_cast<void*>(pUFunction), pParams);
    return true;
}