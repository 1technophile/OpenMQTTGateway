/*
  OpenMQTTGateway - Home Assistant Entity Base Class
  
  Base class for all Home Assistant entities.
  
  Copyright: (c) OpenMQTTGateway Contributors
*/

#pragma once

#include <ArduinoJson.h>
#include <MSG_defaults.h>

#include <memory>
#include <string>

#include "../IMqttPublisher.h"
#include "../core/HassDevice.h"
#include "../core/HassTopicBuilder.h"

namespace omg {
namespace hass {

/**
 * @brief Base class for all Home Assistant entities
 * 
 * Open/Closed Principle: Open for extension (new entity types), 
 * closed for modification (base functionality is stable)
 * 
 * Liskov Substitution Principle: All derived entities can be used 
 * interchangeably through this interface
 */
class HassEntity {
public:
  /**
     * @brief Entity configuration structure
     */
  struct EntityConfig {
    std::string componentType; ///< HA component type (sensor, switch, etc.)
    std::string name; ///< Entity display name
    std::string uniqueId; ///< Unique entity identifier
    std::string deviceClass; ///< HA device class
    std::string valueTemplate; ///< Jinja2 template for value extraction
    std::string unitOfMeasurement; ///< Measurement unit
    std::string stateClass; ///< HA state class (measurement, etc.)
    std::string stateTopic; ///< Custom state topic
    std::string commandTopic; ///< Custom command topic
    std::string availabilityTopic; ///< Custom availability topic
    bool isDiagnostic = false; ///< Whether entity is diagnostic
    int offDelay = 0; ///< Off delay in seconds
    bool retain = false; ///< Whether to retain commands

    /**
         * @brief Default constructor
         */
    EntityConfig() = default;

    /**
         * @brief Validates the entity configuration
         * @return true if valid, false otherwise
         */
    bool isValid() const;

    /**
         * @brief Creates a basic sensor configuration
         * @param name Entity name
         * @param uniqueId Unique identifier
         * @param deviceClass Device class (optional)
         * @param unit Unit of measurement (optional)
         * @return Configured EntityConfig for sensor
         */
    static EntityConfig createSensor(const std::string& name,
                                     const std::string& uniqueId,
                                     const std::string& deviceClass = "",
                                     const std::string& unit = "");

    /**
         * @brief Creates a basic switch configuration
         * @param name Entity name
         * @param uniqueId Unique identifier
         * @return Configured EntityConfig for switch
         */
    static EntityConfig createSwitch(const std::string& name,
                                     const std::string& uniqueId);

    /**
         * @brief Creates a basic button configuration
         * @param name Entity name
         * @param uniqueId Unique identifier
         * @return Configured EntityConfig for button
         */
    static EntityConfig createButton(const std::string& name,
                                     const std::string& uniqueId);
  };

  /**
     * @brief Constructor with entity configuration and device
     * @param config Entity configuration
     * @param device Associated device
     */
  explicit HassEntity(const EntityConfig& config, std::shared_ptr<HassDevice> device);

  /**
     * @brief Virtual destructor for proper inheritance
     */
  virtual ~HassEntity() = default;

  /**
     * @brief Publishes entity discovery message to MQTT
     * @param topicBuilder Topic builder for generating MQTT topics
     * @param publisher MQTT publisher for publishing messages
     * @return true if published successfully, false otherwise
     */
  virtual bool publish(const HassTopicBuilder& topicBuilder, IMqttPublisher& publisher) const;

  /**
     * @brief Erases (removes) entity from Home Assistant
     * @param topicBuilder Topic builder for generating MQTT topics
     * @param publisher MQTT publisher for publishing messages
     * @return true if erased successfully, false otherwise
     */
  virtual bool erase(const HassTopicBuilder& topicBuilder, IMqttPublisher& publisher) const;

  /**
     * @brief Gets the entity configuration
     * @return Reference to entity configuration
     */
  const EntityConfig& getConfig() const { return config_; }

  /**
     * @brief Gets the associated device
     * @return Shared pointer to device
     */
  std::shared_ptr<HassDevice> getDevice() const { return device_; }

  /**
     * @brief Updates the entity configuration
     * @param config New configuration
     * @return true if update successful, false otherwise
     */
  virtual bool updateConfig(const EntityConfig& config);

  /**
     * @brief Gets the discovery topic for this entity
     * @param topicBuilder Topic builder
     * @return Discovery topic string
     */
  std::string getDiscoveryTopic(const HassTopicBuilder& topicBuilder) const;

protected:
  EntityConfig config_; ///< Entity configuration
  std::shared_ptr<HassDevice> device_; ///< Associated device

  /**
     * @brief Adds entity-specific fields to JSON (pure virtual)
     * @param json JSON object to populate
     * @param topicBuilder Topic builder for generating topics
     */
  virtual void addSpecificFields(JsonObject& json, const HassTopicBuilder& topicBuilder) const = 0;

  /**
     * @brief Validates entity configuration
     * @throws std::invalid_argument if configuration is invalid
     */
  virtual void validateConfig() const;

  /**
     * @brief Adds common entity fields to JSON
     * @param json JSON object to populate
     * @param topicBuilder Topic builder for generating topics
     */
  void addCommonFields(JsonObject& json, const HassTopicBuilder& topicBuilder) const;

  /**
     * @brief Adds device information to JSON
     * @param json JSON object to populate
     */
  void addDeviceInfo(JsonObject& json) const;

  /**
     * @brief Creates the JSON discovery message
     * @param topicBuilder Topic builder for generating topics
     * @param publisher MQTT publisher for generating unique IDs
     * @return JSON document with discovery message
     */
  StaticJsonDocument<JSON_MSG_BUFFER> createDiscoveryMessage(const HassTopicBuilder& topicBuilder, IMqttPublisher& publisher) const;
};

} // namespace hass
} // namespace omg
