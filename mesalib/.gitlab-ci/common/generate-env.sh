#!/bin/bash

VARS=(
    ACO_DEBUG
    ARTIFACTS_BASE_URL
    ASAN_OPTIONS
    BASE_SYSTEM_FORK_HOST_PREFIX
    BASE_SYSTEM_MAINLINE_HOST_PREFIX
    CI_COMMIT_BRANCH
    CI_COMMIT_REF_NAME
    CI_COMMIT_TITLE
    CI_JOB_ID
    S3_JWT_FILE
    CI_JOB_STARTED_AT
    CI_JOB_NAME
    CI_JOB_URL
    CI_MERGE_REQUEST_SOURCE_BRANCH_NAME
    CI_MERGE_REQUEST_TITLE
    CI_NODE_INDEX
    CI_NODE_TOTAL
    CI_PAGES_DOMAIN
    CI_PIPELINE_ID
    CI_PIPELINE_URL
    CI_PROJECT_DIR
    CI_PROJECT_NAME
    CI_PROJECT_PATH
    CI_PROJECT_ROOT_NAMESPACE
    CI_RUNNER_DESCRIPTION
    CI_SERVER_URL
    CROSVM_GALLIUM_DRIVER
    CROSVM_GPU_ARGS
    CURRENT_SECTION
    DEQP_BIN_DIR
    DEQP_CONFIG
    DEQP_EXPECTED_RENDERER
    DEQP_FRACTION
    DEQP_HEIGHT
    DEQP_RESULTS_DIR
    DEQP_RUNNER_OPTIONS
    DEQP_SUITE
    DEQP_TEMP_DIR
    DEQP_VER
    DEQP_WIDTH
    DEVICE_NAME
    DRIVER_NAME
    EGL_PLATFORM
    ETNA_MESA_DEBUG
    FDO_CI_CONCURRENT
    FDO_UPSTREAM_REPO
    FD_MESA_DEBUG
    FLAKES_CHANNEL
    FREEDRENO_HANGCHECK_MS
    GALLIUM_DRIVER
    GALLIVM_PERF
    GPU_VERSION
    GTEST
    GTEST_FAILS
    GTEST_FRACTION
    GTEST_RESULTS_DIR
    GTEST_RUNNER_OPTIONS
    GTEST_SKIPS
    HWCI_FREQ_MAX
    HWCI_KERNEL_MODULES
    HWCI_KVM
    HWCI_START_WESTON
    HWCI_START_XORG
    HWCI_TEST_SCRIPT
    IR3_SHADER_DEBUG
    JOB_ARTIFACTS_BASE
    JOB_RESULTS_PATH
    JOB_ROOTFS_OVERLAY_PATH
    KERNEL_IMAGE_BASE
    KERNEL_IMAGE_NAME
    LD_LIBRARY_PATH
    LIBGL_ALWAYS_SOFTWARE
    LP_NUM_THREADS
    MESA_BASE_TAG
    MESA_BUILD_PATH
    MESA_DEBUG
    MESA_GLES_VERSION_OVERRIDE
    MESA_GLSL_VERSION_OVERRIDE
    MESA_GL_VERSION_OVERRIDE
    MESA_IMAGE
    MESA_IMAGE_PATH
    MESA_IMAGE_TAG
    MESA_LOADER_DRIVER_OVERRIDE
    MESA_TEMPLATES_COMMIT
    MESA_VK_ABORT_ON_DEVICE_LOSS
    MESA_VK_IGNORE_CONFORMANCE_WARNING
    S3_HOST
    S3_RESULTS_UPLOAD
    NIR_DEBUG
    PAN_I_WANT_A_BROKEN_VULKAN_DRIVER
    PAN_MESA_DEBUG
    PANVK_DEBUG
    PIGLIT_FRACTION
    PIGLIT_NO_WINDOW
    PIGLIT_OPTIONS
    PIGLIT_PLATFORM
    PIGLIT_PROFILES
    PIGLIT_REPLAY_ANGLE_TAG
    PIGLIT_REPLAY_ARTIFACTS_BASE_URL
    PIGLIT_REPLAY_DEVICE_NAME
    PIGLIT_REPLAY_EXTRA_ARGS
    PIGLIT_REPLAY_LOOP_TIMES
    PIGLIT_REPLAY_REFERENCE_IMAGES_BASE
    PIGLIT_REPLAY_SUBCOMMAND
    PIGLIT_RESULTS
    PIGLIT_TESTS
    PIGLIT_TRACES_FILE
    PIPELINE_ARTIFACTS_BASE
    RADEON_DEBUG
    RADV_DEBUG
    RADV_PERFTEST
    SKQP_ASSETS_DIR
    SKQP_BACKENDS
    TU_DEBUG
    USE_ANGLE
    VIRGL_HOST_API
    VIRGL_RENDER_SERVER
    WAFFLE_PLATFORM
    VK_DRIVER
    VKD3D_PROTON_RESULTS
    VKD3D_CONFIG
    VKD3D_TEST_EXCLUDE
    ZINK_DESCRIPTORS
    ZINK_DEBUG
    LVP_POISON_MEMORY

    # Dead code within Mesa CI, but required by virglrender CI
    # (because they include our files in their CI)
    VK_DRIVER_FILES
)

for var in "${VARS[@]}"; do
  if [ -n "${!var+x}" ]; then
    echo "export $var=${!var@Q}"
  fi
done
