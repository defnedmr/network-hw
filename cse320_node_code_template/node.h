#ifndef NODE_H
#define NODE_H

#define MAX_NEIGHBORS 10
#define MAX_LINE 256
#define MSS 1460  // Maximum Segment Size in bytes

// TCP Congestion Control Algorithms
typedef enum {
    ALG_UNKNOWN = -1,
    ALG_TAHOE = 0,
    ALG_RENO = 1,
    ALG_NEWRENO = 2
} Algorithm;

// TCP Congestion Control States
typedef enum {
    SLOW_START,
    CONGESTION_AVOIDANCE,
    FAST_RECOVERY
} CCState;

// Neighbor structure for routing
typedef struct {
    char id;           // Neighbor node ID (A, B, C, etc.)
    char ip[64];       // IP address
    int port;          // Port number
    int cost;          // Link cost
} Neighbor;

// Node configuration
typedef struct {
    char node_id;                      // This node's ID
    int port;                          // This node's port
    Neighbor neighbors[MAX_NEIGHBORS]; // Array of neighbors
    int neighbor_count;                // Number of neighbors
} NodeConfig;

// TCP State for congestion control
typedef struct {
    Algorithm algorithm;     // Which algorithm (Tahoe, Reno, NewReno)
    CCState state;          // Current state (Slow Start, Congestion Avoidance, Fast Recovery)
    double cwnd;            // Congestion window (in MSS units)
    double ssthresh;        // Slow start threshold (in MSS units)
    int dup_ack_count;      // Count of duplicate ACKs
    int round;              // Current round/step number
    int recover_seq;        // Highest seq number when entering Fast Recovery (for NewReno)
    int last_ack;           // Last ACK number received
} TCPState;

// Function declarations
Algorithm parse_algorithm(const char *name);
const char *algorithm_name(Algorithm algorithm);
const char *state_name(CCState state);
int load_config(const char *filename, NodeConfig *config);
void init_tcp(TCPState *tcp, Algorithm algorithm);
void on_ack(TCPState *tcp, int ack_no);
void on_duplicate_ack(TCPState *tcp, int ack_no);
void on_timeout(TCPState *tcp);

#endif // NODE_H