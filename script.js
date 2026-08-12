const DEFAULT_DATA = {
  state: {
    totalEnergyGeneration: 1300,
    renewableEnergy: 880,
    totalEnergyDemand: 1365,
    batteryCapacity: 250,
    totalWaterAvailability: 1120000,
    freshwaterUsage: 735000,
    recycledWater: 250000,
    totalWaterDemand: 1205000,
    totalUsefulOutput: 6140,
    environmentalScore: 78.5,
    currentMode: 0
  },
  facilities: [
    {id:101,name:"Metropolitan Hospital",energy:120,water:50000,output:950,priority:1},
    {id:102,name:"Central Data Center",energy:250,water:80000,output:880,priority:5},
    {id:103,name:"High-Tech Fabrication Plant",energy:310,water:120000,output:750,priority:7},
    {id:104,name:"Emergency Command Center",energy:45,water:15000,output:990,priority:2},
    {id:105,name:"Municipal Water Treatment",energy:180,water:200000,output:820,priority:3},
    {id:106,name:"Agricultural District A",energy:80,water:350000,output:600,priority:4},
    {id:107,name:"Residential Zone Alpha",energy:220,water:180000,output:700,priority:6},
    {id:108,name:"Heavy Textile Mill",energy:160,water:210000,output:450,priority:8}
  ],
  workloads: [
    {id:401,name:"AI Model Training Batch",energy:80,water:20,output:180,priority:5},
    {id:402,name:"Desalination Processing",energy:60,water:50,output:150,priority:3},
    {id:403,name:"Hospital ICU Backup Grid",energy:30,water:10,output:200,priority:1},
    {id:404,name:"Chip Cleanroom Assembly",energy:100,water:40,output:210,priority:6},
    {id:405,name:"Urban Sewage Treatment",energy:40,water:30,output:110,priority:2},
    {id:406,name:"Automated Vertical Farm",energy:50,water:60,output:130,priority:4}
  ]
};

let appData = loadData();
let state = appData.state;
let facilities = appData.facilities;
let workloads = appData.workloads;

function clone(obj) {
  return JSON.parse(JSON.stringify(obj));
}

function loadData() {
  try {
    const saved = localStorage.getItem("energywiseDataV2");
    if (saved) {
      const parsed = JSON.parse(saved);
      if (parsed.state && Array.isArray(parsed.facilities) && Array.isArray(parsed.workloads)) {
        return parsed;
      }
    }
  } catch (e) {}
  return clone(DEFAULT_DATA);
}

function saveData(showMessage = true) {
  appData = {state, facilities, workloads};
  localStorage.setItem("energywiseDataV2", JSON.stringify(appData));
  renderDashboard();
  renderFacilities();
  renderWorkloadEditor();
  if (showMessage) showDataMessage("Changes saved. All algorithms now use the active dataset.");
}

function showDataMessage(message) {
  const el = document.getElementById("dataMessage");
  if (el) {
    el.textContent = message;
    el.className = "hint";
  }
}

function setText(id, value) {
  const el = document.getElementById(id);
  if (el) el.textContent = value;
}

function renderDashboard() {
  const energyCoverage = state.totalEnergyDemand ? (state.totalEnergyGeneration / state.totalEnergyDemand) * 100 : 0;
  const waterCoverage = state.totalWaterDemand ? (state.totalWaterAvailability / state.totalWaterDemand) * 100 : 0;
  const renewableShare = state.totalEnergyGeneration ? (state.renewableEnergy / state.totalEnergyGeneration) * 100 : 0;
  const energyDiff = state.totalEnergyGeneration - state.totalEnergyDemand;

  setText("heroScore", Number(state.environmentalScore).toFixed(1));
  setText("energyGeneration", Number(state.totalEnergyGeneration).toFixed(0));
  setText("renewableEnergy", Number(state.renewableEnergy).toFixed(0));
  setText("energyDemand", Number(state.totalEnergyDemand).toFixed(0));
  setText("energyBalance", Math.abs(energyDiff).toFixed(0) + " MW " + (energyDiff >= 0 ? "surplus" : "deficit"));
  setText("waterAvailability", (Number(state.totalWaterAvailability) / 1000000).toFixed(2));
  setText("recycledWater", (Number(state.recycledWater) / 1000).toFixed(0) + "k");
  setText("usefulOutput", Number(state.totalUsefulOutput).toFixed(0));
  setText("energyCoverage", energyCoverage.toFixed(1) + "%");
  setText("waterCoverage", waterCoverage.toFixed(1) + "%");
  setText("renewableShare", renewableShare.toFixed(1) + "%");

  const eb = document.getElementById("energyBar");
  const wb = document.getElementById("waterBar");
  const rb = document.getElementById("renewableBar");
  if (eb) eb.style.width = Math.min(100, Math.max(0, energyCoverage)) + "%";
  if (wb) wb.style.width = Math.min(100, Math.max(0, waterCoverage)) + "%";
  if (rb) rb.style.width = Math.min(100, Math.max(0, renewableShare)) + "%";

  setText("greedyTotal", facilities.length);
}

function renderFacilities() {
  const tbody = document.getElementById("facilityTable");
  if (!tbody) return;

  tbody.innerHTML = facilities.map((f, index) => `
    <tr>
      <td><input class="data-id" data-type="facility" data-index="${index}" type="number" value="${f.id}"></td>
      <td><input class="name-input data-name" data-type="facility" data-index="${index}" value="${escapeHtml(f.name)}"></td>
      <td><input class="data-energy" data-type="facility" data-index="${index}" type="number" min="0" step="0.1" value="${f.energy}"></td>
      <td><input class="data-water" data-type="facility" data-index="${index}" type="number" min="0" step="1" value="${f.water}"></td>
      <td><input class="data-output" data-type="facility" data-index="${index}" type="number" min="0" step="0.1" value="${f.output}"></td>
      <td><input class="data-priority" data-type="facility" data-index="${index}" type="number" min="1" step="1" value="${f.priority}"></td>
      <td><button class="icon-btn delete-row" data-type="facility" data-index="${index}">Delete</button></td>
    </tr>
  `).join("");
}

