#!/usr/bin/env bash
# Copyright (c) 2021 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

set -eu

echo "Setting up environment"

export ZEPHYR_SDK_INSTALL_DIR="/opt/toolchains/zephyr-sdk-${ZSDK_VERSION}"

# Skip if a workspace already exists
config_path="/workspaces/.west/config"
if [ -f "$config_path" ]; then
    echo "West .config exists, skipping init and update."
    exit 0
fi

# Can that have bad consequences if host UID != 1000?
sudo chown user:user /workspaces

# Set up west workspace.
west init -l /workspaces/esl
west config --global update.narrow true
west update --path-cache /workspace-cache

# Reset every project except the main zephyr repo.
west forall -c 'pwd | xargs basename | xargs test "esl" != && git reset --hard HEAD' || true

echo "West initialized successfully"
