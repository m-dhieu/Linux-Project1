import math

def _validate_number(x):
    if isinstance(x, bool) or not isinstance(x, (int, float)):
        raise TypeError(f"Expected value, got {type(x).__name__}")
    value = float(x)
    if not math.isfinite(value):
        raise ValueError("Values must be finite numbers")
    return value

def calculate_stats(data):
    """mean & population var"""
    if len(data) == 0:
        raise ValueError("Dataset cannot be empty")
    values = [_validate_number(x) for x in data]
    mean = sum(values) / len(values)
    variance = sum((x - mean) ** 2 for x in values) / len(values)
    return mean, variance

def find_min_max(data):
    """min & max"""
    if len(data) == 0:
        raise ValueError("Dataset cannot be empty")
    values = [_validate_number(x) for x in data]
    return min(values), max(values)

def count_outliers(data, low, high):
    """values outside thresholds"""
    low = _validate_number(low)
    high = _validate_number(high)
    if low > high:
        raise ValueError("Lower threshold cannot exceed upper threshold")
    if len(data) == 0:
        raise ValueError("Dataset cannot be empty")
    values = [_validate_number(x) for x in data]
    return sum(1 for x in values if x < low or x > high)

def normalize(data):
    if len(data) == 0:
        raise ValueError("Dataset cannot be empty")
    values = [_validate_number(x) for x in data]
    min_val = min(values)
    max_val = max(values)
    if max_val == min_val:
        return [0.0] * len(values)
    return [(x - min_val) / (max_val - min_val) for x in values]

