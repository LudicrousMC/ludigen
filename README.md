# LudiGen

GPU-accelerated Minecraft Terrain Generation at Ludicrous Speeds

LudiGen, or Ludicrous Generator, aims to push the limits of GPU computing by offloading and parallelizing the entire chunk generation pipeline onto the GPU using compilation and reverse-engineering techniques. See the [Current Progress](#current-progress) section for the current status of the project.

## ⚠️ In Development Notice

This mod is in very early development and iteration. Due to the possibility of bugs or vulnerabilities, it is highly recommended you do not use this to generate terrain for important or frequently used world saves during this stage of development. Mods like [C2ME](https://github.com/RelativityMC/C2ME-fabric) and it's OpenCL addon are much more stable, faster (for now), and suitable for terrain generation in general.

However, if you do use this mod:
1. **Create world backups before using, and frequently**
2. Terrain blending is not yet implemented, so loading old world saves and generating new chunks will cause discrepancies on chunk borders
3. The terrain generated is very similar to Vanilla, but the parity is not perfect so even loading a world with already generated chunks from the same version may still have minor discrepancies between chunk borders
4. You assume any risks that may be posed to the integrity of your world save

*This project is based off of a previous prototype called Ludicrous. For information and demonstrations of the initial prototype visit the [Ludicrous](https://github.com/LudicrousMC/ludicrous) project.

## Building and Running

### Building Dependencies

- Java >= 25
- cmake >= 3.15
- make or ninja
- clang
- Vulkan SDK or Shaderc (for glslc)
- xxd (comes with Git on Windows)
- MinGW-w64 gendef (for compiling Windows Vulkan library)
- MinGW-w64 dlltool (for compiling Windows Vulkan library)

### Creating a Jar file

To build a jar artifact, run in the project directory:

```
./gradlew build
```

The jar file will be saved in the build/libs directory.

### Running for Development purposes

To start a local server, run:

```
./gradlew runServer
```

To launch the development client, run:

```
./gradlew runClient
```

*Note: Building a jar compiles the LudiGen binary and downloads the Vulkan and JDK libraries for all platforms (Windows and Linux), however runServer and runClient only compiles the LudiGen binary for the host platform and does not automatically download the libraries, so the Vulkan SDK and JDK must be installed on your system

## Compatibility

> LudiGen was mainly developed on Linux with a modern Nvidia GPU, so similar setups will have a higher likelihood of compatibility. Below is a table of what has been tested so far.

**Legend:**

✅ = Guaranteed/tested Compatibility

✅⚠️= Not tested, but strongly confident in compatibility

⚠️= Possibly Compatible but not tested

❌ = Not Compatible

|                                   | Windows 11/10 | Linux (Arch-based and possibly Mainstream distros) | macOS |
|-----------------------------------|:-------------:|:--------------------------------------------------:|:-----:|
| Nvidia RTX 3060TI                 |     ✅⚠️      |                         ✅                         |  ❌   |
| Nvidia RTX 20 Series or higher    |     ✅⚠️      |                        ✅⚠️                        |  ❌   |
| Nvidia GeForce 10 Series or lower |      ⚠️       |                         ⚠️                         |  ❌   |
| AMD Radeon 7700S                  |      ✅       |                         ✅                         |  ❌   |
| AMD Radeon 680M                   |      ✅       |                         ✅                         |  ❌   |
| Modern AMD iGPUs                  |     ✅⚠️      |                        ✅⚠️                        |  ❌   |
| Older AMD iGPUs                   |      ⚠️       |                         ⚠️                         |  ❌   |
| Modern Intel iGPUs                |      ⚠️       |                         ⚠️                         |  ❌   |
| Older Intel iGPUs                 |      ⚠️       |                         ⚠️                         |  ❌   |



## Current Progress

### Roadmap

- [X] Replicate Vanilla terrain generation (Final Density phase) to a realistic degree
- [X] Final Density phase implemented as interpreter on GPU
- [ ] Update to next MC version
- [ ] Implement compiler for Final Density phase to GPU compute code
  - Compiled code is much faster than interpreted, especially on the GPU, so this is pretty much required
- [ ] Implement the 4 or 5 other phases of the chunk generation pipeline as GPU compute code

### Current Performance Results

> [ScalableLux](https://github.com/RelativityMC/ScalableLux) was used in both the Vanilla and LudiGen tests to improve the margin of the performance gains since the Vanilla lighting engine isn't very fast and becomes a bottleneck

**Device 1**

- OS: Linux
- CPU: Ryzen 5 5600X (6-Core)
- Memory: 24GB
- GPU: Nvidia RTX 3060TI (8GB)
- Disk: 1TB custom RAID10 SATA SSDs (network-attached)
  - The connection link is ~28gbps but there may be some weirdness with it

**Device 2**

- OS: Linux or Windows 11
- CPU: Ryzen 7 7735HS (8-Core)
- Memory: 64GB
- iGPU: AMD Radeon 680M
- dGPU: AMD Radeon RX 7700S (8GB)
- Disk: 2TB NVMe PCIe4 SSD

#### Rough Pipeline Performance Outline

> Measured on Device 1

- Per-Chunk Average | Vanilla: \~57ms, LudiGen: \~48ms (\~16% faster)
  - Structures | \~0.5ms (Unchanged)
  - Biomes | \~3.9ms (Unchanged)
  - NoiseChunk Generation | Vanilla: \~12ms, LudiGen: \~7ms (\~42% faster)
    - Final Density Computation | Vanilla: \~7.5ms, LudiGen: \~1.5ms amortized (80% faster)
    - Aquifer/Veinifier | Unmeasured (likely little to no change)
  - Surface | \~3.9ms (Unchanged)
  - Carvers | \~1ms (Unchanged)
  - Features | \~6.5ms (Unchanged)
  - Lighting | \~16ms (Unchanged)

#### Platform-specific Performance Results

> Tests generated 4,200 chunks and measured CPU time from first chunk gen

**Vanilla w/ ScalableLux**
- Device 1: 240 seconds
- Device 2 (Linux): 118 seconds
- Device 2 (Windows): 125 seconds

**LudiGen w/ ScalableLux**
- Device 1: 202 seconds (16% faster)
- Device 2 (Linux)
  - dGPU: 103 seconds (13% faster)
  - iGPU: 105 seconds (11% faster)
- Device 2 (Windows)
  - dGPU: 111 seconds (11% faster)
  - iGPU: 460 seconds (73% slower)
    - Not sure why integrated graphics was so much slower on Windows. It might just be that the Windows GPU driver doesn't like the interpreter architecture, which should hopefully be fixed soon.

## Contributors

- Endrf (Creator and Lead Developer)
- ryanred10 (Contributor)

## License

All code provided in this repository is licensed under the open-source MIT license [here](https://github.com/LudicrousMC/ludigen/blob/main/LICENSE)


ばかげてる
