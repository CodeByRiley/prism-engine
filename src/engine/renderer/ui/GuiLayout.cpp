#include "GuiLayout.h"
#include <iostream>
#include "GuiLayout.h"
#include <functional>
#include <unordered_map>
#include <string>

#include "engine/utils/Time.h"


namespace Gui {



    WidgetType WidgetFactory::GetWidgetType(const std::string& str) {
        if (str == "PROGRESS_BAR") return WidgetType::PROGRESS_BAR;
        if (str == "COLOR_PICKER") return WidgetType::COLOR_PICKER;
        if (str == "TEXT_INPUT") return WidgetType::TEXT_INPUT;
        if (str == "TREE_NODE") return WidgetType::TREE_NODE;
        if (str == "SEPARATOR") return WidgetType::SEPARATOR;
        if (str == "CHECKBOX") return WidgetType::CHECKBOX;
        if (str == "MENU_BAR") return WidgetType::MENU_BAR;
        if (str == "TAB_BAR") return WidgetType::TAB_BAR;
        if (str == "SPINNER") return WidgetType::SPINNER;
        if (str == "WINDOW") return WidgetType::WINDOW;
        if (str == "BUTTON") return WidgetType::BUTTON;
        if (str == "SLIDER") return WidgetType::SLIDER;
        if (str == "COMBO") return WidgetType::COMBO;
        if (str == "IMAGE") return WidgetType::IMAGE;
        if (str == "GROUP") return WidgetType::GROUP;
        if (str == "TEXT") return WidgetType::TEXT;
        if (str == "LIST") return WidgetType::LIST;
        if (str == "TAB") return WidgetType::TAB;

        // If we get here, we have an invalid type
        Logger::Error<GuiLayout>("Invalid widget type: " + str);
        return WidgetType::BUTTON;
    }

    WidgetState WidgetFactory::GetWidgetState(const std::string& str) {
        if (str == "READ_ONLY") return WidgetState::READ_ONLY;
        if (str == "DISABLED") return WidgetState::DISABLED;
        if (str == "HOVERED") return WidgetState::HOVERED;
        if (str == "FOCUSED") return WidgetState::FOCUSED;
        if (str == "NORMAL") return WidgetState::NORMAL;
        if (str == "ACTIVE") return WidgetState::ACTIVE;

        // If we get here, we have an invalid state
        Logger::Error<GuiLayout>("Invalid widget state: " + str);
        return WidgetState::NORMAL;
    }

    WidgetCallback WidgetFactory::GetWidgetCallback(const std::string& str) {
        if (str == "ON_ACTIVE") return WidgetCallback::ON_ACTIVE;
        if (str == "ON_CHANGE") return WidgetCallback::ON_CHANGE;
        if (str == "ON_SCROLL") return WidgetCallback::ON_SCROLL;
        if (str == "ON_CLICK") return WidgetCallback::ON_CLICK;
        if (str == "ON_HOVER") return WidgetCallback::ON_HOVER;
        if (str == "ON_FOCUS") return WidgetCallback::ON_FOCUS;

        // If we get here, we have an invalid callback
        Logger::Error<GuiLayout>("Invalid widget callback: " + str);
        return WidgetCallback::ON_CLICK;
    }

    std::unique_ptr<Widget> WidgetFactory::CreateWidget(const std::string& type, const std::string& name) {
        auto widget = std::make_unique<Widget>();
        widget->type = type;
        widget->widgetType = GetWidgetType(type);
        widget->name = name;
        // Set sensible defaults for each type if needed
        switch (widget->widgetType) {
            case WidgetType::BUTTON:
                widget->label = "Button";
                break;
            case WidgetType::SLIDER:
                widget->label = "Slider";
                break;
            default:
                break;
        }
        return widget;
    }

