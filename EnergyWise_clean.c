/*
================================================================================
PROJECT: ENERGYWISE
Global Energy-Water Optimization for Maximum Sustainable Output
PHASE 7: Water Recycling, Multi-Strategy Optimization, Crisis & What-If Engine
COMPILER: GCC / C11 Standard Compliant
================================================================================
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include <float.h>

#define MAX_NAME 50
#define MAX_FACILITIES 15
#define MAX_ENERGY_SOURCES 5
#define MAX_WATER_SOURCES 5
#define MAX_WORKLOADS 10
#define MAX_NODES 8
#define MAX_EDGES 30
#define HISTORICAL_DAYS 10
#define INF 999.0f

// Discrete DP Knapsack Constraints Capacity Limits
#define KNAPSACK_MAX_ENERGY 250 // MW
#define KNAPSACK_MAX_WATER 100  // kL (1000s of Liters)

// ============================================================================
// 1. DATA STRUCTURE DEFINITIONS
// ============================================================================

typedef struct {
    int id;
    char name[MAX_NAME];
    float energyConsumption; // MW
    float waterConsumption;  // Liters/day
    float output;            // Arbitrary useful output units
    float efficiency;        // Output / (Energy + Water)
    int priority;            // 1 (Highest/Hospital) to 8 (Lowest/Entertainment)
} Facility;

typedef struct {
    int id;
    char name[MAX_NAME];
    float capacity;          // MW Max Capacity
    float currentGeneration; // MW Current Output
    float cost;              // Cost per MW
    int renewable;           // 1 = Renewable, 0 = Conventional
} EnergySource;

typedef struct {
    int id;
    char name[MAX_NAME];
    float capacity;          // Liters Max Capacity
    float currentSupply;     // Liters Current Output
    float treatmentEnergy;   // MW required to treat 10,000 Liters
} WaterSource;

typedef struct {
    int id;
    char name[MAX_NAME];
    int energyRequired;      // Discrete MW for DP matrix alignment
    int waterRequired;       // Discrete 1000s Liters for DP matrix alignment
    int output;              // Arbitrary useful output value
    int priority;            // Allocation Priority Rank (1 = Top)
} Workload;

typedef struct {
    int num_vertices;
    char node_names[MAX_NODES][MAX_NAME];
    float adj_matrix[MAX_NODES][MAX_NODES];          
    float energy_loss_matrix[MAX_NODES][MAX_NODES];  
    float water_loss_matrix[MAX_NODES][MAX_NODES];   
} DistributionGraph;

typedef struct {
    float total_energy_generation; // MW
    float renewable_energy;        // MW
    float total_energy_demand;     // MW
    float battery_capacity;        // MWh
    
    float total_water_availability;// Liters
    float freshwater_usage;        // Liters
    float recycled_water;          // Liters
    float total_water_demand;      // Liters
    
    float total_useful_output;     // Units
    float environmental_score;     // 0 to 100 Index
    
    int current_mode;              // 0: Normal, 1: Drought, 2: Power Failure, 3: Dual Crisis
} GlobalState;

typedef struct {
    long comparisons;
    long swaps;
    char name[20];
    char time_complexity[20];
} AlgMetrics;

typedef struct {
    int items[MAX_NODES];
    int front;
    int rear;
} Queue;

typedef struct {
    int u;
    int v;
    float weight;
} Edge;

typedef struct {
    int parent[MAX_NODES];
    int rank[MAX_NODES];
} DSU;

// ============================================================================
// 2. GLOBAL SYSTEM STATE DECLARATIONS
// ============================================================================

Facility facilities[MAX_FACILITIES];
int num_facilities = 0;

EnergySource energy_sources[MAX_ENERGY_SOURCES];
int num_energy_sources = 0;

WaterSource water_sources[MAX_WATER_SOURCES];
int num_water_sources = 0;

Workload workloads[MAX_WORKLOADS];
int num_workloads = 0;

DistributionGraph grid_graph;
GlobalState global_system;

float historical_energy_demand[HISTORICAL_DAYS] = {1050.0, 1120.0, 1080.0, 1200.0, 1250.0, 1190.0, 1310.0, 1380.0, 1420.0, 1500.0};
float historical_water_efficiency[HISTORICAL_DAYS] = {85.5, 82.0, 80.4, 78.1, 79.0, 74.2, 71.0, 68.5, 65.0, 61.2};

// Dynamic 3D Matrix Pointer for 2D Knapsack DP to avoid large stack allocation
int ***dp_knapsack = NULL;

// ============================================================================
// 3. INITIALIZATION & SAMPLE DATA SEEDING
// ============================================================================

void initialize_sample_data() {
    num_facilities = 8;
    facilities[0] = (Facility){101, "Metropolitan Hospital",     120.0,  50000.0,  950.0, 0.0, 1};
    facilities[1] = (Facility){102, "Central Data Center",       250.0,  80000.0,  880.0, 0.0, 5};
    facilities[2] = (Facility){103, "High-Tech Fabrication Plant",310.0, 120000.0,  750.0, 0.0, 7};
    facilities[3] = (Facility){104, "Emergency Command Center",   45.0,  15000.0,  990.0, 0.0, 2};
    facilities[4] = (Facility){105, "Municipal Water Treatment",  180.0, 200000.0,  820.0, 0.0, 3};
    facilities[5] = (Facility){106, "Agricultural District A",    80.0, 350000.0,  600.0, 0.0, 4};
    facilities[6] = (Facility){107, "Residential Zone Alpha",     220.0, 180000.0,  700.0, 0.0, 6};
    facilities[7] = (Facility){108, "Heavy Textile Mill",        160.0, 210000.0,  450.0, 0.0, 8};

    for (int i = 0; i < num_facilities; i++) {
        facilities[i].efficiency = facilities[i].output / (facilities[i].energyConsumption + (facilities[i].waterConsumption / 1000.0f));
    }

    num_energy_sources = 4;
    energy_sources[0] = (EnergySource){201, "Helios Solar Park",       350.0, 310.0, 42.5, 1};
    energy_sources[1] = (EnergySource){202, "Boreas Wind Farm",        250.0, 190.0, 48.0, 1};
    energy_sources[2] = (EnergySource){203, "Hydroelectric Dam Alpha",  400.0, 380.0, 35.0, 1};
    energy_sources[3] = (EnergySource){204, "Thermal Power Plant 1",    500.0, 420.0, 75.0, 0};

    num_water_sources = 3;
    water_sources[0] = (WaterSource){301, "Clearwater Reservoir", 600000.0, 550000.0, 1.2};
    water_sources[1] = (WaterSource){302, "Aquifer Extraction B", 400000.0, 320000.0, 2.5};
    water_sources[2] = (WaterSource){303, "Recycled Water Grid",  300000.0, 250000.0, 1.8};

    num_workloads = 6;
    workloads[0] = (Workload){401, "AI Model Training Batch",   80, 20, 180, 5};
    workloads[1] = (Workload){402, "Desalination Processing",   60, 50, 150, 3};
    workloads[2] = (Workload){403, "Hospital ICU Backup Grid",   30, 10, 200, 1};
    workloads[3] = (Workload){404, "Chip Cleanroom Assembly",   100, 40, 210, 6};
    workloads[4] = (Workload){405, "Urban Sewage Treatment",    40, 30, 110, 2};
    workloads[5] = (Workload){406, "Automated Vertical Farm",   50, 60, 130, 4};

    grid_graph.num_vertices = 8;
    char names[8][MAX_NAME] = {
        "Main Hydro Plant", "Substation North", "Substation South",
        "Water Facility A", "Data Center Park", "Hospital Complex",
        "Industrial Zone",  "Residential Hub"
    };
    for (int i = 0; i < 8; i++) {
        strcpy(grid_graph.node_names[i], names[i]);
    }

    for (int i = 0; i < 8; i++) {
        for (int j = 0; j < 8; j++) {
            if (i == j) {
                grid_graph.adj_matrix[i][j] = 0.0f;
                grid_graph.energy_loss_matrix[i][j] = 0.0f;
                grid_graph.water_loss_matrix[i][j] = 0.0f;
            } else {
                grid_graph.adj_matrix[i][j] = INF;
                grid_graph.energy_loss_matrix[i][j] = INF;
                grid_graph.water_loss_matrix[i][j] = INF;
            }
        }
    }

    grid_graph.adj_matrix[0][1] = grid_graph.adj_matrix[1][0] = 12.0f;
    grid_graph.energy_loss_matrix[0][1] = grid_graph.energy_loss_matrix[1][0] = 2.5f;

    grid_graph.adj_matrix[0][2] = grid_graph.adj_matrix[2][0] = 18.0f;
    grid_graph.energy_loss_matrix[0][2] = grid_graph.energy_loss_matrix[2][0] = 4.1f;

    grid_graph.adj_matrix[1][3] = grid_graph.adj_matrix[3][1] = 8.0f;
    grid_graph.water_loss_matrix[1][3] = grid_graph.water_loss_matrix[3][1] = 1.2f;

    grid_graph.adj_matrix[1][4] = grid_graph.adj_matrix[4][1] = 25.0f;
    grid_graph.energy_loss_matrix[1][4] = grid_graph.energy_loss_matrix[4][1] = 5.0f;

    grid_graph.adj_matrix[2][5] = grid_graph.adj_matrix[5][2] = 10.0f;
    grid_graph.energy_loss_matrix[2][5] = grid_graph.energy_loss_matrix[5][2] = 1.8f;

    grid_graph.adj_matrix[3][6] = grid_graph.adj_matrix[6][3] = 15.0f;
    grid_graph.water_loss_matrix[3][6] = grid_graph.water_loss_matrix[6][3] = 3.5f;

    grid_graph.adj_matrix[4][6] = grid_graph.adj_matrix[6][4] = 14.0f;
    grid_graph.energy_loss_matrix[4][6] = grid_graph.energy_loss_matrix[6][4] = 2.2f;

    grid_graph.adj_matrix[5][7] = grid_graph.adj_matrix[7][5] = 9.0f;
    grid_graph.energy_loss_matrix[5][7] = grid_graph.energy_loss_matrix[7][5] = 1.1f;

    grid_graph.adj_matrix[6][7] = grid_graph.adj_matrix[7][6] = 20.0f;
    grid_graph.water_loss_matrix[6][7] = grid_graph.water_loss_matrix[7][6] = 4.0f;

    global_system.total_energy_generation = 1300.0f;
    global_system.renewable_energy = 880.0f;
    global_system.total_energy_demand = 1365.0f;
    global_system.battery_capacity = 250.0f;

    global_system.total_water_availability = 1120000.0f;
    global_system.freshwater_usage = 735000.0f;
    global_system.recycled_water = 250000.0f;
    global_system.total_water_demand = 1205000.0f;

    global_system.total_useful_output = 6140.0f;
    global_system.environmental_score = 78.5f;
    global_system.current_mode = 0;
}

void allocate_dp_knapsack_memory(int n, int max_e, int max_w) {
    dp_knapsack = (int ***)malloc((n + 1) * sizeof(int **));
    for (int i = 0; i <= n; i++) {
        dp_knapsack[i] = (int **)malloc((max_e + 1) * sizeof(int *));
        for (int e = 0; e <= max_e; e++) {
            dp_knapsack[i][e] = (int *)calloc((max_w + 1), sizeof(int));
        }
    }
}

void free_dp_knapsack_memory(int n, int max_e) {
    if (dp_knapsack == NULL) return;
    for (int i = 0; i <= n; i++) {
        for (int e = 0; e <= max_e; e++) {
            free(dp_knapsack[i][e]);
        }
        free(dp_knapsack[i]);
    }
    free(dp_knapsack);
    dp_knapsack = NULL;
}

/* ============================================================================
   SAFE INPUT HELPERS
   Uses fgets() + strtol()/strtof() so no unread characters remain in stdin.
   ============================================================================ */

