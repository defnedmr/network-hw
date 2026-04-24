#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "node.h"

Algorithm parse_algorithm(const char *name) {
    if (strcmp(name, "tahoe") == 0) return ALG_TAHOE;
    if (strcmp(name, "reno") == 0) return ALG_RENO;
    if (strcmp(name, "newreno") == 0) return ALG_NEWRENO;
    return ALG_UNKNOWN;
}

const char *algorithm_name(Algorithm algorithm) {
    switch (algorithm) {
        case ALG_TAHOE: return "TCP Tahoe";
        case ALG_RENO: return "TCP Reno";
        case ALG_NEWRENO: return "TCP NewReno";
        default: return "Unknown";
    }
}

const char *state_name(CCState state) {
    switch (state) {
        case SLOW_START: return "Slow Start";
        case CONGESTION_AVOIDANCE: return "Congestion Avoidance";
        case FAST_RECOVERY: return "Fast Recovery";
        default: return "Unknown";
    }
}

int load_config(const char *filename, NodeConfig *config) {
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        return 0;
    }

    char line[MAX_LINE];
    config->neighbor_count = 0;

    /*
       First non-comment line:
       NodeID Port

       Example:
       A 5001
    */
    while (fgets(line, sizeof(line), fp)) {
        if (line[0] == '#' || line[0] == '\n') {
            continue;
        }

        if (sscanf(line, " %c %d", &config->node_id, &config->port) == 2) {
            break;
        }
    }

    /*
       Remaining non-comment lines:
       NeighborID IP Port Cost

       Example:
       B 127.0.0.1 5002 4
    */
    while (fgets(line, sizeof(line), fp)) {
        if (line[0] == '#' || line[0] == '\n') {
            continue;
        }

        if (config->neighbor_count >= MAX_NEIGHBORS) {
            printf("Too many neighbors. Increase MAX_NEIGHBORS.\n");
            fclose(fp);
            return 0;
        }

        Neighbor *n = &config->neighbors[config->neighbor_count];

        if (sscanf(line, " %c %63s %d %d", &n->id, n->ip, &n->port, &n->cost) == 4) {
            config->neighbor_count++;
        }
    }

    fclose(fp);
    return 1;
}

void init_tcp(TCPState *tcp, Algorithm algorithm) {
    tcp->algorithm = algorithm;
    tcp->state = SLOW_START;
    tcp->cwnd = 1.0;           // Start with 1 MSS
    tcp->ssthresh = 16.0;      // Initial ssthresh (common default)
    tcp->dup_ack_count = 0;
    tcp->round = 0;
    tcp->recover_seq = 0;
    tcp->last_ack = 0;
}

static void print_tcp_status(TCPState *tcp, const char *event, int ack_no, const char *note) {
    printf("%5d | %-10s | ACK=%-3d | cwnd=%5.2f | ssthresh=%5.2f | %-22s | %s\n",
           tcp->round,
           event,
           ack_no,
           tcp->cwnd,
           tcp->ssthresh,
           state_name(tcp->state),
           note);
}

// Handle new ACK (acknowledgment for new data)
void on_ack(TCPState *tcp, int ack_no) {
    tcp->round++;
    tcp->last_ack = ack_no;
    tcp->dup_ack_count = 0;  // Reset duplicate ACK counter

    if (tcp->state == SLOW_START) {
        // Slow Start: exponential growth (cwnd += 1 per ACK)
        tcp->cwnd += 1.0;

        // Transition to Congestion Avoidance if cwnd reaches ssthresh
        if (tcp->cwnd >= tcp->ssthresh) {
            tcp->state = CONGESTION_AVOIDANCE;
            print_tcp_status(tcp, "ACK", ack_no, "new ACK; reached ssthresh -> Congestion Avoidance");
        } else {
            print_tcp_status(tcp, "ACK", ack_no, "new ACK; Slow Start (exponential growth)");
        }
    }
    else if (tcp->state == CONGESTION_AVOIDANCE) {
        // Congestion Avoidance: linear growth (cwnd += 1/cwnd per ACK)
        tcp->cwnd += 1.0 / tcp->cwnd;
        print_tcp_status(tcp, "ACK", ack_no, "new ACK; Congestion Avoidance (additive increase)");
    }
    else if (tcp->state == FAST_RECOVERY) {
        // Fast Recovery handling differs by algorithm
        if (tcp->algorithm == ALG_RENO || tcp->algorithm == ALG_NEWRENO) {
            // Full ACK: exit Fast Recovery
            tcp->cwnd = tcp->ssthresh;
            tcp->state = CONGESTION_AVOIDANCE;
            print_tcp_status(tcp, "ACK", ack_no, "full ACK; exit Fast Recovery -> Congestion Avoidance");
        }
    }
}

