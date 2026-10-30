import sys
from setuptools import setup, Extension

compile_args = (
    ["/O2", "/W4"]
    if sys.platform == "win32"
    else ["-O3", "-Wall", "-Wextra"]
)

module = Extension(
    "faststats",
    sources=["faststats.c"],
    extra_compile_args=compile_args,
)

setup(
    name="faststats",
    version="1.0.0",
    description="CPython C extension for statistical analysis",
    ext_modules=[module],
)