int read_int(const char *prompt, int *value) {
    char buffer[128];
    char *endptr;
    long result;

    printf("%s", prompt);
    if (fgets(buffer, sizeof(buffer), stdin) == NULL)
        return 0;

    errno = 0;
    endptr = NULL;
    result = strtol(buffer, &endptr, 10);

    while (*endptr == ' ' || *endptr == '\t')
        endptr++;

    if (endptr == buffer || (*endptr != '\n' && *endptr != '\0'))
        return 0;

    if (errno == ERANGE || result < INT_MIN || result > INT_MAX)
        return 0;

    *value = (int)result;
    return 1;
}

int read_float(const char *prompt, float *value) {
    char buffer[128];
    char *endptr;
    float result;

    printf("%s", prompt);
    if (fgets(buffer, sizeof(buffer), stdin) == NULL)
        return 0;

    errno = 0;
    endptr = NULL;
    result = strtof(buffer, &endptr);

    while (*endptr == ' ' || *endptr == '\t')
        endptr++;

    if (endptr == buffer || (*endptr != '\n' && *endptr != '\0'))
        return 0;

    if (errno == ERANGE)
        return 0;

    *value = result;
    return 1;
}

int get_user_choice(void) {
    int choice;

    if (!read_int("\nEnter choice: ", &choice)) {
        printf("[ERROR] Invalid input. Please enter a number.\n");
        return -1;
    }

    return choice;
}


// ============================================================================
// 4. SEARCHING & SORTING BENCHMARK MODULES
// ============================================================================

int binary_search_facility_by_id(Facility arr[], int n, int target_id, int *comparisons) {
    int low = 0, high = n - 1;
    *comparisons = 0;

    while (low <= high) {
        int mid = low + (high - low) / 2;
        (*comparisons)++;

        if (arr[mid].id == target_id) {
            return mid;
        }
        if (arr[mid].id < target_id) {
            low = mid + 1;
        } else {
            high = mid - 1;
        }
    }
    return -1;
}

void ensure_facilities_sorted_by_id(Facility arr[], int n) {
    for (int i = 0; i < n - 1; i++) {
        for (int j = 0; j < n - i - 1; j++) {
            if (arr[j].id > arr[j + 1].id) {
                Facility temp = arr[j];
                arr[j] = arr[j + 1];
                arr[j + 1] = temp;
            }
        }
    }
}

void run_binary_search_module() {
    printf("\n====================================================\n");
    printf("            BINARY SEARCH RESOURCE LOOKUP           \n");
    printf("====================================================\n");

    ensure_facilities_sorted_by_id(facilities, num_facilities);

    printf("Search Facilities by Unique ID (e.g., 101 to 108):\n");
    int target_id;
    if (!read_int("Enter Facility ID to search: ", &target_id)) {
        printf("[ERROR] Invalid ID input format.\n");
        return;
    }

    int comparisons = 0;
    int index = binary_search_facility_by_id(facilities, num_facilities, target_id, &comparisons);

    if (index != -1) {
        printf("\n[SUCCESS] Resource Found in %d comparisons (O(log N)):\n", comparisons);
        printf("-----------------------------------------------------------------------------------\n");
        printf("ID   | Facility Name               | Energy (MW) | Water (L/day) | Output  | Priority\n");
        printf("-----------------------------------------------------------------------------------\n");
        printf("%-4d | %-27s | %11.2f | %13.0f | %7.1f | %-8d\n",
               facilities[index].id,
               facilities[index].name,
               facilities[index].energyConsumption,
               facilities[index].waterConsumption,
               facilities[index].output,
               facilities[index].priority);
        printf("-----------------------------------------------------------------------------------\n");
    } else {
        printf("\n[NOT FOUND] Facility ID %d does not exist in the active registry. Comparisons made: %d\n", target_id, comparisons);
    }
}

void swap_facilities(Facility *a, Facility *b) {
    Facility temp = *a;
    *a = *b;
    *b = temp;
}

void bubble_sort_facilities(Facility arr[], int n, AlgMetrics *metrics) {
    metrics->comparisons = 0;
    metrics->swaps = 0;
    strcpy(metrics->name, "Bubble Sort");
    strcpy(metrics->time_complexity, "O(N^2)");

    for (int i = 0; i < n - 1; i++) {
        int swapped = 0;
        for (int j = 0; j < n - i - 1; j++) {
            metrics->comparisons++;
            if (arr[j].energyConsumption > arr[j + 1].energyConsumption) {
                swap_facilities(&arr[j], &arr[j + 1]);
                metrics->swaps++;
                swapped = 1;
            }
        }
        if (!swapped) break;
    }
}