function renderWorkloadEditor() {
  const tbody = document.getElementById("workloadTable");
  if (!tbody) return;

  tbody.innerHTML = workloads.map((w, index) => `
    <tr>
      <td><input class="data-id" data-type="workload" data-index="${index}" type="number" value="${w.id}"></td>
      <td><input class="name-input data-name" data-type="workload" data-index="${index}" value="${escapeHtml(w.name)}"></td>
      <td><input class="data-energy" data-type="workload" data-index="${index}" type="number" min="0" step="1" value="${w.energy}"></td>
      <td><input class="data-water" data-type="workload" data-index="${index}" type="number" min="0" step="1" value="${w.water}"></td>
      <td><input class="data-output" data-type="workload" data-index="${index}" type="number" min="0" step="1" value="${w.output}"></td>
      <td><input class="data-priority" data-type="workload" data-index="${index}" type="number" min="1" step="1" value="${w.priority}"></td>
      <td><button class="icon-btn delete-row" data-type="workload" data-index="${index}">Delete</button></td>
    </tr>
  `).join("");
}

function escapeHtml(value) {
  return String(value)
    .replaceAll("&","&amp;").replaceAll("<","&lt;").replaceAll(">","&gt;")
    .replaceAll('"',"&quot;").replaceAll("'","&#039;");
}

function populateSystemEditor() {
  setTextValue("editGeneration", state.totalEnergyGeneration);
  setTextValue("editRenewable", state.renewableEnergy);
  setTextValue("editEnergyDemand", state.totalEnergyDemand);
  setTextValue("editWaterAvailability", state.totalWaterAvailability);
  setTextValue("editWaterDemand", state.totalWaterDemand);
  setTextValue("editRecycledWater", state.recycledWater);
  setTextValue("editUsefulOutput", state.totalUsefulOutput);
  setTextValue("editEnvironmentalScore", state.environmentalScore);
}

function setTextValue(id, value) {
  const el = document.getElementById(id);
  if (el) el.value = value;
}

function readNumber(id) {
  const value = Number(document.getElementById(id).value);
  return Number.isFinite(value) ? value : 0;
}

function readEditorsIntoData() {
  state.totalEnergyGeneration = readNumber("editGeneration");
  state.renewableEnergy = readNumber("editRenewable");
  state.totalEnergyDemand = readNumber("editEnergyDemand");
  state.totalWaterAvailability = readNumber("editWaterAvailability");
  state.totalWaterDemand = readNumber("editWaterDemand");
  state.recycledWater = readNumber("editRecycledWater");
  state.totalUsefulOutput = readNumber("editUsefulOutput");
  state.environmentalScore = Math.min(100, Math.max(0, readNumber("editEnvironmentalScore")));

  document.querySelectorAll("#facilityTable tr").forEach((row, index) => {
    if (!facilities[index]) return;
    facilities[index].id = Math.round(Number(row.querySelector(".data-id").value) || 0);
    facilities[index].name = row.querySelector(".data-name").value.trim() || `Facility ${facilities[index].id}`;
    facilities[index].energy = Math.max(0, Number(row.querySelector(".data-energy").value) || 0);
    facilities[index].water = Math.max(0, Number(row.querySelector(".data-water").value) || 0);
    facilities[index].output = Math.max(0, Number(row.querySelector(".data-output").value) || 0);
    facilities[index].priority = Math.max(1, Math.round(Number(row.querySelector(".data-priority").value) || 1));
  });

  document.querySelectorAll("#workloadTable tr").forEach((row, index) => {
    if (!workloads[index]) return;
    workloads[index].id = Math.round(Number(row.querySelector(".data-id").value) || 0);
    workloads[index].name = row.querySelector(".data-name").value.trim() || `Workload ${workloads[index].id}`;
    workloads[index].energy = Math.max(0, Number(row.querySelector(".data-energy").value) || 0);
    workloads[index].water = Math.max(0, Number(row.querySelector(".data-water").value) || 0);
    workloads[index].output = Math.max(0, Number(row.querySelector(".data-output").value) || 0);
    workloads[index].priority = Math.max(1, Math.round(Number(row.querySelector(".data-priority").value) || 1));
  });

  appData = {state, facilities, workloads};
}

function renderNodes() {
  const grid = document.getElementById("nodeGrid");
  if (!grid) return;
  const nodes = [
    "Main Hydro Plant","Substation North","Substation South","Water Facility A",
    "Data Center Park","Hospital Complex","Industrial Zone","Residential Hub"
  ];
  grid.innerHTML = nodes.map((node, i) => `
    <div><small>NODE ${i}</small><b>${node}</b></div>
  `).join("");
}

function updateAllAfterDataChange() {
  renderDashboard();
  renderFacilities();
  renderWorkloadEditor();
  if (typeof runSortingBenchmark === "function") runSortingBenchmark();
  if (typeof runGreedyAllocation === "function") runGreedyAllocation();
  if (typeof run2DKnapsack === "function") run2DKnapsack();
}

document.querySelectorAll(".nav-item").forEach(button => {
  button.addEventListener("click", () => {
    document.querySelectorAll(".nav-item").forEach(b => b.classList.remove("active"));
    document.querySelectorAll(".section").forEach(s => s.classList.remove("active-section"));
    button.classList.add("active");
    document.getElementById(button.dataset.section).classList.add("active-section");
    window.scrollTo({top: 0, behavior: "smooth"});
  });
});

document.getElementById("saveDataBtn").addEventListener("click", () => {
  readEditorsIntoData();
  saveData();
  showDataMessage("Saved. Binary Search, Sorting, Greedy and 2D DP now use your changed data.");
});

document.getElementById("resetDataBtn").addEventListener("click", () => {
  if (!confirm("Reset all system values, facilities and workloads to the original C-project dataset?")) return;
  appData = clone(DEFAULT_DATA);
  state = appData.state;
  facilities = appData.facilities;
  workloads = appData.workloads;
  localStorage.removeItem("energywiseDataV2");
  populateSystemEditor();
  updateAllAfterDataChange();
  showDataMessage("Original dataset restored.");
});

document.getElementById("addFacilityBtn").addEventListener("click", () => {
  readEditorsIntoData();
  const nextId = facilities.length ? Math.max(...facilities.map(f => f.id)) + 1 : 101;
  facilities.push({id:nextId,name:`New Facility ${nextId}`,energy:0,water:0,output:0,priority:facilities.length+1});
  renderFacilities();
  showDataMessage("New facility added. Fill its values and click Save Changes.");
});