// Handle duplicate ACK
void on_duplicate_ack(TCPState *tcp, int ack_no) {
    tcp->round++;
    tcp->dup_ack_count++;

    if (tcp->dup_ack_count < 3) {
        print_tcp_status(tcp, "DUPACK", ack_no, "duplicate ACK received; waiting for 3rd");
        return;
    }

    if (tcp->dup_ack_count == 3) {
        // Third duplicate ACK received - packet loss detected
        
        // Set ssthresh to half of current cwnd
        tcp->ssthresh = tcp->cwnd / 2.0;
        if (tcp->ssthresh < 2.0) {
            tcp->ssthresh = 2.0;  // Minimum ssthresh
        }

        if (tcp->algorithm == ALG_TAHOE) {
            // TCP TAHOE: Reset cwnd to 1 and go to Slow Start
            // No Fast Recovery in Tahoe
            tcp->cwnd = 1.0;
            tcp->state = SLOW_START;
            print_tcp_status(tcp, "DUPACK", ack_no, "3 DUPACKs; TAHOE: Fast Retransmit -> reset cwnd=1, Slow Start");
        }
        else if (tcp->algorithm == ALG_RENO) {
            // TCP RENO: Fast Retransmit + Fast Recovery
            tcp->cwnd = tcp->ssthresh + 3.0;  // ssthresh + 3 (for the 3 dup ACKs)
            tcp->state = FAST_RECOVERY;
            print_tcp_status(tcp, "DUPACK", ack_no, "3 DUPACKs; RENO: Fast Retransmit + Fast Recovery");
        }
        else if (tcp->algorithm == ALG_NEWRENO) {
            // TCP NewReno: Similar to Reno but handles partial ACKs better
            tcp->cwnd = tcp->ssthresh + 3.0;
            tcp->state = FAST_RECOVERY;
            tcp->recover_seq = ack_no;  // Mark recovery point
            print_tcp_status(tcp, "DUPACK", ack_no, "3 DUPACKs; NEWRENO: Fast Retransmit + Fast Recovery");
        }
    }
    else if (tcp->dup_ack_count > 3) {
        // Additional duplicate ACKs during Fast Recovery
        if (tcp->algorithm == ALG_RENO || tcp->algorithm == ALG_NEWRENO) {
            if (tcp->state == FAST_RECOVERY) {
                // Inflate cwnd by 1 for each additional duplicate ACK
                tcp->cwnd += 1.0;
                print_tcp_status(tcp, "DUPACK", ack_no, "additional DUPACK; inflate cwnd in Fast Recovery");
            }
        }
    }
}

// Handle timeout
void on_timeout(TCPState *tcp) {
    tcp->round++;

    // Set ssthresh to half of current cwnd
    tcp->ssthresh = tcp->cwnd / 2.0;
    if (tcp->ssthresh < 2.0) {
        tcp->ssthresh = 2.0;
    }

    // Reset cwnd to 1 (all algorithms behave the same on timeout)
    tcp->cwnd = 1.0;
    tcp->dup_ack_count = 0;
    tcp->state = SLOW_START;

    print_tcp_status(tcp, "TIMEOUT", 0, "TIMEOUT; reset cwnd=1, ssthresh halved, Slow Start");
}

static void print_config(NodeConfig *config) {
    int i;

    printf("\n========================================\n");
    printf("Node Configuration\n");
    printf("========================================\n");
    printf("Node ID: %c\n", config->node_id);
    printf("Port: %d\n", config->port);
    printf("Neighbors: %d\n", config->neighbor_count);
    
    for (i = 0; i < config->neighbor_count; i++) {
        printf("  [%d] %c @ %s:%d (cost=%d)\n",
               i + 1,
               config->neighbors[i].id,
               config->neighbors[i].ip,
               config->neighbors[i].port,
               config->neighbors[i].cost);
    }
    printf("========================================\n\n");
}