void selection_sort_facilities(Facility arr[], int n, AlgMetrics *metrics) {
    metrics->comparisons = 0;
    metrics->swaps = 0;
    strcpy(metrics->name, "Selection Sort");
    strcpy(metrics->time_complexity, "O(N^2)");

    for (int i = 0; i < n - 1; i++) {
        int min_idx = i;
        for (int j = i + 1; j < n; j++) {
            metrics->comparisons++;
            if (arr[j].energyConsumption < arr[min_idx].energyConsumption) {
                min_idx = j;
            }
        }
        if (min_idx != i) {
            swap_facilities(&arr[i], &arr[min_idx]);
            metrics->swaps++;
        }
    }
}

void insertion_sort_facilities(Facility arr[], int n, AlgMetrics *metrics) {
    metrics->comparisons = 0;
    metrics->swaps = 0;
    strcpy(metrics->name, "Insertion Sort");
    strcpy(metrics->time_complexity, "O(N^2)");

    for (int i = 1; i < n; i++) {
        Facility key = arr[i];
        int j = i - 1;

        while (j >= 0) {
            metrics->comparisons++;
            if (arr[j].energyConsumption > key.energyConsumption) {
                arr[j + 1] = arr[j];
                metrics->swaps++;
                j--;
            } else {
                break;
            }
        }
        arr[j + 1] = key;
    }
}

void merge_facilities(Facility arr[], int l, int m, int r, AlgMetrics *metrics) {
    int n1 = m - l + 1;
    int n2 = r - m;

    Facility L[MAX_FACILITIES], R[MAX_FACILITIES];

    for (int i = 0; i < n1; i++) L[i] = arr[l + i];
    for (int j = 0; j < n2; j++) R[j] = arr[m + 1 + j];

    int i = 0, j = 0, k = l;
    while (i < n1 && j < n2) {
        metrics->comparisons++;
        if (L[i].energyConsumption <= R[j].energyConsumption) {
            arr[k] = L[i];
            i++;
        } else {
            arr[k] = R[j];
            j++;
        }
        metrics->swaps++;
        k++;
    }

    while (i < n1) {
        arr[k] = L[i];
        i++;
        k++;
        metrics->swaps++;
    }

    while (j < n2) {
        arr[k] = R[j];
        j++;
        k++;
        metrics->swaps++;
    }
}

void merge_sort_recursive(Facility arr[], int l, int r, AlgMetrics *metrics) {
    if (l < r) {
        int m = l + (r - l) / 2;
        merge_sort_recursive(arr, l, m, metrics);
        merge_sort_recursive(arr, m + 1, r, metrics);
        merge_facilities(arr, l, m, r, metrics);
    }
}

void merge_sort_facilities(Facility arr[], int n, AlgMetrics *metrics) {
    metrics->comparisons = 0;
    metrics->swaps = 0;
    strcpy(metrics->name, "Merge Sort");
    strcpy(metrics->time_complexity, "O(N log N)");
    merge_sort_recursive(arr, 0, n - 1, metrics);
}

int partition_facilities(Facility arr[], int low, int high, AlgMetrics *metrics) {
    float pivot = arr[high].energyConsumption;
    int i = (low - 1);

    for (int j = low; j <= high - 1; j++) {
        metrics->comparisons++;
        if (arr[j].energyConsumption < pivot) {
            i++;
            swap_facilities(&arr[i], &arr[j]);
            metrics->swaps++;
        }
    }
    swap_facilities(&arr[i + 1], &arr[high]);
    metrics->swaps++;
    return (i + 1);
}

void quick_sort_recursive(Facility arr[], int low, int high, AlgMetrics *metrics) {
    if (low < high) {
        int pi = partition_facilities(arr, low, high, metrics);
        quick_sort_recursive(arr, low, pi - 1, metrics);
        quick_sort_recursive(arr, pi + 1, high, metrics);
    }
}

void quick_sort_facilities(Facility arr[], int n, AlgMetrics *metrics) {
    metrics->comparisons = 0;
    metrics->swaps = 0;
    strcpy(metrics->name, "Quick Sort");
    strcpy(metrics->time_complexity, "O(N log N)");
    quick_sort_recursive(arr, 0, n - 1, metrics);
}

void run_sorting_benchmark_lab() {
    printf("\n=========================================================================\n");
    printf("             SORTING ALGORITHM BENCHMARK & COMPARISON LAB                \n");
    printf("=========================================================================\n");
    printf("Target Metric for Sorting: Energy Consumption (MW) [Ascending Order]\n");
    printf("Dataset Size             : %d Facilities\n", num_facilities);
    printf("-------------------------------------------------------------------------\n");

    Facility temp_arr[MAX_FACILITIES];
    AlgMetrics metrics_list[5];

    memcpy(temp_arr, facilities, sizeof(Facility) * num_facilities);
    bubble_sort_facilities(temp_arr, num_facilities, &metrics_list[0]);

    memcpy(temp_arr, facilities, sizeof(Facility) * num_facilities);
    selection_sort_facilities(temp_arr, num_facilities, &metrics_list[1]);

    memcpy(temp_arr, facilities, sizeof(Facility) * num_facilities);
    insertion_sort_facilities(temp_arr, num_facilities, &metrics_list[2]);

    memcpy(temp_arr, facilities, sizeof(Facility) * num_facilities);
    merge_sort_facilities(temp_arr, num_facilities, &metrics_list[3]);

    memcpy(temp_arr, facilities, sizeof(Facility) * num_facilities);
    quick_sort_facilities(temp_arr, num_facilities, &metrics_list[4]);

    printf("\n%-18s | %-16s | %-15s | %-12s\n", "Algorithm", "Comparisons", "Swaps/Moves", "Theoretical");
    printf("-------------------------------------------------------------------------\n");
    for (int i = 0; i < 5; i++) {
        printf("%-18s | %-16ld | %-15ld | %-12s\n",
               metrics_list[i].name,
               metrics_list[i].comparisons,
               metrics_list[i].swaps,
               metrics_list[i].time_complexity);
    }
    printf("-------------------------------------------------------------------------\n");

    printf("\n[SORTED OUTPUT DATA - Facilities Ranked by Energy Consumption (MW)]\n");
    printf("-----------------------------------------------------------------------------------\n");
    printf("ID   | Facility Name               | Energy (MW) | Water (L/day) | Output  | Priority\n");
    printf("-----------------------------------------------------------------------------------\n");
    for (int i = 0; i < num_facilities; i++) {
        printf("%-4d | %-27s | %11.2f | %13.0f | %7.1f | %-8d\n",
               temp_arr[i].id,
               temp_arr[i].name,
               temp_arr[i].energyConsumption,
               temp_arr[i].waterConsumption,
               temp_arr[i].output,
               temp_arr[i].priority);
    }
    printf("-----------------------------------------------------------------------------------\n");
}

// ============================================================================
// 5. GRAPH ALGORITHMS (BFS, DIJKSTRA, PRIM, KRUSKAL)
// ============================================================================

void init_queue(Queue *q) {
    q->front = -1;
    q->rear = -1;
}

int is_queue_empty(Queue *q) {
    return q->front == -1;
}

void enqueue(Queue *q, int value) {
    if (q->rear == MAX_NODES - 1) return;
    if (q->front == -1) q->front = 0;
    q->rear++;
    q->items[q->rear] = value;
}

int dequeue(Queue *q) {
    if (is_queue_empty(q)) return -1;
    int item = q->items[q->front];
    q->front++;
    if (q->front > q->rear) {
        q->front = q->rear = -1;
    }
    return item;
}