document.getElementById("addWorkloadBtn").addEventListener("click", () => {
  readEditorsIntoData();
  const nextId = workloads.length ? Math.max(...workloads.map(w => w.id)) + 1 : 401;
  workloads.push({id:nextId,name:`New Workload ${nextId}`,energy:0,water:0,output:0,priority:workloads.length+1});
  renderWorkloadEditor();
  showDataMessage("New workload added. Fill its values and click Save Changes.");
});

document.addEventListener("click", event => {
  const button = event.target.closest(".delete-row");
  if (!button) return;
  readEditorsIntoData();
  const index = Number(button.dataset.index);
  const type = button.dataset.type;
  if (type === "facility") {
    facilities.splice(index,1);
    renderFacilities();
  } else {
    workloads.splice(index,1);
    renderWorkloadEditor();
  }
  showDataMessage(`${type === "facility" ? "Facility" : "Workload"} deleted. Click Save Changes to persist it.`);
});

renderDashboard();
renderFacilities();
renderWorkloadEditor();
populateSystemEditor();
renderNodes();

// ==================== PHASE 2: SEARCHING + SORTING ====================

function compareEnergy(a, b) {
  return a.energy - b.energy;
}

function swap(arr, i, j) {
  const temp = arr[i];
  arr[i] = arr[j];
  arr[j] = temp;
}

// Binary Search: same core idea as the C implementation.
function binarySearchFacilityById(arr, targetId) {
  let low = 0;
  let high = arr.length - 1;
  let comparisons = 0;

  while (low <= high) {
    const mid = low + Math.floor((high - low) / 2);
    comparisons++;

    if (arr[mid].id === targetId) return { index: mid, comparisons };
    if (arr[mid].id < targetId) low = mid + 1;
    else high = mid - 1;
  }

  return { index: -1, comparisons };
}

// Bubble Sort — counts comparisons and swaps like the C version.
function bubbleSort(input) {
  const arr = input.map(x => ({...x}));
  let comparisons = 0, swaps = 0;

  for (let i = 0; i < arr.length - 1; i++) {
    let swapped = false;
    for (let j = 0; j < arr.length - i - 1; j++) {
      comparisons++;
      if (arr[j].energy > arr[j + 1].energy) {
        swap(arr, j, j + 1);
        swaps++;
        swapped = true;
      }
    }
    if (!swapped) break;
  }
  return { arr, comparisons, swaps, complexity: "O(N²)" };
}

// Selection Sort — counts comparisons and actual swaps.
function selectionSort(input) {
  const arr = input.map(x => ({...x}));
  let comparisons = 0, swaps = 0;

  for (let i = 0; i < arr.length - 1; i++) {
    let minIndex = i;
    for (let j = i + 1; j < arr.length; j++) {
      comparisons++;
      if (arr[j].energy < arr[minIndex].energy) minIndex = j;
    }
    if (minIndex !== i) {
      swap(arr, i, minIndex);
      swaps++;
    }
  }
  return { arr, comparisons, swaps, complexity: "O(N²)" };
}

// Insertion Sort — moves are counted as swaps/moves, matching the C metric label.
function insertionSort(input) {
  const arr = input.map(x => ({...x}));
  let comparisons = 0, swaps = 0;

  for (let i = 1; i < arr.length; i++) {
    const key = arr[i];
    let j = i - 1;

    while (j >= 0) {
      comparisons++;
      if (arr[j].energy > key.energy) {
        arr[j + 1] = arr[j];
        swaps++;
        j--;
      } else {
        break;
      }
    }
    arr[j + 1] = key;
  }
  return { arr, comparisons, swaps, complexity: "O(N²)" };
}

function mergeSortInternal(arr, left, right, metrics) {
  if (left >= right) return;

  const mid = left + Math.floor((right - left) / 2);
  mergeSortInternal(arr, left, mid, metrics);
  mergeSortInternal(arr, mid + 1, right, metrics);

  const L = arr.slice(left, mid + 1);
  const R = arr.slice(mid + 1, right + 1);
  let i = 0, j = 0, k = left;

  while (i < L.length && j < R.length) {
    metrics.comparisons++;
    if (L[i].energy <= R[j].energy) arr[k++] = L[i++];
    else arr[k++] = R[j++];
    metrics.swaps++;
  }
  while (i < L.length) { arr[k++] = L[i++]; metrics.swaps++; }
  while (j < R.length) { arr[k++] = R[j++]; metrics.swaps++; }
}

function mergeSort(input) {
  const arr = input.map(x => ({...x}));
  const metrics = { comparisons: 0, swaps: 0 };
  mergeSortInternal(arr, 0, arr.length - 1, metrics);
  return { arr, ...metrics, complexity: "O(N log N)" };
}

function partition(arr, low, high, metrics) {
  const pivot = arr[high].energy;
  let i = low - 1;

  for (let j = low; j <= high - 1; j++) {
    metrics.comparisons++;
    if (arr[j].energy < pivot) {
      i++;
      swap(arr, i, j);
      metrics.swaps++;
    }
  }
  swap(arr, i + 1, high);
  metrics.swaps++;
  return i + 1;
}

function quickSortInternal(arr, low, high, metrics) {
  if (low >= high) return;
  const pi = partition(arr, low, high, metrics);
  quickSortInternal(arr, low, pi - 1, metrics);
  quickSortInternal(arr, pi + 1, high, metrics);
}

function quickSort(input) {
  const arr = input.map(x => ({...x}));
  const metrics = { comparisons: 0, swaps: 0 };
  quickSortInternal(arr, 0, arr.length - 1, metrics);
  return { arr, ...metrics, complexity: "O(N log N)" };
}

function renderSortedTable(arr) {
  const tbody = document.getElementById("sortedOutput");
  tbody.innerHTML = arr.map((f, i) => `
    <tr>
      <td>${i + 1}</td>
      <td>${f.id}</td>
      <td>${f.name}</td>
      <td>${f.energy.toFixed(2)}</td>
      <td>${f.water.toLocaleString()}</td>
    </tr>
  `).join("");
}

