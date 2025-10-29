# Building and Running BangUI

## Build Instructions

### Prerequisites
- CMake 3.16 or higher
- Visual Studio 2022 (or compatible C++ compiler)
- vcpkg with SDL3, SDL3_image[png], and SDL3_ttf installed

**IMPORTANT**: SDL3_image must be installed with PNG feature:
```bash
./vcpkg install sdl3-image[png]:x64-windows --recurse
```

### Building the Project

```bash
# Configure
cmake -B build -S .

# Build Debug configuration
cmake --build build --config Debug

# Build Release configuration
cmake --build build --config Release
```

## Running the Demo

The main demo executable is `BangUI_sdl_demo.exe` located in:
- Debug: `build/src/Debug/BangUI_sdl_demo.exe`
- Release: `build/src/Release/BangUI_sdl_demo.exe`

### DLL Dependencies

The project requires the following DLLs to run:
- `SDL3.dll` (~2.2 MB)
- `SDL3_image.dll` (~106 KB)
- `SDL3_ttf.dll` (~95 KB)

**These DLLs are automatically copied to the output directory during build.**

The CMakeLists.txt includes a post-build step that copies these DLLs from the vcpkg installation to the executable directory.

## Troubleshooting

### Error: 0xc0000135 (Missing DLL)

**Symptom:** Program fails to start with exit code -1073741515 (0xc0000135)

**Cause:** Required SDL3 DLLs are not in the executable directory or system PATH

**Solution:**

1. **Automatic (Recommended):** The build system should copy DLLs automatically. Rebuild the project:
   ```bash
   cmake --build build --config Debug --target BangUI_sdl_demo
   ```

2. **Manual Copy:** If automatic copying fails, manually copy DLLs:
   ```bash
   # From vcpkg installation to build output
   cp vcpkg/installed/x64-windows/bin/SDL3.dll build/src/Debug/
   cp vcpkg/installed/x64-windows/bin/SDL3_image.dll build/src/Debug/
   cp vcpkg/installed/x64-windows/bin/SDL3_ttf.dll build/src/Debug/
   ```

3. **Verify DLLs are present:**
   ```bash
   ls build/src/Debug/*.dll
   ```

   You should see:
   - SDL3.dll
   - SDL3_image.dll
   - SDL3_ttf.dll

### Build Errors

**CMake can't find SDL3:**
- Ensure vcpkg is properly installed
- Check that vcpkg/installed/x64-windows/ contains SDL3

**Linker errors:**
- Make sure you're building for the correct architecture (x64)
- Verify vcpkg triplet is x64-windows

### Runtime Issues

**Window doesn't appear:**
- Check console output for error messages
- Verify graphics drivers are up to date
- Try running as administrator

**Textures don't load:**
- Check file paths in the demo code
- Ensure texture files exist if using custom textures
- Check console for "Unable to load image" messages

**SDL_image Error: "Unsupported image format":**
- **Cause**: SDL3_image was installed without PNG feature support
- **Solution**: Reinstall SDL3_image with PNG support:
  ```bash
  cd vcpkg
  ./vcpkg install sdl3-image[png]:x64-windows --recurse
  ```
- Then rebuild your project:
  ```bash
  cmake --build build --config Debug
  ```

## Running from IDE (Visual Studio)

If running from Visual Studio:

1. Set `BangUI_sdl_demo` as the startup project
2. Build the project (F7)
3. Run with debugging (F5) or without (Ctrl+F5)

The DLLs will be in the same directory as the executable, so it should run without issues.

## Running from Command Line

```bash
# Navigate to build output directory
cd build/src/Debug

# Run the demo
./BangUI_sdl_demo.exe
```

## Expected Output

When the demo runs successfully, you should see:

```
=== BangUI Interactive Demo ===
Testing docking and size modes
SDL initialized successfully

=== Creating UI Elements with Docking ===
Panel 1: Top-Left docked, 200x150px, rounded
Panel 2: Center-Top docked, 250x200px, sharp corners
Window 1: Bottom-Left, resizable, closeable, red title bar
Window 2: Bottom-Right, resizable, purple title bar

=== Controls ===
- Elements auto-position based on dock settings
- Drag Windows by title bar
- Drag Panels anywhere on their surface
- Hover bottom-right corner of resizable windows to see resize handle
- Drag resize handle to resize windows
- Click X to close windows
- ESC or close window to quit

=== Starting Render Loop ===
```

A window should appear with:
- Two colored panels (blue and green)
- Two resizable windows with colored title bars
- Interactive dragging and resizing

## Build Configuration

### Debug vs Release

**Debug Configuration:**
- Includes debug symbols
- No optimization
- Larger executable size
- Better for development and debugging

**Release Configuration:**
- Optimized for performance
- Smaller executable size
- Better for distribution

Build Release:
```bash
cmake --build build --config Release
```

## Distribution

When distributing your BangUI application, include:
1. Your executable
2. SDL3.dll
3. SDL3_image.dll
4. SDL3_ttf.dll
5. Any texture/asset files you're using

Place all files in the same directory.

## Platform Notes

### Windows
- Tested on Windows 11 with Visual Studio 2022
- Requires Visual C++ Redistributable

### Future Platforms
- Linux: Will require SDL3 system packages
- macOS: Will require SDL3 frameworks

## Performance Tips

1. **Texture Loading:** Textures are cached, so loading the same texture multiple times is efficient
2. **Window Count:** Performance scales well with multiple windows
3. **Resize Operations:** Live resizing is smooth even with complex layouts
4. **Frame Rate:** Demo runs at ~60 FPS (16ms delay)

## Clean Build

If you encounter build issues, try a clean build:

```bash
# Remove build directory
rm -rf build

# Reconfigure and build
cmake -B build -S .
cmake --build build --config Debug
```

## Additional Resources

- [SDL3 Documentation](https://wiki.libsdl.org/SDL3/)
- [CMake Documentation](https://cmake.org/documentation/)
- [vcpkg Documentation](https://vcpkg.io/)