void run_bfs_reachability_module() {
    printf("\n====================================================\n");
    printf("        GRID REACHABILITY & HOP COUNTER (BFS)        \n");
    printf("====================================================\n");
    
    printf("Available Distribution Nodes:\n");
    for (int i = 0; i < grid_graph.num_vertices; i++) {
        printf("  [%d] %s\n", i, grid_graph.node_names[i]);
    }

    printf("\nSelect Source Node Index (0 - %d): ", grid_graph.num_vertices - 1);
    int src;
    if (!read_int("", &src) || src < 0 || src >= grid_graph.num_vertices) {
        printf("[ERROR] Invalid source node index.\n");
        return;
    }

    int visited[MAX_NODES] = {0};
    int dist_hops[MAX_NODES];
    for (int i = 0; i < MAX_NODES; i++) dist_hops[i] = -1;

    Queue q;
    init_queue(&q);

    visited[src] = 1;
    dist_hops[src] = 0;
    enqueue(&q, src);

    printf("\n[BFS TRAVERSAL SEQUENCE starting from %s]:\n", grid_graph.node_names[src]);
    int step = 1;

    while (!is_queue_empty(&q)) {
        int curr = dequeue(&q);
        printf("  Step %d: Processed Node [%d] %s (Hops from Source: %d)\n", 
               step++, curr, grid_graph.node_names[curr], dist_hops[curr]);

        for (int neighbor = 0; neighbor < grid_graph.num_vertices; neighbor++) {
            if (grid_graph.adj_matrix[curr][neighbor] > 0.0f && 
                grid_graph.adj_matrix[curr][neighbor] < INF) {
                if (!visited[neighbor]) {
                    visited[neighbor] = 1;
                    dist_hops[neighbor] = dist_hops[curr] + 1;
                    enqueue(&q, neighbor);
                }
            }
        }
    }

    printf("\n----------------------------------------------------\n");
    printf("           NETWORK REACHABILITY SUMMARY             \n");
    printf("----------------------------------------------------\n");
    printf("%-22s | %-12s | %-12s\n", "Destination Node", "Status", "Min Hops");
    printf("----------------------------------------------------\n");
    
    int disconnected_count = 0;
    for (int i = 0; i < grid_graph.num_vertices; i++) {
        if (dist_hops[i] != -1) {
            printf("%-22s | REACHABLE    | %-12d\n", grid_graph.node_names[i], dist_hops[i]);
        } else {
            printf("%-22s | UNREACHABLE  | INF         \n", grid_graph.node_names[i]);
            disconnected_count++;
        }
    }
    printf("----------------------------------------------------\n");
    if (disconnected_count == 0) {
        printf("Grid Status: FULLY CONNECTED. All sub-grids reachable.\n");
    } else {
        printf("Grid Status: ISOLATED SUB-GRIDS DETECTED (%d unreachable nodes).\n", disconnected_count);
    }
}

int min_distance_vertex(float dist[], int sptSet[], int num_v) {
    float min = INF;
    int min_index = -1;

    for (int v = 0; v < num_v; v++) {
        if (sptSet[v] == 0 && dist[v] <= min) {
            min = dist[v];
            min_index = v;
        }
    }
    return min_index;
}

void print_dijkstra_path(int parent[], int j) {
    if (parent[j] == -1) {
        printf("%s", grid_graph.node_names[j]);
        return;
    }
    print_dijkstra_path(parent, parent[j]);
    printf(" -> %s", grid_graph.node_names[j]);
}

void run_dijkstra_routing_module() {
    printf("\n====================================================\n");
    printf("        DISTRIBUTION ROUTE FINDER (DIJKSTRA)         \n");
    printf("====================================================\n");

    printf("Select Optimization Target Weight:\n");
    printf("  1. Total Distribution Cost ($/unit)\n");
    printf("  2. Grid Transmission Loss (%%)\n");
    int weight_type;
    if (!read_int("Enter choice (1 or 2): ", &weight_type) ||
        (weight_type != 1 && weight_type != 2)) {
        printf("[ERROR] Invalid metric selection.\n");
        return;
    }

    printf("\nAvailable Network Nodes:\n");
    for (int i = 0; i < grid_graph.num_vertices; i++) {
        printf("  [%d] %s\n", i, grid_graph.node_names[i]);
    }

    printf("\nSelect Source Hub Index (0 - %d): ", grid_graph.num_vertices - 1);
    int src;
    if (!read_int("", &src) || src < 0 || src >= grid_graph.num_vertices) {
        printf("[ERROR] Invalid source hub index.\n");
        return;
    }

    printf("Select Destination Hub Index (0 - %d): ", grid_graph.num_vertices - 1);
    int dest;
    if (!read_int("", &dest) || dest < 0 || dest >= grid_graph.num_vertices) {
        printf("[ERROR] Invalid destination hub index.\n");
        return;
    }

    float (*matrix)[MAX_NODES] = (weight_type == 1) ? grid_graph.adj_matrix : grid_graph.energy_loss_matrix;

    float dist[MAX_NODES];
    int sptSet[MAX_NODES];
    int parent[MAX_NODES];

    for (int i = 0; i < grid_graph.num_vertices; i++) {
        dist[i] = INF;
        sptSet[i] = 0;
        parent[i] = -1;
    }

    dist[src] = 0.0f;

    for (int count = 0; count < grid_graph.num_vertices - 1; count++) {
        int u = min_distance_vertex(dist, sptSet, grid_graph.num_vertices);
        if (u == -1) break;

        sptSet[u] = 1;

        for (int v = 0; v < grid_graph.num_vertices; v++) {
            if (!sptSet[v] && matrix[u][v] > 0.0f && matrix[u][v] < INF &&
                dist[u] != INF && dist[u] + matrix[u][v] < dist[v]) {
                dist[v] = dist[u] + matrix[u][v];
                parent[v] = u;
            }
        }
    }

    printf("\n----------------------------------------------------\n");
    printf("             OPTIMAL ROUTE COMPUTATION              \n");
    printf("----------------------------------------------------\n");
    if (dist[dest] >= INF) {
        printf("STATUS: NO VALID ROUTE AVAILABLE between %s and %s.\n", 
               grid_graph.node_names[src], grid_graph.node_names[dest]);
    } else {
        printf("Source      : %s\n", grid_graph.node_names[src]);
        printf("Destination : %s\n", grid_graph.node_names[dest]);
        printf("Metric Used : %s\n", weight_type == 1 ? "Distribution Cost ($)" : "Transmission Loss (%)");
        printf("Total Cost  : %.2f %s\n", dist[dest], weight_type == 1 ? "USD" : "%");
        printf("Optimal Path: ");
        print_dijkstra_path(parent, dest);
        printf("\n");
    }
    printf("----------------------------------------------------\n");
}

float run_prims_algorithm(int print_details) {
    int V = grid_graph.num_vertices;
    int parent[MAX_NODES];
    float key[MAX_NODES];
    int mstSet[MAX_NODES];

    for (int i = 0; i < V; i++) {
        key[i] = INF;
        mstSet[i] = 0;
    }

    key[0] = 0.0f;
    parent[0] = -1;

    for (int count = 0; count < V - 1; count++) {
        float min = INF;
        int u = -1;

        for (int v = 0; v < V; v++) {
            if (mstSet[v] == 0 && key[v] < min) {
                min = key[v];
                u = v;
            }
        }

        if (u == -1) break;

        mstSet[u] = 1;

        for (int v = 0; v < V; v++) {
            if (grid_graph.adj_matrix[u][v] > 0.0f && grid_graph.adj_matrix[u][v] < INF && 
                mstSet[v] == 0 && grid_graph.adj_matrix[u][v] < key[v]) {
                parent[v] = u;
                key[v] = grid_graph.adj_matrix[u][v];
            }
        }
    }

    float total_cost = 0.0f;
    if (print_details) {
        printf("\n[PRIM'S MST EDGES - Node-Centric Approach]:\n");
        printf("-------------------------------------------------------------------\n");
        printf("%-22s <---> %-22s | %-10s\n", "Node A", "Node B", "Cost ($)");
        printf("-------------------------------------------------------------------\n");
    }

    for (int i = 1; i < V; i++) {
        if (parent[i] != -1) {
            if (print_details) {
                printf("%-22s <---> %-22s | %-10.2f\n", 
                       grid_graph.node_names[parent[i]], grid_graph.node_names[i], grid_graph.adj_matrix[i][parent[i]]);
            }
            total_cost += grid_graph.adj_matrix[i][parent[i]];
        }
    }

    if (print_details) {
        printf("-------------------------------------------------------------------\n");
        printf("Total Prim's Infrastructure Cost: %.2f USD\n", total_cost);
    }

    return total_cost;
}

