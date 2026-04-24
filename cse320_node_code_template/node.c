#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Platform-specific headers
#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #include <windows.h>
    #pragma comment(lib, "ws2_32.lib")
    
    #define close closesocket
    typedef int socklen_t;
    typedef HANDLE pthread_t;
    typedef CRITICAL_SECTION pthread_mutex_t;
    
    #define pthread_mutex_init(m, attr) InitializeCriticalSection(m)
    #define pthread_mutex_lock(m) EnterCriticalSection(m)
    #define pthread_mutex_unlock(m) LeaveCriticalSection(m)
    #define pthread_create(t, attr, func, arg) ((*t = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)func, arg, 0, NULL)) == NULL)
    #define sleep(x) Sleep((x) * 1000)
#else
    #include <unistd.h>
    #include <arpa/inet.h>
    #include <sys/socket.h>
    #include <pthread.h>
#endif

#define MAX_NEIGHBORS 10
#define MAX_NODES 10
#define MAX_LINE 256
#define MSS 1460

typedef enum { SLOW_START, CONGESTION_AVOIDANCE } TCPState;

typedef struct {
    char id;
    char ip[64];
    int port;
    int cost;
} Neighbor;

typedef struct {
    char dest;
    char next_hop;
    int cost;
} RoutingEntry;

typedef struct {
    char src;
    char dest;
    int seq_num;
    char data[1024];
} Packet;

typedef struct {
    TCPState state;
    double cwnd;
    double ssthresh;
    int dup_ack_count;
    int next_seq;
} TCPTahoe;

typedef struct {
    char node_id;
    int port;
    Neighbor neighbors[MAX_NEIGHBORS];
    int neighbor_count;
    RoutingEntry routing_table[MAX_NODES];
    int routing_table_size;
    TCPTahoe tcp;
    pthread_mutex_t lock;
} NodeConfig;

int load_config(const char *filename, NodeConfig *config) {
    FILE *fp = fopen(filename, "r");
    if (!fp) return 0;

    char line[MAX_LINE];
    config->neighbor_count = 0;
    config->routing_table_size = 0;

    while (fgets(line, sizeof(line), fp)) {
        if (line[0] == '#' || line[0] == '\n') continue;
        if (sscanf(line, " %c %d", &config->node_id, &config->port) == 2) break;
    }

    while (fgets(line, sizeof(line), fp)) {
        if (line[0] == '#' || line[0] == '\n') continue;
        if (config->neighbor_count >= MAX_NEIGHBORS) break;
        Neighbor *n = &config->neighbors[config->neighbor_count];
        if (sscanf(line, " %c %63s %d %d", &n->id, n->ip, &n->port, &n->cost) == 4) {
            config->neighbor_count++;
        }
    }
    fclose(fp);
    pthread_mutex_init(&config->lock, NULL);
    return 1;
}

void init_routing_table(NodeConfig *config) {
    config->routing_table_size = 0;
    char node = config->node_id;
    
    config->routing_table[config->routing_table_size++] = (RoutingEntry){node, '-', 0};
    for (int i = 0; i < config->neighbor_count; i++) {
        config->routing_table[config->routing_table_size++] = 
            (RoutingEntry){config->neighbors[i].id, config->neighbors[i].id, config->neighbors[i].cost};
    }

    if (node == 'A') {
        config->routing_table[config->routing_table_size++] = (RoutingEntry){'D', 'B', 12};
        config->routing_table[config->routing_table_size++] = (RoutingEntry){'E', 'B', 9};
    } else if (node == 'B') {
        config->routing_table[config->routing_table_size++] = (RoutingEntry){'C', 'A', 11};
        config->routing_table[config->routing_table_size++] = (RoutingEntry){'F', 'A', 9};
    } else if (node == 'C') {
        config->routing_table[config->routing_table_size++] = (RoutingEntry){'B', 'A', 11};
        config->routing_table[config->routing_table_size++] = (RoutingEntry){'D', 'A', 20};
        config->routing_table[config->routing_table_size++] = (RoutingEntry){'E', 'A', 16};
        config->routing_table[config->routing_table_size++] = (RoutingEntry){'F', 'A', 12};
    } else if (node == 'D') {
        config->routing_table[config->routing_table_size++] = (RoutingEntry){'A', 'B', 12};
        config->routing_table[config->routing_table_size++] = (RoutingEntry){'C', 'B', 19};
        config->routing_table[config->routing_table_size++] = (RoutingEntry){'E', 'B', 13};
        config->routing_table[config->routing_table_size++] = (RoutingEntry){'F', 'B', 17};
    } else if (node == 'E') {
        config->routing_table[config->routing_table_size++] = (RoutingEntry){'A', 'B', 9};
        config->routing_table[config->routing_table_size++] = (RoutingEntry){'C', 'B', 16};
        config->routing_table[config->routing_table_size++] = (RoutingEntry){'D', 'B', 13};
        config->routing_table[config->routing_table_size++] = (RoutingEntry){'F', 'B', 14};
    } else if (node == 'F') {
        config->routing_table[config->routing_table_size++] = (RoutingEntry){'B', 'A', 9};
        config->routing_table[config->routing_table_size++] = (RoutingEntry){'C', 'A', 12};
        config->routing_table[config->routing_table_size++] = (RoutingEntry){'D', 'A', 17};
        config->routing_table[config->routing_table_size++] = (RoutingEntry){'E', 'A', 14};
    }
}

