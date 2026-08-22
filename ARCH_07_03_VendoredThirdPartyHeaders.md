[← ARCH index](ARCH.md) · [§7 ARCH_07_M3Resolutions](ARCH_07_M3Resolutions.md) · SanGen ARCH §7.3. **Only the ARCH Expert writes this file.**

### 7.3 Vendored third-party headers
Third-party vendored code (`FastNoiseLite.h`, `miniz`, `stb_*`) does **not** belong in a
layer folder and is **exempt from the naming law**. It lives in **`src/third_party/`**
with its upstream names/style unchanged; our code includes it as
`third_party/<Header>`. This keeps the layer folders pure (our code, our naming) and
vendored code clearly quarantined.

