#!/usr/bin/env bash
#
# Reproducible FRDM-MCXN947 environment manager.
# Supported host: Linux x86_64, especially Ubuntu 22.04/24.04 and WSL2.
#
set -uo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"

BOARD="${MCXN_BOARD:-frdmmcxn947}"
MANIFEST_HTTPS_URL="https://github.com/nxp-mcuxpresso/mcuxsdk-manifests.git"
SDK_TAG="${MCXN_SDK_TAG:-v26.09.00}"
TRANSPORT="${MCXN_GIT_TRANSPORT:-}"
ASSUME_YES=0
INTERACTIVE=1
[[ -t 0 && -t 1 ]] || INTERACTIVE=0
if (( EUID == 0 )); then
    SUDO=()
else
    SUDO=(sudo)
fi

ARM_ARCHIVE="arm-gnu-toolchain-14.2.rel1-x86_64-arm-none-eabi.tar.xz"
ARM_URL="https://developer.arm.com/-/media/Files/downloads/gnu/14.2.rel1/binrel/$ARM_ARCHIVE"
ARM_SHA256="62a63b981fe391a9cbad7ef51b17e49aeaa3e7b0d029b36ca1e9c3b2a9b78823"

LINKSERVER_ARCHIVE="LinkServer_26.9.130.x86_64.deb.bin"
LINKSERVER_URL="https://www.nxp.com/lgfiles/updates/mcuxpresso/$LINKSERVER_ARCHIVE"
LINKSERVER_SHA256="09c0c02635cba4e73ab9b25f7dd1422880db1ca3d6e34886964b4d7ecfa77cbf"

APT_PACKAGES=(
    git python3 python3-venv curl xz-utils unzip dpkg-dev
    build-essential ninja-build pkg-config usbutils
)

PYTHON_PACKAGES=(
    "west==1.5.0"
    "cmake==3.31.10"
    "ninja==1.13.2"
    "pyserial==3.5"
    "pyelftools==0.33"
    "jsonschema==4.26.0"
    "requests==2.34.2"
    "Jinja2==3.1.6"
)

pass() { printf 'PASS  %s\n' "$*"; }
warn() { printf 'WARN  %s\n' "$*"; }
fail() { printf 'FAIL  %s\n' "$*"; }

die() {
    printf 'ERROR %s\n' "$*" >&2
    exit 1
}

usage() {
    cat <<'EOF'
Usage: manage_dev_env.sh [OPTIONS]

Options:
  --check                 Check the development environment only
  --setup                 Install or repair missing dependencies
  --check-updates         Check for a newer SDK manifest release
  --update                Update the current SDK after confirmation
  --fresh                 Create a separate workspace from the latest release
  --transport ssh|https   Select Git transport (default: ask, SSH preferred)
  --yes                   Accept non-destructive confirmation prompts
  --no-input              Disable interactive prompts
  -h, --help              Show this help

Without an action option, an interactive menu is displayed.
EOF
}

source_project_env() {
    # shellcheck disable=SC1091
    source "$SCRIPT_DIR/env.sh" >/dev/null 2>&1 || true
}

confirm() {
    local prompt="$1"
    local answer

    if (( ASSUME_YES )); then
        return 0
    fi
    if (( ! INTERACTIVE )); then
        warn "confirmation required for: $prompt; rerun with --yes"
        return 1
    fi
    read -r -p "$prompt [y/N] " answer
    [[ "$answer" =~ ^[Yy]$ ]]
}

version_ge() {
    local actual="$1"
    local minimum="$2"

    [[ "$(printf '%s\n%s\n' "$minimum" "$actual" | sort -V | tail -n 1)" == "$actual" ]]
}

choose_transport() {
    local answer

    if [[ "$TRANSPORT" == "ssh" || "$TRANSPORT" == "https" ]]; then
        return 0
    fi
    if [[ -n "$TRANSPORT" ]]; then
        die "invalid transport '$TRANSPORT'; use ssh or https"
    fi
    if (( ! INTERACTIVE )); then
        TRANSPORT="ssh"
        return 0
    fi

    printf '\nGit transport:\n'
    printf '  1) SSH (recommended)\n'
    printf '  2) HTTPS\n'
    read -r -p 'Select [1]: ' answer
    case "${answer:-1}" in
        2) TRANSPORT="https" ;;
        *) TRANSPORT="ssh" ;;
    esac
}