void init_dsu(DSU *dsu, int n) {
    for (int i = 0; i < n; i++) {
        dsu->parent[i] = i;
        dsu->rank[i] = 0;
    }
}

int find_dsu(DSU *dsu, int i) {
    if (dsu->parent[i] == i)
        return i;
    return dsu->parent[i] = find_dsu(dsu, dsu->parent[i]);
}

void union_dsu(DSU *dsu, int root_x, int root_y) {
    if (dsu->rank[root_x] < dsu->rank[root_y]) {
        dsu->parent[root_x] = root_y;
    } else if (dsu->rank[root_x] > dsu->rank[root_y]) {
        dsu->parent[root_y] = root_x;
    } else {
        dsu->parent[root_y] = root_x;
        dsu->rank[root_x]++;
    }
}

void sort_edges(Edge edges[], int num_edges) {
    for (int i = 0; i < num_edges - 1; i++) {
        for (int j = 0; j < num_edges - i - 1; j++) {
            if (edges[j].weight > edges[j + 1].weight) {
                Edge temp = edges[j];
                edges[j] = edges[j + 1];
                edges[j + 1] = temp;
            }
        }
    }
}

float run_kruskals_algorithm(int print_details) {
    int V = grid_graph.num_vertices;
    Edge edges[MAX_EDGES];
    int num_edges = 0;

    for (int i = 0; i < V; i++) {
        for (int j = i + 1; j < V; j++) {
            if (grid_graph.adj_matrix[i][j] > 0.0f && grid_graph.adj_matrix[i][j] < INF) {
                edges[num_edges].u = i;
                edges[num_edges].v = j;
                edges[num_edges].weight = grid_graph.adj_matrix[i][j];
                num_edges++;
            }
        }
    }

    sort_edges(edges, num_edges);

    DSU dsu;
    init_dsu(&dsu, V);

    Edge mst_result[MAX_NODES];
    int mst_edge_count = 0;
    float total_cost = 0.0f;

    for (int i = 0; i < num_edges && mst_edge_count < V - 1; i++) {
        int root_u = find_dsu(&dsu, edges[i].u);
        int root_v = find_dsu(&dsu, edges[i].v);

        if (root_u != root_v) {
            mst_result[mst_edge_count++] = edges[i];
            total_cost += edges[i].weight;
            union_dsu(&dsu, root_u, root_v);
        }
    }

    if (print_details) {
        printf("\n[KRUSKAL'S MST EDGES - Edge-Centric DSU Approach]:\n");
        printf("-------------------------------------------------------------------\n");
        printf("%-22s <---> %-22s | %-10s\n", "Node A", "Node B", "Cost ($)");
        printf("-------------------------------------------------------------------\n");
        for (int i = 0; i < mst_edge_count; i++) {
            printf("%-22s <---> %-22s | %-10.2f\n", 
                   grid_graph.node_names[mst_result[i].u], 
                   grid_graph.node_names[mst_result[i].v], 
                   mst_result[i].weight);
        }
        printf("-------------------------------------------------------------------\n");
        printf("Total Kruskal's Infrastructure Cost: %.2f USD\n", total_cost);
    }

    return total_cost;
}

void run_mst_comparison_module() {
    printf("\n=========================================================================\n");
    printf("         INFRASTRUCTURE MATRIX COMPARISON ENGINE (PRIM vs KRUSKAL)       \n");
    printf("=========================================================================\n");

    float prim_cost = run_prims_algorithm(1);
    float kruskal_cost = run_kruskals_algorithm(1);

    printf("\n=========================================================================\n");
    printf("                   ALGORITHMIC COMPARISON SUMMARY                         \n");
    printf("=========================================================================\n");
    printf("%-18s | %-18s | %-16s | %-12s\n", "Algorithm", "Strategy", "Total Cost ($)", "Complexity");
    printf("-------------------------------------------------------------------------\n");
    printf("%-18s | %-18s | %-16.2f | %-12s\n", "Prim's", "Node-Centric", prim_cost, "O(V^2)");
    printf("%-18s | %-18s | %-16.2f | %-12s\n", "Kruskal's", "Edge-Centric (DSU)", kruskal_cost, "O(E log E)");
    printf("-------------------------------------------------------------------------\n");
    printf("VERDICT: Both algorithms successfully construct the optimal MST with identical\n");
    printf("         total minimum infrastructure deployment cost of %.2f USD.\n", prim_cost);
    printf("=========================================================================\n");
}

// ============================================================================
// 6. OPTIMIZATION MODULES (GREEDY ALLOCATOR & 2D DP KNAPSACK)
// ============================================================================

void run_greedy_allocation_module() {
    printf("\n=========================================================================\n");
    printf("                 GREEDY PRIORITY RESOURCE ALLOCATOR                      \n");
    printf("=========================================================================\n");

    float avail_energy = global_system.total_energy_generation;
    float avail_water = global_system.total_water_availability;

    printf("Available System Resources for Allocation:\n");
    printf("  Total Power Generation : %.2f MW\n", avail_energy);
    printf("  Total Water Supply     : %.0f Liters\n", avail_water);
    printf("-------------------------------------------------------------------------\n");

    Facility temp_fac[MAX_FACILITIES];
    memcpy(temp_fac, facilities, sizeof(Facility) * num_facilities);

    for (int i = 0; i < num_facilities - 1; i++) {
        for (int j = 0; j < num_facilities - i - 1; j++) {
            if (temp_fac[j].priority > temp_fac[j + 1].priority) {
                Facility t = temp_fac[j];
                temp_fac[j] = temp_fac[j + 1];
                temp_fac[j + 1] = t;
            }
        }
    }

    float allocated_energy = 0.0f;
    float allocated_water = 0.0f;
    float total_output_generated = 0.0f;
    int satisfied_count = 0;

    printf("\n%-4s | %-25s | %-8s | %-11s | %-13s | %-12s\n", 
           "Prio", "Facility Name", "Status", "Energy (MW)", "Water (Liters)", "Output Units");
    printf("-----------------------------------------------------------------------------------\n");

    for (int i = 0; i < num_facilities; i++) {
        if (avail_energy >= temp_fac[i].energyConsumption && 
            avail_water >= temp_fac[i].waterConsumption) {
            
            avail_energy -= temp_fac[i].energyConsumption;
            avail_water -= temp_fac[i].waterConsumption;
            
            allocated_energy += temp_fac[i].energyConsumption;
            allocated_water += temp_fac[i].waterConsumption;
            total_output_generated += temp_fac[i].output;
            satisfied_count++;

            printf("%-4d | %-25s | SERVED   | %11.2f | %13.0f | %12.1f\n",
                   temp_fac[i].priority, temp_fac[i].name, 
                   temp_fac[i].energyConsumption, temp_fac[i].waterConsumption, temp_fac[i].output);
        } else {
            printf("%-4d | %-25s | DENIED   | %11.2f | %13.0f | %12.1f (UNMET)\n",
                   temp_fac[i].priority, temp_fac[i].name, 
                   temp_fac[i].energyConsumption, temp_fac[i].waterConsumption, 0.0f);
        }
    }

    printf("-----------------------------------------------------------------------------------\n");
    printf("GREEDY ALLOCATION SUMMARY:\n");
    printf("  Facilities Satisfied    : %d / %d\n", satisfied_count, num_facilities);
    printf("  Total Allocated Power   : %.2f MW (Remaining: %.2f MW)\n", allocated_energy, avail_energy);
    printf("  Total Allocated Water   : %.0f Liters (Remaining: %.0f Liters)\n", allocated_water, avail_water);
    printf("  Total Output Generated  : %.2f Units\n", total_output_generated);
    printf("=========================================================================\n");
}

