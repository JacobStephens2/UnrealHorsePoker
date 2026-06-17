import unreal

# Create a minimal empty level and save it as /Game/Maps/Main.
# The game is entirely UI (Slate), so the level only needs to exist as a world to run in.

les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)

map_package = "/Game/Maps/Main"

# new_level overwrites if it exists; create a blank level.
created = les.new_level(map_package)
print("new_level({}) -> {}".format(map_package, created))

# Save everything (the new map).
les.save_current_level()

# Also make sure the package is saved to disk.
unreal.EditorAssetLibrary.save_directory("/Game/Maps", only_if_is_dirty=False, recursive=True)
print("Map saved at {}".format(map_package))
