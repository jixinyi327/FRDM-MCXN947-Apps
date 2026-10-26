#!/usr/bin/env python3
"""Plan a stable FRDM-MCXN947 ADC + eDMA sampling configuration."""

from __future__ import annotations

import argparse
import json
import math
import re
from decimal import Decimal, InvalidOperation


ADC_CLOCK_HZ = 48_000_000
TIMER_CLOCK_HZ = 12_000_000
ADC_CYCLES = {12: 19, 16: 24}
RESULT_SHIFT = {12: 3, 16: 0}
FULL_SCALE = {12: 4096, 16: 65536}
MAX_LPTMR_TICKS = 65_536


def parse_rate_k(value: str) -> int:
    normalized = value.strip().lower().replace("_", "").replace(" ", "")
    multiplier = Decimal(1)

    if normalized.endswith("ksps"):
        normalized = normalized[:-4]
    elif normalized.endswith("ks/s"):
        normalized = normalized[:-4]
    elif normalized.endswith("khz"):
        normalized = normalized[:-3]
    elif normalized.endswith("k"):
        normalized = normalized[:-1]
    elif normalized.endswith("msps"):
        normalized = normalized[:-4]
        multiplier = Decimal(1000)
    elif normalized.endswith("m"):
        normalized = normalized[:-1]
        multiplier = Decimal(1000)

    try:
        rate_k = Decimal(normalized) * multiplier
    except InvalidOperation as error:
        raise argparse.ArgumentTypeError(
            "sample rate must look like 2000k or 2M"
        ) from error

    if rate_k <= 0:
        raise argparse.ArgumentTypeError("sample rate must be greater than zero")

    rate = int(rate_k * 1000)
    if rate <= 0:
        raise argparse.ArgumentTypeError("sample rate is below 1 sample/s")
    return rate


def relative_error_percent(actual: float, target: int) -> float:
    return abs(actual - target) * 100.0 / target


def periodic_candidates(
    requested_hz: int,
    max_continuous_hz: float,
    minimum_period_ns: float,
) -> list[dict[str, object]]:
    options: list[dict[str, object]] = []
    ideal_ticks = TIMER_CLOCK_HZ / requested_hz
    candidates = range(
        max(1, math.floor(ideal_ticks) - 32),
        min(MAX_LPTMR_TICKS + 1, math.ceil(ideal_ticks) + 33),
    )

    for ticks in candidates:
        actual_hz = TIMER_CLOCK_HZ / ticks
        period_ns = 1_000_000_000.0 * ticks / TIMER_CLOCK_HZ
        if (actual_hz > max_continuous_hz) or (
            period_ns < minimum_period_ns
        ):
            continue
        options.append(
            {
                "mode": "lptmr-periodic",
                "actual_sample_rate_hz": int(round(actual_hz)),
                "error_percent": round(
                    relative_error_percent(actual_hz, requested_hz), 6
                ),
                "timer_ticks": ticks,
                "sample_period_ns": round(period_ns, 3),
            }
        )

    return sorted(
        options,
        key=lambda item: (
            float(item["error_percent"]),
            int(item["timer_ticks"]),
        ),
    )


