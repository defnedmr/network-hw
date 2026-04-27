# TCP Tahoe Congestion Control Simulator

## Project Overview

This project implements a **TCP Tahoe Congestion Control Algorithm** simulator on a 6-node network topology. The simulator demonstrates how TCP Tahoe responds to network congestion through congestion window (cwnd) evolution, slow start, congestion avoidance, and packet loss recovery.

**Algorithm Selected:** TCP Tahoe  
**Student IDs:** 20220808038 + 20230808010  
**Calculation:** (20220808038 + 20230808010) % 3 = 0 → TCP Tahoe ✅

---

## System Requirements

- **C Compiler:** gcc (Linux/Unix) or MSVC (Windows)
- **Libraries:** pthread (POSIX threads)
- **Network:** UDP-based socket programming
- **Platforms:** Linux, Windows, macOS

---

## Project Structure

```
network-hw/
├── cse320_node_code_template/
│   ├── node.c              # Main implementation
│   ├── node.h              # Header file (declarations)
│   ├── A.conf to F.conf    # Node configuration files
│   ├── node                # Compiled binary (Linux)
│   ├── node.exe            # Compiled binary (Windows)
│   ├── README.md           # Detailed documentation
│   └── events/             # Test event files
├── Roadmap.md              # Project roadmap
├── VIDEO_SCRIPT.md         # Demo video script
└── CSE320_Congestion_Control_Assignment.pdf  # Assignment
```

---

## Compilation

### Linux/Unix
```bash
cd cse320_node_code_template
gcc -pthread -o node node.c
```

### Windows (with MinGW)
```bash
cd cse320_node_code_template
gcc -pthread -o node.exe node.c -lws2_32
```

### Verify Compilation
```bash
./node  # Should show usage message
```

---

## Running the Simulator

### Step 1: Open 6 Terminal Windows
Arrange them side by side for visibility.

### Step 2: Start Each Node

```bash
# Terminal 1 - Node A
cd cse320_node_code_template
./node A.conf

# Terminal 2 - Node B
./node B.conf

# Terminal 3 - Node C
./node C.conf

# Terminal 4 - Node D
./node D.conf

# Terminal 5 - Node E
./node E.conf

# Terminal 6 - Node F
./node F.conf
```

Each node will output:
```
==========================================
TCP TAHOE - Node X (Port 500X)
==========================================
Routing Table for Node X
[X] Listening on port 500X
```

---

## Interactive Commands

### Send Message
```bash
send <destination> <message>
```
**Example:**
```
A> send D hello_from_A
[A] Destination D, next hop B
[A] Forwarding message from A to D, next hop D
  [TCP-TAHOE] Sent, cwnd=2.00

[D] Received from A (seq=1): hello_from_A
  [TCP-TAHOE] ACK: cwnd=2.00 (Slow Start)
```

### Show Routing Table
```bash
route
```
Displays the routing table for the current node with destinations, next hops, and costs.

### Display TCP State
```bash
tcp
```
Shows current TCP TAHOE state:
```
[TCP TAHOE STATE]
cwnd: 2.00 MSS
ssthresh: 16.00 MSS
State: Slow Start
Dup ACK Count: 0
```

### Simulate Timeout (Packet Loss)
```bash
timeout
```
Triggers a timeout event:
```
[SIMULATION] Timeout triggered

  *** TIMEOUT: PACKET LOSS DETECTED ***
  cwnd: 16.00 -> 1.00
  ssthresh: 8.00
  State: Slow Start (restart)
```

### Simulate Duplicate ACK
```bash
dupack
```
Simulates receiving a duplicate ACK. When triggered 3 times, triggers timeout:
```
[SIMULATION] Duplicate ACK
  [TCP-TAHOE] Duplicate ACK #1/3

[SIMULATION] Duplicate ACK
  [TCP-TAHOE] Duplicate ACK #2/3

[SIMULATION] Duplicate ACK
  [TCP-TAHOE] Duplicate ACK #3/3
  *** TRIPLE DUPLICATE ACK ***
  *** TIMEOUT: PACKET LOSS DETECTED ***
```

