add_library(entity STATIC
    entity.cpp
    entity.hpp
    entity_manager.cpp
    entity_manager.hpp
    player.cpp
    player.hpp
    inventory/item_stack.cpp
    inventory/item_stack.hpp
    inventory/inventory.cpp
    inventory/inventory.hpp
    item/item_entity.hpp
    item/item_entity_manager.cpp
    item/item_entity_manager.hpp
    crafting/crafting_recipe.cpp
    crafting/crafting_recipe.hpp
    mob/mob_type.hpp
    mob/mob_ai.hpp
    mob/mob_metadata.cpp
    mob/mob_metadata.hpp
    mob/pathfinding.cpp
    mob/pathfinding.hpp
    mob/mob.cpp
    mob/mob.hpp
    mob/passive_mob.cpp
    mob/passive_mob.hpp
    mob/hostile_mob.cpp
    mob/hostile_mob.hpp
    mob/mob_manager.cpp
    mob/mob_manager.hpp
    mob/mob_spawner.cpp
    mob/mob_spawner.hpp
)

target_include_directories(entity PUBLIC
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(entity PUBLIC core util platform)

set_target_properties(entity PROPERTIES FOLDER "Gameplay")