static int run_event_file(const char *event_file, TCPState *tcp) {
    FILE *fp = fopen(event_file, "r");
    if (!fp) {
        printf("ERROR: Could not open event file: %s\n", event_file);
        return 0;
    }

    char event[32];
    int ack_no;

    printf("\n========================================\n");
    printf("TCP Congestion Control Simulation\n");
    printf("Algorithm: %s\n", algorithm_name(tcp->algorithm));
    printf("Initial State: %s\n", state_name(tcp->state));
    printf("Initial cwnd: %.2f MSS (%.0f bytes)\n", tcp->cwnd, tcp->cwnd * MSS);
    printf("Initial ssthresh: %.2f MSS (%.0f bytes)\n", tcp->ssthresh, tcp->ssthresh * MSS);
    printf("========================================\n\n");

    printf("Round | Event      | ACK    | cwnd       | ssthresh   | State                  | Explanation\n");
    printf("------+------------+--------+------------+------------+------------------------+-------------------------------------------\n");

    while (fscanf(fp, "%31s", event) == 1) {
        if (strcmp(event, "ACK") == 0) {
            if (fscanf(fp, "%d", &ack_no) != 1) {
                printf("ERROR: Malformed ACK event.\n");
                fclose(fp);
                return 0;
            }
            on_ack(tcp, ack_no);
        }
        else if (strcmp(event, "DUPACK") == 0) {
            if (fscanf(fp, "%d", &ack_no) != 1) {
                printf("ERROR: Malformed DUPACK event.\n");
                fclose(fp);
                return 0;
            }
            on_duplicate_ack(tcp, ack_no);
        }
        else if (strcmp(event, "TIMEOUT") == 0) {
            on_timeout(tcp);
        }
        else {
            printf("ERROR: Unknown event: %s\n", event);
            fclose(fp);
            return 0;
        }
    }

    printf("------+------------+--------+------------+------------+------------------------+-------------------------------------------\n");
    printf("\nFinal State:\n");
    printf("  cwnd: %.2f MSS (%.0f bytes)\n", tcp->cwnd, tcp->cwnd * MSS);
    printf("  ssthresh: %.2f MSS (%.0f bytes)\n", tcp->ssthresh, tcp->ssthresh * MSS);
    printf("  State: %s\n", state_name(tcp->state));
    printf("  Total rounds: %d\n", tcp->round);

    fclose(fp);
    return 1;
}

int main(int argc, char *argv[]) {
    NodeConfig config;
    TCPState tcp;

    printf("\n");
    printf("╔═══════════════════════════════════════════════════════╗\n");
    printf("║   TCP Congestion Control Algorithm Simulator          ║\n");
    printf("║   CSE 320 - Computer Networks                         ║\n");
    printf("╚═══════════════════════════════════════════════════════╝\n");

    if (argc != 4) {
        printf("\nUsage: %s <config_file> <algorithm> <event_file>\n", argv[0]);
        printf("\nExample:\n");
        printf("  %s A.conf tahoe events/tahoe_test.txt\n\n", argv[0]);
        printf("Supported Algorithms:\n");
        printf("  - tahoe    : TCP Tahoe (Fast Retransmit, no Fast Recovery)\n");
        printf("  - reno     : TCP Reno (Fast Retransmit + Fast Recovery)\n");
        printf("  - newreno  : TCP NewReno (improved partial ACK handling)\n\n");
        return 1;
    }

    // Parse algorithm
    Algorithm selected = parse_algorithm(argv[2]);
    if (selected == ALG_UNKNOWN) {
        printf("\nERROR: Invalid algorithm '%s'\n", argv[2]);
        printf("Valid options: tahoe, reno, newreno\n\n");
        return 1;
    }

    // Load node configuration
    if (!load_config(argv[1], &config)) {
        printf("\nERROR: Could not load config file: %s\n\n", argv[1]);
        return 1;
    }

    print_config(&config);

    // Initialize TCP state
    init_tcp(&tcp, selected);

    if (!run_event_file(argv[3], &tcp)) {
        printf("\nSimulation failed.\n\n");
        return 1;
    }

    printf("\n✓ Simulation completed successfully.\n\n");
    return 0;
}