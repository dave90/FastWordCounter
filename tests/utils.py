import time
import os
import subprocess
import signal
import re
from typing import List
import select

VALGRIND_LOG = "valgrind.out"

# def start_server(executable_path:str):
#     """Starts the server (optionally under Valgrind)."""
#     command = [executable_path]

#     process = subprocess.Popen(
#         command,
#         stdout=subprocess.PIPE,
#         stderr=subprocess.PIPE,
#         preexec_fn=os.setsid  # So we can kill the whole process group
#     )

#     # Wait a moment to ensure the server starts and binds to the port
#     time.sleep(1)

#     return process

def start_server(executable_path: str, use_valgrind: bool = False):
    """Starts the server (optionally under Valgrind)."""
    if use_valgrind:
        command = [
            "valgrind",
            "--leak-check=full",
            "--show-leak-kinds=all",
            "--log-file=" + VALGRIND_LOG,
            executable_path
        ]
    else:
        command = [executable_path]

    process = subprocess.Popen(
        command,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        preexec_fn=os.setsid  # So we can kill the whole process group
    )

    # Wait a moment to ensure the server starts and binds to the port
    time.sleep(1)

    return process

def parse_leaks(leak_out:str):
    match = re.search(r"\s+(\d+) leaks for (\d+) total leaked bytes", leak_out, re.IGNORECASE)
    if match:
        return [int(match.group(1)),int(match.group(2)) ]
    return 1,0

def parse_valgrind_log():
    """Parses valgrind log file for memory leak summary."""
    with open(VALGRIND_LOG, "r") as f:
        log = f.read()

    match = re.search(r"definitely lost: (\d+)", log)
    if match:
        lost_bytes = int(match.group(1))
        leaks = 0 if lost_bytes == 0 else 1
        return leaks, lost_bytes

    return 1, 0  # Assume leak if can't parse

def stop_server(process:subprocess.Popen, use_leaks:bool= False, use_valgrind=False):
    """Terminates the server process."""

    if use_leaks:
        # Run the leaks tool on the server's PID
        leaks_result = subprocess.run(
            ["leaks", str(process.pid)],
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True
        )
        print("========== leaks output ==========")
        print(leaks_result.stdout)
        f = open("leaks.out","w")
        f.write(leaks_result.stdout)
        print("==================================")
        leaks, tot_bytes = parse_leaks(leaks_result.stdout)
    os.killpg(os.getpgid(process.pid), signal.SIGTERM)
    process.wait()

    if use_leaks:
        assert leaks == 0

    if use_valgrind:
        print("========== Valgrind output ==========")
        with open(VALGRIND_LOG, "r") as f:
            valgrind_output = f.read()
            print(valgrind_output)
        print("=====================================")
        leaks, tot_bytes = parse_valgrind_log()
        assert leaks == 0, f"Memory leaks detected: {tot_bytes} bytes"


def run_cli_commands(cli_path:str, commands:List[str], timeout=5):

    # Start the CLI process
    process = subprocess.Popen(
        [cli_path],
        stdin=subprocess.PIPE,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
        bufsize=1,  
    )

    results =[]

    for command in commands:
        # Ensure each command ends with a newline and ;
        command_input = command 

        if not command.endswith(";"):
            command += ";"
        command += "\n"
        try:
            # Send  command and collect output
            print(f"sending {command}")
            process.stdin.write(command_input)
            process.stdin.flush()
            line = process.stdout.readline()
            print(f"received {line}")
            results.append(line.strip())
        except: 
            process.kill()

    process.kill()

    return results


