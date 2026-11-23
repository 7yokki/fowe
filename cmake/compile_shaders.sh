#!/usr/bin/env bash
# Compiles The-Forge FSL shader lists for Vulkan into the FOWE run directory.
# The-Forge ships .list files that reference CamelCase filenames while the
# actual files are lowercase; Linux filesystems are case-sensitive, so we add
# idempotent aliases before invoking the FSL compiler.
set -euo pipefail

FORGE_ROOT="$1"
RUN_DIR="$2"
PYTHON="${3:-python3}"

UI_DIR="$FORGE_ROOT/Common_3/Application/UI/Shaders/FSL"
FONT_DIR="$FORGE_ROOT/Common_3/Application/Fonts/Shaders/FSL"
SHOT_DIR="$FORGE_ROOT/Common_3/Application/Screenshot/Shaders/FSL"
ANIM_DIR="$FORGE_ROOT/Common_3/Resources/AnimationSystem/Animation/Shaders/FSL"

link() { # dir, target, linkname
    ln -sf "$2" "$1/$3"
}

# UI
link "$UI_DIR" imgui.frag.fsl ImGui.frag.fsl
link "$UI_DIR" imgui.vert.fsl ImGui.vert.fsl
# Fonts
link "$FONT_DIR" fontstash.frag.fsl FontStash.frag.fsl
link "$FONT_DIR" fontstash2D.vert.fsl FontStash2D.vert.fsl
link "$FONT_DIR" fontstash3D.vert.fsl FontStash3D.vert.fsl
link "$FONT_DIR" resources.h Resources.h
link "$FONT_DIR" fontstash.srt.h FontStash.srt.h
# Screenshot
link "$SHOT_DIR" copy.comp.fsl Copy.comp.fsl
link "$SHOT_DIR" copy.comp.srt.h Copy.comp.srt.h
# Animation (used by scripting/skeleton later)
link "$ANIM_DIR" joint.frag.fsl Joint.frag.fsl
link "$ANIM_DIR" joint.vert.fsl Joint.vert.fsl
link "$ANIM_DIR" bone.frag.fsl Bone.frag.fsl
link "$ANIM_DIR" bone.vert.fsl Bone.vert.fsl

GLSLANG="$FORGE_ROOT/Common_3/Graphics/ThirdParty/OpenSource/VulkanSDK/bin/Linux/glslangValidator"
GLSLANG_MODE=""
if [ -f "$GLSLANG" ]; then
    GLSLANG_MODE="$(stat -c %a "$GLSLANG")"
    chmod +x "$GLSLANG"
    restore_glslang() { [ -n "$GLSLANG_MODE" ] && chmod "$GLSLANG_MODE" "$GLSLANG" 2>/dev/null || true; }
    trap restore_glslang EXIT
fi

mkdir -p "$RUN_DIR/Shaders" "$RUN_DIR/CompiledShaders"

FSL="$FORGE_ROOT/Common_3/Tools/ForgeShadingLanguage/fsl.py"
for list in \
    "$UI_DIR/UIShaders.list" \
    "$FONT_DIR/FontShaders.list" \
    "$SHOT_DIR/ScreenshotShaders.list"; do
    echo "== FSL: $(basename "$list") =="
    "$PYTHON" "$FSL" -l VULKAN \
        -d "$RUN_DIR/Shaders" \
        -b "$RUN_DIR/CompiledShaders" \
        -i "$RUN_DIR" \
        --cache-args --incremental \
        --compile "$list"
done

echo "Shader compilation complete -> $RUN_DIR/CompiledShaders/VULKAN"
