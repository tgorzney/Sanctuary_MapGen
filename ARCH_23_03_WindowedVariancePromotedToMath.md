[← ARCH index](ARCH.md) · [§23 ARCH_23_ProceduralStratumMaskFilterChain](ARCH_23_ProceduralStratumMaskFilterChain.md) · SanGen ARCH §23.3. **Only the ARCH Expert writes this file.**

### 23.3 `SampleHeightVariance` promoted to `MATH`, mirroring `RadialClearance_MATH.h`

**Ruled: promote, following the exact precedent already sitting next to it in the same file.**
`PlacementStage::SampleHeightVariance` (`Placement_Metrics_PROC.cpp:27-52`) is a private member
of `PlacementStage`: `float SampleHeightVariance(int cellX, int cellY) const`, closing over
`this->mapFields.heightfield` and `this->constants.varianceSampleRadius`. Its sibling in the same
file, `SampleClearanceRadius`, already solved this exact problem for radial-clearance scoring by
calling out to `Math::ScoreRadialClearance(mapFields.heightfield.Data(), vertexSize, vertexSize,
cellX, cellY, ...)` — a free `MATH` function taking a **raw pointer + dimensions**, never the
`Data::FloatField` wrapper type, which is precisely how `RadialClearance_MATH.h` avoids a
`MATH → DATA` dependency (its own header comment: "operates on a raw height array, not the DATA
FloatMask — fixes the old MATH->DATA include"). The windowed-variance math promotes the same way.

**Binding shape for the coder ticket:**
- New file `src/math/WindowedVariance_MATH.h` (paired conceptually with `RadialClearance_MATH.h`,
  not merged into it — different quantity, own file, same convention):
  ```cpp
  inline float WindowedHeightVariance(const float* heightField, int width, int height,
                                      int centerX, int centerY, int windowRadius) {
      // body moved verbatim from Placement_Metrics_PROC.cpp:28-52's loop, generalized to take
      // width/height/windowRadius as parameters instead of reading them off `this`.
  }
  ```
  Named for the **quantity** (windowed height variance), not the role ("Sample"/"Score") —
  Constitution §2's "a name states the quantity, not the role." Zero `Params::`/`Data::` types in
  the signature satisfies ARCH §3.5's MATH-placement test.
- `PlacementStage::SampleHeightVariance` becomes a thin wrapper, unchanged in behavior and call
  sites, exactly mirroring `SampleClearanceRadius`'s existing shape:
  ```cpp
  float PlacementStage::SampleHeightVariance(int cellX, int cellY) const {
      const int vertexSize = mapFields.VertexSize();
      int windowRadius = static_cast<int>(constants.varianceSampleRadius);
      if (windowRadius < 1) windowRadius = 1;
      return Math::WindowedHeightVariance(mapFields.heightfield.Data(), vertexSize, vertexSize,
                                          cellX, cellY, windowRadius);
  }
  ```
- The new Roughness filter's evaluator (Mask stage, `§23.5`) calls
  `Math::WindowedHeightVariance` directly with its own filter instance's `windowRadius` setting —
  no duplicated window-sum logic, which is exactly what the design doc's §3 asked for.

**Not ruled here, left to the Compute Optimization Expert (per the design doc's own §3/§4 flag):**
whether multiple strata's Roughness filters requesting the same `windowRadius` justify a shared
precomputed variance field computed once per generation rather than recomputed per stratum per
filter. That is a caching/dispatch-shape optimization layered on top of this MATH primitive, not
a reason to withhold the promotion — the primitive is correct and shared either way; only the
call frequency is in question.
