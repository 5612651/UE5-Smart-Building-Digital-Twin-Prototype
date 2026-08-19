import unreal


ASSET_PATH = "/Game/DigitalTwin/M_DTTemperatureOverlay"


material = unreal.load_asset(ASSET_PATH)
if not material:
    raise RuntimeError(f"Unable to load {ASSET_PATH}")

material.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
material.set_editor_property("two_sided", True)
material.set_editor_property("disable_depth_test", True)

if not unreal.EditorAssetLibrary.save_loaded_asset(material, only_if_is_dirty=False):
    raise RuntimeError(f"Unable to save {ASSET_PATH}")

unreal.log(
    "Configured M_DTTemperatureOverlay: translucent, two-sided, depth test disabled"
)