### Exit Program
```bash
quit
```

---

## Network Topology

```
         A (5001)
       / | \
      B  C  F
      |\ |  |
      D  E--+
```

### Node Configuration Files

Each `X.conf` file contains:
- Node ID and Port
- Neighbor information (ID, IP, Port, Cost)

**Example A.conf:**
```
# Node A Configuration
A 5001

# Neighbors
B 127.0.0.1 5002 4
C 127.0.0.1 5003 7
D 127.0.0.1 5004 13
F 127.0.0.1 5006 5
```

---

## TCP Tahoe Algorithm Implementation

### State Machine

```
                    ┌─────────────┐
                    │ SLOW_START  │
                    └──────┬──────┘
                           │ cwnd >= ssthresh
                           ▼
                  ┌────────────────────┐
                  │ CONGESTION_AVOIDANCE│
                  └──────────┬─────────┘
                             │ Packet Loss
                             ▼
                        SLOW_START (restart)
```

### Key Behaviors

#### 1. Slow Start
- **Condition:** cwnd < ssthresh
- **Action on ACK:** cwnd += 1 (exponential growth)
- **Example:** 1 → 2 → 3 → 4 → 8 → 16

#### 2. Congestion Avoidance
- **Condition:** cwnd >= ssthresh
- **Action on ACK:** cwnd += 1/cwnd (linear growth)
- **Example:** 16 → 16.06 → 16.12 → 16.19

#### 3. Timeout Handling
- **Trigger:** Manual `timeout` command OR 3 duplicate ACKs
- **Actions:**
  - ssthresh = cwnd / 2
  - cwnd = 1
  - State = SLOW_START
  - dup_ack_count = 0

### Parameters

```c
typedef struct {
    TCPState state;        // SLOW_START or CONGESTION_AVOIDANCE
    double cwnd;           // Congestion window (in MSS)
    double ssthresh;       // Slow-start threshold (default: 16.0)
    int dup_ack_count;     // Duplicate ACK counter
    int next_seq;          // Next sequence number
} TCPTahoe;
```

---

## Test Scenarios

### Scenario 1: Normal Send (Slow Start)
```bash
# Terminal A
A> send D msg1
A> send D msg2
A> send D msg3
A> tcp
```

**Expected Output:**
- cwnd increases: 1 → 2 → 3 → 4
- State remains "Slow Start"

### Scenario 2: Timeout Event
```bash
# Terminal A (after Slow Start phase)
A> timeout
A> tcp
```

**Expected Output:**
- cwnd: 4.00 → 1.00
- ssthresh: 2.00
- State: Slow Start (restarted)

### Scenario 3: Triple Duplicate ACK
```bash
# Terminal A
A> dupack
A> dupack
A> dupack
A> tcp
```

**Expected Output:**
- After 3rd dupack: timeout triggered
- cwnd reset to 1.00
- State: Slow Start

---

## Routing Behavior

The simulator implements optimal routing using Dijkstra-like path selection:

| From | To | Next Hop | Cost | Path    |
|------|-------|----------|------|---------|
| A    | D     | B        | 12   | A→B→D   |
| A    | E     | B        | 9    | A→B→E   |
| D    | A     | B        | 12   | D→B→A   |

Messages automatically forward through the network:
```
[A] Destination D, next hop B
    └─→ [B] Forwarding A→D, next hop D
        └─→ [D] Received from A
            └─→ ACK increases cwnd
```

---

## Logging Output Explanation

### ACK Event
```
[A] Received from B (seq=1): acknowledgment
  [TCP-TAHOE] ACK: cwnd=2.00 (Slow Start)
```
- Shows received packet details
- TCP state update
- Current algorithm phase

