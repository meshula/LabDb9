"""
Setup configuration for LabDb Python bindings
"""

from setuptools import setup, find_packages
from pybind11.setup_helpers import Pybind11Extension, build_ext
from pybind11 import get_cmake_dir
from pathlib import Path

# Read version from __init__.py
version_file = Path(__file__).parent / "python" / "labdb" / "__init__.py"
version = None
for line in version_file.read_text().splitlines():
    if line.startswith("__version__"):
        version = line.split('"')[1]
        break

if not version:
    raise RuntimeError("Version not found")

# Read README for long description
readme_file = Path(__file__).parent / "README.md"
long_description = readme_file.read_text() if readme_file.exists() else ""

setup(
    name="labdb",
    version=version,
    author="Lab Team",
    author_email="lab@example.com",
    description="Triadic Consciousness Database - A nonostore with ontological awareness",
    long_description=long_description,
    long_description_content_type="text/markdown",
    url="https://github.com/lab/labdb",
    
    packages=find_packages(where="python"),
    package_dir={"": "python"},
    
    # Note: The C++ extension is built via CMake, not setuptools
    # This setup.py is primarily for metadata and pure Python components
    
    python_requires=">=3.7",
    
    install_requires=[
        # Core dependencies would go here
        # pybind11 is a build dependency, not runtime
    ],
    
    extras_require={
        "dev": [
            "pytest",
            "black",
            "isort",
            "mypy",
        ],
        "examples": [
            "jupyter",
            "matplotlib", 
            "networkx",
            "pandas",
        ]
    },
    
    classifiers=[
        "Development Status :: 3 - Alpha",
        "Intended Audience :: Developers",
        "Intended Audience :: Science/Research",
        "License :: OSI Approved :: MIT License",
        "Programming Language :: Python :: 3",
        "Programming Language :: Python :: 3.7",
        "Programming Language :: Python :: 3.8", 
        "Programming Language :: Python :: 3.9",
        "Programming Language :: Python :: 3.10",
        "Programming Language :: Python :: 3.11",
        "Programming Language :: C++",
        "Topic :: Database",
        "Topic :: Scientific/Engineering :: Information Analysis",
    ],
    
    keywords="database triadic consciousness rdf knowledge-graph nonostore",
    
    project_urls={
        "Documentation": "https://github.com/lab/labdb/docs",
        "Source": "https://github.com/lab/labdb",
        "Tracker": "https://github.com/lab/labdb/issues",
    },
)
