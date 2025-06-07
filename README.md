

# 🚀 Fast Word Counter (FWC)

A high-performance word counting tool designed to process large text files quickly using C. Built for experimentation, speed, and scalability.

## 📦 Features

- Load and unload text files into memory  
- Fast word querying from specific or all loaded files  
- Clear all loaded data  
- Server-client architecture (planned)  
- Lightweight and extensible  

---

## 🛠️ Commands

All commands must end with a semicolon (`;`).


### 🔹 Load a file
```bash
load tests/files/small_1.txt;
```

### 🔹 Unload a file
```bash
unload tests/files/small_1.txt;
```

### 🔹 Unload a file
```bash
clear;
```

### 🔹 Query

Query a specific file:
```bash
query tests/files/small_1.txt ciao;
```

Query across all loaded files:
```bash
query * ciao;
```

## 🐳 Docker
Build docker
```bash
docker build -t fwc .
```

Run the service
```bash
docker run -p 8124:8124 --rm fwc
```

Run tests inside Docker
```bash
docker run -p 8124:8124 --rm fwc make test
```

## CLI
Build CLI:
```bash
make rebuild
```

Run it:
```bash
./fwd-cli
```

## 🧪 Benchmarking
This project includes a benchmarking suite to evaluate the performance of various CLI test scenarios using the internal server (fwc) and CLI tool (fwc-cli).

Benchmarks are located in the benchmarks/ directory, with:
- benchmarks.json: JSON file containing the list of benchmark test cases.
- output/: Folder where CSV files of benchmark results are saved.

### 🏁 Running the Benchmark

Build:
```bash
make rebuild 
```

To execute all tests defined in benchmarks.json:
```bash
python benchmarks/benchmarks.py 
```

Or:
```bash
make benchmarks 
```

You can run a single test by passing its name as a command-line argument:
```bash
python benchmarks/benchmarks.py "load medium 1"
```

## ✅ TODO Roadmap
 - Implement slave failover
 - Persistent disk storage
 - Use epool for server side

## 🧰 Utilities

List processes attached to a specific port (MacOs):
```bash
sudo lsof -iTCP -sTCP:LISTEN -n -P
```
