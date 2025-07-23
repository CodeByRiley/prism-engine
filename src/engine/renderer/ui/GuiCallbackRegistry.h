#include <functional>
#include <unordered_map>
#include <string>

class GuiCallbackRegistry {
public:
    using Callback = std::function<void(const std::string& param)>;

    static GuiCallbackRegistry& Instance() {
        static GuiCallbackRegistry instance;
        return instance;
    }

    void Register(const std::string& name, Callback cb) {
        callbacks[name] = cb;
    }

    bool Execute(const std::string& name, const std::string& param = "") {
        auto it = callbacks.find(name);
        if (it != callbacks.end()) {
            it->second(param);
            return true;
        }
        return false;
    }

    bool IsRegistered(const std::string& name) const {
        return callbacks.find(name) != callbacks.end();
    }

private:
    std::unordered_map<std::string, Callback> callbacks;
};
