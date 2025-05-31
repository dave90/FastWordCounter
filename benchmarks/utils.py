import time
import os
import subprocess
import signal
import re
from typing import List
import select

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
