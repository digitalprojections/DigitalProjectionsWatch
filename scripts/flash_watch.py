"""Flash a committed build after validating its private recovery snapshot."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import struct
import subprocess

ROOT = Path(__file__).resolve().parents[1]
BUILD = ROOT / "FirmWare/watch/build"

def partitions(data):
    result = {}
    for i in range(0, min(len(data), 0xC00), 32):
        entry = data[i:i+32]
        if len(entry) != 32 or entry[:2] != b"\xaa\x50":
            continue
        _, kind, subtype, offset, size, label, flags = struct.unpack("<HBBII16sI", entry)
        result[label.split(b"\0")[0].decode()] = (offset, size, kind, subtype)
    return result

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--port", required=True)
    parser.add_argument("--backup", type=Path, required=True)
    parser.add_argument("--migrate-original-nvs", action="store_true")
    parser.add_argument("--app-only", action="store_true", help="Update the app after verifying the installed partition table")
    parser.add_argument("--write", action="store_true", help="Perform writes; otherwise validate only")
    args = parser.parse_args()
    status = subprocess.check_output(["git", "status", "--porcelain"], cwd=ROOT, text=True)
    if status.strip():
        raise SystemExit("Commit the source changes before flashing.")
    manifest = json.loads(args.backup.with_name("manifest.json").read_text())
    snapshot = args.backup.read_bytes()
    if len(snapshot) != 33554432 or hashlib.sha256(snapshot).hexdigest() != manifest["sha256"]:
        raise SystemExit("Recovery snapshot validation failed.")
    config = json.loads((BUILD / "flasher_args.json").read_text())
    if config["extra_esptool_args"]["chip"] != "esp32s3" or config["flash_settings"]["flash_size"] != "32MB":
        raise SystemExit("Build is not configured for this 32 MB ESP32-S3.")
    files = {int(offset, 0): BUILD / name for offset, name in config["flash_files"].items()}
    if args.app_only and args.migrate_original_nvs:
        raise SystemExit("App-only updates cannot migrate NVS.")
    if args.app_only:
        app = config["app"]
        files = {int(app["offset"], 0): BUILD / app["file"]}
    if args.migrate_original_nvs:
        original = partitions(snapshot[0x8000:0x9000])["nvs"]
        target = partitions((BUILD / "partition_table/partition-table.bin").read_bytes())["nvs"]
        if original[1] > target[1]:
            raise SystemExit("Original NVS does not fit the new partition.")
        private = ROOT / ".local"
        private.mkdir(exist_ok=True)
        migrated = private / "migrated-nvs.bin"
        migrated.write_bytes(snapshot[original[0]:original[0]+original[1]] + b"\xff"*(target[1]-original[1]))
        files[target[0]] = migrated
    for offset, file in files.items():
        if not file.is_file() or offset + file.stat().st_size > 33554432:
            raise SystemExit("Missing or out-of-range firmware image: " + file.name)
    revision = subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=ROOT, text=True).strip()
    print("Validated recovery snapshot and firmware for commit", revision)
    if not args.write:
        print("Validation only. Add --write to install.")
        return
    python = Path(os.environ.get("IDF_PYTHON_ENV_PATH", str(Path.home()/".espressif/python_env/idf5.5_py3.10_env"))) / "Scripts/python.exe"
    command = [str(python), "-m", "esptool", "--chip", "esp32s3", "--port", args.port, "--baud", "460800"]
    subprocess.run(command + ["flash_id"], check=True)
    if args.app_only:
        subprocess.run(command + ["--after", "no_reset", "verify_flash", "0x8000", str(BUILD / "partition_table/partition-table.bin")], check=True)
    addresses = [item for offset,file in sorted(files.items()) for item in (hex(offset),str(file))]
    subprocess.run(command + ["--after", "no_reset", "write_flash"] + config["write_flash_args"] + addresses, check=True)
    subprocess.run(command + ["--after", "hard_reset", "verify_flash"] + addresses, check=True)

if __name__ == "__main__":
    main()