function runSortingBenchmark() {
  const algorithms = [
    ["Bubble Sort", bubbleSort],
    ["Selection Sort", selectionSort],
    ["Insertion Sort", insertionSort],
    ["Merge Sort", mergeSort],
    ["Quick Sort", quickSort]
  ];

  const results = algorithms.map(([name, fn]) => {
    const result = fn(facilities);
    return { name, ...result };
  });

  const tbody = document.getElementById("sortMetrics");
  tbody.innerHTML = results.map(r => `
    <tr>
      <td><b>${r.name}</b></td>
      <td>${r.comparisons}</td>
      <td>${r.swaps}</td>
      <td>${r.complexity}</td>
    </tr>
  `).join("");

  renderSortedTable(results[3].arr);

  const best = [...results].sort((a,b) => a.comparisons - b.comparisons)[0];
  document.getElementById("sortInsight").innerHTML = `
    <div class="insight-icon">✓</div>
    <div>
      <strong>${best.name} used the fewest comparisons on this dataset.</strong>
      <p>These are measured operations for the current 8-facility sample, not a claim that this algorithm is always fastest for every dataset.</p>
    </div>
  `;
}

function runBinarySearch() {
  const input = document.getElementById("facilityId");
  const target = Number(input.value);
  const status = document.getElementById("searchStatus");
  const resultBox = document.getElementById("searchResult");

  if (!Number.isInteger(target)) {
    status.textContent = "Invalid input";
    status.className = "error-text";
    resultBox.innerHTML = "Please enter a valid integer facility ID.";
    return;
  }

  const sorted = [...facilities].sort((a,b) => a.id - b.id);
  const result = binarySearchFacilityById(sorted, target);

  if (result.index !== -1) {
    const f = sorted[result.index];
    status.textContent = `Found in ${result.comparisons} comparisons`;
    status.className = "success-text";
    resultBox.innerHTML = `
      <div class="search-card">
        <div><span>ID</span><b>${f.id}</b></div>
        <div><span>Facility</span><b>${f.name}</b></div>
        <div><span>Energy</span><b>${f.energy.toFixed(2)} MW</b></div>
        <div><span>Water</span><b>${f.water.toLocaleString()} L/day</b></div>
        <div><span>Useful Output</span><b>${f.output.toFixed(1)}</b></div>
        <div><span>Priority</span><b>${f.priority}</b></div>
      </div>
    `;
  } else {
    status.textContent = `Not found • ${result.comparisons} comparisons`;
    status.className = "error-text";
    resultBox.innerHTML = `Facility ID <b>${target}</b> does not exist in the active registry.`;
  }
}

document.querySelectorAll(".lab-tab").forEach(tab => {
  tab.addEventListener("click", () => {
    document.querySelectorAll(".lab-tab").forEach(t => t.classList.remove("active"));
    document.querySelectorAll(".lab-panel").forEach(p => p.classList.remove("active-lab"));
    tab.classList.add("active");
    document.getElementById(tab.dataset.lab).classList.add("active-lab");
  });
});

document.getElementById("searchBtn").addEventListener("click", runBinarySearch);
document.getElementById("facilityId").addEventListener("keydown", e => {
  if (e.key === "Enter") runBinarySearch();
});
document.getElementById("runSortBtn").addEventListener("click", runSortingBenchmark);

// Run once so the benchmark is immediately visible.
runSortingBenchmark();


// ==================== PHASE 3: GREEDY + 2D DP ====================


// Exact facility-based greedy logic from the C module:
// sort by priority ascending, then serve when BOTH resources are available.
function runGreedyAllocation() {
  let availableEnergy = Number(document.getElementById("greedyEnergyLimit").value);
  let availableWater = Number(document.getElementById("greedyWaterLimit").value);
  const strategy = document.getElementById("greedyStrategy").value;

  if (!Number.isFinite(availableEnergy) || !Number.isFinite(availableWater) ||
      availableEnergy < 0 || availableWater < 0) {
    alert("Please enter valid non-negative resource limits.");
    return;
  }

  let ordered = [...facilities];

  if (strategy === "priority") {
    ordered.sort((a,b) => a.priority - b.priority);
  } else if (strategy === "output") {
    ordered.sort((a,b) => b.output - a.output);
  } else {
    ordered.sort((a,b) => (b.output / b.energy) - (a.output / a.energy));
  }

  const originalEnergy = availableEnergy;
  const originalWater = availableWater;
  let allocatedEnergy = 0;
  let allocatedWater = 0;
  let totalOutput = 0;
  let satisfied = 0;
  const rows = [];

  for (const f of ordered) {
    const canServe = availableEnergy >= f.energy && availableWater >= f.water;

    if (canServe) {
      availableEnergy -= f.energy;
      availableWater -= f.water;
      allocatedEnergy += f.energy;
      allocatedWater += f.water;
      totalOutput += f.output;
      satisfied++;
    }

    rows.push({ ...f, served: canServe });
  }

  const strategyText = {
    priority: "Priority",
    output: "Highest Output",
    efficiency: "Output / Energy"
  }[strategy];

  document.getElementById("greedyOutput").innerHTML = rows.map(f => `
    <tr>
      <td>${f.priority}</td>
      <td>${f.name}</td>
      <td class="${f.served ? "status-served" : "status-denied"}">${f.served ? "SERVED" : "DENIED"}</td>
      <td>${f.energy.toFixed(2)}</td>
      <td>${f.water.toLocaleString()}</td>
      <td>${f.served ? f.output.toFixed(1) : "0.0"}</td>
    </tr>
  `).join("");

  setText("greedyAvailableEnergy", originalEnergy.toLocaleString());
  setText("greedyAvailableWater", originalWater.toLocaleString());
  setText("greedyStrategyLabel", strategyText);
  setText("greedySatisfied", satisfied);
  setText("greedyEnergy", allocatedEnergy.toFixed(2));
  setText("greedyWater", allocatedWater.toFixed(0));
  setText("greedyOutputTotal", totalOutput.toFixed(2));
}