    std::unique_ptr<Widget> WidgetFactory::CreateWidgetFromYaml(const YAML::Node& node) {
        std::string type = node["type"] ? node["type"].as<std::string>() : "BUTTON";
        std::string name = node["name"] ? node["name"].as<std::string>() : "";

        // Assign a default name for LIST widgets if name is empty
        if (type == "LIST" && name.empty()) {
            static int yamlListCounter = 0;
            name = "list_widget_yaml_" + std::to_string(yamlListCounter++);
        }

        auto widget = CreateWidget(type, name);

        if (node["label"]) widget->label = node["label"].as<std::string>();
        if (node["tooltip"]) widget->tooltip = node["tooltip"].as<std::string>();
        if (node["icon"]) widget->icon = node["icon"].as<std::string>();
        if (node["position"]) {
            widget->position.x = node["position"][0].as<float>();
            widget->position.y = node["position"][1].as<float>();
        }
        if (node["size"]) {
            widget->size.x = node["size"][0].as<float>();
            widget->size.y = node["size"][1].as<float>();
        }
        if (node["color"]) {
            widget->color.r = node["color"][0].as<float>();
            widget->color.g = node["color"][1].as<float>();
            widget->color.b = node["color"][2].as<float>();
            widget->color.a = node["color"][3].as<float>();
        }
        if (node["rotation"]) widget->rotation = node["rotation"].as<float>();
        if (node["state"]) widget->widgetState = GetWidgetState(node["state"].as<std::string>());

        // Events
        if (node["events"]) {
            for (const auto& event : node["events"]) {
                std::string eventName = event.first.as<std::string>();
                std::string action = event.second.as<std::string>();
                widget->events[GetWidgetCallback(eventName)] = action;
            }
        }
        // States (optional, for custom state strings)
        if (node["states"]) {
            for (const auto& state : node["states"]) {
                std::string stateName = state.first.as<std::string>();
                std::string value = state.second.as<std::string>();
                widget->states[GetWidgetState(stateName)] = value;
            }
        }
        // Children (recursive)
        if (node["children"]) {
            for (const auto& childNode : node["children"]) {
                auto child = CreateWidgetFromYaml(childNode);
                child->parent = widget.get();
                widget->children.push_back(std::move(child));
            }
        }
        if (node["items"]) {
            for (const auto& item : node["items"]) {
                widget->items.push_back(item.as<std::string>());
            }
        }
        if (node["value"]) {
            if (node["value"].IsScalar()) {
                try {
                    widget->selectedIndex = node["value"].as<int>();
                } catch (const YAML::BadConversion&) {
                    widget->valueStr = node["value"].as<std::string>();
                }
            }
        }

        // For list widgets, check for a source variable
        if (widget->widgetType == WidgetType::LIST && node["source"]) {
            widget->listSourceVariable = node["source"].as<std::string>();
            //::Info("List widget " + widget->name + " will use source variable: " + widget->listSourceVariable);
        }
        return widget;
    }





}



void GuiLayout::Render() {
    std::unordered_map<std::string, std::string> dummy;
    Render(dummy);
}


int d = 0;
void GuiLayout::Render(std::unordered_map<std::string, std::string>& variables) {
    std::vector<std::pair<std::string, std::string>> changes;
    d += 1 * Time::DeltaTime();
    for (const auto& widget : m_Widgets) {
        if (d >= 1) {
            Logger::Info("Got Widget: " + widget->name);
        }
        Render(*widget, variables, changes);
    }
    // Apply changes
    for (const auto& [var, value] : changes) {
        variables[var] = value;
    }
}



