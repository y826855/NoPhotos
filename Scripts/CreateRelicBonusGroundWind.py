import unreal


ASSET_FOLDER = "/Game/NoPhotos/Blueprints/MapEvent"
SYSTEM_NAME = "NS_RelicBonus_GroundWind"
SYSTEM_PATH = f"{ASSET_FOLDER}/{SYSTEM_NAME}"
SOURCE_EMITTER_PATH = f"{ASSET_FOLDER}/NE_RelicBonus_GroundWind_Source"
TEMPLATE_PATH = "/Niagara/DefaultAssets/Templates/Emitters/BlowingParticles.BlowingParticles"


system = unreal.load_asset(SYSTEM_PATH)
if system and not isinstance(system, unreal.NiagaraSystem):
    if not unreal.EditorAssetLibrary.does_asset_exist(SOURCE_EMITTER_PATH):
        if not unreal.EditorAssetLibrary.rename_asset(SYSTEM_PATH, SOURCE_EMITTER_PATH):
            raise RuntimeError(f"Failed to preserve source emitter as {SOURCE_EMITTER_PATH}")
    system = None

if not system:
    asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
    system = asset_tools.create_asset(
        SYSTEM_NAME,
        ASSET_FOLDER,
        unreal.NiagaraSystem,
        unreal.NiagaraSystemFactoryNew(),
    )
if not system or not isinstance(system, unreal.NiagaraSystem):
    raise RuntimeError(f"Failed to create Niagara System: {SYSTEM_PATH}")

conversion = unreal.FXConverterUtilitiesLibrary.create_system_conversion_context(system)
emitter = conversion.add_template_emitter("E_GroundDust", TEMPLATE_PATH)
if not emitter:
    raise RuntimeError(f"Failed to add Niagara emitter template: {TEMPLATE_PATH}")

emitter.set_local_space(False)

# BlowingParticles is a looping sprite template with the core lifetime, spawn,
# motion, and renderer stack already wired. Tune its continuous density for the
# rotor-wash use case when the SpawnRate module is available in this engine build.
spawn_rate = emitter.find_module_script("SpawnRate")
if spawn_rate:
    spawn_rate.set_parameter(
        "SpawnRate.SpawnRate",
        unreal.FXConverterUtilitiesLibrary.create_script_input_float(65.0),
    )

conversion.finalize()
unreal.EditorAssetLibrary.save_loaded_asset(system, only_if_is_dirty=False)
unreal.log(f"Configured {SYSTEM_PATH} with E_GroundDust")
unreal.SystemLibrary.quit_editor()
