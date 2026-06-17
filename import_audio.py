import unreal

SRC = "/Users/admin/UnrealProjects/CardGame/SourceAudio"
DEST = "/Game/Audio"

files = ["poker_theme", "sfx_deal", "sfx_chip", "sfx_check", "sfx_win", "sfx_lose"]

tasks = []
for f in files:
    t = unreal.AssetImportTask()
    t.filename = "{}/{}.wav".format(SRC, f)
    t.destination_path = DEST
    t.automated = True
    t.replace_existing = True
    t.save = True
    tasks.append(t)

unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)

for f in files:
    path = "{}/{}".format(DEST, f)
    a = unreal.EditorAssetLibrary.load_asset(path)
    if not a:
        print("IMPORT FAILED:", path)
        continue
    looping = (f == "poker_theme")
    try:
        a.set_editor_property("looping", looping)
    except Exception as e:
        print("looping set failed for", f, e)
    unreal.EditorAssetLibrary.save_asset(path)
    print("OK", path, "looping=", a.get_editor_property("looping"))

print("audio import complete")
