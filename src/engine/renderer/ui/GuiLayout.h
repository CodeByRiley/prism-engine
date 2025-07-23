#pragma once

#include <vector>
#include <string>
#include <memory>
#include <yaml-cpp/yaml.h>
#include <glm/glm.hpp>
#include <unordered_map>

#include <imgui.h>

#include "GuiCallbackRegistry.h"
#include "../../utils/ResourcePath.h"
#include "../../utils/Logger.h"

namespace Gui {
    enum class WidgetType {
        IMAGE_BUTTON,
        PROGRESS_BAR,
        COLOR_PICKER,
        TEXT_INPUT,
        SEPARATOR,
        TREE_NODE,
        MENU_BAR,
        CHECKBOX,
        TAB_BAR,
        SPINNER,
        WINDOW,
        SLIDER,
        BUTTON,
        COMBO,
        GROUP,
        IMAGE,
        TEXT,
        LIST,
        TAB,
    };

    enum class WidgetState {
        NORMAL,
        ACTIVE,
        HOVERED,
        FOCUSED,
        DISABLED,
        READ_ONLY,
        ERROR,
        SUCCESS,
        WARNING,
        INFO,
        DEBUG,
        TRACE,
    };

    enum class WidgetCallback {
        ON_CLICK,
        ON_HOVER,
        ON_FOCUS,
        ON_ACTIVE,
        ON_CHANGE,
        ON_SCROLL,
    };

    struct Widget {
        std::string name;
        std::string type; // For YAML mapping, but WidgetType is preferred in code
        std::string label;
        std::string tooltip;
        std::string icon;
        float value = 0.0f;

        std::vector<std::string> items;
        int selectedIndex = 0;
        std::string valueStr; // For string value (variable or label)

        WidgetType widgetType = WidgetType::BUTTON;
        WidgetState widgetState = WidgetState::NORMAL;

        glm::vec2 position = {0.0f, 0.0f};
        glm::vec2 size = {0.0f, 0.0f};
        glm::vec4 color = {1.0f, 1.0f, 1.0f, 1.0f};
        float rotation = 0.0f;

        // Events and states
        std::unordered_map<WidgetCallback, std::string> events;
        std::unordered_map<WidgetState, std::string> states;

        
        std::vector<std::unique_ptr<Widget>> children;
        Widget* parent = nullptr;

        // Utility functions
        std::string GetState(WidgetState state) const;
        void SetState(WidgetState state, const std::string& value);

        void AddEvent(WidgetCallback callback, const std::string& value);
        void RemoveEvent(WidgetCallback callback);

        // Helper functions
        void AddChild(std::unique_ptr<Widget> child);
        void RemoveChild(const std::string& name);
        Widget* FindChild(const std::string& name);
        void SetParent(Widget* parent);
        Widget* GetParent() const;

        inline std::string SubstituteVariables(const std::string& input, const std::unordered_map<std::string, std::string>& vars) const {
            std::string output = input;
            for (const auto& pair : vars) {
                // Try both ${var} and var
                std::string key = pair.first;
                std::string key_braced = "${" + key + "}";
                // Replace ${var}
                //Logger::Info("Substituting " + key_braced + " with " + pair.second);
                size_t pos = output.find(key_braced);
                while (pos != std::string::npos) {
                    output.replace(pos, key_braced.length(), pair.second);
                    pos = output.find(key_braced, pos + pair.second.length());
                }
                // Replace var (optional, if you want)
                pos = output.find(key);
                while (pos != std::string::npos) {
                    output.replace(pos, key.length(), pair.second);
                    pos = output.find(key, pos + pair.second.length());
                }
            }
            return output;
        }
    };

    class WidgetFactory {
    public:
        static WidgetType GetWidgetType(const std::string& str);
        static WidgetState GetWidgetState(const std::string& str);
        static WidgetCallback GetWidgetCallback(const std::string& str);

        static std::unique_ptr<Widget> CreateWidget(const std::string& type, const std::string& name = "");
        static std::unique_ptr<Widget> CreateWidgetFromYaml(const YAML::Node& node); // Should be default
        
    };
}


class GuiLayout {
public:
    explicit GuiLayout(const std::string& name) : m_WidgetFactory(), m_Type(name) {
        // Load on creation
        LoadFromYaml(ResourcePath::GetFullPath("gui/layouts/" + name + ".yaml"));
        Reload();
    }

    void Render();
    void Render(std::unordered_map<std::string, std::string>& variables);
    void Render(const Gui::Widget& widget,
                const std::unordered_map<std::string, std::string>& variables,
                std::vector<std::pair<std::string, std::string>>& outChanges);
    void Reload();

    const std::vector<std::unique_ptr<Gui::Widget>>& GetWidgets() const;
    Gui::Widget* GetWidget(const std::string& name) const;

private:
    void LoadFromYaml(const std::string& path);
    void RegisterWidgetEvents(const YAML::Node& node);

    Gui::WidgetFactory m_WidgetFactory;

    std::string m_Type;
    std::string m_Title;
    std::string m_FileName;
    std::vector<std::unique_ptr<Gui::Widget>> m_Widgets;

};