// Exact 2D 0/1 knapsack recurrence from the C module.
// Dimensions: workload × energy × water; value = output.
function run2DKnapsack() {
  let maxE = Number(document.getElementById("dpEnergy").value);
  let maxW = Number(document.getElementById("dpWater").value);

  if (!Number.isInteger(maxE) || !Number.isInteger(maxW) ||
      maxE < 0 || maxW < 0 || maxE > 500 || maxW > 250) {
    alert("For browser performance, DP limits are 0–500 MW and 0–250 kL.");
    return;
  }

  const n = workloads.length;
  const dp = Array.from({length:n + 1}, () =>
    Array.from({length:maxE + 1}, () => new Int32Array(maxW + 1))
  );

  for (let i = 1; i <= n; i++) {
    const item = workloads[i - 1];

    for (let e = 0; e <= maxE; e++) {
      for (let w = 0; w <= maxW; w++) {
        dp[i][e][w] = dp[i - 1][e][w];

        if (e >= item.energy && w >= item.water) {
          const includeValue = item.output + dp[i - 1][e - item.energy][w - item.water];
          if (includeValue > dp[i][e][w]) {
            dp[i][e][w] = includeValue;
          }
        }
      }
    }
  }

  const selected = [];
  let currE = maxE;
  let currW = maxW;

  for (let i = n; i > 0; i--) {
    if (dp[i][currE][currW] !== dp[i - 1][currE][currW]) {
      const item = workloads[i - 1];
      selected.push(item);
      currE -= item.energy;
      currW -= item.water;
    }
  }

  selected.reverse();

  const usedEnergy = selected.reduce((sum,x) => sum + x.energy, 0);
  const usedWater = selected.reduce((sum,x) => sum + x.water, 0);

  setText("dpMaxOutput", dp[n][maxE][maxW]);
  setText("dpSelectedCount", selected.length);
  setText("dpUsedEnergy", usedEnergy + " / " + maxE + " MW");
  setText("dpUsedWater", usedWater + " / " + maxW + " kL");

  document.getElementById("dpOutput").innerHTML = selected.length
    ? selected.map(w => `
      <tr>
        <td>${w.id}</td>
        <td>${w.name}</td>
        <td>${w.energy}</td>
        <td>${w.water}</td>
        <td>${w.output}</td>
        <td>${w.priority}</td>
      </tr>
    `).join("")
    : `<tr><td colspan="6">No workload fits within the selected constraints.</td></tr>`;
}

document.getElementById("runGreedyBtn").addEventListener("click", runGreedyAllocation);
document.getElementById("runDpBtn").addEventListener("click", run2DKnapsack);

["greedyEnergyLimit", "greedyWaterLimit", "greedyStrategy"].forEach(id => {
  document.getElementById(id).addEventListener("keydown", e => {
    if (e.key === "Enter") runGreedyAllocation();
  });
});

// Show meaningful results immediately.
runGreedyAllocation();
run2DKnapsack();


// ==================== PHASE 4: GRAPH & NETWORKING ====================

// The graph follows the same adjacency-matrix idea used by the C project.
// Each undirected edge has cost and transmission-loss metrics.
const networkNodes = [
  "Solar Farm", "Wind Farm", "Battery Hub", "Main Grid",
  "Factory A", "Factory B", "Water Plant", "Hospital"
];

const networkEdges = [
  [0,1,18,2.0],[0,2,12,1.2],[0,3,25,2.8],
  [1,2,10,1.0],[1,3,20,2.1],[2,3,8,0.8],
  [2,4,14,1.5],[2,5,17,1.8],[3,4,9,1.0],
  [3,5,11,1.2],[3,6,15,1.6],[4,5,7,0.7],
  [4,7,13,1.4],[5,7,10,1.1],[6,7,6,0.6]
];

const adj = Array.from({length: networkNodes.length}, () => Array(networkNodes.length).fill(null));
networkEdges.forEach(([a,b,cost,loss]) => {
  adj[a][b] = {cost,loss}; adj[b][a] = {cost,loss};
});

function fillNodeSelect(id) {
  const el = document.getElementById(id);
  if (!el) return;
  el.innerHTML = networkNodes.map((n,i)=>`<option value="${i}">${i} — ${n}</option>`).join("");
}
["bfsSource","dijkstraSource","dijkstraDestination"].forEach(fillNodeSelect);
document.getElementById("dijkstraDestination").value = "7";

function runBFS() {
  const src = Number(document.getElementById("bfsSource").value);
  const visited = Array(networkNodes.length).fill(false);
  const hops = Array(networkNodes.length).fill(-1);
  const order = [];
  const q = [src];
  visited[src] = true; hops[src] = 0;

  while (q.length) {
    const u = q.shift();
    order.push(u);
    for (let v=0; v<networkNodes.length; v++) {
      if (adj[u][v] && !visited[v]) {
        visited[v] = true;
        hops[v] = hops[u] + 1;
        q.push(v);
      }
    }
  }

  const step = new Map(order.map((v,i)=>[v,i+1]));
  document.getElementById("bfsOutput").innerHTML = networkNodes.map((name,i)=>`
    <tr>
      <td>${step.get(i) || "—"}</td><td>${name}</td>
      <td class="${visited[i] ? "status-served":"status-denied"}">${visited[i] ? "REACHABLE":"UNREACHABLE"}</td>
      <td>${hops[i] === -1 ? "INF" : hops[i]}</td>
    </tr>`).join("");

  const unreachable = networkNodes.filter((_,i)=>!visited[i]).length;
  document.getElementById("bfsSummary").textContent =
    unreachable === 0
      ? `FULLY CONNECTED — ${order.length}/${networkNodes.length} nodes reachable from ${networkNodes[src]}.`
      : `${unreachable} node(s) unreachable from ${networkNodes[src]}.`;
}

function dijkstra(src, metric) {
  const n = networkNodes.length;
  const dist = Array(n).fill(Infinity);
  const parent = Array(n).fill(-1);
  const used = Array(n).fill(false);
  dist[src] = 0;

  for (let k=0;k<n;k++) {
    let u=-1;
    for(let i=0;i<n;i++) if(!used[i] && (u===-1 || dist[i]<dist[u])) u=i;
    if(u===-1 || dist[u]===Infinity) break;
    used[u]=true;
    for(let v=0;v<n;v++){
      if(!adj[u][v] || used[v]) continue;
      const w=adj[u][v][metric];
      if(dist[u]+w<dist[v]){
        dist[v]=dist[u]+w;
        parent[v]=u;
      }
    }
  }
  return {dist,parent};
}

