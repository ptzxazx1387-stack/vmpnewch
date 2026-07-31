#pragma once
#include <cstdint>
#include <vector>
#include "entity.h"

namespace GameContext {
    void UpdateEntities();
    EntityBase* GetLocalPlayer();
    std::vector<EntityBase>& GetAllEntities();
}