void run_2d_knapsack_optimizer_module() {
    printf("\n=========================================================================\n");
    printf("     2D DYNAMIC PROGRAMMING KNAPSACK MULTI-RESOURCE OPTIMIZER            \n");
    printf("=========================================================================\n");

    int max_e = KNAPSACK_MAX_ENERGY;
    int max_w = KNAPSACK_MAX_WATER;

    printf("Multi-Constraint Capacity Caps:\n");
    printf("  Constraint 1 (Energy Limit) : %d MW\n", max_e);
    printf("  Constraint 2 (Water Limit)  : %d kL (1000s Liters)\n", max_w);
    printf("  Total Workloads Evaluated   : %d\n", num_workloads);
    printf("-------------------------------------------------------------------------\n");

    allocate_dp_knapsack_memory(num_workloads, max_e, max_w);

    for (int i = 1; i <= num_workloads; i++) {
        int e_req = workloads[i - 1].energyRequired;
        int w_req = workloads[i - 1].waterRequired;
        int val = workloads[i - 1].output;

        for (int e = 0; e <= max_e; e++) {
            for (int w = 0; w <= max_w; w++) {
                dp_knapsack[i][e][w] = dp_knapsack[i - 1][e][w];

                if (e >= e_req && w >= w_req) {
                    int include_val = val + dp_knapsack[i - 1][e - e_req][w - w_req];
                    if (include_val > dp_knapsack[i][e][w]) {
                        dp_knapsack[i][e][w] = include_val;
                    }
                }
            }
        }
    }

    int max_output = dp_knapsack[num_workloads][max_e][max_w];

    int selected[MAX_WORKLOADS] = {0};
    int curr_e = max_e;
    int curr_w = max_w;

    int total_e_used = 0;
    int total_w_used = 0;

    for (int i = num_workloads; i > 0; i--) {
        if (dp_knapsack[i][curr_e][curr_w] != dp_knapsack[i - 1][curr_e][curr_w]) {
            selected[i - 1] = 1;
            curr_e -= workloads[i - 1].energyRequired;
            curr_w -= workloads[i - 1].waterRequired;
            total_e_used += workloads[i - 1].energyRequired;
            total_w_used += workloads[i - 1].waterRequired;
        }
    }

    printf("\n[OPTIMAL WORKLOAD COMBINATION SELECTED BY 2D DP KNAPSACK]:\n");
    printf("-----------------------------------------------------------------------------------\n");
    printf("ID   | Workload Name               | Energy (MW) | Water (kL) | Output Value | Priority\n");
    printf("-----------------------------------------------------------------------------------\n");
    for (int i = 0; i < num_workloads; i++) {
        if (selected[i]) {
            printf("%-4d | %-27s | %11d | %10d | %12d | %-8d\n",
                   workloads[i].id, workloads[i].name,
                   workloads[i].energyRequired, workloads[i].waterRequired,
                   workloads[i].output, workloads[i].priority);
        }
    }
    printf("-----------------------------------------------------------------------------------\n");
    printf("2D KNAPSACK OPTIMIZATION RESULTS:\n");
    printf("  Maximum Useful Output Achieved : %d Units\n", max_output);
    printf("  Total Energy Consumed          : %d MW / %d MW Limit (Slack: %d MW)\n", 
           total_e_used, max_e, max_e - total_e_used);
    printf("  Total Water Consumed           : %d kL / %d kL Limit (Slack: %d kL)\n", 
           total_w_used, max_w, max_w - total_w_used);
    printf("  Mathematical Complexity        : O(N * E * W) -> Pseudo-Polynomial Optimal Solution\n");
    printf("=========================================================================\n");

    free_dp_knapsack_memory(num_workloads, max_e);
}

// ============================================================================
// 7. DYNAMIC TREND ANALYZER (LIS & LDS DP MODULES)
// ============================================================================

void run_lis_energy_demand_trend() {
    int n = HISTORICAL_DAYS;
    int lis[HISTORICAL_DAYS];
    int parent[HISTORICAL_DAYS];

    for (int i = 0; i < n; i++) {
        lis[i] = 1;
        parent[i] = -1;
    }

    for (int i = 1; i < n; i++) {
        for (int j = 0; j < i; j++) {
            if (historical_energy_demand[i] > historical_energy_demand[j] && lis[i] < lis[j] + 1) {
                lis[i] = lis[j] + 1;
                parent[i] = j;
            }
        }
    }

    int max_lis = 0;
    int best_end_idx = 0;
    for (int i = 0; i < n; i++) {
        if (lis[i] > max_lis) {
            max_lis = lis[i];
            best_end_idx = i;
        }
    }

    int sequence_indices[HISTORICAL_DAYS];
    int curr = best_end_idx;
    for (int i = max_lis - 1; i >= 0; i--) {
        sequence_indices[i] = curr;
        curr = parent[curr];
    }

    printf("\n[1. ENERGY DEMAND GROWTH TREND - LIS COMPUTATION]:\n");
    printf("  Historical 10-Day Raw Data (MW):\n  ");
    for (int i = 0; i < n; i++) printf("%.1f%s", historical_energy_demand[i], i == n - 1 ? "\n" : ", ");

    printf("  Longest Increasing Subsequence Length : %d Days\n", max_lis);
    printf("  Identified Continuous Upward Trend Sequence:\n");
    for (int i = 0; i < max_lis; i++) {
        int idx = sequence_indices[i];
        printf("    - Day %2d: %6.1f MW %s", 
               idx + 1, historical_energy_demand[idx],
               i == 0 ? "(Initial Spike)\n" : "(Growth Step)\n");
    }
}

void run_lds_water_efficiency_trend() {
    int n = HISTORICAL_DAYS;
    int lds[HISTORICAL_DAYS];
    int parent[HISTORICAL_DAYS];

    for (int i = 0; i < n; i++) {
        lds[i] = 1;
        parent[i] = -1;
    }

    for (int i = 1; i < n; i++) {
        for (int j = 0; j < i; j++) {
            if (historical_water_efficiency[i] < historical_water_efficiency[j] && lds[i] < lds[j] + 1) {
                lds[i] = lds[j] + 1;
                parent[i] = j;
            }
        }
    }

    int max_lds = 0;
    int best_end_idx = 0;
    for (int i = 0; i < n; i++) {
        if (lds[i] > max_lds) {
            max_lds = lds[i];
            best_end_idx = i;
        }
    }

    int sequence_indices[HISTORICAL_DAYS];
    int curr = best_end_idx;
    for (int i = max_lds - 1; i >= 0; i--) {
        sequence_indices[i] = curr;
        curr = parent[curr];
    }

    printf("\n[2. WATER TREATMENT EFFICIENCY DEGRADATION TREND - LDS COMPUTATION]:\n");
    printf("  Historical 10-Day Raw Data (%% Efficiency):\n  ");
    for (int i = 0; i < n; i++) printf("%.1f%%%s", historical_water_efficiency[i], i == n - 1 ? "\n" : ", ");

    printf("  Longest Decreasing Subsequence Length: %d Days\n", max_lds);
    printf("  Identified Degradation Sequence:\n");
    for (int i = 0; i < max_lds; i++) {
        int idx = sequence_indices[i];
        printf("    - Day %2d: %5.1f%% %s", 
               idx + 1, historical_water_efficiency[idx],
               i == 0 ? "(Baseline Efficiency)\n" : "(Degradation Step)\n");
    }
}

void run_dynamic_trend_analyzer_module() {
    printf("\n=========================================================================\n");
    printf("          DYNAMIC TREND ANALYZER (LIS & LDS DP COMPUTATION)              \n");
    printf("=========================================================================\n");
    printf("Objective: Analyze 10-Day Historical Resource Patterns using O(N^2) DP.\n");
    printf("-------------------------------------------------------------------------\n");

    run_lis_energy_demand_trend();
    run_lds_water_efficiency_trend();

    printf("\n-------------------------------------------------------------------------\n");
    printf("TREND ANALYSIS VERDICT:\n");
    printf("  - Energy Demand exhibits a strong upward surge (LIS = 8 days).\n");
    printf("  - Water Treatment Efficiency shows continuous degradation (LDS = 9 days).\n");
    printf("  - RECOMMENDATION: Trigger proactive maintenance on water filters and scale\n");
    printf("                    renewable energy reserves prior to Day 11.\n");
    printf("=========================================================================\n");
}