function getPath(parent, src, dst) {
  const path=[];
  for(let v=dst; v!==-1; v=parent[v]) path.push(v);
  path.reverse();
  return path[0]===src ? path : [];
}

function runDijkstra() {
  const src=Number(document.getElementById("dijkstraSource").value);
  const dst=Number(document.getElementById("dijkstraDestination").value);
  const metric=document.getElementById("dijkstraWeight").value;
  const {dist,parent}=dijkstra(src,metric);
  const path=getPath(parent,src,dst);

  if(!path.length){
    document.getElementById("dijkstraSummary").textContent="No route found.";
    document.getElementById("dijkstraPath").textContent="UNREACHABLE";
    return;
  }

  const unit = metric==="cost" ? "$/unit" : "% loss";
  document.getElementById("dijkstraSummary").textContent =
    `Minimum ${metric==="cost"?"cost":"transmission loss"} = ${dist[dst].toFixed(2)} ${unit}`;
  document.getElementById("dijkstraPath").textContent =
    path.map(i=>networkNodes[i]).join("  →  ");
}

class DSU {
  constructor(n){this.p=Array.from({length:n},(_,i)=>i);this.r=Array(n).fill(0);}
  find(x){while(this.p[x]!==x){this.p[x]=this.p[this.p[x]];x=this.p[x];}return x;}
  union(a,b){a=this.find(a);b=this.find(b);if(a===b)return false;if(this.r[a]<this.r[b])[a,b]=[b,a];this.p[b]=a;if(this.r[a]===this.r[b])this.r[a]++;return true;}
}

function primMST() {
  const n=networkNodes.length, key=Array(n).fill(Infinity), parent=Array(n).fill(-1), used=Array(n).fill(false);
  key[0]=0;
  for(let k=0;k<n;k++){
    let u=-1;
    for(let i=0;i<n;i++) if(!used[i]&&(u===-1||key[i]<key[u]))u=i;
    if(u===-1)break; used[u]=true;
    for(let v=0;v<n;v++) if(adj[u][v]&&!used[v]&&adj[u][v].cost<key[v]){key[v]=adj[u][v].cost;parent[v]=u;}
  }
  return parent.slice(1).map((p,i)=>({a:p,b:i+1,w:adj[p][i+1].cost}));
}

function kruskalMST() {
  const edges=networkEdges.map(([a,b,cost])=>({a,b,w:cost})).sort((x,y)=>x.w-y.w);
  const dsu=new DSU(networkNodes.length), out=[];
  for(const e of edges) if(dsu.union(e.a,e.b)){out.push(e);if(out.length===networkNodes.length-1)break;}
  return out;
}

function renderMst(id, edges) {
  document.getElementById(id).innerHTML=edges.map(e=>`
    <div class="mst-edge"><span>${networkNodes[e.a]} ↔ ${networkNodes[e.b]}</span><b>${e.w.toFixed(1)}</b></div>
  `).join("");
}

function runMST() {
  const prim=primMST(), kr=kruskalMST();
  renderMst("primResult",prim);
  renderMst("kruskalResult",kr);
  const p=prim.reduce((s,e)=>s+e.w,0), k=kr.reduce((s,e)=>s+e.w,0);
  document.getElementById("mstSummary").textContent =
    `Prim total cost: ${p.toFixed(2)}  |  Kruskal total cost: ${k.toFixed(2)}  |  Both produce the same minimum spanning-tree cost.`;
}

document.getElementById("runBfsBtn").addEventListener("click",runBFS);
document.getElementById("runDijkstraBtn").addEventListener("click",runDijkstra);
document.getElementById("runMstBtn").addEventListener("click",runMST);

document.querySelectorAll("[data-netlab]").forEach(btn=>{
  btn.addEventListener("click",()=>{
    document.querySelectorAll("[data-netlab]").forEach(x=>x.classList.remove("active"));
    document.querySelectorAll(".network-panel").forEach(x=>x.classList.remove("active"));
    btn.classList.add("active");
    document.getElementById(btn.dataset.netlab).classList.add("active");
  });
});

// Useful default results.
runBFS(); runDijkstra(); runMST();



// ==================== PHASE 5 FINAL: DISTINCT MODULES ====================
function runWaterRecyclingFinal(){
  const fresh=+document.getElementById("recycleFresh").value;
  const waste=+document.getElementById("recycleWaste").value;
  const rate=+document.getElementById("recycleRate").value;
  const demand=+document.getElementById("recycleDemand").value;
  const cost=+document.getElementById("recycleCost").value;
  if(![fresh,waste,rate,demand,cost].every(Number.isFinite)||fresh<0||waste<0||rate<0||rate>100||demand<0||cost<0){
    alert("Enter valid water values."); return;
  }
  const recovered=waste*rate/100;
  const total=fresh+recovered;
  const saved=Math.min(recovered,Math.max(0,demand-fresh));
  const totalCost=recovered*cost;
  setText("recoveredWater",recovered.toFixed(2));
  setText("freshSaved",saved.toFixed(2));
  setText("effectiveWater",total.toFixed(2));
  setText("recycleTotalCost",totalCost.toFixed(2));
  setText("recycleSummary",total>=demand
    ? `DEMAND COVERED — ${recovered.toFixed(2)} kL recovered; ${saved.toFixed(2)} kL fresh water can be saved.`
    : `SHORTFALL — ${(demand-total).toFixed(2)} kL more water is required.`);
}

