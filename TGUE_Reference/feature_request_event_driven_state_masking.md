# Feature Request: Event-Driven State Masking

## Project Context & Ultimate Goal
1. **Invoking Project**: TG_CitySim
2. **Origin of Need**: GridLAB-D (Aggregate Demand & Smart Grid Loading)
3. **The Ultimate Goal (The 'Why')**: To prevent massive memory bandwidth waste by querying millions of entities (e.g., houses checking HVAC power draw) when the vast majority of them are inactive or in an 'off' state.

## Required Hardware Capability
The framework must implement a localized 64-bit boolean mask to track state flags. High-throughput calculation arrays must only pull data from indices whose active bitmask is $1$. This ensures the CPU skips processing inactive entities entirely at the hardware level, keeping memory bandwidth overhead extremely low.

## Architectural Cost-Benefit Analysis
1. **Estimated Performance Boost**: Massive reduction in memory bandwidth waste and cache eviction, scaling linearly with the percentage of inactive entities (e.g., skipping 90% inactive entities yields a 10x throughput boost) (+1,000% throughput increase).
2. **Accuracy Discrepancy Amount**: 0% loss.
3. **Estimate Confidence**: High Confidence - strictly equivalent boolean masking.
