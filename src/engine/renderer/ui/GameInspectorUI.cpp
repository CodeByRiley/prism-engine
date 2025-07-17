#include "GameInspectorUI.h"
#include <engine/utils/Logger.h>
#include <engine/scene/component/CommonComponents.h>
#include <cstring>
#include <algorithm>
#include <cmath>

// ImGui includes
#include "imgui.h"

// Remove the old interfaces - we'll include the actual components
// Define component interfaces for the custom game components
// This allows us to handle them without circular dependencies
struct PlayerComponentInterface {
    float speed;
    float directionX, directionY;
    float sizeX, sizeY;
};

struct ObstacleComponentInterface {
    float sizeX, sizeY;
};

struct InputComponentInterface {
    bool enabled;
};

GameInspectorUI::GameInspectorUI() : InspectorUI() {
    // Constructor
}

void GameInspectorUI::Render(Scene* scene) {
    // Call the base class render method
    InspectorUI::Render(scene);
}

void GameInspectorUI::DrawEntityInspector(Entity& entity) {
    EntityID entityID = entity.GetID();
    ImGui::Text("Entity ID: %u", entityID);
    
    // Entity actions
    if (ImGui::Button("Destroy Entity")) {
        if (m_entityDestructionCallback) {
            m_entityDestructionCallback(entityID);
            m_selectedEntityID = INVALID_ENTITY_ID;
            return;
        } else {
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "(Callback not set)");
        }
    }
    
    ImGui::Separator();
    ImGui::Text("Components:");
    
    // First draw the standard components using the base class method
    InspectorUI::DrawEntityInspector(entity);
    
    // Then draw the game-specific components
    DrawGameComponents(entity);
}

