#include "event_manager.hpp"
#include "plugin/plugin.hpp"

namespace mcserver {

void EventManager::unregister_plugin(Plugin* plugin) {
    std::lock_guard<std::mutex> lock(mutex_);

    for (auto& [type, listeners] : listeners_) {
        listeners.erase(
            std::remove_if(listeners.begin(), listeners.end(),
                          [plugin](const EventListener& listener) {
                              return listener.plugin == plugin;
                          }),
            listeners.end()
        );
    }
}

void EventManager::unregister_all() {
    std::lock_guard<std::mutex> lock(mutex_);
    listeners_.clear();
}

} // namespace mcserver