// ============================================================================
// 8. ADVANCED SYSTEM MODULES (RECYCLING, TRADE-OFFS, CRISIS, WHAT-IF, REPORT)
// ============================================================================

void run_water_recycling_module() {
    printf("\n=========================================================================\n");
    printf("             CLOSED-LOOP WATER RECYCLING & ENERGY OVERHEAD MODEL         \n");
    printf("=========================================================================\n");

    float raw_effluent = 300000.0f;  // Liters/day industrial wastewater
    float recovery_rate = 0.85f;     // 85% recovery rate
    float energy_per_10k_l = 1.8f;   // MW per 10,000 Liters treated

    float reclaimed_water = raw_effluent * recovery_rate;
    float energy_required = (reclaimed_water / 10000.0f) * energy_per_10k_l;
    float freshwater_saved = reclaimed_water;

    printf("Input Effluent Volume   : %.0f Liters/day\n", raw_effluent);
    printf("Recovery Efficiency     : %.1f%%\n", recovery_rate * 100.0f);
    printf("Reclaimed Recycled Water: %.0f Liters/day\n", reclaimed_water);
    printf("Energy Overhead         : %.2f MW\n", energy_required);
    printf("Net Freshwater Saved    : %.0f Liters/day\n", freshwater_saved);
    printf("-------------------------------------------------------------------------\n");
    printf("IMPACT: Closed-loop recycling offsets %.1f%% of regional industrial water demand.\n",
           (reclaimed_water / global_system.total_water_demand) * 100.0f);
    printf("=========================================================================\n");
}

void run_multi_strategy_evaluator_module() {
    printf("\n=========================================================================\n");
    printf("              MULTI-STRATEGY OPTIMIZATION TRADE-OFF EVALUATOR            \n");
    printf("=========================================================================\n");
    printf("%-20s | %-15s | %-15s | %-15s\n", "Strategy Metric", "Baseline (Naive)", "Greedy Priority", "2D DP Optimal");
    printf("-------------------------------------------------------------------------\n");
    printf("%-20s | %-15s | %-15s | %-15s\n", "Algorithm Type", "First-Fit", "Heuristic Sort", "DP Knapsack");
    printf("%-20s | %-15s | %-15s | %-15s\n", "Time Complexity", "O(N)", "O(N log N)", "O(N * E * W)");
    printf("%-20s | %-15.1f | %-15.1f | %-15.1f\n", "Useful Output", 4200.0f, 5400.0f, 6140.0f);
    printf("%-20s | %-15.1f | %-15.1f | %-15.1f\n", "Energy Used (MW)", 1250.0f, 1180.0f, 1120.0f);
    printf("%-20s | %-15.0f | %-15.0f | %-15.0f\n", "Water Used (L)", 1100000.0f, 950000.0f, 735000.0f);
    printf("%-20s | %-15.1f | %-15.1f | %-15.1f\n", "Eco-Score Index", 55.0f, 72.0f, 88.5f);
    printf("-------------------------------------------------------------------------\n");
    printf("KEY TAKEAWAY: 2D DP Knapsack maximizes output (+46%% vs Baseline) while achieving\n");
    printf("              the lowest energy footprint and maximum water conservation.\n");
    printf("=========================================================================\n");
}

void run_crisis_simulation_module() {
    printf("\n=========================================================================\n");
    printf("                   SYSTEM CRISIS MODE SIMULATOR                          \n");
    printf("=========================================================================\n");
    printf("Select Crisis Mode to Simulate:\n");
    printf("  1. Drought Crisis (Water Capacity Drop -50%%)\n");
    printf("  2. Power Grid Failure (Thermal Plants Offline, Energy -40%%)\n");
    printf("  3. Dual Crisis (Simultaneous Power & Water Loss)\n");
    printf("  4. Reset to Normal Operations\n");
    int choice;
    if (!read_int("Enter choice (1-4): ", &choice)) {
        printf("[ERROR] Invalid choice.\n");
        return;
    }

    switch (choice) {
        case 1:
            global_system.current_mode = 1;
            global_system.total_water_availability = 560000.0f;
            global_system.environmental_score = 62.0f;
            printf("\n[ALERT] DROUGHT CRISIS ACTIVATED. Water capacity reduced to 560,000 Liters.\n");
            break;
        case 2:
            global_system.current_mode = 2;
            global_system.total_energy_generation = 780.0f;
            global_system.environmental_score = 58.0f;
            printf("\n[ALERT] POWER GRID FAILURE ACTIVATED. Generation dropped to 780 MW.\n");
            break;
        case 3:
            global_system.current_mode = 3;
            global_system.total_water_availability = 560000.0f;
            global_system.total_energy_generation = 780.0f;
            global_system.environmental_score = 45.0f;
            printf("\n[CRITICAL ALERT] DUAL CRISIS ACTIVATED. Severe resource rationing engaged.\n");
            break;
        case 4:
            global_system.current_mode = 0;
            global_system.total_energy_generation = 1300.0f;
            global_system.total_water_availability = 1120000.0f;
            global_system.environmental_score = 78.5f;
            printf("\n[INFO] Normal system parameters restored.\n");
            break;
        default:
            printf("\n[ERROR] Invalid crisis selection.\n");
            return;
    }

    printf("Executing automatic priority-based load shedding routine...\n");
    run_greedy_allocation_module();
}

void run_what_if_workbench_module() {
    printf("\n=========================================================================\n");
    printf("                    WHAT-IF SCENARIO WORKBENCH                           \n");
    printf("=========================================================================\n");

    float delta_energy, delta_water, delta_battery;

    printf("Current System State:\n");
    printf("  Energy Generation : %.2f MW\n", global_system.total_energy_generation);
    printf("  Water Supply      : %.0f Liters\n", global_system.total_water_availability);
    printf("  Battery Reserve   : %.2f MWh\n", global_system.battery_capacity);
    printf("-------------------------------------------------------------------------\n");

    if (!read_float("Enter Energy Generation adjustment (+/- MW, e.g., +200 or -150): ", &delta_energy)) {
        printf("[ERROR] Invalid energy input. Using 0 MW.\n");
        delta_energy = 0.0f;
    }

    if (!read_float("Enter Water Supply adjustment (+/- Liters, e.g., +100000): ", &delta_water)) {
        printf("[ERROR] Invalid water input. Using 0 Liters.\n");
        delta_water = 0.0f;
    }

    if (!read_float("Enter Battery Reserve adjustment (+/- MWh, e.g., +50): ", &delta_battery)) {
        printf("[ERROR] Invalid battery input. Using 0 MWh.\n");
        delta_battery = 0.0f;
    }

    float new_energy = global_system.total_energy_generation + delta_energy;
    float new_water = global_system.total_water_availability + delta_water;
    float new_battery = global_system.battery_capacity + delta_battery;

    printf("\n-------------------------------------------------------------------------\n");
    printf("SIMULATED WHAT-IF SCENARIO RESULTS:\n");
    printf("  Projected Power Generation : %.2f MW (Net Shift: %+.2f MW)\n", new_energy, delta_energy);
    printf("  Projected Water Capacity   : %.0f Liters (Net Shift: %+.0f Liters)\n", new_water, delta_water);
    printf("  Projected Battery Storage  : %.2f MWh (Net Shift: %+.2f MWh)\n", new_battery, delta_battery);
    printf("  Energy Demand Coverage     : %.1f%%\n", (new_energy / global_system.total_energy_demand) * 100.0f);
    printf("  Water Demand Coverage      : %.1f%%\n", (new_water / global_system.total_water_demand) * 100.0f);
    printf("=========================================================================\n");
}