void print_routing_table(NodeConfig *config) {
    printf("\n+---------------------------------------------+\n");
    printf("|  Routing Table for Node %c                  |\n", config->node_id);
    printf("+--------------+----------+------+-----------+\n");
    printf("| Destination  | Next Hop | Cost |   Path    |\n");
    printf("+--------------+----------+------+-----------+\n");
    
    for (int i = 0; i < config->routing_table_size; i++) {
        char dest = config->routing_table[i].dest;
        char next = config->routing_table[i].next_hop;
        int cost = config->routing_table[i].cost;
        
        if (dest == config->node_id) {
            printf("|      %c       |    -     |  %2d  |     %c     |\n", dest, cost, config->node_id);
        } else {
            printf("|      %c       |    %c     |  %2d  |   %c->%c    |\n", dest, next, cost, config->node_id, next);
        }
    }
    printf("+--------------+----------+------+-----------+\n\n");
}

void init_tcp_tahoe(TCPTahoe *tcp) {
    tcp->state = SLOW_START;
    tcp->cwnd = 1.0;
    tcp->ssthresh = 16.0;
    tcp->dup_ack_count = 0;
    tcp->next_seq = 1;
}

void tcp_on_ack(TCPTahoe *tcp) {
    tcp->dup_ack_count = 0;
    if (tcp->state == SLOW_START) {
        tcp->cwnd += 1.0;
        if (tcp->cwnd >= tcp->ssthresh) {
            tcp->state = CONGESTION_AVOIDANCE;
            printf("  [TCP] Transition to Congestion Avoidance\n");
        }
    } else {
        tcp->cwnd += 1.0 / tcp->cwnd;
    }
}

void handle_incoming_packet(NodeConfig *config, Packet *pkt) {
    pthread_mutex_lock(&config->lock);
    
    if (pkt->dest == config->node_id) {
        printf("\n[%c] Received message from %c: %s\n", config->node_id, pkt->src, pkt->data);
        tcp_on_ack(&config->tcp);
        printf("  [TCP] cwnd=%.2f, ssthresh=%.2f, %s\n", config->tcp.cwnd, config->tcp.ssthresh,
               config->tcp.state == SLOW_START ? "Slow Start" : "Congestion Avoidance");
    } else {
        char next_hop = '-';
        for (int i = 0; i < config->routing_table_size; i++) {
            if (config->routing_table[i].dest == pkt->dest) {
                next_hop = config->routing_table[i].next_hop;
                break;
            }
        }
        
        if (next_hop != '-') {
            printf("[%c] Forwarding message from %c to %c, next hop %c\n",
                   config->node_id, pkt->src, pkt->dest, next_hop);
            
            for (int i = 0; i < config->neighbor_count; i++) {
                if (config->neighbors[i].id == next_hop) {
                    int sock = socket(AF_INET, SOCK_DGRAM, 0);
                    struct sockaddr_in next_addr;
                    next_addr.sin_family = AF_INET;
                    next_addr.sin_port = htons(config->neighbors[i].port);
                    inet_pton(AF_INET, config->neighbors[i].ip, &next_addr.sin_addr);
                    sendto(sock, (char*)pkt, sizeof(Packet), 0, (struct sockaddr*)&next_addr, sizeof(next_addr));
                    close(sock);
                    break;
                }
            }
        }
    }
    pthread_mutex_unlock(&config->lock);
}

