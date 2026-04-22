"""
Generate CC-11/12/13 missing data files from existing domain data.

Reads data/domain/ JSON files and produces:
  - data/hardware_constraints.json (CC-12)
  - data/project_dependencies.json  (CC-13)
  - data/api_manifest.json          (CC-11)
"""

import json
from pathlib import Path
from datetime import datetime

PERIPHERALS = [
    "GPIO", "UART", "SPI", "I2C", "ADC", "DAC", "TIMER", "PWM", "DMA",
    "EXTI", "RTC", "WDT", "FLASH", "USB", "BLE", "IWDG", "LDO", "RCO",
    "PWC", "SADC", "STEPPER", "IR",
]


def _classify_peripheral(api_name: str) -> str:
    """Extract peripheral prefix from API name."""
    for p in PERIPHERALS:
        if api_name.upper().startswith(p):
            return p
    return ""


def _classify_module(api_name: str) -> str:
    """Classify API into module (ble/driver)."""
    if api_name.startswith(("ble_", "BLE", "ke_", "KE")):
        return "ble"
    return "driver"


def generate_hardware_constraints(data_dir: Path) -> None:
    """CC-12: Generate hardware_constraints.json."""
    print("Generating hardware_constraints.json...")
    pin_mux = json.loads(
        (data_dir / "domain" / "hardware" / "pin_mux.json").read_text(encoding="utf-8")
    )
    memory_regions = json.loads(
        (data_dir / "domain" / "hardware" / "memory_regions.json").read_text(encoding="utf-8")
    )

    pin_to_peripheral = {}
    peripheral_to_pin = {}

    for entry in pin_mux:
        pin = entry.get("pin_name", "")
        for func in entry.get("functions", []):
            periph = func.get("peripheral", "")
            if not periph:
                continue
            pin_to_peripheral.setdefault(pin, set()).add(periph)
            peripheral_to_pin.setdefault(periph, set()).add(pin)

    hw_data = {
        "version": "1.0.0",
        "generated": datetime.now().isoformat(),
        "total_pins": len(pin_mux),
        "total_peripherals": len(peripheral_to_pin),
        "pin_mux": pin_mux,
        "memory_regions": memory_regions,
        "pin_to_peripheral": {k: sorted(v) for k, v in sorted(pin_to_peripheral.items())},
        "peripheral_to_pin": {k: sorted(v) for k, v in sorted(peripheral_to_pin.items())},
    }

    out = data_dir / "hardware_constraints.json"
    out.write_text(json.dumps(hw_data, indent=2, ensure_ascii=False), encoding="utf-8")
    print(f"  -> {len(pin_mux)} pins, {len(peripheral_to_pin)} peripherals")


def generate_project_dependencies(data_dir: Path) -> None:
    """CC-13: Generate project_dependencies.json."""
    print("Generating project_dependencies.json...")
    call_chains = json.loads(
        (data_dir / "domain" / "applications" / "call_chains.json").read_text(encoding="utf-8")
    )
    project_configs = json.loads(
        (data_dir / "domain" / "applications" / "project_configs.json").read_text(encoding="utf-8")
    )

    peripheral_to_projects = {}
    api_to_projects = {}
    projects = {}

    for proj_name, chain in call_chains.items():
        sdk_apis = chain.get("sdk_apis_used", [])

        # Derive peripheral usage from API names
        periph_usage = {}
        for api in sdk_apis:
            periph = _classify_peripheral(api)
            if periph:
                periph_usage[periph] = periph_usage.get(periph, 0) + 1
                peripheral_to_projects.setdefault(periph, set()).add(proj_name)

            api_to_projects.setdefault(api, set()).add(proj_name)

        projects[proj_name] = {
            "call_chain": {
                "entry_point": chain.get("entry_point", "main"),
                "init_sequence": [],
                "total_apis": len(sdk_apis),
                "peripheral_usage": periph_usage,
                "user_funcs_used": chain.get("user_funcs_used", []),
                "max_depth": chain.get("max_depth", 0),
            },
            "sdk_apis_used": sdk_apis,
        }

    # Merge project config data
    for proj_name, config in project_configs.items():
        if proj_name in projects:
            projects[proj_name]["config"] = {
                "enabled_profiles": config.get("enabled_profiles", []),
                "linked_libraries": config.get("linked_libraries", []),
                "include_paths": config.get("include_paths", []),
            }

    dep_data = {
        "version": "1.0.0",
        "generated": datetime.now().isoformat(),
        "total_projects": len(projects),
        "library_to_projects": {},
        "api_to_projects": {k: sorted(v) for k, v in sorted(api_to_projects.items())},
        "peripheral_to_projects": {
            k: sorted(v) for k, v in sorted(peripheral_to_projects.items())
        },
        "projects": {k: v for k, v in sorted(projects.items())},
    }

    out = data_dir / "project_dependencies.json"
    out.write_text(json.dumps(dep_data, indent=2, ensure_ascii=False), encoding="utf-8")
    print(
        f"  -> {len(projects)} projects, "
        f"{len(api_to_projects)} APIs, "
        f"{len(peripheral_to_projects)} peripherals"
    )