void GameInspectorUI::DrawGameComponents(Entity& entity) {
    // Handle custom game components
    ImGui::Separator();
    ImGui::Text("Game Components:");
    
    // Light Component - Advanced editing
    if (auto* lightComp = entity.GetComponent<LightComponent>()) {
        if (ImGui::CollapsingHeader("LightComponent")) {
            Light& light = lightComp->light;
            
            // Light Type Selection
            ImGui::Text("Light Type");
            const char* lightTypes[] = { "Point Light", "Directional Light", "Spot Light" };
            int currentType = static_cast<int>(light.type);
            if (ImGui::Combo("Type", &currentType, lightTypes, IM_ARRAYSIZE(lightTypes))) {
                light.type = static_cast<LightType>(currentType);
            }
            
            ImGui::Separator();
            
            // Color controls with multiple options
            ImGui::Text("Light Color");
            ImGui::ColorEdit3("RGB Color", &light.color.x, ImGuiColorEditFlags_DisplayRGB | ImGuiColorEditFlags_InputRGB | ImGuiColorEditFlags_PickerHueWheel);
            
            // Color presets
            if (ImGui::Button("White")) light.color = glm::vec3(1.0f, 1.0f, 1.0f);
            ImGui::SameLine();
            if (ImGui::Button("Warm White")) light.color = glm::vec3(1.0f, 0.95f, 0.8f);
            ImGui::SameLine();
            if (ImGui::Button("Cool White")) light.color = glm::vec3(0.8f, 0.9f, 1.0f);
            
            if (ImGui::Button("Red")) light.color = glm::vec3(1.0f, 0.0f, 0.0f);
            ImGui::SameLine();
            if (ImGui::Button("Green")) light.color = glm::vec3(0.0f, 1.0f, 0.0f);
            ImGui::SameLine();
            if (ImGui::Button("Blue")) light.color = glm::vec3(0.0f, 0.0f, 1.0f);
            
            if (ImGui::Button("Orange")) light.color = glm::vec3(1.0f, 0.5f, 0.0f);
            ImGui::SameLine();
            if (ImGui::Button("Purple")) light.color = glm::vec3(0.5f, 0.0f, 1.0f);
            ImGui::SameLine();
            if (ImGui::Button("Yellow")) light.color = glm::vec3(1.0f, 1.0f, 0.0f);
            
            ImGui::Separator();
            
            // Intensity controls
            ImGui::Text("Light Intensity");
            ImGui::SliderFloat("Intensity", &light.intensity, 0.0f, 10.0f, "%.2f");
            
            // Intensity presets
            if (ImGui::Button("Dim (0.5)")) light.intensity = 0.5f;
            ImGui::SameLine();
            if (ImGui::Button("Normal (1.0)")) light.intensity = 1.0f;
            ImGui::SameLine();
            if (ImGui::Button("Bright (2.0)")) light.intensity = 2.0f;
            ImGui::SameLine();
            if (ImGui::Button("Very Bright (5.0)")) light.intensity = 5.0f;
            
            // Fine-tune intensity with +/- buttons
            if (ImGui::Button("-0.1")) light.intensity = std::max(0.0f, light.intensity - 0.1f);
            ImGui::SameLine();
            if (ImGui::Button("+0.1")) light.intensity = std::min(10.0f, light.intensity + 0.1f);
            ImGui::SameLine();
            if (ImGui::Button("-0.5")) light.intensity = std::max(0.0f, light.intensity - 0.5f);
            ImGui::SameLine();
            if (ImGui::Button("+0.5")) light.intensity = std::min(10.0f, light.intensity + 0.5f);
            
            ImGui::Separator();
            
            // Position controls (for Point and Spot lights)
            if (light.type != LightType::DIRECTIONAL_LIGHT) {
                ImGui::Text("Light Position");
                ImGui::DragFloat2("Position", &light.position.x, 1.0f);
            }
            
            // Direction controls (for Directional and Spot lights)
            if (light.type == LightType::DIRECTIONAL_LIGHT || light.type == LightType::SPOT_LIGHT) {
                ImGui::Text("Light Direction");
                ImGui::DragFloat2("Direction", &light.direction.x, 0.01f, -1.0f, 1.0f);
                
                // Normalize direction button
                if (ImGui::Button("Normalize Direction")) {
                    light.direction = glm::normalize(light.direction);
                }
            }
            
            // Range controls (for Point and Spot lights)
            if (light.type != LightType::DIRECTIONAL_LIGHT) {
                ImGui::Text("Light Range");
                ImGui::DragFloat("Range", &light.range, 10.0f, 10.0f, 5000.0f, "%.1f");
                
                // Range presets
                if (ImGui::Button("Small (100)")) light.range = 100.0f;
                ImGui::SameLine();
                if (ImGui::Button("Medium (500)")) light.range = 500.0f;
                ImGui::SameLine();
                if (ImGui::Button("Large (1000)")) light.range = 1000.0f;
                ImGui::SameLine();
                if (ImGui::Button("Huge (2000)")) light.range = 2000.0f;
            }
            
            // Spot light specific controls
            if (light.type == LightType::SPOT_LIGHT) {
                ImGui::Separator();
                ImGui::Text("Spot Light Cone");
                ImGui::SliderFloat("Inner Angle", &light.innerAngle, 0.0f, 3.14159f, "%.3f rad");
                ImGui::SliderFloat("Outer Angle", &light.outerAngle, 0.0f, 3.14159f, "%.3f rad");
                
                // Ensure inner angle <= outer angle
                if (light.innerAngle > light.outerAngle) {
                    light.innerAngle = light.outerAngle;
                }
                
                // Angle presets
                if (ImGui::Button("Narrow (15°)")) {
                    light.innerAngle = 0.2f;
                    light.outerAngle = 0.26f;
                }
                ImGui::SameLine();
                if (ImGui::Button("Medium (45°)")) {
                    light.innerAngle = 0.6f;
                    light.outerAngle = 0.78f;
                }
                ImGui::SameLine();
                if (ImGui::Button("Wide (90°)")) {
                    light.innerAngle = 1.3f;
                    light.outerAngle = 1.57f;
                }
            }
            
            ImGui::Separator();
            
            // Bloom effect
            ImGui::Text("Bloom Effect");
            ImGui::SliderFloat("Bloom", &light.bloom, 0.0f, 2.0f, "%.2f");
            
            ImGui::Separator();
            
            // Advanced controls
            if (ImGui::TreeNode("Advanced Light Settings")) {
                // Display current values for reference
                ImGui::Text("Current Values:");
                ImGui::Text("Type: %s", lightTypes[static_cast<int>(light.type)]);
                ImGui::Text("Color: (%.2f, %.2f, %.2f)", light.color.r, light.color.g, light.color.b);
                ImGui::Text("Intensity: %.2f", light.intensity);
                if (light.type != LightType::DIRECTIONAL_LIGHT) {
                    ImGui::Text("Position: (%.1f, %.1f)", light.position.x, light.position.y);
                    ImGui::Text("Range: %.1f", light.range);
                }
                if (light.type != LightType::POINT_LIGHT) {
                    ImGui::Text("Direction: (%.2f, %.2f)", light.direction.x, light.direction.y);
                }
                if (light.type == LightType::SPOT_LIGHT) {
                    ImGui::Text("Inner Angle: %.2f° (%.3f rad)", light.innerAngle * 180.0f / 3.14159f, light.innerAngle);
                    ImGui::Text("Outer Angle: %.2f° (%.3f rad)", light.outerAngle * 180.0f / 3.14159f, light.outerAngle);
                }
                ImGui::Text("Bloom: %.2f", light.bloom);
                
                ImGui::Separator();
                
                // Color temperature simulation
                static float colorTemperature = 6500.0f; // Default daylight
                if (ImGui::SliderFloat("Color Temperature (K)", &colorTemperature, 1000.0f, 12000.0f, "%.0f K")) {
                    // Simple color temperature to RGB conversion
                    float temp = colorTemperature / 100.0f;
                    glm::vec3 tempColor;
                    
                    if (temp <= 66.0f) {
                        tempColor.r = 1.0f;
                        tempColor.g = std::max(0.0f, std::min(1.0f, static_cast<float>((99.4708025861f * log(temp) - 161.1195681661f) / 255.0f)));
                    } else {
                        tempColor.r = std::max(0.0f, std::min(1.0f, static_cast<float>((329.698727446f * pow(temp - 60.0f, -0.1332047592f)) / 255.0f)));
                        tempColor.g = std::max(0.0f, std::min(1.0f, static_cast<float>((288.1221695283f * pow(temp - 60.0f, -0.0755148492f)) / 255.0f)));
                    }
                    
                    if (temp >= 66.0f) {
                        tempColor.b = 1.0f;
                    } else if (temp <= 19.0f) {
                        tempColor.b = 0.0f;
                    } else {
                        tempColor.b = std::max(0.0f, std::min(1.0f, static_cast<float>((138.5177312231f * log(temp - 10.0f) - 305.0447927307f) / 255.0f)));
                    }
                    
                    light.color = tempColor;
                }
                
                // Quick temperature presets
                if (ImGui::Button("Candle (1900K)")) {
                    colorTemperature = 1900.0f;
                    light.color = glm::vec3(1.0f, 0.6f, 0.2f);
                }
                ImGui::SameLine();
                if (ImGui::Button("Tungsten (3200K)")) {
                    colorTemperature = 3200.0f;
                    light.color = glm::vec3(1.0f, 0.8f, 0.6f);
                }
                ImGui::SameLine();
                if (ImGui::Button("Daylight (6500K)")) {
                    colorTemperature = 6500.0f;
                    light.color = glm::vec3(1.0f, 1.0f, 1.0f);
                }
                
                ImGui::Separator();
                
                // Light creation presets
                if (ImGui::TreeNode("Light Presets")) {
                    if (ImGui::Button("Torch")) {
                        light = Light(glm::vec2(0, 0), 300.0f, glm::vec3(1.0f, 0.6f, 0.2f), 2.0f, 0.3f);
                    }
                    ImGui::SameLine();
                    if (ImGui::Button("Flashlight")) {
                        light = Light::CreateSpotLight(glm::vec2(0, 0), glm::vec2(1, 0), 500.0f, 0.3f, 0.5f, glm::vec3(1.0f, 1.0f, 0.9f), 3.0f);
                    }
                    ImGui::SameLine();
                    if (ImGui::Button("Sunlight")) {
                        light = Light::CreateDirectionalLight(glm::vec2(0.2f, -1.0f), glm::vec3(1.0f, 0.95f, 0.8f), 1.5f);
                    }
                    
                    if (ImGui::Button("Street Lamp")) {
                        light = Light(glm::vec2(0, 0), 800.0f, glm::vec3(1.0f, 0.8f, 0.5f), 2.5f, 0.2f);
                    }
                    ImGui::SameLine();
                    if (ImGui::Button("Neon Light")) {
                        light = Light(glm::vec2(0, 0), 200.0f, glm::vec3(0.0f, 1.0f, 1.0f), 4.0f, 0.8f);
                    }
                    ImGui::SameLine();
                    if (ImGui::Button("Fire")) {
                        light = Light(glm::vec2(0, 0), 150.0f, glm::vec3(1.0f, 0.3f, 0.0f), 3.0f, 0.5f);
                    }
                    
                    ImGui::TreePop();
                }
                
                ImGui::TreePop();
            }
        }
    }
    
    // For other custom components, keep the existing placeholder for now
    // You can extend this pattern for PlayerComponent, ObstacleComponent, etc.
    bool hasOtherComponents = false;
    
    // Check for other game components (placeholder - implement as needed)
    if (!hasOtherComponents) {
        // Only show this if no light component was found
        if (!entity.GetComponent<LightComponent>()) {
            ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "No custom game components found");
        }
    }
} 