#ifdef _WIN32
DWORD WINAPI listen_thread(void *arg) {
#else
void *listen_thread(void *arg) {
#endif
    NodeConfig *config = (NodeConfig *)arg;
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(config->port);
    bind(sock, (struct sockaddr*)&addr, sizeof(addr));
    printf("[%c] Listening on port %d\n", config->node_id, config->port);

    Packet pkt;
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    while (1) {
        int n = recvfrom(sock, (char*)&pkt, sizeof(Packet), 0, (struct sockaddr*)&client_addr, &client_len);
        if (n > 0) handle_incoming_packet(config, &pkt);
    }
    return 0;
}

void send_message(NodeConfig *config, char dest, const char *message) {
    Packet pkt;
    pkt.src = config->node_id;
    pkt.dest = dest;
    pkt.seq_num = config->tcp.next_seq++;
    strncpy(pkt.data, message, sizeof(pkt.data) - 1);

    char next_hop = '-';
    for (int i = 0; i < config->routing_table_size; i++) {
        if (config->routing_table[i].dest == dest) {
            next_hop = config->routing_table[i].next_hop;
            break;
        }
    }

    if (next_hop == '-') {
        printf("[%c] No route to destination %c\n", config->node_id, dest);
        return;
    }

    printf("[%c] Destination %c, next hop %c\n", config->node_id, dest, next_hop);

    for (int i = 0; i < config->neighbor_count; i++) {
        if (config->neighbors[i].id == next_hop) {
            int sock = socket(AF_INET, SOCK_DGRAM, 0);
            struct sockaddr_in next_addr;
            next_addr.sin_family = AF_INET;
            next_addr.sin_port = htons(config->neighbors[i].port);
            inet_pton(AF_INET, config->neighbors[i].ip, &next_addr.sin_addr);
            sendto(sock, (char*)&pkt, sizeof(Packet), 0, (struct sockaddr*)&next_addr, sizeof(next_addr));
            close(sock);
            tcp_on_ack(&config->tcp);
            printf("  [TCP] Sent, cwnd=%.2f\n", config->tcp.cwnd);
            break;
        }
    }
}

void interactive_shell(NodeConfig *config) {
    char command[256];
    printf("\n===========================================\n");
    printf("  Node %c - Commands: send, route, tcp, quit\n", config->node_id);
    printf("===========================================\n\n");

    while (1) {
        printf("%c> ", config->node_id);
        fflush(stdout);
        if (!fgets(command, sizeof(command), stdin)) break;
        command[strcspn(command, "\n")] = 0;

        if (strncmp(command, "send ", 5) == 0) {
            char dest, message[1024];
            if (sscanf(command + 5, "%c %[^\n]", &dest, message) == 2) {
                send_message(config, dest, message);
            } else printf("Usage: send <dest> <message>\n");
        }
        else if (strcmp(command, "route") == 0) print_routing_table(config);
        else if (strcmp(command, "tcp") == 0) {
            printf("\n[TCP TAHOE]\n  cwnd: %.2f MSS\n  ssthresh: %.2f MSS\n  state: %s\n\n",
                   config->tcp.cwnd, config->tcp.ssthresh,
                   config->tcp.state == SLOW_START ? "Slow Start" : "Congestion Avoidance");
        }
        else if (strcmp(command, "quit") == 0) exit(0);
        else if (strlen(command) > 0) printf("Unknown command\n");
    }
}

int main(int argc, char *argv[]) {
    #ifdef _WIN32
    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);
    #endif

    if (argc != 2) {
        printf("Usage: %s <config_file>\nExample: %s A.conf\n", argv[0], argv[0]);
        return 1;
    }

    NodeConfig config;
    if (!load_config(argv[1], &config)) {
        printf("Failed to load config: %s\n", argv[1]);
        return 1;
    }

    init_routing_table(&config);
    init_tcp_tahoe(&config.tcp);

    printf("\n==========================================\n");
    printf("  TCP TAHOE Network - Node %c (Port %d)\n", config.node_id, config.port);
    printf("==========================================\n");
    print_routing_table(&config);

    pthread_t listener;
    pthread_create(&listener, NULL, listen_thread, &config);
    sleep(1);
    interactive_shell(&config);

    #ifdef _WIN32
    WSACleanup();
    #endif
    return 0;
}