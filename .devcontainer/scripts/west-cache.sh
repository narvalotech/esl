#!/usr/bin/env bash
# Copyright (c) 2024 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

echo "Making the west cache"

# Can that have bad consequences if host UID != 1000?
sudo mkdir -p /workspace-cache/esl
sudo chown -R user:user /workspace-cache

# Fetch the workspace into the cache
cd /workspace-cache/esl
cp /west-cache.yml ./west.yml
west init -l .
west config --global update.narrow true
west update

echo "West cache initialized successfully"