function runTradeoffFinal(){
  const E=+document.getElementById("tradeEnergy").value;
  const W=+document.getElementById("tradeWater").value;
  const weight=+document.getElementById("tradeOutputWeight").value;
  if(![E,W,weight].every(Number.isFinite)||E<=0||W<=0||weight<0||weight>100){
    alert("Enter valid trade-off settings."); return;
  }
  const data=[
    ["Strategy A",+aEnergy.value,+aWater.value,+aOutput.value],
    ["Strategy B",+bEnergy.value,+bWater.value,+bOutput.value],
    ["Strategy C",+cEnergy.value,+cWater.value,+cOutput.value]
  ];
  const maxOut=Math.max(1,...data.map(x=>x[3]));
  const rows=data.map(x=>{
    const feasible=x[1]<=E&&x[2]<=W;
    const resourceScore=Math.max(0,100-50*(x[1]/E)-50*(x[2]/W));
    const outputScore=x[3]/maxOut*100;
    const score=(weight/100)*outputScore+(1-weight/100)*resourceScore;
    return {name:x[0],e:x[1],w:x[2],o:x[3],score,feasible};
  });
  const feasible=rows.filter(x=>x.feasible);
  const best=feasible.length?feasible.reduce((a,b)=>b.score>a.score?b:a):null;
  document.getElementById("tradeoffOutput").innerHTML=rows.map(x=>`
    <tr><td>${x.name}</td><td>${x.e} MW</td><td>${x.w} kL</td><td>${x.o}</td>
    <td>${x.score.toFixed(1)}</td><td>${x.feasible?(best&&x.name===best.name?"RECOMMENDED":"FEASIBLE"):"OVER BUDGET"}</td></tr>
  `).join("");
  setText("tradeoffSummary",best
    ? `BEST TRADE-OFF: ${best.name} — score ${best.score.toFixed(1)}.`
    : "NO FEASIBLE STRATEGY — increase one or both resource budgets.");
}

document.getElementById("runRecycleBtn").addEventListener("click",runWaterRecyclingFinal);
document.getElementById("runTradeoffBtn").addEventListener("click",runTradeoffFinal);
runWaterRecyclingFinal();
runTradeoffFinal();

function seq(arr,dec){const n=arr.length,dp=Array(n).fill(1),p=Array(n).fill(-1);let b=0;for(let i=0;i<n;i++){for(let j=0;j<i;j++){if((dec?arr[j]>arr[i]:arr[j]<arr[i])&&dp[j]+1>dp[i]){dp[i]=dp[j]+1;p[i]=j;}}if(dp[i]>dp[b])b=i;}const out=[];for(let i=b;i>=0&&i!==-1;i=p[i])out.push(arr[i]);return out.reverse();}
function runSequenceAnalysis(){const a=sequenceInput.value.trim().split(/[,\s]+/).filter(Boolean).map(Number);if(!a.length||a.some(x=>!Number.isFinite(x))){alert("Enter a valid numeric sequence.");return;}const lis=seq(a,false),lds=seq(a,true);setText("lisLength",lis.length);setText("ldsLength",lds.length);setText("lisResult",lis.join(" → "));setText("ldsResult",lds.join(" → "));}


// FIX: wire the LIS/LDS Analyze button to the existing sequence analyzer.
const sequenceButton = document.getElementById("runSequenceBtn");
if (sequenceButton) {
  sequenceButton.addEventListener("click", runSequenceAnalysis);
}
// Phase 5 navigation: Water Recycling, Trade-off Evaluator and LIS/LDS use the existing section router.


// ==================== PHASE 6: SIMULATION + WHAT-IF REPORT ====================

let lastScenario = null;

function clampPercent(v){ return Math.max(-100, Math.min(100, Number(v)||0)); }
function applyScenarioPreset(type){
  const presets={
    custom:[0,0,0,0,0],
    energy:[-25,0,0,0,0],
    water:[0,-30,0,0,0],
    renewable:[0,0,25,0,10],
    emergency:[-10,-5,0,15,5]
  };
  const p=presets[type]||presets.custom;
  ["scenarioEnergy","scenarioWater","scenarioRenewable","scenarioDemand","scenarioRecycle"].forEach((id,i)=>setInputValue(id,p[i]));
}
function setInputValue(id,v){const e=document.getElementById(id);if(e)e.value=v;}

document.getElementById("scenarioType").addEventListener("change",e=>applyScenarioPreset(e.target.value));

function runScenario(){
  const eCh=clampPercent(document.getElementById("scenarioEnergy").value);
  const wCh=clampPercent(document.getElementById("scenarioWater").value);
  const rCh=clampPercent(document.getElementById("scenarioRenewable").value);
  const dCh=clampPercent(document.getElementById("scenarioDemand").value);
  const recCh=clampPercent(document.getElementById("scenarioRecycle").value);

  const base={
    energyGen:Number(state.totalEnergyGeneration),
    energyDemand:Number(state.totalEnergyDemand),
    waterAvail:Number(state.totalWaterAvailability),
    waterDemand:Number(state.totalWaterDemand),
    renewable:Number(state.renewableEnergy),
    recycled:Number(state.recycledWater),
    output:Number(state.totalUsefulOutput),
    score:Number(state.environmentalScore)
  };

  const scenario={
    energyGen:base.energyGen*(1+eCh/100),
    energyDemand:base.energyDemand*(1+dCh/100),
    waterAvail:base.waterAvail*(1+wCh/100),
    waterDemand:base.waterDemand*(1+dCh/100),
    renewable:base.renewable*(1+rCh/100),
    recycled:base.recycled*(1+recCh/100)
  };

  // Renewable generation cannot exceed total generation in the simulation.
  scenario.renewable=Math.min(scenario.renewable,scenario.energyGen);

  const energyBaseBal=base.energyGen-base.energyDemand;
  const energyBal=scenario.energyGen-scenario.energyDemand;
  const waterBaseBal=base.waterAvail+base.recycled-base.waterDemand;
  const waterBal=scenario.waterAvail+scenario.recycled-scenario.waterDemand;
  const baseRenew=base.energyGen?base.renewable/base.energyGen*100:0;
  const renew=scenario.energyGen?scenario.renewable/scenario.energyGen*100:0;

  const energyCoverage=Math.min(100,Math.max(0,scenario.energyGen/scenario.energyDemand*100));
  const waterCoverage=Math.min(100,Math.max(0,(scenario.waterAvail+scenario.recycled)/scenario.waterDemand*100));
  const score=Math.max(0,Math.min(100,
    base.score + (energyCoverage-100)*0.12 + (waterCoverage-100)*0.10 + (renew-baseRenew)*0.12
  ));

  // Output is kept as a decision-support estimate, not a physical prediction.
  const output=base.output*(0.70+0.30*Math.min(energyCoverage,waterCoverage)/100);

  lastScenario={base,scenario,energyBaseBal,energyBal,waterBaseBal,waterBal,baseRenew,renew,score,output};

  setText("simEnergyBalance",energyBal.toFixed(0)+" MW");
  setText("simWaterBalance",(waterBal/1000).toFixed(1)+"k L");
  setText("simRenewableShare",renew.toFixed(1)+"%");
  setText("simScore",score.toFixed(1));

  const rows=[
    ["Energy balance",energyBaseBal,energyBal,"MW",energyBal>=0],
    ["Water balance",waterBaseBal,waterBal/1000,"kL",waterBal>=0],
    ["Renewable share",baseRenew,renew,"% ",renew>=baseRenew],
    ["Useful output",base.output,output,"units",output>=base.output*0.95]
  ];
  document.getElementById("scenarioTable").innerHTML=rows.map(r=>{
    const delta=r[2]-r[1];
    return `<tr><td>${r[0]}</td><td>${Number(r[1]).toFixed(1)} ${r[3]}</td><td>${Number(r[2]).toFixed(1)} ${r[3]}</td><td>${delta>=0?"+":""}${delta.toFixed(1)}</td><td class="${r[4]?"status-served":"status-denied"}">${r[4]?"HEALTHY":"ATTENTION"}</td></tr>`;
  }).join("");

  const problems=[];
  if(energyBal<0) problems.push("energy deficit");
  if(waterBal<0) problems.push("water deficit");
  if(renew<baseRenew) problems.push("lower renewable share");
  setText("scenarioMessage",problems.length
    ? "Scenario completed — attention required: "+problems.join(", ")+"."
    : "Scenario completed — all primary resource balances remain healthy.");

  renderScenarioReport();
}

