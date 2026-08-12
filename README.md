# EnergyWise — Phase 3

Global Energy-Water Optimization System.

## Phase 3 completed

### Greedy Priority Resource Allocator
Based on the original C implementation, with interactive web controls:
- Default: 1300 MW energy and 1,120,000 L water
- User can change available energy
- User can change available water
- User can choose Priority, Highest Output, or Output/Energy strategy
- Serves a facility only if BOTH energy and water requirements fit
- Tracks served/denied facilities
- Reports allocated energy, water and output

### 2D Dynamic Programming Knapsack
Based on the original C implementation:
- 6 workloads
- Default energy limit: 250 MW
- Default water limit: 100 kL
- Objective: maximize workload output
- Uses 0/1 two-resource DP
- Backtracks the DP table to recover the selected workloads
- Reports output, selected workloads, energy used and water used
- Complexity: O(N * E * W)

## Offline

Double-click `index.html`. No internet/server is required.

## GitHub Pages

Still a static HTML/CSS/JavaScript project. The complete folder can be uploaded to GitHub Pages later.

## Next phase

Phase 4:
- BFS
- Dijkstra
- Prim vs Kruskal
- Network visualization


## Dynamic data manager

Phase 3 is now fully scenario-oriented:
- Edit system generation, demand, renewable energy, water availability/demand, recycled water, useful output and environmental score.
- Edit every facility row.
- Add/delete facilities.
- Edit every workload row used by 2D DP.
- Add/delete workloads.
- Save to browser localStorage for offline use.
- Reset to the original C-project dataset.
- Binary Search, all sorting algorithms, Greedy and 2D DP read the active saved dataset.
- Greedy resource limits are fully changeable.
- 2D DP limits are changeable up to 500 MW / 250 kL to keep browser memory/time practical.


## Phase 4 — Graph & Networking

Implemented:
- BFS reachability + minimum hops
- Dijkstra shortest route by distribution cost or transmission loss
- Prim minimum spanning tree
- Kruskal minimum spanning tree
- Interactive source/destination/metric controls
- Offline-compatible static implementation

The graph algorithms mirror the original C project's networking module structure. The original C menu lists Dijkstra, BFS, and Prim-vs-Kruskal under Graph & Networking.


## Phase 5
- Water Recycling Simulator
- Energy-Water Trade-off Evaluator
- LIS/LDS Analyzer
- Offline-compatible


## Phase 5 Fixed v2
- Integrated Phase 5 into the actual sidebar navigation system.
- Added Water Recycling, Trade-off Evaluator and LIS/LDS navigation buttons.
- Existing section router now opens these sections like Dashboard, Algorithms and Network.


## Phase 6 — Simulation & Reporting
- What-if scenario workbench
- Energy, water, renewable and demand sensitivity controls
- Baseline vs scenario comparison
- Browser-only executive report
- No backend required; existing saved dataset remains unchanged


## Phase 7 — Final Control Center
- Project overview and algorithm complexity table
- Browser-state JSON export/import
- Local-state reset
- GitHub/offline readiness checker
- Final Control Center navigation


## Phase 5 New Design
- Water Recycling is now a dedicated recovery calculator.
- Trade-off Evaluator is now a separate three-strategy decision matrix.
- The two modules use different inputs, calculations and outputs.


## Final Phase 5 Fix
Water Recycling and Trade-off Evaluator are separate modules:
- Water Recycling: wastewater recovery, fresh-water saving, supply and recycling cost.
- Trade-off Evaluator: three strategies, energy/water budgets, output priority and recommendation.


## LIS/LDS Fix
- Connected the Analyze Sequence button to the LIS/LDS dynamic-programming analyzer.