transport_exec() {
    if [[ "$TRANSPORT" == "ssh" ]]; then
        env \
            GIT_CONFIG_COUNT=1 \
            GIT_CONFIG_KEY_0="url.git@github.com:.insteadOf" \
            GIT_CONFIG_VALUE_0="https://github.com/" \
            GIT_SSH_COMMAND="ssh -o BatchMode=yes -o StrictHostKeyChecking=accept-new -o ConnectTimeout=10" \
            "$@"
    else
        "$@"
    fi
}

ensure_transport_access() {
    local answer

    choose_transport
    if [[ "$TRANSPORT" != "ssh" ]]; then
        pass 'using HTTPS for GitHub access'
        return 0
    fi

    printf 'Checking GitHub SSH access...\n'
    if transport_exec git ls-remote --heads "$MANIFEST_HTTPS_URL" HEAD >/dev/null 2>&1; then
        pass 'GitHub SSH access works'
        return 0
    fi

    warn 'GitHub SSH access failed (key, known_hosts or network)'
    if (( INTERACTIVE )) && confirm 'Use HTTPS instead?'; then
        TRANSPORT="https"
        return 0
    fi
    die 'configure SSH access or rerun with --transport https'
}

check_environment() {
    local failures=0
    local required
    local available_kb
    local python_version
    local missing=()

    source_project_env
    printf '\n== Host and kernel ==\n'
    printf 'System: %s\n' "$(uname -srm)"
    if [[ -r /etc/os-release ]]; then
        # shellcheck disable=SC1091
        source /etc/os-release
        printf 'Distribution: %s\n' "${PRETTY_NAME:-unknown}"
        if [[ "${ID:-}" != "ubuntu" && "${ID:-}" != "debian" ]]; then
            warn 'automatic package installation supports Ubuntu/Debian only'
        fi
    else
        warn '/etc/os-release is unavailable'
    fi

    if [[ "$(uname -m)" != "x86_64" ]]; then
        fail "unsupported architecture $(uname -m); pinned NXP tools are x86_64"
        failures=$((failures + 1))
    else
        pass 'x86_64 architecture'
    fi

    if [[ "$(uname -s)" != "Linux" ]]; then
        fail 'Linux is required; use WSL2 on Windows'
        failures=$((failures + 1))
    else
        pass 'Linux kernel'
    fi

    printf '\n== Required commands ==\n'
    for required in git python3 curl tar xz unzip ninja west cmake \
                    arm-none-eabi-gcc LinkServer; do
        if command -v "$required" >/dev/null 2>&1; then
            pass "$required: $(command -v "$required")"
        else
            fail "missing command: $required"
            missing+=("$required")
            failures=$((failures + 1))
        fi
    done

    if (( EUID == 0 )); then
        pass 'running as root; sudo is not required'
    elif command -v sudo >/dev/null 2>&1; then
        pass "sudo: $(command -v sudo)"
    else
        fail 'sudo is required for system package installation'
        failures=$((failures + 1))
    fi

    if command -v python3 >/dev/null; then
        python_version="$(python3 -c 'import sys; print("%d.%d" % sys.version_info[:2])')"
        if awk -v v="$python_version" 'BEGIN { exit !(v >= 3.10) }'; then
            pass "Python $python_version"
        else
            fail "Python 3.10+ is required, found $python_version"
            failures=$((failures + 1))
        fi
    fi

    if command -v arm-none-eabi-gcc >/dev/null; then
        local gcc_version
        gcc_version="$(arm-none-eabi-gcc -dumpversion)"
        if [[ "$gcc_version" == 14.2.* ]]; then
            pass "Arm GCC $gcc_version"
        else
            fail "Arm GCC 14.2.x is required, found $gcc_version"
            failures=$((failures + 1))
        fi
    fi

    if command -v west >/dev/null; then
        local west_version
        west_version="$(west --version | awk '{print $NF}' | sed 's/^v//')"
        if [[ "$west_version" == "1.5.0" ]]; then
            pass "west $west_version"
        else
            fail "west 1.5.0 is required, found $west_version"
            failures=$((failures + 1))
        fi
    fi

    if command -v cmake >/dev/null; then
        local cmake_version
        cmake_version="$(cmake --version | awk 'NR==1 {print $3}')"
        if version_ge "$cmake_version" "3.22.0"; then
            pass "CMake $cmake_version"
        else
            fail "CMake 3.22+ is required, found $cmake_version"
            failures=$((failures + 1))
        fi
    fi

    if command -v ninja >/dev/null; then
        local ninja_version
        ninja_version="$(ninja --version | sed -E 's/^([0-9]+\.[0-9]+\.[0-9]+).*/\1/')"
        if version_ge "$ninja_version" "1.11.0"; then
            pass "Ninja $ninja_version"
        else
            fail "Ninja 1.11+ is required, found $ninja_version"
            failures=$((failures + 1))
        fi
    fi

    if command -v git >/dev/null; then
        local git_version
        git_version="$(git --version | awk '{print $3}')"
        if version_ge "$git_version" "2.22.0"; then
            pass "Git $git_version"
        else
            fail "Git 2.22+ is required, found $git_version"
            failures=$((failures + 1))
        fi
    fi

    if command -v LinkServer >/dev/null; then
        local linkserver_version
        linkserver_version="$(LinkServer --version 2>/dev/null | sed -n 's/^LinkServer v\([0-9.]*\).*/\1/p' | head -n 1)"
        if [[ "$linkserver_version" == "26.9.130" ]]; then
            pass "LinkServer $linkserver_version"
        else
            fail "LinkServer 26.9.130 is required, found ${linkserver_version:-unknown}"
            failures=$((failures + 1))
        fi
    fi

    available_kb="$(df -Pk "$REPO_ROOT" | awk 'NR==2 {print $4}')"
    if [[ "$available_kb" =~ ^[0-9]+$ ]] && (( available_kb >= 20 * 1024 * 1024 )); then
        pass "free disk: $((available_kb / 1024 / 1024)) GiB"
    else
        fail 'at least 20 GiB free disk is recommended for SDK, tools and builds'
        failures=$((failures + 1))
    fi

    printf '\n== Project workspace ==\n'
    if [[ -f "$MCXN_SDK/.west/config" ]]; then
        pass "west workspace: $MCXN_SDK"
    else
        fail "west workspace not initialized: $MCXN_SDK"
        failures=$((failures + 1))
    fi
    if [[ -x "$REPO_ROOT/apps/touchscreen_tester/build.sh" ]]; then
        pass "application build entry: $REPO_ROOT/apps/touchscreen_tester/build.sh"
    else
        fail 'touchscreen_tester build.sh is missing or not executable'
        failures=$((failures + 1))
    fi

    if command -v LinkServer >/dev/null 2>&1; then
        if LinkServer probes 2>/dev/null | grep -q 'FRDM-MCXN947'; then
            pass 'FRDM-MCXN947 MCU-Link probe detected'
        else
            warn 'MCU-Link probe not detected; building still works, flashing will not'
        fi
    fi

    printf '\n== Environment result ==\n'
    if (( failures == 0 )); then
        pass 'environment satisfies current project requirements'
        return 0
    fi

    fail "$failures requirement(s) missing or invalid"
    if (( ${#missing[@]} > 0 )); then
        printf 'Missing commands: %s\n' "${missing[*]}"
    fi
    return 1
}

download_verified() {
    local url="$1"
    local output="$2"
    local expected="$3"
    local partial="${output}.part"
    local actual

    mkdir -p "$(dirname "$output")"
    if [[ -f "$output" ]]; then
        actual="$(sha256sum "$output" | awk '{print $1}')"
        if [[ "$actual" == "$expected" ]]; then
            pass "verified cached $(basename "$output")"
            return 0
        fi
        die "cached file checksum mismatch: $output; remove it and retry"
    fi

    printf 'Downloading %s\n' "$url"
    curl --fail --location --retry 3 --continue-at - -o "$partial" "$url" ||
        die "download failed: $url"
    actual="$(sha256sum "$partial" | awk '{print $1}')"
    if [[ "$actual" != "$expected" ]]; then
        warn 'downloaded checksum mismatch; retrying once without resume data'
        rm -f "$partial"
        curl --fail --location --retry 3 -o "$partial" "$url" ||
            die "download retry failed: $url"
        actual="$(sha256sum "$partial" | awk '{print $1}')"
    fi
    [[ "$actual" == "$expected" ]] ||
        die "checksum mismatch for $output"
    mv "$partial" "$output"
    pass "verified $(basename "$output")"
}

install_system_packages() {
    command -v apt-get >/dev/null ||
        die 'automatic setup currently supports Ubuntu/Debian apt-get only'
    "${SUDO[@]}" apt-get update
    "${SUDO[@]}" apt-get install -y "${APT_PACKAGES[@]}"
}

install_python_environment() {
    if [[ ! -x "$REPO_ROOT/.venv/bin/python" ]]; then
        python3 -m venv "$REPO_ROOT/.venv"
    fi
    "$REPO_ROOT/.venv/bin/python" -m pip install --upgrade pip
    "$REPO_ROOT/.venv/bin/pip" install "${PYTHON_PACKAGES[@]}"
}

install_toolchain() {
    local archive="$REPO_ROOT/downloads/$ARM_ARCHIVE"
    local toolchain_dir="$REPO_ROOT/toolchains/arm-gnu-toolchain-14.2.rel1-x86_64-arm-none-eabi"

    download_verified "$ARM_URL" "$archive" "$ARM_SHA256"
    if [[ ! -x "$toolchain_dir/bin/arm-none-eabi-gcc" ]]; then
        mkdir -p "$REPO_ROOT/toolchains"
        tar -xJf "$archive" -C "$REPO_ROOT/toolchains"
    fi
}

install_linkserver() {
    local installer="$REPO_ROOT/downloads/$LINKSERVER_ARCHIVE"
    local extract_base="$REPO_ROOT/downloads/linkserver_extract"
    local extract_dir="$extract_base/LinkServer_26.9.130.x86_64.deb"
    local install_root="$REPO_ROOT/tools/linkserver"

    if [[ -x "$install_root/usr/local/LinkServer_26.9.130/LinkServer" ]]; then
        return 0
    fi

    download_verified "$LINKSERVER_URL" "$installer" "$LINKSERVER_SHA256"
    chmod +x "$installer"
    if [[ ! -d "$extract_dir" ]]; then
        mkdir -p "$extract_base"
        env DISPLAY= "$installer" --noexec --target "$extract_base"
    fi
    mkdir -p "$install_root"
    dpkg-deb -x "$extract_dir" "$install_root"
}

init_sdk_workspace() {
    local target_root="$1"
    local target_tag="$2"

    if [[ -f "$target_root/.west/config" ]]; then
        pass "reuse west workspace: $target_root"
        return 0
    fi
    if [[ -e "$target_root" && -n "$(find "$target_root" -mindepth 1 -maxdepth 1 -print -quit 2>/dev/null)" ]]; then
        die "SDK directory exists but is not a west workspace: $target_root"
    fi

    mkdir -p "$(dirname "$target_root")"
    transport_exec west init \
        -m "$MANIFEST_HTTPS_URL" \
        "$target_root" \
        --mr "$target_tag"

    (
        cd "$target_root"
        transport_exec west update_board --set board "$BOARD"
    )
}

setup_environment() {
    printf '\n== Install or repair development environment ==\n'
    install_system_packages
    install_python_environment
    source_project_env
    install_toolchain
    source_project_env
    ensure_transport_access
    init_sdk_workspace "$MCXN_SDK" "$SDK_TAG"
    install_linkserver
    source_project_env

    if [[ -f "$REPO_ROOT/tools/linkserver/lib/udev/rules.d/85-linkserver_26.9.130.rules" ]]; then
        if (( INTERACTIVE )) && confirm 'Install LinkServer udev permissions now?'; then
            bash "$SCRIPT_DIR/install_udev_rules.sh"
        fi
    fi

    check_environment
}

latest_release_tag() {
    local tags

    tags="$(transport_exec git ls-remote --tags --refs \
        "$MANIFEST_HTTPS_URL" 'v*' 2>/dev/null)" ||
        return 1

    printf '%s\n' "$tags" |
        sed -n 's#.*refs/tags/\(v[0-9][0-9]\.[0-9][0-9]\.[0-9][0-9]\)$#\1#p' |
        sort -V |
        tail -n 1
}

remote_tag_hash() {
    local tag="$1"

    transport_exec git ls-remote --tags --refs \
        "$MANIFEST_HTTPS_URL" "refs/tags/$tag" 2>/dev/null |
        awk 'NR==1 {print $1}'
}

check_updates() {
    local current_tag
    local current_hash=""
    local latest_tag
    local latest_hash
    local compare_output
    local has_local_changes=0

    if [[ ! -f "$MCXN_SDK/.west/config" ]]; then
        fail 'SDK workspace is not initialized; run --setup first'
        return 1
    fi
    ensure_transport_access || return 1

    latest_tag="$(latest_release_tag)"
    [[ -n "$latest_tag" ]] ||
        die 'unable to query SDK release tags'

    current_tag="$(git -C "$MCXN_SDK/manifests" describe --tags --exact-match 2>/dev/null || true)"
    [[ -n "$current_tag" ]] ||
        current_tag="$(git -C "$MCXN_SDK/manifests" rev-parse --short HEAD)"
    if [[ "$current_tag" == v* ]]; then
        current_hash="$(git -C "$MCXN_SDK/manifests" rev-parse "$current_tag^{commit}")"
    fi
    latest_hash="$(remote_tag_hash "$latest_tag")"

    printf '\n== SDK update check ==\n'
    printf 'Current manifest: %s %s\n' "$current_tag" "$current_hash"
    printf 'Latest release:   %s %s\n' "$latest_tag" "$latest_hash"
    printf 'Transport:        %s\n' "$TRANSPORT"

    UPDATE_AVAILABLE=0
    UPDATE_TAG="$latest_tag"
    if [[ "$(printf '%s\n%s\n' "$current_tag" "$latest_tag" | sort -V | tail -n 1)" == "$latest_tag" &&
          "$current_tag" != "$latest_tag" ]]; then
        printf 'A newer SDK release is available.\n'
        UPDATE_AVAILABLE=1
    elif [[ -n "$current_hash" && -n "$latest_hash" &&
            "$current_hash" != "$latest_hash" ]]; then
        printf 'The same release tag points to a different remote commit.\n'
        UPDATE_AVAILABLE=1
    else
        printf 'No newer stable SDK release tag was found.\n'
    fi

    compare_output="$(
        cd "$MCXN_SDK" || exit 1
        west compare --ignore-branches 2>&1 || true
    )"
    if [[ -n "$compare_output" ]]; then
        if printf '%s\n' "$compare_output" |
            grep -Eq 'Changes not staged|Changes to be committed|Unmerged path'; then
            has_local_changes=1
        fi
        printf '\nProjects differing from the last west update or containing local changes:\n'
        printf '%s\n' "$compare_output" | sed -n '1,100p'
    else
        printf 'All active projects match the current manifest and working-tree state.\n'
    fi

    if (( has_local_changes )); then
        warn 'tracked local SDK changes were detected; an in-place update will be refused'
    fi
    return 0
}

apply_project_patches() {
    local patch_dir="$REPO_ROOT/firmware/patches"
    local examples="$MCXN_SDK/mcuxsdk/examples"
    local patch_file

    [[ -d "$patch_dir" ]] || return 0
    for patch_file in "$patch_dir"/*.patch; do
        [[ -f "$patch_file" ]] || continue
        git -C "$examples" apply --check "$patch_file" ||
            die "patch no longer applies: $(basename "$patch_file")"
    done
    for patch_file in "$patch_dir"/*.patch; do
        [[ -f "$patch_file" ]] || continue
        git -C "$examples" apply "$patch_file"
        pass "applied $(basename "$patch_file")"
    done
}

detect_tracked_sdk_changes() {
    local report_file
    local result=1

    report_file="$(mktemp)"
    (
        cd "$MCXN_SDK" || exit 1
        west compare --ignore-branches >"$report_file" 2>&1 || true
    )
    if grep -Eq 'Changes not staged|Changes to be committed|Unmerged path' "$report_file"; then
        result=0
    fi
    rm -f "$report_file"
    return "$result"
}

update_current_workspace() {
    check_updates || return 1
    if (( ! UPDATE_AVAILABLE )); then
        warn 'no release update selected; current SDK remains unchanged'
        return 0
    fi

    if ! bash "$SCRIPT_DIR/manage_dev_env.sh" --check >/dev/null 2>&1; then
        die 'environment check failed; run --setup before updating'
    fi

    if git -C "$MCXN_SDK/manifests" diff --quiet &&
       git -C "$MCXN_SDK/manifests" diff --cached --quiet; then
        :
    else
        die 'manifest repository has tracked changes; commit or save them first'
    fi

    if detect_tracked_sdk_changes; then
        warn 'SDK projects contain tracked local changes'
        printf 'Use a separate new workspace for a safe update, or preserve/remove those changes.\n'
        return 1
    fi

    confirm "Update SDK to $UPDATE_TAG and synchronize $BOARD components?" ||
        return 1

    transport_exec git -C "$MCXN_SDK/manifests" fetch origin --tags --force
    git -C "$MCXN_SDK/manifests" checkout "$UPDATE_TAG"
    (
        cd "$MCXN_SDK" || exit 1
        transport_exec west update_board --set board "$BOARD"
    )

    if (( INTERACTIVE )) && confirm 'Apply project display patches to the updated SDK?'; then
        apply_project_patches
    fi
    if (( INTERACTIVE )) && confirm 'Build and audit the updated application now?'; then
        "$REPO_ROOT/apps/touchscreen_tester/build.sh"
    fi
}

create_fresh_workspace() {
    local latest_tag
    local target="$REPO_ROOT/mcuxpresso-sdk-next"

    ensure_transport_access || return 1
    latest_tag="$(latest_release_tag)"
    [[ -n "$latest_tag" ]] || die 'unable to determine latest SDK release'
    [[ ! -e "$target" ]] ||
        die "target already exists: $target"

    confirm "Create separate workspace $target from $latest_tag?" ||
        return 1
    init_sdk_workspace "$target" "$latest_tag"

    printf '\nFresh workspace created:\n  %s\n' "$target"
    printf 'Use it with:\n  export MCXN_SDK=%q\n' "$target"
}

interactive_menu() {
    local choice

    while true; do
        cat <<EOF

FRDM-MCXN947 development environment
  1) Check environment
  2) Install or repair environment
  3) Check SDK/component updates
  4) Update current SDK
  5) Create separate updated SDK workspace
  0) Exit
EOF
        read -r -p 'Select: ' choice
        case "$choice" in
            1)
                if ! check_environment && confirm 'Install missing environment now?'; then
                    setup_environment
                fi
                ;;
            2) setup_environment ;;
            3) check_updates ;;
            4) update_current_workspace ;;
            5) create_fresh_workspace ;;
            0) return 0 ;;
            *) warn 'invalid selection' ;;
        esac
    done
}

ACTION=""
while (( $# > 0 )); do
    case "$1" in
        --check) ACTION="check" ;;
        --setup) ACTION="setup" ;;
        --check-updates) ACTION="check-updates" ;;
        --update) ACTION="update" ;;
        --fresh) ACTION="fresh" ;;
        --transport)
            shift
            [[ $# -gt 0 ]] || die '--transport requires ssh or https'
            TRANSPORT="$1"
            ;;
        --yes) ASSUME_YES=1 ;;
        --no-input) INTERACTIVE=0 ;;
        -h|--help) usage; exit 0 ;;
        *) die "unknown option: $1" ;;
    esac
    shift
done

source_project_env

case "$ACTION" in
    check) check_environment ;;
    setup) setup_environment ;;
    check-updates) check_updates ;;
    update) update_current_workspace ;;
    fresh) create_fresh_workspace ;;
    "")
        if (( INTERACTIVE )); then
            if ! check_environment && confirm 'Environment is incomplete. Install or repair it now?'; then
                setup_environment
            fi
            interactive_menu
        else
            usage
            exit 2
        fi
        ;;
esac