function renderScenarioReport(){
  if(!lastScenario)return;
  const s=lastScenario,b=s.base,n=s.scenario;
  const rows=[
    ["Energy balance",s.energyBaseBal,s.energyBal,"MW"],
    ["Water balance",s.waterBaseBal/1000,s.waterBal/1000,"kL"],
    ["Renewable share",s.baseRenew,s.renew,"%"],
    ["Useful output",b.output,s.output,"units"],
    ["Scenario score",b.score,s.score,"/100"]
  ];
  document.getElementById("reportTable").innerHTML=rows.map(r=>{
    const d=r[2]-r[1];
    return `<tr><td>${r[0]}</td><td>${r[1].toFixed(1)} ${r[3]}</td><td>${r[2].toFixed(1)} ${r[3]}</td><td>${d>=0?"+":""}${d.toFixed(1)}</td></tr>`;
  }).join("");

  const good=s.energyBal>=0&&s.waterBal>=0;
  const summary=good
    ? "The simulated scenario keeps both energy and water balances non-negative."
    : "The simulated scenario creates at least one resource deficit and should be reviewed before adoption.";
  setText("reportSummary",summary);
  setText("reportRecommendation",good
    ? "Recommendation: the scenario is operationally feasible within the modeled resource balances. Compare it with other what-if cases before selecting a final plan."
    : "Recommendation: do not treat this scenario as a final plan. Adjust resource allocations, demand, or recycling assumptions and run another scenario.");
}

document.getElementById("runScenarioBtn").addEventListener("click",runScenario);
document.getElementById("refreshReportBtn").addEventListener("click",renderScenarioReport);
applyScenarioPreset("custom");
runScenario();



// ==================== PHASE 7: FINAL CONTROL CENTER ====================
function projectState() {
  const keys = [
    "energywise_facilities","energywise_workloads","energywise_controls",
    "energywise_lastScenario","energywise_scenario"
  ];
  const state = {};
  keys.forEach(k => {
    const v = localStorage.getItem(k);
    if (v !== null) state[k] = v;
  });
  state.exportedAt = new Date().toISOString();
  return state;
}

function saveDownload(name, text, type) {
  const blob = new Blob([text], {type});
  const a = document.createElement("a");
  a.href = URL.createObjectURL(blob);
  a.download = name;
  document.body.appendChild(a);
  a.click();
  a.remove();
  setTimeout(()=>URL.revokeObjectURL(a.href), 500);
}

function runReadiness() {
  const checks = [
    ["HTML entry point", !!document.querySelector("main") || !!document.body],
    ["JavaScript loaded", true],
    ["Local storage available", (()=>{try{localStorage.setItem("__ew_test","1");localStorage.removeItem("__ew_test");return true}catch(e){return false}})()],
    ["Simulation module", !!document.querySelector("#simulation")],
    ["Graph module", !!document.querySelector("#network")],
    ["Optimization module", !!document.querySelector("#water")],
    ["Sequence module", !!document.querySelector("#sequences")],
    ["Report module", !!document.querySelector("#reports")]
  ];
  document.getElementById("readinessOutput").innerHTML = checks.map(([name,ok]) =>
    `<div class="readiness-item"><span>${name}</span><span class="readiness-ok">${ok ? "READY" : "CHECK"}</span></div>`
  ).join("");
  setText("stateSummary", `${checks.filter(x=>x[1]).length}/${checks.length} project checks passed.`);
}

function exportProjectState() {
  const state = projectState();
  saveDownload("energywise-project-state.json", JSON.stringify(state,null,2), "application/json");
  setText("stateSummary","Project state exported successfully.");
}

function importProjectState(file) {
  const reader = new FileReader();
  reader.onload = () => {
    try {
      const state = JSON.parse(reader.result);
      Object.keys(state).forEach(k => {
        if(k !== "exportedAt" && typeof state[k] === "string") localStorage.setItem(k,state[k]);
      });
      setText("stateSummary","State imported. Reload the page to apply all saved values.");
      setText("projectDataMode","Imported");
    } catch(e) {
      setText("stateSummary","Invalid JSON state file.");
    }
  };
  reader.readAsText(file);
}

function clearProjectState() {
  if(confirm("Clear saved EnergyWise browser state?")) {
    Object.keys(localStorage).filter(k=>k.toLowerCase().includes("energywise")).forEach(k=>localStorage.removeItem(k));
    setText("stateSummary","Local EnergyWise state cleared.");
    setText("projectDataMode","Local");
  }
}

document.getElementById("runReadinessBtn").addEventListener("click",runReadiness);
document.getElementById("exportStateBtn").addEventListener("click",exportProjectState);
document.getElementById("clearStateBtn").addEventListener("click",clearProjectState);
document.getElementById("importStateInput").addEventListener("change",e=>{
  if(e.target.files[0]) importProjectState(e.target.files[0]);
});
runReadiness();
