"""Check rendered heat contrast, occlusion and restoration in all packaged maps."""
import json
from pathlib import Path
import numpy as np
from PIL import Image

native = Path(__file__).resolve().parents[3]
maps = ["Blackwood", "Frostwood", "Farmstead", "Neighborhood", "NeighborhoodCedar"]
report = {"maps": {}, "passed": True}
for name in maps:
    shots = {state: np.asarray(Image.open(native / "Preview" / f"{name}_Thermal{state}.png").convert("RGB"))
             for state in ["Normal", "Hot", "Cover", "Dead", "Expired"]}
    hot = shots["Hot"]
    mask = hot.min(axis=2) >= 245
    count = int(mask.sum())
    checks = {
        "packaged_gameplay_probe_passed": "WILDLINE THERMAL REVIEW: PASS" in (native / f"thermal-{name}.log").read_text(errors="replace"),
        "white_hot_squirrel_visible": count > 100,
        "thermal_scene_grayscale": float(np.percentile(np.ptp(hot.astype(float), axis=2), 99)) <= 2,
        "background_stays_cool": float(np.percentile(hot[~mask], 99)) < 180,
        "solid_cover_blocks_heat": int((shots["Cover"].min(axis=2) >= 245).sum()) <= max(25, count * .01),
        "dead_squirrel_has_no_heat": int((shots["Dead"].min(axis=2) >= 245).sum()) <= max(25, count * .01),
        "normal_squirrel_is_not_white": count > 0 and float(shots["Normal"][mask].mean()) < 230,
        "expiry_restores_original_color": count > 0 and float(shots["Expired"][mask].mean()) < 230
                                         and float(np.ptp(shots["Expired"][mask].astype(float), axis=1).mean()) > 3,
    }
    passed = all(checks.values())
    report["maps"][name] = {"passed": passed, "white_heat_pixels": count,
                           "background_p99": float(np.percentile(hot[~mask], 99)), "checks": checks}
    report["passed"] &= passed
    print(f"{name}: {'PASS' if passed else 'FAIL'}, {count} white heat pixels; {checks}")
intro_log = (native / "presentation-smoke.log").read_text(errors="replace")
report["intro_esc_skip_passed"] = all(text in intro_log for text in [
    "INTRO CHECK Space and Enter leave intro playing: PASS",
    "INTRO CHECK intro advertises Esc to skip: PASS",
    "INTRO CHECK Esc key skips intro immediately: PASS", "WILDLINE INTRO SMOKE: PASS"])
report["passed"] &= report["intro_esc_skip_passed"]
(native / "thermal-verification.json").write_text(json.dumps(report, indent=2) + "\n")
raise SystemExit(0 if report["passed"] else 1)
