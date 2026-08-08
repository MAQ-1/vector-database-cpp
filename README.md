# VectorDB

A high-performance, in-memory vector database written in C++ with an HTTP REST API. Supports multiple indexing algorithms for approximate and exact nearest-neighbor search over high-dimensional float vectors.

---

## Features

### Implemented
- **4 search algorithms**: Brute Force, KD-Tree, HNSW, LSH
- **3 similarity metrics**: Euclidean Distance, Cosine Similarity, Dot Product
- **REST API** via [cpp-httplib](https://github.com/yhirose/cpp-httplib) with JSON responses
- **Persistence**: Save/load vectors to/from plain-text file (`vectors.txt`)
- **Benchmarking**: Built-in `/benchmark` endpoint runs 1000-iteration timing across all algorithms
- **Auto-seeding**: Generates 10,000 random 128-dim vectors on first run if database is empty
- **KNN search**: Both sort-based and heap-based (optimized) implementations

### Planned / In Progress
- HNSW `remove()` — currently a stub (`// TODO`)
- True random-projection LSH (current implementation uses a simple sum-based hash)
- Multi-dimensional HTTP search (current `/search` endpoint only accepts 2D queries via `x`, `y` params)
- Persistent index snapshots (indexes are rebuilt from `vectors.txt` on each load)
- Unit test framework integration (current tests are manual only)
- CMake build system
- License

---

## Architecture

```
VectorDB/
├── main.cpp                  # HTTP server, route handlers, auto-seed logic
├── include/
│   ├── VectorDatabase.h      # Core DB interface, SearchAlgorithm enum
│   ├── VectorRecord.h        # Record: id (int), embedding (vector<float>), metadata (string)
│   ├── Similarity.h          # Static similarity/distance functions
│   ├── Metric.h              # Enum: COSINE, EUCLIDEAN, DOT_PRODUCT
│   ├── SearchResult.h        # SearchResult struct + min-heap comparator
│   ├── Benchmark.h           # Benchmark timing struct
│   ├── KDTree.h / KDNode.h   # KD-Tree index
│   ├── HNSW.h / HNSWNode.h   # HNSW graph index
│   └── LSH.h                 # LSH bucket index
├── src/
│   ├── VectorDatabase.cpp    # Insert/remove/search dispatch, benchmark, knnSearch
│   ├── KDTree.cpp            # KD-Tree: insert, remove, nearestNeighbor
│   ├── HNSW.cpp              # HNSW: insert, efSearch, greedySearch, pruneNeighbors
│   ├── LSH.cpp               # LSH: insert, search, remove
│   ├── similarity.cpp        # euclideanDistance, cosine, dotProduct, manhattan, magnitude
│   └── VectorRecord.cpp      # VectorRecord constructor
├── test/
│   └── test_main.cpp         # Manual benchmark test (10,000 2D vectors)
├── external/
│   ├── httplib.h             # cpp-httplib (header-only)
│   └── nlohmann/json.hpp     # nlohmann JSON (header-only)
└── vectors.txt               # Persistence: pipe-delimited, 128-dim float vectors
```

### Index Architecture

All three indexes (`KDTree`, `HNSW`, `LSH`) are maintained in sync with the primary `records` vector inside `VectorDatabase`. Every `insert` and `remove` operation updates all indexes simultaneously.

```
VectorDatabase
├── vector<VectorRecord> records   ← source of truth
├── KDTree kdTree                  ← exact NN, O(log n) avg
├── HNSW hnsw                      ← approx NN, O(log n)
└── LSH lsh                        ← approx NN, O(1) bucket lookup
```

---

## Algorithms

### Brute Force
Exhaustive linear scan over all records. Exact results. Used as the correctness baseline.
- Time: O(n)

### KD-Tree
Binary space-partitioning tree. Splits on alternating dimensions. Includes plane-distance pruning for branch elimination.
- Time: O(log n) average, O(n) worst case (high dimensions)
- Best for: low-to-medium dimensional vectors

### HNSW (Hierarchical Navigable Small World)
Proximity graph with multiple layers. Entry point at top layer, greedy descent to layer 0.
- `M = 4` connections per node
- `efSearch = 50` candidates during search
- `efConstruction = 50` during insert
- Level assignment: `floor(-ln(uniform(0,1)) * mL)`, capped at `maxLevel = 16`
- Candidate pruning during `efSearch` for early termination
- Time: O(log n)

### LSH (Locality-Sensitive Hashing)
Buckets vectors by a hash of their embedding sum (`(int)(sum / 10)`). Searches only within the matching bucket using exact Euclidean distance.
- Time: O(1) bucket lookup + O(bucket_size) scan
- Note: Current hash is not random-projection LSH — collision quality depends on vector distribution

---

## Benchmark Results

Measured on 10,000 vectors × 128 dimensions, 1000 search iterations, on a single machine.

| Algorithm   | Avg Time (µs) | Type        |
|-------------|---------------|-------------|
| LSH         | ~43           | Approximate |
| KD-Tree     | ~53           | Exact       |
| HNSW        | ~798          | Approximate |
| Brute Force | ~90,601       | Exact       |

> Results from the built-in `/benchmark` endpoint. Hardware-dependent — run `/benchmark` for results on your machine.

---

## REST API

Base URL: `http://localhost:8080`

### `GET /`
Health check.

**Response**
```json
{ "message": "VectorDB is running!" }
```

---

### `GET /stats`
Returns record count and index status.

**Response**
```json
{
  "record_count": 10000,
  "kd_tree": "active",
  "hnsw": "active",
  "lsh": "active"
}
```

---

### `POST /insert`
Insert a new vector record.

**Request Body**
```json
{
  "id": 1,
  "embedding": [0.1, 0.5, 0.3, ...],
  "metadata": "my vector"
}
```

**Response**
```json
{ "message": "Record inserted successfully" }
```

---

### `GET /items`
Returns all stored records.

**Response**
```json
[
  { "id": 1, "embedding": [0.1, 0.5, ...], "metadata": "my vector" },
  ...
]
```

---

### `DELETE /delete`
Remove a record by ID.

**Query Params**: `id` (int)

**Example**: `DELETE /delete?id=42`

**Response**
```json
{ "message": "Record deleted successfully" }
```

---

### `GET /search`
Find the nearest neighbor to a 2D query point.

**Query Params**: `x` (float), `y` (float), `algorithm` (string: `brute`, `kdtree`, `hnsw`, `lsh`)

**Example**: `GET /search?x=100.5&y=200.3&algorithm=hnsw`

**Response**
```json
{
  "id": 4821,
  "embedding": [100.1, 199.8],
  "metadata": "Random Vector",
  "score": 0.812
}
```

> **Limitation**: This endpoint only supports 2D queries. For higher-dimensional search, use `knnSearch` directly via the C++ API.

---

### `GET /benchmark`
Runs 1000 search iterations for each algorithm and returns timing results.

**Response**
```json
{
  "brute_force_us": 90601.4,
  "kd_tree_us": 53.2,
  "lsh_us": 43.1,
  "hnsw_us": 798.3
}
```

---

## Persistence Format

Vectors are saved to and loaded from `vectors.txt` in pipe-delimited plain text:

```
id|f1,f2,f3,...,f128|metadata
```

**Example**
```
1|0.132,0.874,0.021,...,0.553|Random Vector
2|0.991,0.004,0.762,...,0.118|Random Vector
```

> Indexes are not persisted — they are rebuilt in memory from `vectors.txt` on each load.

---

## C++ API

```cpp
VectorDatabase db;

// Insert
db.insert(VectorRecord(1, {0.1f, 0.5f, 0.3f}, "my vector"));

// Nearest neighbor search
SearchResult result = db.search({0.1f, 0.5f, 0.3f}, SearchAlgorithm::HNSW, Metric::EUCLIDEAN);

// KNN search (top-k)
vector<SearchResult> results = db.knnSearch({0.1f, 0.5f}, 5, Metric::COSINE);
vector<SearchResult> results = db.knnSearchOptimized({0.1f, 0.5f}, 5, Metric::COSINE);

// Remove
db.remove(1);

// Persistence
db.saveToFile("vectors.txt");
db.loadFromFile("vectors.txt");

// Benchmark
Benchmark b = db.benchmark({0.1f, 0.5f});
```

---

## Building

No CMake is currently provided. Compile with any C++17-compatible compiler, linking all `src/` files and including `include/` and `external/`.

**Example (g++)**
```bash
g++ -std=c++17 -O2 -Iinclude -Iexternal main.cpp src/*.cpp -o VectorDB
```

**Example (MSVC)**
```
cl /std:c++17 /O2 /Iinclude /Iexternal main.cpp src\*.cpp /Fe:VectorDB.exe
```

---

## Dependencies

| Library | Version | Usage |
|---|---|---|
| [cpp-httplib](https://github.com/yhirose/cpp-httplib) | header-only | HTTP server |
| [nlohmann/json](https://github.com/nlohmann/json) | header-only | JSON serialization |

Both are vendored under `external/`. No package manager required.

---

## Roadmap

- [ ] HNSW `remove()` implementation
- [ ] Random-projection LSH
- [ ] N-dimensional `/search` endpoint
- [ ] Persistent index snapshots
- [ ] CMakeLists.txt
- [ ] Unit test framework (Google Test or Catch2)
- [ ] Concurrent insert/search (thread safety)
- [ ] gRPC interface
- [ ] License

---

## License

To be decided.