### Timeout Event
```
  *** TIMEOUT: PACKET LOSS DETECTED ***
  cwnd: 8.00 -> 1.00
  ssthresh: 4.00
  State: Slow Start (restart)
```
- Old vs new cwnd values
- New ssthresh calculation
- State transition

### Transition Event
```
  *** TRANSITION: Slow Start -> Congestion Avoidance ***
```
- Occurs when cwnd reaches ssthresh
- Marks algorithmic phase change

---

## Performance Metrics

The simulator tracks and displays:
- **cwnd Evolution:** Shows growth pattern (exponential vs linear)
- **State Transitions:** Logs phase changes
- **Packet Loss Recovery:** Demonstrates timeout and recovery
- **Duplicate ACK Handling:** Shows fast retransmit mechanism

---

## Assumptions & Limitations

1. **Simplified Packet Loss:** Uses manual commands (`timeout`, `dupack`) instead of random loss simulation
2. **No Real Retransmission:** Lost packets aren't actually retransmitted, only state is updated
3. **Single TCP Connection:** Simulates one sender-receiver pair per node
4. **UDP-based:** Uses UDP sockets (simpler than TCP)
5. **Localhost Only:** All nodes run on 127.0.0.1
6. **Synchronized Clocks:** No real RTT calculation, manual timeout triggers

---

## Expected Output Examples

### Starting Node A
```
==========================================
TCP TAHOE - Node A (Port 5001)
==========================================

+---------------------------------------------+
|  Routing Table for Node A                  |
+--------------+----------+------+-----------+
| Destination  | Next Hop | Cost |   Path    |
+--------------+----------+------+-----------+
|      A       |    -     |   0  |     A     |
|      B       |    B     |   4  |   A->B    |
|      C       |    C     |   7  |   A->C    |
|      D       |    B     |  12  |   A->B    |
|      E       |    B     |   9  |   A->B    |
|      F       |    F     |   5  |   A->F    |
+--------------+----------+------+-----------+

[A] Listening on port 5001
```

### Sending Message
```
A> send D test_message
[A] Destination D, next hop B
  [TCP-TAHOE] Sent, cwnd=2.00

[B] Forwarding A->D, next hop D (seq=1)

[D] Received from A (seq=1): test_message
  [TCP-TAHOE] ACK: cwnd=2.00 (Slow Start)
  *** TRANSITION: Slow Start -> Congestion Avoidance ***
```

---

## Troubleshooting

### Issue: "Address already in use"
**Solution:** Wait 30 seconds or change port numbers in `.conf` files

### Issue: "Failed to load config"
**Solution:** Ensure you're in the correct directory with `.conf` files

### Issue: Messages not received
**Solution:** 
1. Check all 6 nodes are running
2. Verify destination node ID is valid (A-F)
3. Check routing table with `route` command

### Issue: Compilation error on Windows
**Solution:** Add `-lws2_32` flag:
```bash
gcc -pthread -o node.exe node.c -lws2_32
```

---

## Future Enhancements

1. **Automatic Packet Loss:** Random loss simulation based on percentage
2. **Real RTT Measurement:** Implement timing mechanisms
3. **Multiple Connections:** Support multiple TCP flows
4. **TCP Reno/NewReno:** Implement alternative algorithms
5. **Performance Metrics:** Throughput calculation, fairness index
6. **Visualization:** Real-time graphs of cwnd evolution

---

## References

- RFC 5681: TCP Congestion Control
- W. Richard Stevens: "TCP/IP Illustrated"
- TCP Tahoe vs Reno detailed comparison

---

## Authors

**Group Members:**
- Student 1 (ID: 20220808038)
- Student 2 (ID: 20230808010)

**Assignment:** CSE 320 - Computer Networks  
**Date:** April 2026

---

## License

This project is for educational purposes as part of CSE 320 course.

---

## Support

For issues or questions:
1. Check the troubleshooting section
2. Review VIDEO_SCRIPT.md for demonstration
3. Examine node.c source code for implementation details
