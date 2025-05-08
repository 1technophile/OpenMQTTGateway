/*
  OpenMQTTGateway - Home Assistant Discovery Manager
  
  Main orchestrator for the Home Assistant Discovery system.
  Implements Dependency Inversion Principle.
  
  Copyright: (c) OpenMQTTGateway Contributors
*/

#pragma once

#include <memory>
#include <vector>

#include "../IMqttPublisher.h"
#include "../ISettingsProvider.h"
#include "../core/HassDevice.h"
#include "../core/HassTopicBuilder.h"
#include "../entities/HassEntity.h"

namespace omg {
namespace hass {

/**
 * @brief Main Home Assistant discovery manager
 * 
 * Dependency Inversion Principle: Depends on abstractions (HassEntity interface)
 * rather than concrete implementations
 * 
 * Performance: Manages entity lifecycle efficiently
 * Reliability: Handles errors gracefully and validates all inputs
 * Operational Excellence: Provides comprehensive logging and monitoring
 */
class HassDiscoveryManager {
public:
  /**
     * @brief Constructor with dependency injection
     * @param settingsProvider Settings provider implementation
     * @param mqttPublisher MQTT publisher implementation
     */
  explicit HassDiscoveryManager(ISettingsProvider& settingsProvider,
                                IMqttPublisher& mqttPublisher);

  /**
     * @brief Destructor
     */
  ~HassDiscoveryManager() = default;

  /**
     * @brief Publishes a single entity
     * @param entity Entity to publish
     * @return true if published successfully
     */
  bool publishEntity(std::unique_ptr<HassEntity> entity);

  /**
     * @brief Publishes entities from legacy array format
     * @param entityArray Legacy entity array (13-column format)
     * @param count Number of entities in array
     * @param device Device to associate entities with
     */
  void publishEntityFromArray(const char* entityArray[][13], int count,
                              std::shared_ptr<HassDevice> device);

  /**
     * @brief Erases (removes) an entity from Home Assistant
     * @param componentType Component type (sensor, switch, etc.)
     * @param uniqueId Unique entity identifier
     */
  void eraseEntity(const char* componentType, const char* uniqueId);

  /**
     * @brief Creates or retrieves the gateway device
     * @return Shared pointer to gateway device
     */
  std::shared_ptr<HassDevice> getGatewayDevice();

  /**
     * @brief Creates an external device
     * @param name Device name
     * @param manufacturer Device manufacturer
     * @param model Device model
     * @param identifier Device identifier
     * @return Shared pointer to external device
     */
  std::shared_ptr<HassDevice> createExternalDevice(const char* name,
                                                   const char* manufacturer,
                                                   const char* model,
                                                   const char* identifier);

  /**
     * @brief Gets the topic builder instance
     * @return Reference to topic builder
     */
  const HassTopicBuilder& getTopicBuilder() const {
    return topicBuilder_;
  }

  /**
     * @brief Gets the number of published entities
     * @return Number of entities
     */
  size_t getEntityCount() const {
    return entities_.size();
  }

  /**
     * @brief Clears all entities (for cleanup)
     */
  void clearEntities();

  /**
     * @brief Publishes all entities (refresh)
     */
  void republishAllEntities();

private:
  HassTopicBuilder topicBuilder_; ///< Topic builder instance
  std::shared_ptr<HassDevice> gatewayDevice_; ///< Gateway device instance
  std::vector<std::unique_ptr<HassEntity>> entities_; ///< Managed entities
  ISettingsProvider& settingsProvider_; ///< Settings provider interface
  IMqttPublisher& mqttPublisher_; ///< MQTT publisher interface

  /**
     * @brief Initializes the gateway device
     */
  void initializeGatewayDevice();

  /**
     * @brief Creates entity from legacy array row
     * @param row Single row from legacy entity array
     * @param device Device to associate entity with
     * @return Unique pointer to created entity
     */
  std::unique_ptr<HassEntity> createEntityFromArray(const char* row[13],
                                                    std::shared_ptr<HassDevice> device);

  /**
     * @brief Validates entity before adding
     * @param entity Entity to validate
     * @return true if valid
     */
  bool validateEntity(const HassEntity* entity) const;
};

} // namespace hass
} // namespace omg
