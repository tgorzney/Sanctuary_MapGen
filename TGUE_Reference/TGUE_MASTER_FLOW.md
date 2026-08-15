@Title: TGUE - Master Execution Flow

@Tab: 6-Phase Execution Graph
    @Dictionary
        @Category: Flow Concepts
            @Term: Threading Boundaries
                > Definition: Phases 1, 4, 5, and 6 execute strictly on a single Master Thread to ensure data stability and networking consistency. Phases 2 and 3 are hyper-threaded across all available CPU cores.

    @Section[icon="⏱️"]: The Master Tick Sequence
        @Class: Phase 1: The Input Synchronization
            > Description: Seal the universe state so all downstream math is 100% deterministic.
            @Method: Local Input
                > Description: TG_UE::Input_BRIDGE reads local player input and formats to raw command data.
            @Method: Network Input
                > Description: TG_NET::NetworkPlayout_SYS pulls validated UDP packets from ring buffer.
            @Method: Prediction
                > Description: TG_NET::InputPrediction_PROC dynamically guesses missing input to prevent stalling.
            @Method: The Lock
                > Description: All inputs pushed to Command_SOA. Universe mathematically sealed for current frame.

        @Class: Phase 2: The Topological Compilation
            > Description: Prepare the spatial boards so units can query environment in O(1) time.
            @Method: Spatial Binning
                > Description: TG_SPAQ::SpatialQuery_PROC reads previous frame's positions and hashes into SpatialGrid_SOA.
            @Method: Wavefront Flow
                > Description: TG_WAVE triggers Poisson_PROC to recalculate CellularGrid_SOA vector fields.

        @Class: Phase 3: The Linear Crunch
            > Description: The core simulation. AVX-512 processors evaluate sealed inputs against compiled topology.
            @Method: State Assessment
                > Description: MatrixGOAP_PROC evaluates states against ActionTensor_SOA.
            @Method: Continuous Collision
                > Description: Raycast_PROC tests rays against Spline_SOA.
            @Method: Local Avoidance
                > Description: RVO_PROC solves reciprocal velocity constraints using SpatialGrid_SOA.
            @Method: Integration
                > Description: Calculated velocities are applied to positions (Physics_PROC). Outputs to Thread-Local Buffers.

        @Class: Phase 4: Defragmentation & Cleanup
            > Description: Prepare memory arrays for the next frame's contiguous execution.
            @Method: Buffer Merge
                > Description: ThreadManager_SYS pauses workers. Master thread merges Thread-Local Buffers into master DataStream_SOA.
            @Method: Array Defragmentation
                > Description: Dead units deleted by swapping the last array element into the dead index.
            @Method: Predictive Allocation
                > Description: ArenaAllocator_SYS dispatches async OS_VirtualAlloc if utilization hits 85%.

        @Class: Phase 5: The Determinism Hash
            > Description: Prove that the simulation has not desynced.
            @Method: Hashing
                > Description: TG_NET executes CRC32/MurmurHash over merged Position_SOA.
            @Method: Validation
                > Description: Hash is transmitted. If mismatched, error recovery engages.

        @Class: Phase 6: The Presentation Bridge
            > Description: Render the math to the human player without stalling the engine.
            @Method: Immutable State
                > Description: Simulation tick is formally complete. Data is immutable.
            @Method: Render Dispatch
                > Description: Render_BRIDGE reads Position_SOA, Rotation_SOA. Dispatches to HISM, Niagara, or SceneProxy.
