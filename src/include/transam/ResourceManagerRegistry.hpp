#pragma once

#include <array>
#include <stdexcept>

#include "transam/IResourceManager.hpp"
#include "transam/ResourceManagerId.hpp"

namespace mi::transam {
class ResourceManagerRegistry {
  public:
    static constexpr const int MaxResourceManagers = 128;

  private:
    // Resource managers
    std::array<IResourceManager *, MaxResourceManagers> _managers;

  public:
    IResourceManager &GetManager(ResourceManagerId rmgrId) {
        auto manager = this->_managers[static_cast<uint32_t>(rmgrId)];
        if (manager == nullptr) {
            throw std::runtime_error("No resource manager registered");
        }

        return *manager;
    }
    void RegisterManager(ResourceManagerId rmgrId, IResourceManager *manager) {
        if (this->_managers[static_cast<uint32_t>(rmgrId)]) {
            throw std::runtime_error("Resource manager already exists");
        }
        
        this->_managers[static_cast<uint32_t>(rmgrId)] = manager;
    }
};
} // namespace mi::transam