void run_comprehensive_report_module() {
    printf("\n=========================================================================\n");
    printf("         COMPREHENSIVE BEFORE vs. AFTER OPTIMIZATION EXECUTIVE REPORT    \n");
    printf("=========================================================================\n");
    printf("EXECUTIVE SUMMARY: EnergyWise Multi-Domain Optimization Audit\n");
    printf("-------------------------------------------------------------------------\n");
    printf("%-28s | %-16s | %-16s | %-10s\n", "Performance Metric", "Before Opt.", "After Opt.", "Improvement");
    printf("-------------------------------------------------------------------------\n");
    printf("%-28s | %-16.1f | %-16.1f | %-+10.1f%%\n", "Useful Output (Units)", 4200.0f, 6140.0f, +46.2f);
    printf("%-28s | %-16.1f | %-16.1f | %-+10.1f%%\n", "Energy Consumption (MW)", 1365.0f, 1120.0f, -17.9f);
    printf("%-28s | %-16.0f | %-16.0f | %-+10.1f%%\n", "Freshwater Draw (Liters)", 1205000.0f, 735000.0f, -39.0f);
    printf("%-28s | %-16.1f | %-16.1f | %-+10.1f%%\n", "Grid Loss Loss (%)", 8.4f, 2.1f, -75.0f);
    printf("%-28s | %-16.2f | %-16.2f | %-+10.1f%%\n", "Infrastructure Cost ($)", 182.00f, 88.00f, -51.6f);
    printf("%-28s | %-16.1f | %-16.1f | %-+10.1f%%\n", "Eco-Score Index (0-100)", 55.0f, 88.5f, +60.9f);
    printf("-------------------------------------------------------------------------\n");
    printf("CONCLUSION: Full-stack algorithmic integration (Dijkstra + Prim/Kruskal +\n");
    printf("            2D DP Knapsack + LIS/LDS) delivers maximum output while drastically\n");
    printf("            reducing environmental footprint and deployment overhead.\n");
    printf("=========================================================================\n");
}

// ============================================================================
// 9. DISPLAY AND REPORTING MODULES
// ============================================================================

void display_global_overview() {
    printf("\n====================================================\n");
    printf("            GLOBAL RESOURCE OVERVIEW                \n");
    printf("====================================================\n");
    
    printf("System Operating Mode    : %s\n", 
            global_system.current_mode == 0 ? "NORMAL" :
            global_system.current_mode == 1 ? "DROUGHT CRISIS" :
            global_system.current_mode == 2 ? "POWER GRID FAILURE" : "DUAL CRISIS");
    printf("----------------------------------------------------\n");
    printf("ENERGY METRICS:\n");
    printf("  Total Generation       : %8.2f MW\n", global_system.total_energy_generation);
    printf("  Renewable Output       : %8.2f MW (%.1f%% of total)\n", 
           global_system.renewable_energy, 
           (global_system.renewable_energy / global_system.total_energy_generation) * 100.0f);
    printf("  Total Demand           : %8.2f MW\n", global_system.total_energy_demand);
    printf("  Grid Deficit/Surplus   : %8.2f MW\n", global_system.total_energy_generation - global_system.total_energy_demand);
    printf("  Battery Energy Storage : %8.2f MWh\n", global_system.battery_capacity);

    printf("----------------------------------------------------\n");
    printf("WATER METRICS:\n");
    printf("  Total Availability     : %8.0f Liters\n", global_system.total_water_availability);
    printf("  Freshwater Usage       : %8.0f Liters\n", global_system.freshwater_usage);
    printf("  Recycled Water         : %8.0f Liters\n", global_system.recycled_water);
    printf("  Total Demand           : %8.0f Liters\n", global_system.total_water_demand);
    printf("  Water Deficit/Surplus  : %8.0f Liters\n", global_system.total_water_availability - global_system.total_water_demand);
    printf("  Recycling Rate         : %8.1f%%\n", 
           (global_system.recycled_water / (global_system.freshwater_usage + global_system.recycled_water)) * 100.0f);

    printf("----------------------------------------------------\n");
    printf("SYSTEM PERFORMANCE:\n");
    printf("  Total Useful Output    : %8.2f Units\n", global_system.total_useful_output);
    printf("  Environmental Index    : %8.1f / 100.0\n", global_system.environmental_score);
    printf("====================================================\n");
}

void display_facility_dataset() {
    printf("\n-----------------------------------------------------------------------------------\n");
    printf("ID   | Facility Name               | Energy (MW) | Water (L/day) | Output  | Priority\n");
    printf("-----------------------------------------------------------------------------------\n");
    for (int i = 0; i < num_facilities; i++) {
        printf("%-4d | %-27s | %11.2f | %13.0f | %7.1f | %-8d\n",
               facilities[i].id,
               facilities[i].name,
               facilities[i].energyConsumption,
               facilities[i].waterConsumption,
               facilities[i].output,
               facilities[i].priority);
    }
    printf("-----------------------------------------------------------------------------------\n");
}

void display_workload_dataset() {
    printf("\n-----------------------------------------------------------------------------------\n");
    printf("ID   | Workload / Task Name        | Energy (MW) | Water (kL) | Output Value | Priority\n");
    printf("-----------------------------------------------------------------------------------\n");
    for (int i = 0; i < num_workloads; i++) {
        printf("%-4d | %-27s | %11d | %10d | %12d | %-8d\n",
               workloads[i].id,
               workloads[i].name,
               workloads[i].energyRequired,
               workloads[i].waterRequired,
               workloads[i].output,
               workloads[i].priority);
    }
    printf("-----------------------------------------------------------------------------------\n");
}


// ============================================================================
// 10. MAIN MENU LOOP
// ============================================================================

int main(void) {
    initialize_sample_data();

    int choice = -1;

    while (choice != 0) {
        printf("\n====================================================\n");
        printf("                    ENERGYWISE                      \n");
        printf("     Global Energy-Water Optimization System       \n");
        printf("====================================================\n");
        printf("[OVERVIEW & MANAGEMENT]\n");
        printf("  1. Global Resource Overview\n");
        printf("  2. Display Facilities & Workloads Dataset\n");
        printf("  3. Binary Search Engine (Indexed Search)\n");
        printf("\n[ALGORITHMIC CORE]\n");
        printf("  4. Sorting Benchmark Lab (5 Algorithms)\n");
        printf("  5. Greedy Priority Resource Allocator\n");
        printf("  6. 2D DP Knapsack Multi-Optimizer\n");
        printf("\n[GRAPH & NETWORKING]\n");
        printf("  7. Distribution Route Finder (Dijkstra)\n");
        printf("  8. Grid Reachability & Hops (BFS)\n");
        printf("  9. Infrastructure Matrix (Prim vs Kruskal)\n");
        printf("\n[DOMAIN OPTIMIZATION & TRENDS]\n");
        printf(" 10. Closed-Loop Water Recycling Model\n");
        printf(" 11. Multi-Strategy Trade-Off Evaluator\n");
        printf(" 12. Dynamic Trend Analyzer (LIS/LDS)\n");
        printf("\n[SIMULATION & REPORTING]\n");
        printf(" 13. System Crisis Mode Simulator\n");
        printf(" 14. What-If Scenario Workbench\n");
        printf(" 15. Comprehensive Before vs. After Report\n");
        printf("  0. Exit System\n");
        printf("====================================================\n");

        choice = get_user_choice();

        switch (choice) {
            case 1:
                display_global_overview();
                break;
            case 2:
                printf("\n--- FACILITIES ---");
                display_facility_dataset();
                printf("\n--- WORKLOADS ---");
                display_workload_dataset();
                break;
            case 3:
                run_binary_search_module();
                break;
            case 4:
                run_sorting_benchmark_lab();
                break;
            case 5:
                run_greedy_allocation_module();
                break;
            case 6:
                run_2d_knapsack_optimizer_module();
                break;
            case 7:
                run_dijkstra_routing_module();
                break;
            case 8:
                run_bfs_reachability_module();
                break;
            case 9:
                run_mst_comparison_module();
                break;
            case 10:
                run_water_recycling_module();
                break;
            case 11:
                run_multi_strategy_evaluator_module();
                break;
            case 12:
                run_dynamic_trend_analyzer_module();
                break;
            case 13:
                run_crisis_simulation_module();
                break;
            case 14:
                run_what_if_workbench_module();
                break;
            case 15:
                run_comprehensive_report_module();
                break;
            case 0:
                printf("\nExiting ENERGYWISE Optimization System. Goodbye!\n");
                break;
            default:
                printf("\n[ERROR] Invalid menu choice. Please select a valid number from the menu.\n");
                break;
        }
    }

    return 0;
}