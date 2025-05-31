import json
import os
import time
from typing import Dict
import statistics
from utils import start_server, stop_server, run_cli_commands
import csv
from datetime import datetime
import sys
from typing import List

CSV_OUTPUT_FOLDER =os.path.join("benchmarks","output") 
TEST_FILE = os.path.join("benchmarks","benchmarks.json") 
NUM_TRY = 3
BIN=os.path.join(".","fwc") 
BIN_CLI=os.path.join(".","fwc-cli") 

def run_test(test:Dict):
    results = []
    name = test["test"]
    commands = test["commands"]

    for _ in range(NUM_TRY):
        try:
            result = ""
            start = time.perf_counter()
            p = start_server(BIN)
            run_cli_commands(BIN_CLI, commands)
            end = time.perf_counter()
            stop_server(p)
            time.sleep(5)
            results.append((end - start)) 
        except Exception as e:
            print(e)
            print(f"------------ WARNING test {name} goes in exception!!! --------------")
            try:
                stop_server(p)
            except:
                pass
    return results


def run_benchmarks(selected_tests: List[str] = None):
    tests = json.load(open(TEST_FILE,"r"))
    print(f"Tests: {tests}")
    all_results = []
    for test in tests:
        if selected_tests and test["test"] not in selected_tests:
            continue

        results = run_test(test)
        avg = statistics.mean(results)
        test_name = test["test"]
        row = [
            test_name,
            NUM_TRY,
            len(results),
            f"{avg:.4f}",
            f"{min(results):.4f}",
            f"{max(results):.4f}",
            f"{statistics.stdev(results):.4f}" if len(results) > 1 else "0.00"
        ]
        print(f"✅ Completed {test_name} : Avg = {avg:.2f} ms, Min = {min(results):.2f}, Max = {max(results):.2f}, StdDev = {row[-1]}")

        all_results.append(row)
    return all_results


def log_to_csv(rows):
    timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    csv_file = os.path.join(CSV_OUTPUT_FOLDER, f"benchmark_{timestamp}.csv")

    csvfile = open(csv_file, mode='w', newline='')
    writer = csv.writer(csvfile)
    writer.writerow(["Test File", "Total Requests", "Successes", "Avg Latency (ms)", "Min", "Max", "Std Dev"])
    for row in rows:
        writer.writerow(row)

def get_selected_tests():
    # Check command-line arguments first
    if len(sys.argv) > 1:
        return sys.argv[1:]
    
    return None

if __name__ == "__main__":
    print("Starting Benchmark!")
    selected_tests = get_selected_tests()
    results = run_benchmarks(selected_tests=selected_tests)
    log_to_csv(results)