def plan(args: argparse.Namespace) -> dict[str, object]:
    cycles = ADC_CYCLES[args.resolution]
    adc_clock_ns = 1_000_000_000.0 / args.adc_clock_hz
    conversion_ns = cycles * adc_clock_ns
    minimum_period_ns = conversion_ns + adc_clock_ns
    max_continuous_hz = args.adc_clock_hz / cycles
    requested_hz = args.sample_rate_hz
    tolerance = args.tolerance_percent

    periodic = periodic_candidates(
        requested_hz, max_continuous_hz, minimum_period_ns
    )
    continuous = {
        "mode": "continuous-self-chain",
        "actual_sample_rate_hz": int(round(max_continuous_hz)),
        "error_percent": round(
            relative_error_percent(max_continuous_hz, requested_hz), 6
        ),
        "timer_ticks": None,
        "sample_period_ns": round(conversion_ns, 3),
    }

    selected: dict[str, object] | None = None
    if periodic and float(periodic[0]["error_percent"]) <= tolerance:
        selected = periodic[0]
    elif float(continuous["error_percent"]) <= tolerance:
        selected = continuous

    nearest = sorted(
        periodic + [continuous],
        key=lambda item: float(item["error_percent"]),
    )[:5]

    feasible = selected is not None
    result: dict[str, object] = {
        "feasible": feasible,
        "requested_resolution_bits": args.resolution,
        "requested_sample_rate_hz": requested_hz,
        "requested_sample_rate_k_sps": round(requested_hz / 1000.0, 3),
        "adc_clock_hz": args.adc_clock_hz,
        "adc_clock_source": "FRO_HF on FRDM-MCXN947",
        "cycles_per_conversion": cycles,
        "conversion_time_ns": round(conversion_ns, 3),
        "minimum_safe_trigger_period_ns": round(minimum_period_ns, 3),
        "max_continuous_sample_rate_hz": int(round(max_continuous_hz)),
        "result_mask": "ADC_RESFIFO_D_MASK",
        "result_shift": RESULT_SHIFT[args.resolution],
        "logical_full_scale": FULL_SCALE[args.resolution],
        "buffer_samples": args.buffer_samples,
        "buffer_duration_us": round(
            args.buffer_samples * 1_000_000.0 / requested_hz, 6
        ),
        "nearest_supported": nearest,
    }

    if feasible and selected is not None:
        actual_hz = int(selected["actual_sample_rate_hz"])
        result.update(
            {
                "mode": selected["mode"],
                "actual_sample_rate_hz": actual_hz,
                "actual_sample_rate_k_sps": round(actual_hz / 1000.0, 3),
                "error_percent": selected["error_percent"],
                "timer_ticks": selected["timer_ticks"],
                "actual_buffer_duration_us": round(
                    args.buffer_samples * 1_000_000.0 / actual_hz, 6
                ),
            }
        )
        result["configuration"] = {
            "conversion_resolution_mode": (
                "kLPADC_ConversionResolutionStandard"
                if args.resolution == 12
                else "kLPADC_ConversionResolutionHigh"
            ),
            "sample_time_mode": "kLPADC_SampleTimeADCK3",
            "command_chaining": (
                "chain command 1 to itself"
                if selected["mode"] == "continuous-self-chain"
                else "hardware trigger command 1"
            ),
            "trigger": (
                "LPADC_DoSoftwareTrigger(ADC0, 1UL)"
                if selected["mode"] == "continuous-self-chain"
                else (
                    "LPTMR0 12 MHz with "
                    f"LPTMR_SetTimerPeriod(LPTMR0, {selected['timer_ticks']})"
                )
            ),
            "dma": (
                f"DMA0 channel 0, FIFO0 watermark, two {args.buffer_samples}-sample "
                "ping-pong buffers"
            ),
        }
    else:
        result.update(
            {
                "mode": None,
                "actual_sample_rate_hz": None,
                "actual_sample_rate_k_sps": None,
                "error_percent": None,
                "timer_ticks": None,
                "actual_buffer_duration_us": None,
                "configuration": None,
            }
        )

    warnings: list[str] = []
    if not feasible:
        warnings.append(
            "Requested rate cannot be generated accurately with the default "
            "48 MHz ADC clock and 12 MHz LPTMR. Select a nearest supported rate "
            "or redesign clocking."
        )
    if selected and selected["mode"] == "continuous-self-chain":
        warnings.append(
            "Continuous self-chain runs at the ADC throughput limit. If the "
            "runtime must consume every sample, verify DMA and DSP headroom."
        )
    if selected and selected["mode"] == "lptmr-periodic":
        warnings.append(
            "The reported sample rate is the exact 12 MHz / timer_ticks value; "
            "do not relabel it as the rounded requested value."
        )
    result["warnings"] = warnings
    return result


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description="Plan a stable FRDM-MCXN947 ADC + DMA sample rate"
    )
    parser.add_argument("--resolution", type=int, choices=(12, 16), required=True)
    parser.add_argument(
        "--sample-rate-k",
        type=parse_rate_k,
        required=True,
        help="Target rate in kS/s; 2000k and 2M both mean 2,000,000 S/s",
    )
    parser.add_argument(
        "--tolerance-percent",
        type=float,
        default=1.0,
        help="Maximum accepted actual-rate error (default: 1.0)",
    )
    parser.add_argument(
        "--adc-clock-hz",
        type=int,
        default=ADC_CLOCK_HZ,
        help="ADC ADCK frequency (default: 48,000,000)",
    )
    parser.add_argument(
        "--buffer-samples",
        type=int,
        default=3072,
        help="Samples per DMA ping-pong buffer (default: 3072)",
    )
    return parser


def main() -> None:
    parser = build_parser()
    args = parser.parse_args()

    if args.adc_clock_hz <= 0:
        parser.error("--adc-clock-hz must be greater than zero")
    if args.buffer_samples <= 0:
        parser.error("--buffer-samples must be greater than zero")
    if args.tolerance_percent < 0:
        parser.error("--tolerance-percent cannot be negative")

    args.sample_rate_hz = args.sample_rate_k
    print(json.dumps(plan(args), indent=2))


if __name__ == "__main__":
    main()
