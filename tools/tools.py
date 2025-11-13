#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import argparse
import os
import io

def read_config(path):
    """Read sdkconfig-style file into dict preserving order."""
    if not os.path.exists(path):
        print(f"Warning: {path} not found.")
        return []

    lines = []
    with io.open(path, "r", encoding="utf-8") as f:
        for line in f:
            line = line.rstrip("\n")
            if not line.strip():
                lines.append(("", "", line))  # blank
                continue
            if line.startswith("#"):
                lines.append(("#", "", line))
                continue

            # Split key/value
            if "=" in line:
                k, v = line.split("=", 1)
                lines.append((k.strip(), v.strip(), line))
            else:
                lines.append((line.strip(), "", line))
    return lines


def merge_configs(base_lines, override_lines):
    """Merge base and override sdkconfig lines."""
    merged = {}
    ordered_keys = []

    # Load base
    for key, value, raw in base_lines:
        if key and not key.startswith("#"):
            merged[key] = raw
            ordered_keys.append(key)
        else:
            ordered_keys.append(raw)  # keep comments/blanks

    # Apply overrides
    for key, value, raw in override_lines:
        if key and not key.startswith("#"):
            merged[key] = raw
            if key not in ordered_keys:
                ordered_keys.append(key)
        else:
            ordered_keys.append(raw)

    # Compose final output
    final_lines = []
    for item in ordered_keys:
        if isinstance(item, str) and (item.startswith("#") or "=" not in item):
            final_lines.append(item)
        elif item in merged:
            final_lines.append(merged[item])
    return final_lines


def ensure_required_settings(config_lines):
    """Inject or correct essential PSRAM and SR settings."""
    required_settings = {
        "CONFIG_SPIRAM": "y",
        "CONFIG_SPIRAM_MODE_OCT": "y",
        "CONFIG_SPIRAM_SPEED_80M": "y",
        "CONFIG_SPIRAM_SUPPORT": "y",
        "CONFIG_SPIRAM_USE_CAPS_ALLOC": "y",
        "CONFIG_DL_LIB_USE_PSRAM": "y",
        "CONFIG_SR_USE_PSRAM": "y",
        "CONFIG_AFE_USE_PSRAM": "y",
        "CONFIG_ESP_AUDIO_FRONT_END_USE_PSRAM": "y",
    }

    config_map = {}
    for i, line in enumerate(config_lines):
        if line.startswith("#") or "=" not in line:
            continue
        k, v = line.split("=", 1)
        config_map[k.strip()] = i

    for k, v in required_settings.items():
        line = f"{k}={v}"
        if k in config_map:
            config_lines[config_map[k]] = line
        else:
            config_lines.append(line)
            print(f"Added required: {line}")

    return config_lines


def write_config(lines, output_path):
    """Write final sdkconfig file."""
    os.makedirs(os.path.dirname(output_path) or ".", exist_ok=True)
    with io.open(output_path, "w", encoding="utf-8") as f:
        f.write("\n".join(lines) + "\n")
    print(f"Generated: {output_path}")
    print(f"Total {len(lines)} lines written.")


def main():
    parser = argparse.ArgumentParser(description="Generate merged sdkconfig from defaults")
    parser.add_argument("--target", required=True, help="Board target, e.g. esp32s3")
    parser.add_argument("--output", default="sdkconfig", help="Output file path")
    parser.add_argument("--project-root", default=".", help="Project root directory")
    args = parser.parse_args()

    base_path = os.path.join(args.project_root, "sdkconfig.defaults")
    target_path = os.path.join(args.project_root, f"sdkconfig.defaults.{args.target}")

    print(f"Merging configs:")
    print(f"Base: {base_path}")
    print(f"Target: {target_path}")

    base_lines = read_config(base_path)
    target_lines = read_config(target_path)

    merged_lines = merge_configs(base_lines, target_lines)
    merged_lines = ensure_required_settings(merged_lines)

    write_config(merged_lines, args.output)

    print("Merge completed successfully.")


if __name__ == "__main__":
    main()