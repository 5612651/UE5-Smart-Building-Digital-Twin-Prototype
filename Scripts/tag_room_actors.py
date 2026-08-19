from pathlib import Path

import unreal


unreal.EditorLoadingAndSavingUtils.load_map("/Game/Untitled")
library = unreal.WorldPartitionBlueprintLibrary.get_default_object()
descs = library.get_actor_descs() or []
modified_packages = []

for desc in descs:
    label = str(desc.label)
    if not label.lower().startswith("roombox_"):
        continue

    suffix = label[-3:]
    if not suffix.isdigit():
        continue

    unreal.load_package(str(desc.actor_package))
    actor = unreal.load_object(None, str(desc.actor_path))
    if not actor:
        raise RuntimeError(f"Unable to load external actor {desc.actor_path}")

    room_id = f"Room{suffix}"
    tags = [str(tag) for tag in actor.tags]
    if "DT_RoomBox" not in tags:
        tags.append("DT_RoomBox")
    if room_id not in tags:
        tags.append(room_id)

    actor.modify()
    actor.set_editor_property("tags", tags)
    # The visualization layer must exist before the manager scans the world and
    # must remain available regardless of the current camera streaming cell.
    actor.set_editor_property("is_spatially_loaded", False)
    package = actor.get_outermost()
    if not unreal.EditorLoadingAndSavingUtils.save_packages([package], True):
        raise RuntimeError(f"Unable to save external actor package {desc.actor_package}")
    modified_packages.append(str(desc.actor_package))
    unreal.log(f"Tagged {label}: {tags}")

Path("C:/DTBuildingPackage_20260819/room_actor_packages.txt").write_text(
    "\n".join(modified_packages), encoding="utf-8"
)
unreal.log(f"Tagged {len(modified_packages)} room box actors for runtime discovery")
