cmake_minimum_required(VERSION 3.16)

project(Bomberman LANGUAGES C CXX)

include(FetchContent)

set(SDL_STATIC ON CACHE BOOL "Build static SDL" FORCE)
set(SDL_SHARED OFF CACHE BOOL "Do not build shared SDL" FORCE)
set(SDL_VIDEO ON CACHE BOOL "Build platform video support" FORCE)
foreach(option IN ITEMS
    SDL_TESTS SDL_TEST_LIBRARY SDL_EXAMPLES SDL_INSTALL
    SDL_RENDER SDL_GPU SDL_CAMERA SDL_JOYSTICK SDL_HAPTIC
    SDL_HIDAPI SDL_POWER SDL_SENSOR SDL_DIALOG SDL_TRAY
)
    set(${option} OFF CACHE BOOL "Not needed by the current setup increment" FORCE)
endforeach()
set(SDL_AUDIO ON CACHE BOOL "Enable the Engine Core audio service" FORCE)

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(SDL_X11 ON CACHE BOOL "Use X11 as the initial Linux backend" FORCE)
    set(SDL_OPENGL ON CACHE BOOL "Enable the Phase 2 OpenGL renderer" FORCE)
    foreach(option IN ITEMS
        SDL_WAYLAND SDL_KMSDRM SDL_OPENGLES SDL_VULKAN
        SDL_X11_XSCRNSAVER SDL_X11_XTEST
    )
        set(${option} OFF CACHE BOOL "Not needed by the initial Linux backend" FORCE)
    endforeach()
endif()

FetchContent_Declare(SDL3
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG fa2c02bb6e21974a89ea9824bc53c9932abe5f9c
    GIT_SHALLOW FALSE
)
FetchContent_MakeAvailable(SDL3)

FetchContent_Declare(stb
	GIT_REPOSITORY https://github.com/nothings/stb.git
	GIT_TAG 2c980bb59875b0d32144a71867fbdebb2f77cd20
	GIT_SHALLOW FALSE
)
FetchContent_MakeAvailable(stb)

