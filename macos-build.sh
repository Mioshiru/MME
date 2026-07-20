#!/bin/bash
# ============================================================
#   Mios Map Editor - Professional macOS Build Script
# ============================================================

# --- Color helpers ---
GREEN="\e[92m"
YELLOW="\e[93m"
RED="\e[91m"
CYAN="\e[96m"
RESET="\e[0m"
BOLD="\e[1m"

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="${SCRIPT_DIR}"
BUILD_DIR="${PROJECT_ROOT}/build"
ERROR_FILE="${PROJECT_ROOT}/compiler_error_latest.md"
INSTALL_DIR="${BUILD_DIR}"
LOG_FILE="/tmp/mme_build_macos.log"
TOTAL_STEPS=5

rm -f "${ERROR_FILE}"
echo "Initializing Build Log" > "${LOG_FILE}"

echo -e "\n${BOLD}${CYAN}========================================================${RESET}"
echo -e "${BOLD}${CYAN}   Mios Map Editor macOS Build Script${RESET}"
echo -e "${BOLD}${CYAN}========================================================${RESET}\n"

# --- STEP 1: Verify Environment ---
echo -e "${BOLD}[1/${TOTAL_STEPS}] Verifying environment...${RESET}"

if ! command -v git &> /dev/null; then
    echo -e "  ${RED}ERROR: Git not found.${RESET}"
    exit 1
fi

if ! command -v cmake &> /dev/null; then
    if command -v brew &> /dev/null; then
        echo "  CMake not found. Installing via Homebrew..."
        brew install cmake >> "${LOG_FILE}" 2>&1
    else
        echo -e "  ${RED}ERROR: CMake not found and Homebrew not installed.${RESET}"
        exit 1
    fi
fi

# Try to find Ninja, fallback to Make if not present
GENERATOR="Unix Makefiles"
if command -v ninja &> /dev/null; then
    GENERATOR="Ninja"
elif command -v brew &> /dev/null; then
    echo "  Ninja not found. Installing via Homebrew..."
    brew install ninja >> "${LOG_FILE}" 2>&1
    if command -v ninja &> /dev/null; then
        GENERATOR="Ninja"
    fi
fi

echo -e "  ${GREEN}Environment OK (using ${GENERATOR})${RESET}"

# --- STEP 2: Resolving Dependencies (vcpkg, Vulkan, Shaders) ---
echo -e "\n${BOLD}[2/${TOTAL_STEPS}] Resolving Dependencies & Tools (vcpkg, Vulkan, Shaders)...${RESET}"

# Determine vcpkg directory
VCPKG_DIR=""
if [ -n "${VCPKG_ROOT}" ] && [ -d "${VCPKG_ROOT}" ]; then
    VCPKG_DIR="${VCPKG_ROOT}"
elif [ -d "${HOME}/vcpkg" ]; then
    VCPKG_DIR="${HOME}/vcpkg"
else
    echo "  Cloning vcpkg..."
    git clone https://github.com/microsoft/vcpkg.git "${HOME}/vcpkg" --depth=1 &> /dev/null
    VCPKG_DIR="${HOME}/vcpkg"
fi
export VCPKG_ROOT="${VCPKG_DIR}"

if [ ! -f "${VCPKG_DIR}/vcpkg" ]; then
    echo "  Bootstrapping vcpkg..."
    bash "${VCPKG_DIR}/bootstrap-vcpkg.sh" >> "${LOG_FILE}" 2>&1
fi

# Shaders
GLSLANG="glslangValidator"
if command -v glslangValidator &> /dev/null; then
    echo "  Compiling shaders..."
    pushd "${PROJECT_ROOT}/data/shaders" > /dev/null
    for f in *_vk.glsl; do
        if [ -f "$f" ]; then
            STAGE=""
            if [[ "$f" == *"vertex"* ]]; then STAGE="vert"; fi
            if [[ "$f" == *"fragment"* ]]; then STAGE="frag"; fi
            if [ -n "$STAGE" ]; then
                glslangValidator -V -S "$STAGE" "$f" -o "$f.spv" >> "${LOG_FILE}" 2>&1
            fi
        fi
    done
    popd > /dev/null
fi

echo -e "  ${GREEN}Dependencies OK${RESET}"

# --- STEP 3: Configure Project ---
echo -e "\n${BOLD}[3/${TOTAL_STEPS}] Configuring CMake with ${GENERATOR}...${RESET}"
mkdir -p "${BUILD_DIR}"

