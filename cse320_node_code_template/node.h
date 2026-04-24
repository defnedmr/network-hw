#ifndef NODE_H
#define NODE_H

#include <pthread.h>

#define MAX_NEIGHBORS 10
#define MAX_NODES 10
#define MAX_LINE 256
#define MSS 1460

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
    char id;
    char ip[64];
    int port;
    int cost;
} Neighbor;

// Routing table entry
typedef struct {
    char dest;
    char next_hop;
    int cost;
} RoutingEntry;

// Network packet
typedef struct {
    char src;
    char dest;
    int seq_num;
    char data[1024];
} Packet;

// TCP State for congestion control
typedef struct {
    Algorithm algorithm;
    CCState state;
    double cwnd;
    double ssthresh;
    int dup_ack_count;
    int round;
    int recover_seq;
    int last_ack;
    int next_seq;
} TCPState;

// Node configuration
typedef struct {
    char node_id;
    int port;
    Neighbor neighbors[MAX_NEIGHBORS];
    int neighbor_count;
    RoutingEntry routing_table[MAX_NODES];
    int routing_table_size;
    TCPState tcp;
    pthread_mutex_t lock;
} NodeConfig;

// Function declarations
Algorithm parse_algorithm(const char *name);
const char *algorithm_name(Algorithm algorithm);
const char *state_name(CCState state);
int load_config(const char *filename, NodeConfig *config);
void init_tcp(TCPState *tcp, Algorithm algorithm);
void on_ack(TCPState *tcp, int ack_no);
void on_duplicate_ack(TCPState *tcp, int ack_no);
void on_timeout(TCPState *tcp);
void init_routing_table(NodeConfig *config);
void print_routing_table(NodeConfig *config);
void *listen_thread(void *arg);
void send_message(NodeConfig *config, char dest, const char *message);
void interactive_shell(NodeConfig *config);

#endif 