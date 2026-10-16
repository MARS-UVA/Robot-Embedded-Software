# Robot Embedded Software

The `urc main body` project exchanges HELLO messages between two ESP32-S3 boards
over classical CAN at 500 kbit/s. Motor control is not implemented yet.

## Code layout

- `components/can`: CAN transport, frame conversion, transmission alerts and
  automatic bus-off recovery. No knowledge of HELLO or board identities.
- `components/protocol`: constructs and validates messages. HELLO is a standard
  data frame containing exactly `48 45 4C 4C 4F` (no terminating zero).
- `urc main body/main/board_config.h`: pins, board IDs and timing settings.
- `urc main body/main/main.c`: starts communication, sends periodic HELLOs and
  tracks whether valid peer messages are arriving.

The CAN API has one owning task; that task calls `can_service()` regularly.
The application uses bounded receive waits and nonblocking send queuing so
recovery, periodic sends and peer timeouts continue without separate tasks or
core pinning. Future message types belong in the protocol component; hardware
control should live in its own component.

## Configure and flash both boards

Use an ESP-IDF terminal and run these commands from `urc main body`:

```text
idf.py menuconfig
idf.py build
idf.py -p PORT flash monitor
```

In **URC communication > Board role**, select **Board A** for the first board
and **Board B** for the second. Rebuild after changing the role before flashing
the second board. Board A is the default.

| Board | Sends with ID | Accepts HELLO from ID |
| --- | --- | --- |
| A | 0x101 | 0x102 |
| B | 0x102 | 0x101 |

CAN IDs label broadcast message streams, not destination addresses. Both boards
must use different roles and the same bitrate.

Each ESP needs an external CAN transceiver compatible with 3.3 V logic.
Connect GPIO 4 to transceiver TXD and GPIO 5 to RXD. Connect CANH to CANH and
CANL to CANL between transceivers, with a shared ground for non-isolated hardware
and one 120 ohm termination at each physical end. Account for termination already
installed on transceiver modules. Power and enable each transceiver according
to its datasheet; never connect ESP GPIO directly to CANH/CANL.

## Expected behavior and checks

Each board queues HELLO every two seconds and reports valid peer HELLOs.
`Queued HELLO` means only that the driver accepted the frame; the separate
CAN transmission alert confirms that it was transmitted and acknowledged on
the bus. CAN acknowledgement does not prove the peer application processed it.
The unchanged HELLO format has no per-message application acknowledgement or
sequence number; peer monitoring is based on incoming HELLOs.

- Check that both monitors show opposite node/peer IDs and received HELLOs.
- With both powered, interrupt one peer's communication. After six seconds
  without a valid HELLO, the other board reports the missing peer once.
- Restore the peer. The next valid HELLO reports `Peer online` again.
- Bus errors and receive queue overflow are reported. If the controller enters
  bus-off, the CAN layer requests recovery, discards queued transmissions as
  required by the driver, and restarts once recovery completes. Recovery needs
  suitable bus conditions; a wiring fault can prevent it or cause repeated errors.

Use these hardware checks after building; a successful build cannot establish
that the transceivers, wiring and physical bus operate correctly.

