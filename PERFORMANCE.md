# Performance Optimization Guide

## Build Optimizations

### Parallel Compilation

- **Automatic core detection**: `deploy.sh` detects CPU cores and uses `N+1` parallel jobs
- **Link-Time Optimization (LTO)**: Enabled in CMakeLists.txt for smaller binaries
- **Incremental builds**: Only rebuilds changed files (huge speedup on updates)
- **Compiler flags**:
  - `-O2`: Moderate optimization (good balance for embedded systems)
  - `-march=native`: CPU-specific optimizations
  - `-fno-exceptions -fno-rtti`: Reduces binary size by ~10%
  - `-ffunction-sections -fdata-sections`: Enables dead code elimination

### Build Time Expectations

- **First build**: ~2-3 minutes (Pi 400)
- **Incremental rebuild**: ~5-10 seconds (only changed files)
- **Clean build with cache**: ~1-2 minutes

## Runtime Optimizations

### GUI Update Rate

- **Update interval**: 50ms (20 Hz)
- **Reason**: Pi 400 can render ~20 FPS comfortably over VNC
- **Memory efficiency**: ~1500 data points per signal with circular buffer

### Rendering Optimizations

- **Adaptive sampling**: QCustomPlot intelligently reduces plotted points
- **Antialiasing disabled**: Trades quality for performance on weak GPUs
- **Batch replot**: Uses `rpQueuedReplot` to batch multiple redraws
- **Visibility check**: Skips rendering if no plots are visible
- **Cached visibility**: Visibility states cached to avoid repeated Qt property calls

### Memory Efficiency

- **Vector pre-allocation**: Data vectors reserved at startup to avoid reallocations
- **Circular buffer**: Removes oldest samples when exceeding `maxPoints_`
- **Const references**: Used throughout to avoid unnecessary copies
- **Static objects**: Pens and brushes created once in `setupPlots()`

## Deployment Optimization

### Fast Updates

```bash
./deploy.sh  # Only rebuilds changed files
```

### VNC Optimization Tips

- Disable animated backgrounds on Pi
- Use solid colors instead of gradients
- Limit window resolution to 1024x768 or less
- Run VNC server at same display size as monitor

## Code Quality

### Memory Safety

- Reserve vectors early to prevent reallocations
- Use const references for parameters
- Timer stopped in destructor to prevent leaks

### Performance Considerations

- No dynamic allocations in hot path (`onTick()`)
- Visibility checks prevent unnecessary rendering
- Batch operations reduce function call overhead

## Future Optimizations

1. **Multi-threaded data acquisition**: Separate thread for sensor data
2. **Double buffering**: Prevent tearing during redraws
3. **CPU affinity**: Pin application to specific cores
4. **Memory pooling**: Pre-allocate data buffers
5. **Native rendering**: Use EGLFS instead of VNC for direct rendering

## Monitoring Performance

Check systemd logs:

```bash
journalctl -u curecraft.service -f
```

Monitor CPU/Memory:

```bash
top -p $(pgrep -f curecraft)
```

## Technical Details

### CMake Optimizations

- `CMAKE_INTERPROCEDURAL_OPTIMIZATION`: Link-time optimization
- `CMAKE_CXX_VISIBILITY_PRESET=hidden`: Only export public symbols
- `CMAKE_VISIBILITY_INLINES_HIDDEN`: Inline functions hidden from external linkage

### Raspberry Pi 400 Specifications

- **CPU**: BCM2711 (4 cores @ 1.8 GHz)
- **RAM**: 4 GB
- **GPU**: VideoCore VI
- **Optimization**: Perfect for O2 level optimizations and LTO
