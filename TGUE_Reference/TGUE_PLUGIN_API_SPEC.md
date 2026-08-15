@Title: TGUE - Modular Plugin API Specification

@Tab: TGUE_PLUGIN_API
    @Dictionary
        @Category: Plugin Security Constraints
            @Term: The Quarantine Rule
                > Definition: Modders and 3rd-Party plugins are strictly banned from executing loops inside the mainframe's TG_LPROC sequence to prevent cache stalling and desyncs.
            @Term: Event Sourcing Input
                > Definition: Plugins mutate the game state exclusively by appending structured BytePayload commands to the TGUE_EventStream.
            @Term: Asynchronous Hooks
                > Definition: Plugins subscribe to data mutation callbacks rather than polling memory, maintaining 0% idle CPU overhead.

    @Section[icon="🔌"]: The Core API Boundaries
        @Class: TG_API_BRIDGE
            > Description: Standard C-ABI interface exposing pure Read-Only flat memory arrays to external DLLs.
            @Method: GetStreamPointer(StreamID)
                > Description: Returns a const pointer to the requested DataStream_SOA array.
        @Class: Mod_Input_Bridge
            > Description: The single authorized input tunnel for Mod logic to enter the mainframe.
            @Method: PushToLedger(ActionID, BytePayload)
                > Description: Appends a deterministic command to the TGUE_EventStream for execution on the next frame.
        @Class: DataStream_Hook_SYS
            > Description: The lock-free notification broadcaster.
            @Method: SubscribeToDelta(StreamID, CallbackPtr)
                > Description: Wakes the sleeping Mod Thread only when a specific stream mathematically changes.
