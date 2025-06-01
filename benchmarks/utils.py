import time
import os
import subprocess
import signal
import re
from typing import List
import select
import socket
import zipfile

def start_server(executable_path: str):
    command = [executable_path]

    process = subprocess.Popen(
        command,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        preexec_fn=os.setsid  # So we can kill the whole process group
    )
    return process

def stop_server(process:subprocess.Popen):
    """Terminates the server process."""

    os.killpg(os.getpgid(process.pid), signal.SIGTERM)
    process.wait()

def wait_for_port_release(port, host='127.0.0.1', timeout=10.0):
    start_time = time.time()
    while time.time() - start_time < timeout:
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
            try:
                s.connect((host, port))
                # Still in use
            except (ConnectionRefusedError, OSError):
                return  # Port is free
        time.sleep(0.1)
    raise TimeoutError(f"Port {port} still in use after {timeout:.1f}s")

def run_cli_commands(cli_path:str, commands:List[str]):
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


def unzip_tests(test_zip_file):
    print(f"Unzipping test file: {test_zip_file}")
    with zipfile.ZipFile(test_zip_file, 'r') as zip_ref:
        zip_ref.extractall(os.path.dirname(test_zip_file))
    print("✅ Test files extracted.")