add_executable(bomberman
    src/main.cpp
	src/bootstrap/AnimationPreparation.cpp
	src/bootstrap/ApplicationComposition.cpp
	src/bootstrap/AssetDiagnostics.cpp
	src/bootstrap/AssetPreparation.cpp
	src/bootstrap/BootstrapPaths.cpp
	src/bootstrap/ModelPreparation.cpp
	engine/animation/SkeletalComposition.cpp
	engine/animation/SkeletalMath.cpp
	engine/animation/SkeletalPose.cpp
	engine/animation/SkeletalPoseSource.cpp
	engine/animation/SkeletalSampling.cpp
	engine/audio/AudioSystem.cpp
    engine/assets/GlbLoader.cpp
	engine/assets/PngDecoder.cpp
    engine/core/Application.cpp
	engine/graphics/runtime/OpenGlFunctions.cpp
	engine/graphics/runtime/RenderingBootstrap.cpp
	engine/graphics/runtime/RenderingResources.cpp
	engine/graphics/foundation/RenderingMath.cpp
	engine/graphics/foundation/RenderingTransforms.cpp
	engine/graphics/features/RenderingAnimation.cpp
	engine/graphics/features/RenderingCamera.cpp
	engine/graphics/features/RenderingDirectionalShadow.cpp
	engine/graphics/features/RenderingDebugGeometry.cpp
	engine/graphics/features/RenderingHud.cpp
	engine/graphics/features/SmokeEmitter.cpp
	engine/graphics/features/FireballEmitter.cpp
	engine/graphics/features/VfxSystem.cpp
	engine/graphics/resources/RenderingVfxSprites.cpp
	engine/graphics/passes/RenderingVfxPass.cpp
	engine/graphics/features/RenderingPointLight.cpp
	engine/graphics/features/RenderingWorldGeometry.cpp
	engine/graphics/resources/RenderingFullscreenQuad.cpp
	engine/graphics/resources/RenderingHudResources.cpp
	engine/graphics/resources/RenderingPostProcess.cpp
	engine/graphics/resources/RenderingPrograms.cpp
	engine/graphics/resources/RenderingShadowMap.cpp
	engine/graphics/resources/RenderingStaticMeshes.cpp
	engine/graphics/resources/RenderingTransientLines.cpp
	engine/graphics/resources/RenderingWorldRectangles.cpp
	engine/graphics/passes/RenderingDebugPass.cpp
	engine/graphics/passes/RenderingHudPass.cpp
	engine/graphics/passes/RenderingPostProcessPass.cpp
	engine/graphics/passes/RenderingScenePass.cpp
	engine/graphics/passes/RenderingShadowPass.cpp
    engine/graphics/Renderer.cpp
    engine/input/KeyboardState.cpp
    engine/input/SdlKeyboardAdapter.cpp
	game/collision/ActorCollision.cpp
	game/actors/PlayerController.cpp
	game/actors/EnemyRoster.cpp
	game/ai/EnemyController.cpp
	game/ai/EnemyBrain.cpp
	game/ai/EnemyNavigation.cpp
	game/ai/EnemyObjectivePlanner.cpp
    game/world/Arena.cpp
	game/world/PrototypeArenaGenerator.cpp
    game/combat/Bomb.cpp
	game/combat/BombSystem.cpp
	game/combat/ExplosionDamage.cpp
    game/combat/Explosion.cpp
	game/presentation/GameAudioFeedback.cpp
	game/presentation/GamePresentation.cpp
	game/pickups/PickupSystem.cpp
	game/config/GameplayTuning.cpp
    game/input/PlayerInput.cpp
    game/GameSession.cpp
)
target_include_directories(bomberman PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/engine"
    "${CMAKE_CURRENT_SOURCE_DIR}/game"
	"${CMAKE_CURRENT_SOURCE_DIR}/src"
	"${stb_SOURCE_DIR}"
)
target_compile_definitions(bomberman PRIVATE
    BOMBERMAN_DEFAULT_GLB_PATH=\"${CMAKE_CURRENT_SOURCE_DIR}/assets/models/prototype_arena.glb\"
	BOMBERMAN_DEFAULT_GAMEPLAY_TUNING_PATH=\"${CMAKE_CURRENT_SOURCE_DIR}/config/gameplay_tuning.cfg\"
    BOMBERMAN_DEFAULT_AUDIO_CLIP_0_PATH=\"${CMAKE_CURRENT_SOURCE_DIR}/assets/audio/bomb_place.wav\"
    BOMBERMAN_DEFAULT_AUDIO_CLIP_1_PATH=\"${CMAKE_CURRENT_SOURCE_DIR}/assets/audio/explosion.wav\"
    BOMBERMAN_DEFAULT_AUDIO_CLIP_2_PATH=\"${CMAKE_CURRENT_SOURCE_DIR}/assets/audio/pickup.wav\"
    BOMBERMAN_DEFAULT_AUDIO_CLIP_3_PATH=\"${CMAKE_CURRENT_SOURCE_DIR}/assets/audio/damage.wav\"
    BOMBERMAN_DEFAULT_AUDIO_CLIP_4_PATH=\"${CMAKE_CURRENT_SOURCE_DIR}/assets/audio/defeat.wav\"
    BOMBERMAN_DEFAULT_AUDIO_CLIP_5_PATH=\"${CMAKE_CURRENT_SOURCE_DIR}/assets/audio/victory.wav\"
)
target_link_libraries(bomberman PRIVATE SDL3::SDL3-static)
set_target_properties(bomberman PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

if(MSVC)
    target_compile_options(bomberman PRIVATE /W4)
elseif(CMAKE_CXX_COMPILER_ID MATCHES "^(GNU|Clang|AppleClang)$")
    target_compile_options(bomberman PRIVATE -Wall -Wextra -Wpedantic)
endif()