void GuiLayout::Render(const Gui::Widget& widget,
                       const std::unordered_map<std::string, std::string>& variables,
                       std::vector<std::pair<std::string, std::string>>& outChanges)
{
    switch (widget.widgetType) {
        case Gui::WidgetType::TEXT:
            ImGui::Text("%s", widget.SubstituteVariables(widget.label, variables).c_str());
            break;
        case Gui::WidgetType::TEXT_INPUT:
        {
            std::string varName = widget.valueStr;
            std::string value = (!varName.empty() && variables.count(varName)) ? variables.at(varName) : "";
            char buf[256];
            strncpy(buf, value.c_str(), sizeof(buf));
            buf[sizeof(buf)-1] = '\0';
            if (ImGui::InputText(widget.SubstituteVariables(widget.label, variables).c_str(), buf, sizeof(buf))) {
                if (!varName.empty()) {
                    outChanges.emplace_back(varName, std::string(buf));
                    Logger::Info("Text input changed: " + varName + " to " + std::string(buf));
                }
            }
        }
        break;
        case Gui::WidgetType::COMBO:
        {
            std::string varName = widget.valueStr;
            std::string value = (!varName.empty() && variables.count(varName)) ? variables.at(varName) : (widget.items.empty() ? "" : widget.items[0]);
            int currentIndex = 0;
            for (size_t i = 0; i < widget.items.size(); ++i) {
                if (widget.items[i] == value) {
                    currentIndex = static_cast<int>(i);
                    break;
                }
            }
            std::vector<const char*> cstrItems;
            for (const auto& item : widget.items) cstrItems.push_back(item.c_str());
            if (ImGui::Combo(widget.SubstituteVariables(widget.label, variables).c_str(), &currentIndex, cstrItems.data(), static_cast<int>(cstrItems.size()))) {
                if (!varName.empty() && currentIndex < (int)widget.items.size()) {
                    outChanges.emplace_back(varName, widget.items[currentIndex]);
                }
            }
        }
        break;
        case Gui::WidgetType::COLOR_PICKER:
        {
            std::string varName = widget.valueStr;
            float colorArr[3] = {1.0f, 1.0f, 1.0f};
            if (!varName.empty() && variables.count(varName)) {
                std::istringstream ss(variables.at(varName));
                std::string token;
                int idx = 0;
                while (std::getline(ss, token, ',') && idx < 3) {
                    colorArr[idx++] = std::stof(token);
                }
            }
            if (ImGui::ColorPicker3(widget.SubstituteVariables(widget.label, variables).c_str(), colorArr)) {
                if (!varName.empty()) {
                    std::string newColor = std::to_string(colorArr[0]) + "," + std::to_string(colorArr[1]) + "," + std::to_string(colorArr[2]);
                    outChanges.emplace_back(varName, newColor);
                }
            }
        }
        break;
        case Gui::WidgetType::BUTTON:
            if (ImGui::Button(widget.SubstituteVariables(widget.label, variables).c_str())) {
                if (widget.events.count(Gui::WidgetCallback::ON_CLICK)) {
                    std::string action = widget.events.at(Gui::WidgetCallback::ON_CLICK);
                    GuiCallbackRegistry::Instance().Execute(action);
                }
            }
            break;
        case Gui::WidgetType::SEPARATOR:
            ImGui::Separator();
            break;
        case Gui::WidgetType::CHECKBOX:
            if (widget.states.count(Gui::WidgetState::ACTIVE)) {
                bool checked = widget.states.at(Gui::WidgetState::ACTIVE) == "true";
                if (ImGui::Checkbox(widget.SubstituteVariables(widget.label, variables).c_str(), &checked)) {
                    // handle event/callback if needed
                }
            }
            break;
        case Gui::WidgetType::SLIDER:
            if (widget.states.count(Gui::WidgetState::ACTIVE)) {
                float value = std::stof(widget.states.at(Gui::WidgetState::ACTIVE));
                if (ImGui::SliderFloat(widget.SubstituteVariables(widget.label, variables).c_str(), &value, 0.0f, 1.0f)) {
                    if (widget.events.count(Gui::WidgetCallback::ON_CHANGE)) {
                        std::string action = widget.events.at(Gui::WidgetCallback::ON_CHANGE);
                        GuiCallbackRegistry::Instance().Execute(action, std::to_string(value));
                    }
                    // handle event/callback if needed
                }
            }
            break;
        case Gui::WidgetType::LIST: {
             // If the widget doesn't have a name, assign a unique default name
            if (widget.name.empty()) {
                static int listCounter = 0;
                std::string uniqueName = "list_widget_" + std::to_string(listCounter++);
                const_cast<Gui::Widget&>(widget).name = uniqueName;
                Logger::Info("Assigned unique default name '" + uniqueName + "' to unnamed list widget");
            }

            // First check if widget has a custom source variable specified
            std::string varName = "";
            bool sourceFound = false;
            
            // Check for a custom source variable first
            if (!widget.listSourceVariable.empty()) {
                if (variables.find(widget.listSourceVariable) != variables.end()) {
                    varName = widget.listSourceVariable;
                    sourceFound = true;
                    Logger::Info("Using custom source variable '" + varName + "' for list widget '" + widget.name + "'");
                } else {
                    Logger::Warn<GuiLayout>("Could not find custom source variable '" + widget.listSourceVariable + "' for list widget '" + widget.name + "'");
                }
            }
            
            // If no custom source or it wasn't found, try using widget name as source
            if (!sourceFound && !widget.name.empty() && variables.find(widget.name) != variables.end()) {
                varName = widget.name;
                sourceFound = true;
                Logger::Info("Using widget name '" + widget.name + "' as source variable");
            }
            
            // If still no source, check for common variable names (as a fallback)
            if (!sourceFound) {
                // Try common variable names in order of preference
                const std::vector<std::string> commonVarNames = {
                    "components_list", "entity_components", "selected_entity", "entity_list"
                };
                
                for (const auto& commonVar : commonVarNames) {
                    if (variables.find(commonVar) != variables.end()) {
                        varName = commonVar;
                        sourceFound = true;
                        break;
                    }
                }
            }

            // If we found a source variable, process it
            if (sourceFound) {
                std::string sourceContent = variables.at(varName);
                // Split by comma
                std::string token;
                std::istringstream tokenStream(sourceContent);
                std::vector<std::string> items;

                while (std::getline(tokenStream, token, ',')) {
                    // Trim whitespace
                    token.erase(0, token.find_first_not_of(" \t\n\r\f\v"));
                    token.erase(token.find_last_not_of(" \t\n\r\f\v") + 1);

                    if (!token.empty()) {
                        items.push_back(token);
                    }
                }

                // IMPORTANT: Always clear the items first before populating from source
                const_cast<Gui::Widget&>(widget).items.clear();
                
                // Then populate with new items
                if (!items.empty()) {
                    const_cast<Gui::Widget&>(widget).items = std::move(items);
                    Logger::Info("Added " + std::to_string(const_cast<Gui::Widget&>(widget).items.size()) + 
                         " items to list widget '" + widget.name + "' from variable '" + varName + "'");
                } else {
                    Logger::Info("List widget '" + widget.name + "' found variable '" + varName + "' but it has no items");
                }
            } else if (widget.items.empty()) {
                // Only log if the widget doesn't already have predefined items
                Logger::Info("List widget '" + widget.name + "' has no source variable in the variables map");
            }

            // Continue with the original list rendering code
            if (ImGui::BeginListBox((widget.label.empty() ? "##" + widget.name : widget.label.c_str()).c_str(),
                                   ImVec2(widget.size.x > 0 ? widget.size.x : 200,
                                          widget.size.y > 0 ? widget.size.y : 200))) {
                for (int i = 0; i < widget.items.size(); i++) {
                    std::string item = widget.SubstituteVariables(widget.items[i], variables);
                    bool isSelected = (widget.selectedIndex == i);

                    if (ImGui::Selectable(item.c_str(), isSelected)) {
                        // Call the handler
                        Gui::Widget::HandleWidgetSelection(widget, i, item, outChanges);
                    }

                    if (isSelected) {
                        ImGui::SetItemDefaultFocus();
                    }
                }
                ImGui::EndListBox();
            }
            break;
        }
        break;
        // Containers: only recurse in these cases
        case Gui::WidgetType::WINDOW:
            ImGui::Begin(widget.SubstituteVariables(widget.label, variables).c_str());
            for (const auto& child : widget.children)
                Render(*child, variables, outChanges);
            ImGui::End();
            // if (ImGui::Begin(widget.SubstituteVariables(widget.label, variables).c_str())) {
            //     for (const auto& child : widget.children)
            //         Render(*child, variables, outChanges);
            //     ImGui::End();
            // }
            break;
        case Gui::WidgetType::TAB_BAR:
            if (ImGui::BeginTabBar(widget.SubstituteVariables(widget.label, variables).c_str())) {
                for (const auto& child : widget.children)
                    Render(*child, variables, outChanges);
                ImGui::EndTabBar();
            }
            break;
        case Gui::WidgetType::TAB:
            if (ImGui::BeginTabItem(widget.SubstituteVariables(widget.label, variables).c_str())) {
                for (const auto& child : widget.children)
                    Render(*child, variables, outChanges);
                ImGui::EndTabItem();
            }
            break;
        case Gui::WidgetType::GROUP:
            ImGui::BeginGroup();
            for (const auto& child : widget.children)
                Render(*child, variables, outChanges);
            ImGui::EndGroup();
            break;
        case Gui::WidgetType::TREE_NODE:
            if (ImGui::TreeNode(widget.SubstituteVariables(widget.label, variables).c_str())) {
                for (const auto& child : widget.children) {
                    Render(*child, variables, outChanges);
                }
                ImGui::TreePop();
            }
            break;
        case Gui::WidgetType::PROGRESS_BAR:
            ImGui::ProgressBar(widget.value, ImVec2(widget.size.x, widget.size.y));
            break;
        case Gui::WidgetType::SPINNER:
            ImGui::Text("Spinner not implemented");
            break;
        case Gui::WidgetType::MENU_BAR:
            if (ImGui::BeginMenuBar()) {
                for (const auto& child : widget.children) {
                    Render(*child, variables, outChanges);
                }
                ImGui::EndMenuBar();
            }
            break;
        default:
            Logger::Error<GuiLayout>("Unknown widget type: " + widget.type);
            break;
    }
}

