import gc
import math
import random
import time
import baseline
import faststats

def check_error(function, error_type):
    try:
        function()
    except error_type:
        return
    except Exception as error:
        raise AssertionError(
            f"Expected {error_type.__name__}, got {type(error).__name__}"
        ) from error
    raise AssertionError("Expected xception but none was raised")

def verify_equivalence():
    """Python vs C results, plus edge cases"""
    implementations = (baseline, faststats)

    # empty datasets
    for mod in implementations:
        for fn in (
            mod.calculate_stats,
            mod.find_min_max,
            mod.normalize
        ):
            check_error(lambda fn=fn: fn([]), ValueError)
        check_error(
            lambda mod=mod: mod.count_outliers([], 0, 10),
            ValueError
        )

    # invalid dataset values
    invalid_data = (
        [1, "bad", 3],
        [1, True, 3],
        [1, float("nan"), 3],
        [1, float("inf"), 3]
    )
    for mod in implementations:
        for data in invalid_data:
            for fn in (
                mod.calculate_stats,
                mod.find_min_max,
                mod.count_outliers,
                mod.normalize
            ):
                check_error(
                    lambda fn=fn, data=data: (
                        fn(data, 0, 10)
                        if fn == mod.count_outliers
                        else fn(data)
                    ),
                    TypeError if isinstance(data[1], str)
                    or isinstance(data[1], bool)
                    else ValueError
                )

    # invalid thresholds
    invalid_thresholds = (
        ("bad", 5, TypeError),
        (0, True, TypeError),
        (float("nan"), 5, ValueError),
        (0, float("inf"), ValueError)
    )
    for mod in implementations:
        check_error(
            lambda mod=mod: mod.count_outliers([1, 2], 10, 5),
            ValueError
        )
        for low, high, error_type in invalid_thresholds:
            check_error(
                lambda mod=mod, low=low, high=high:
                    mod.count_outliers([1, 2], low, high),
                error_type
            )

    # extremely large
    large_integer_data = [1, 10**10000, 3]
    for mod in implementations:
        for fn in (
            mod.calculate_stats,
            mod.find_min_max,
            mod.count_outliers,
            mod.normalize
        ):
            check_error(
                lambda fn=fn: (
                    fn(large_integer_data, 0, 10)
                    if fn == mod.count_outliers
                    else fn(large_integer_data)
                ),
                OverflowError
            )

    # non-sequence input
    for mod in implementations:
        check_error(
            lambda mod=mod: mod.calculate_stats(12345),
            TypeError
        )

    # boundary test
    data = [1, 2, 3, 4, 5]
    for mod in implementations:
        assert mod.count_outliers(data, 2, 4) == 2

    # constant, single-element & tuple datasets
    special_data = (
        [42.0],
        [5.0, 5.0, 5.0, 5.0],
        (1, 2, 3, 4, 5)
    )
    for data in special_data:
        compare_results(data)

    # random datasets
    random.seed(42)
    for size in (10, 50_000):
        data = [random.uniform(-1000, 1000) for _ in range(size)]
        compare_results(data)
    print("PASS: All equivalence and edge-case tests passed\n")

def compare_results(data):
    py_mean, py_var = baseline.calculate_stats(data)
    c_mean, c_var = faststats.calculate_stats(data)

    assert math.isclose(py_mean, c_mean, rel_tol=1e-12, abs_tol=1e-12)
    assert math.isclose(py_var, c_var, rel_tol=1e-12, abs_tol=1e-12)
    assert baseline.find_min_max(data) == faststats.find_min_max(data)
    assert (
        baseline.count_outliers(data, -750, 750)
        == faststats.count_outliers(data, -750, 750)
    )

    py_norm = baseline.normalize(data)
    c_norm = faststats.normalize(data)

    assert len(py_norm) == len(c_norm)
    assert all(
        math.isclose(a, b, rel_tol=1e-12, abs_tol=1e-12)
        for a, b in zip(py_norm, c_norm)
    )

def benchmark(sizes=(1_000, 100_000, 1_000_000), runs=10):
    """benchmark with repeated measurements"""

    random.seed(42)
    print(
        f"{'Dataset':<12} | {'Operation':<18} | "
        f"{'Python (ms)':<13} | {'C (ms)':<13} | {'Speedup':<10}"
    )
    print("-" * 79)

    operations = (
        ("calculate_stats",
         lambda mod, data: mod.calculate_stats(data)),
        ("find_min_max",
         lambda mod, data: mod.find_min_max(data)),
        ("count_outliers",
         lambda mod, data: mod.count_outliers(data, -800, 800)),
        ("normalize",
         lambda mod, data: mod.normalize(data))
    )

    for size in sizes:
        data = [random.uniform(-1000, 1000) for _ in range(size)]

        for name, operation in operations:
            operation(baseline, data)
            operation(faststats, data)

            py_times = []
            c_times = []
            for run in range(runs):
                order = (
                    (baseline, faststats)
                    if run % 2 == 0
                    else (faststats, baseline)
                )
                for mod in order:
                    gc.collect()
                    start = time.perf_counter()
                    operation(mod, data)
                    elapsed = time.perf_counter() - start
                    if mod is baseline:
                        py_times.append(elapsed)
                    else:
                        c_times.append(elapsed)

            py_avg = sum(py_times) / runs
            c_avg = sum(c_times) / runs
            speedup = py_avg / c_avg
            print(
                f"{size:<12,} | {name:<18} | "
                f"{py_avg * 1000:<13.4f} | "
                f"{c_avg * 1000:<13.4f} | "
                f"{speedup:<9.2f}x"
            )

        print("-" * 79)

if __name__ == "__main__":
    verify_equivalence()
    benchmark()