# Detect Apple Silicon vs Intel for OSX triplet
ARCH=$(uname -m)
if [ "$ARCH" = "arm64" ]; then
    TRIPLET="arm64-osx"
else
    TRIPLET="x64-osx"
fi

cmake -G "${GENERATOR}" -S "${PROJECT_ROOT}" -B "${BUILD_DIR}" \
    -DCMAKE_BUILD_TYPE=Release \
    -DVCPKG_BUILD_TYPE=release \
    -DVCPKG_INSTALLED_DIR="${PROJECT_ROOT}/.vcpkg_cache" \
    -DCMAKE_TOOLCHAIN_FILE="${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" \
    -DVCPKG_TARGET_TRIPLET="${TRIPLET}" \
    -DCMAKE_INSTALL_PREFIX="${INSTALL_DIR}" >> "${LOG_FILE}" 2>&1

if [ $? -ne 0 ]; then
    echo -e "  ${RED}ERROR: CMake configuration failed!${RESET}"
    tail -n 50 "${LOG_FILE}"
    exit 1
fi
echo -e "  ${GREEN}OK (Incremental)${RESET}"

# --- STEP 4: Build C++ Native Core ---
echo -e "\n${BOLD}[4/${TOTAL_STEPS}] Building Native C++ Components...${RESET}"
CORES=$(sysctl -n hw.ncpu 2>/dev/null || echo 4)

cmake --build "${BUILD_DIR}" --config Release --parallel "${CORES}" >> "${LOG_FILE}" 2>&1

if [ $? -ne 0 ]; then
    echo -e "  ${RED}ERROR: Compilation failed!${RESET}"
    
    # Extract first error
    ERR_LINE=$(grep -E "error:|FAILED:|fatal error" "${LOG_FILE}" | head -n 1)
    echo -e "  ${RED}${ERR_LINE}${RESET}"
    
    # Write report
    echo "# Compiler Error Report" > "${ERROR_FILE}"
    echo "- **Step:** C++ Compilation (macOS)" >> "${ERROR_FILE}"
    echo "- **Time:** $(date)" >> "${ERROR_FILE}"
    echo -e "\n## First Error\n${ERR_LINE}" >> "${ERROR_FILE}"
    echo -e "\n## Log Snippet\n\`\`\`text" >> "${ERROR_FILE}"
    tail -n 80 "${LOG_FILE}" >> "${ERROR_FILE}"
    echo "\`\`\`" >> "${ERROR_FILE}"
    
    echo -e "${YELLOW}Details written to compiler_error_latest.md for review.${RESET}"
    exit 1
fi
echo -e "  ${GREEN}Native C++ Build completed successfully.${RESET}"

# --- STEP 5: Packaging Runtime Release Artifacts ---
echo -e "\n${BOLD}[5/${TOTAL_STEPS}] Packaging Runtime Release Artifacts...${RESET}"

# Copy resource folders into bundle (or relative folders)
for D in data brushes scripts extensions icons; do
    if [ -d "${PROJECT_ROOT}/${D}" ]; then
        cp -R "${PROJECT_ROOT}/${D}" "${BUILD_DIR}/"
    fi
done
mkdir -p "${BUILD_DIR}/Saves"

# Clean build metadata & development artifacts from Release folder
rm -rf "${BUILD_DIR}/CMakeFiles" "${BUILD_DIR}/vcpkg_installed" "${BUILD_DIR}/rme.dir" "${BUILD_DIR}/ALL_BUILD.dir" "${BUILD_DIR}/ZERO_CHECK.dir" "${BUILD_DIR}/.ninja_deps" "${BUILD_DIR}/.ninja_log" "${BUILD_DIR}/build.ninja" "${BUILD_DIR}/rules.ninja" "${BUILD_DIR}/CMakeCache.txt" "${BUILD_DIR}/cmake_install.cmake" "${BUILD_DIR}/vcpkg-manifest-install.log"
find "${BUILD_DIR}" -type f \( -name "*.o" -o -name "*.a" -o -name "*.log" \) -delete 2>/dev/null || true

echo -e "  ${GREEN}Deployment OK${RESET}"

echo -e "\n${GREEN}========================================================${RESET}"
echo -e "  Build Successful! Binary pack location:"
echo -e "  ${BUILD_DIR}"
echo -e "${GREEN}========================================================${RESET}\n"
exit 0