const std::vector<std::unique_ptr<Gui::Widget>>& GuiLayout::GetWidgets() const {
    return m_Widgets;
}

Gui::Widget* GuiLayout::GetWidget(const std::string& name) const {
    for (const auto& widget : m_Widgets) {
        if (widget->name == name) {
            return widget.get();
        }
    }
    return nullptr;
}



void GuiLayout::Reload() {
    Logger::Info("Reloading GUI");
    // Reload from YAML file
    LoadFromYaml(ResourcePath::GetFullPath("gui/layouts/" + m_Type + ".yaml"));
}

// Clear all widget state (items, selections, etc.)
void GuiLayout::Reset() {
    Logger::Info("Resetting GUI state");
    for (auto& widget : m_Widgets) {
        ResetWidgetState(widget.get());
    }
}

// Recursively reset widget state
void GuiLayout::ResetWidgetState(Gui::Widget* widget) {
    if (!widget) return;

    // Clear items and reset selection
    widget->items.clear();
    widget->selectedIndex = 0;

    // Reset children recursively
    for (auto& child : widget->children) {
        ResetWidgetState(child.get());
    }
}

void GuiLayout::LoadFromYaml(const std::string& path) {
    Logger::SetNewLine(false);
    try {
        Logger::Info("Loading GUI from: " + path);
        YAML::Node node = YAML::LoadFile(path);
        m_Widgets.clear();
        if (node["widgets"]) {
            for (const auto& widgetNode : node["widgets"]) {
                RegisterWidgetEvents(widgetNode); // Register events for all widgets
                auto widget = m_WidgetFactory.CreateWidgetFromYaml(widgetNode);
                m_Widgets.push_back(std::move(widget));
            }
        } else if (node["type"]) {
            RegisterWidgetEvents(node);
            auto widget = m_WidgetFactory.CreateWidgetFromYaml(node);
            m_Widgets.push_back(std::move(widget));
        } else {
            Logger::Error<GuiLayout>("No widgets found in GUI: " + path);
        }
    } catch (const YAML::Exception& e) {
        Logger::Error<GuiLayout>("Error loading GUI from: " + path + " - " + e.what());
    }
    Logger::SetNewLine(true);
}

void GuiLayout::RegisterWidgetEvents(const YAML::Node& node) {
    if (node["events"]) {
        for (const auto& event : node["events"]) {
            std::string callbackName = event.second.as<std::string>();
            if (!GuiCallbackRegistry::Instance().IsRegistered(callbackName)) {
                // Register a default handler or log a warning
                GuiCallbackRegistry::Instance().Register(callbackName, [callbackName](const std::string& param){
                    Logger::Info("Default handler for callback: " + callbackName + " param: " + param);
                });
            }
        }
    }
    if (node["children"]) {
        for (const auto& child : node["children"]) {
            RegisterWidgetEvents(child);
        }
    }
}