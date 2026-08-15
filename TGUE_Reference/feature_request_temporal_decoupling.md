# Feature Request: Temporal Decoupling via Localized Sub-stepping

## Project Context & Ultimate Goal
1. **Invoking Project**: TG_CitySim
2. **Origin of Need**: EPA_SWMM (Hydraulics Engine - Routing Flow)
3. **The Ultimate Goal (The 'Why')**: To prevent the mathematical matrices of massive network topologies from exploding due to the Courant condition when high-velocity elements (like surcharging pipes) require sub-second tick rates, without forcing the entire city to run at that extreme tick rate.

## Required Hardware Capability
The `TG_UE` framework needs native temporal decoupling. The `_PROC` must be able to run stable, slow-moving topology chunks at a standard 1.0s tick while dynamically isolating volatile chunks into a localized 0.1s sub-step. The boundaries must be mathematically blended using ghost nodes, completely bypassing the global timestep bottleneck.

## Architectural Cost-Benefit Analysis
1. **Estimated Performance Boost**: Actively prevents a 10x frame rate computational explosion by ensuring localized 0.1s ticks are absolutely sequestered from the global 1.0s network processing loops (+1,000% throughput increase).
2. **Accuracy Discrepancy Amount**: Minor fractional discrepancies along mathematical boundary ghost nodes during decoupling blending.
3. **Estimate Confidence**: High Confidence - proven standard practice in modern explicit temporal integration schemes.