def generate_api_manifest(data_dir: Path) -> None:
    """CC-11: Generate api_manifest.json."""
    print("Generating api_manifest.json...")
    examples = json.loads(
        (data_dir / "domain" / "applications" / "examples.json").read_text(encoding="utf-8")
    )

    api_to_examples = {}
    example_to_apis = {}
    api_call_sites = {}
    api_stats = {}
    example_metadata = {}
    scan_scope = {}
    all_calls = 0

    for ex in examples:
        ex_name = ex.get("name", "unknown")
        ex_type = ex.get("type", "unknown")
        ex_key = f"{ex_type}/{ex_name}"
        scan_scope[ex_type] = scan_scope.get(ex_type, 0) + 1

        used = ex.get("used_apis", [])
        for api in used:
            api_to_examples.setdefault(api, set()).add(ex_key)
            example_to_apis.setdefault(ex_key, set()).add(api)
            all_calls += 1

            api_call_sites.setdefault(api, []).append({
                "file": ex.get("file_path", ""),
                "example": ex_name,
                "function": "",
                "context": "",
            })

        # Example metadata
        periph_breakdown = {}
        for api in used:
            periph = _classify_peripheral(api)
            if periph:
                periph_breakdown[periph] = periph_breakdown.get(periph, 0) + 1

        example_metadata[ex_key] = {
            "name": ex_name,
            "type": ex_type,
            "api_count": len(used),
            "file_count": 1,
            "peripheral_breakdown": periph_breakdown,
        }

    # Build api_statistics
    for api in api_to_examples:
        call_sites = api_call_sites.get(api, [])
        api_stats[api] = {
            "usage_count": len(call_sites),
            "example_count": len(api_to_examples[api]),
            "peripheral": _classify_peripheral(api),
            "module": _classify_module(api),
            "files": list({s["file"] for s in call_sites if s["file"]}),
        }

    manifest = {
        "manifest_version": "1.0.0",
        "generation_timestamp": datetime.now().isoformat(),
        "scan_scope": scan_scope,
        "total_examples": len(example_to_apis),
        "total_files": len(example_metadata),
        "total_api_calls": all_calls,
        "unique_sdk_apis_used": len(api_to_examples),
        "api_to_examples": {k: sorted(v) for k, v in sorted(api_to_examples.items())},
        "example_to_apis": {k: sorted(v) for k, v in sorted(example_to_apis.items())},
        "api_call_sites": api_call_sites,
        "api_statistics": api_stats,
        "example_metadata": example_metadata,
    }

    out = data_dir / "api_manifest.json"
    out.write_text(json.dumps(manifest, indent=2, ensure_ascii=False), encoding="utf-8")
    print(
        f"  -> {len(api_to_examples)} APIs, "
        f"{len(example_to_apis)} examples, "
        f"{all_calls} calls"
    )


def main():
    data_dir = Path(__file__).parent.parent / "data"

    if not data_dir.exists():
        print(f"ERROR: data directory not found: {data_dir}")
        return

    generate_hardware_constraints(data_dir)
    generate_project_dependencies(data_dir)
    generate_api_manifest(data_dir)
    print("\nDone! All 3 files generated.")


if __name__ == "__main__":
    main()
