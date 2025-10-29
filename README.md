# Building Dependencies

To rebuild dependencies (like DiligentEngine), configure CMake with the BUILD_DEPENDENCIES option:

	cmake -S . -B build -DBUILD_DEPENDENCIES=ON

This will build all dependencies. For normal development (Debug/Release), leave BUILD_DEPENDENCIES OFF (default) and only BangUI will be rebuilt.
# Dependency Build Instructions

To avoid rebuilding dependencies every time you build BangUI:

1. Build and install DiligentEngine separately (outside this repo):
	- Clone DiligentEngine and follow its build/install instructions.
	- Install it to a known location (e.g., C:/DiligentEngine/install).
2. Set CMAKE_PREFIX_PATH to the install location when configuring BangUI:
	- Example: `cmake -S . -B build -DCMAKE_PREFIX_PATH="C:/DiligentEngine/install"`
3. BangUI will now use the prebuilt DiligentEngine libraries and only rebuild its own code.

This prevents unnecessary recompilation of DiligentEngine and other dependencies.
# BangUI

This is the initial README for the BangUI project. Project setup is in progress.

## Next Steps
- Project requirements and tech stack will be clarified.
- Scaffold the project structure for a modern UI application.
- Update this README with build, run, and usage instructions once setup is complete.

## Build & Test Instructions

### Prerequisites
- C++17 or newer
- SDL3
- Diligent Engine
- Catch2 (for testing)
- CMake >= 3.16

### Build
1. Clone the repository.
2. Run `cmake -S . -B build`
3. Run `cmake --build build`

### Run Tests
1. From the build directory, run `BangUI_tests` executable.
2. All unit and integration tests will execute via Catch2.

### Project Structure
- `src/` - Core library and models
- `tests/` - Unit and integration tests
- `CMakeLists.txt` - Build configuration

### Notes
- All code must follow the Operational Paradigm and constitution rules.
- Tests are required for all features.
