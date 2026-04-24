# TCP Congestion Control Algorithms Implementation
## CSE 320 - Computer Networks Programming Assignment

### Project Overview
This project implements and compares three TCP congestion control algorithms:
- **TCP Tahoe**: Fast Retransmit, no Fast Recovery
- **TCP Reno**: Fast Retransmit + Fast Recovery
- **TCP NewReno**: Improved Fast Recovery with partial ACK handling

### Team Members
- Student 1 ID: [20230808038]
- Student 2 ID: [20230808010]

### Algorithm Assignment
Based on: (student1_id + student2_id) % 3
- 0 → TCP Tahoe ✓
- 1 → TCP Reno 
- 2 → TCP NewReno 

---

## Project Structure
```
.
├── node.c                      # Main implementation
├── node.h                      # Header file with structures
├── Makefile                    # Build automation
├── A.conf                      # Node A configuration
├── B.conf                      # Node B configuration
├── events/                     # Test scenarios
│   ├── tahoe_timeout.txt       # Timeout scenario
│   ├── tahoe_dupack.txt        # Triple duplicate ACK scenario
│   └── comprehensive_test.txt  # Combined scenarios
└── README.md                   # This file
```

---

## Building the Project

### Compile
```bash
make
```

### Clean
```bash
make clean
```

---

## Running Simulations

### Basic Usage
```bash
./node <config_file> <algorithm> <event_file>
```

### Examples

#### Test TCP Tahoe with Timeout
```bash
make test-tahoe-timeout
# or
./node A.conf tahoe events/tahoe_timeout.txt
```

#### Test TCP Tahoe with Triple Duplicate ACK
```bash
make test-tahoe-dupack
# or
./node A.conf tahoe events/tahoe_dupack.txt
```

#### Compare All Algorithms
```bash
make test-comprehensive
```

#### Individual Algorithm Tests
```bash
# TCP Tahoe
./node A.conf tahoe events/comprehensive_test.txt

# TCP Reno
./node A.conf reno events/comprehensive_test.txt

# TCP NewReno
./node A.conf newreno events/comprehensive_test.txt
```

---

## Event File Format

Event files simulate network conditions. Each line contains an event:

### Supported Events
- `ACK <number>`: New acknowledgment received
- `DUPACK <number>`: Duplicate acknowledgment
- `TIMEOUT`: Retransmission timeout

### Example Event File
```
ACK 1
ACK 2
ACK 3
DUPACK 3
DUPACK 3
DUPACK 3
TIMEOUT
ACK 4
```

---

## Algorithm Implementations

### TCP Tahoe
**Characteristics:**
- ✓ Slow Start: exponential growth (cwnd += 1 per ACK)
- ✓ Congestion Avoidance: linear growth (cwnd += 1/cwnd per ACK)
- ✓ Fast Retransmit: on 3 duplicate ACKs
- ✗ Fast Recovery: NOT implemented (resets cwnd to 1)

**Behavior on Loss:**
- **Timeout**: ssthresh = cwnd/2, cwnd = 1, Slow Start
- **3 Duplicate ACKs**: ssthresh = cwnd/2, cwnd = 1, Slow Start

### TCP Reno
**Characteristics:**
- ✓ Slow Start
- ✓ Congestion Avoidance
- ✓ Fast Retransmit
- ✓ Fast Recovery: avoids full reset on duplicate ACKs

**Behavior on Loss:**
- **Timeout**: ssthresh = cwnd/2, cwnd = 1, Slow Start
- **3 Duplicate ACKs**: ssthresh = cwnd/2, cwnd = ssthresh + 3, Fast Recovery

### TCP NewReno
**Characteristics:**
- ✓ All Reno features
- ✓ Improved partial ACK handling during Fast Recovery
- ✓ Better performance with multiple packet losses

**Behavior on Loss:**
- Same as Reno, but handles partial ACKs better during Fast Recovery

---

## Configuration Files

### Format
```
# First line: NodeID Port
A 5001

# Subsequent lines: NeighborID IP Port Cost
B 127.0.0.1 5002 4
C 127.0.0.1 5003 7
```

### Example: A.conf
```
A 5001
B 127.0.0.1 5002 4
C 127.0.0.1 5003 7
D 127.0.0.1 5004 13
F 127.0.0.1 5006 5
```

---

## Understanding the Output

### Output Format
```
Round | Event      | ACK    | cwnd       | ssthresh   | State                  | Explanation
------+------------+--------+------------+------------+------------------------+-------------
    1 | ACK        | ACK=1  | cwnd= 2.00 | ssthresh=16.00 | Slow Start            | new ACK; ...
    2 | ACK        | ACK=2  | cwnd= 3.00 | ssthresh=16.00 | Slow Start            | new ACK; ...
```

