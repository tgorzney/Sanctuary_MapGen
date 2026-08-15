@Title: TGUE - TGUE_EventStream Specification

@Tab: TGUE_EventStream
    @Dictionary
        @Category: Data-Oriented Concepts
            @Term: The Delta Array
                > Definition: An Event Sourcing Ledger using a pure DataStream_SOA array of tiny inputs [Time_Microsecond, Target_Identity, ActionID, BytePayload] instead of bloated snapshots.
            @Term: Time Scrubbing
                > Definition: The TG_JITRS_CMPLR replays mathematical sequences from time zero to the target microsecond to calculate historical states instantly.

    @Section[icon="📜"]: Event Sourcing
        @Struct: DataStream_SOA (Delta Array)
            > Description: The immutable ledger of actions. The BytePayload acts as a dynamic raw memory block interpreted by the ActionID for infinite flexibility.