### Key Metrics
- **Round**: Simulation step number
- **Event**: ACK, DUPACK, or TIMEOUT
- **cwnd**: Congestion window (in MSS units)
- **ssthresh**: Slow start threshold
- **State**: Current TCP state

---

## Testing Scenarios

### Scenario 1: Timeout Loss
**File**: `events/tahoe_timeout.txt`
**Tests**: Timeout-based packet loss recovery

### Scenario 2: Triple Duplicate ACK
**File**: `events/tahoe_dupack.txt`
**Tests**: Fast Retransmit mechanism

### Scenario 3: Comprehensive Test
**File**: `events/comprehensive_test.txt`
**Tests**: Multiple loss types and recovery patterns

---

## Key Differences Between Algorithms

| Feature | Tahoe | Reno | NewReno |
|---------|-------|------|---------|
| Slow Start | ✓ | ✓ | ✓ |
| Congestion Avoidance | ✓ | ✓ | ✓ |
| Fast Retransmit | ✓ | ✓ | ✓ |
| Fast Recovery | ✗ | ✓ | ✓ |
| Partial ACK Handling | ✗ | ✗ | ✓ |
| Recovery Speed | Slow | Fast | Fastest |

### Performance Under Different Loss Patterns

**Single Packet Loss:**
- Tahoe: Slowest (full reset)
- Reno: Fast recovery
- NewReno: Fast recovery

**Multiple Packet Loss:**
- Tahoe: Slowest
- Reno: May timeout
- NewReno: Best performance

---

## Implementation Details

### Core Data Structures

```c
typedef struct {
    Algorithm algorithm;    // TAHOE, RENO, or NEWRENO
    CCState state;         // SLOW_START, CONGESTION_AVOIDANCE, FAST_RECOVERY
    double cwnd;           // Congestion window (MSS units)
    double ssthresh;       // Slow start threshold
    int dup_ack_count;     // Duplicate ACK counter
    int round;             // Current round
} TCPState;
```

### Key Functions

- `init_tcp()`: Initialize TCP state
- `on_ack()`: Handle new ACK
- `on_duplicate_ack()`: Handle duplicate ACK
- `on_timeout()`: Handle timeout

---

## Submission Checklist

- [x] Source code (node.c, node.h)
- [x] Makefile
- [x] Configuration files (A.conf, B.conf, etc.)
- [x] Event files (test scenarios)
- [x] README.md
- [ ] Demonstration video showing:
  - Execution of algorithms
  - Congestion window evolution
  - Timeout scenario
  - Triple duplicate ACK scenario
  - Explanation of differences

---

## Video Demonstration Script

### Part 1: Introduction (30 seconds)
"Hello, this is our TCP Congestion Control implementation for CSE 320. We've implemented TCP Tahoe, Reno, and NewReno algorithms."

### Part 2: Compilation (15 seconds)
```bash
make clean
make
```

### Part 3: TCP Tahoe - Timeout (1 minute)
```bash
./node A.conf tahoe events/tahoe_timeout.txt
```
"Notice how Tahoe resets cwnd to 1 after timeout..."

### Part 4: TCP Tahoe vs Reno - Triple Duplicate ACK (1.5 minutes)
```bash
./node A.conf tahoe events/tahoe_dupack.txt
./node A.conf reno events/tahoe_dupack.txt
```
"Observe the key difference: Tahoe resets cwnd to 1, but Reno uses Fast Recovery..."

### Part 5: Comprehensive Comparison (1.5 minutes)
```bash
make test-comprehensive
```
"Let's compare all three algorithms under the same conditions..."

### Part 6: Conclusion (30 seconds)
"As we've seen, Reno and NewReno recover faster from packet loss compared to Tahoe..."

---

## Troubleshooting

### Compilation Errors
```bash
# Make sure you have gcc installed
gcc --version

# Clean and rebuild
make clean
make
```

### Event File Errors
- Check file format (ACK/DUPACK/TIMEOUT)
- Ensure proper line endings
- Verify ACK numbers are integers

---

## References

- RFC 2001: TCP Slow Start, Congestion Avoidance, Fast Retransmit, and Fast Recovery Algorithms
- RFC 2581: TCP Congestion Control
- RFC 3782: The NewReno Modification to TCP's Fast Recovery Algorithm

---

## Authors
CSE 320 Computer Networks Programming Assignment
[Your Names and Student IDs]

**Last Updated**: April 2026