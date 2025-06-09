#!/bin/bash

# clear the log
echo "" > log.txt

# Start the server in the background
./fwc &
SERVER_PID=$!

# Give the server time to start (adjust if necessary)
sleep 2

# Run the client and pass command to its stdin
# (echo "load benchmarks/files/large_2.txt;" | ./fwc-cli)&
(echo "load benchmarks/files/small_3.txt;" | ./fwc-cli)&
CLIENTPID=$!
sleep 20
kill $CLIENTPID

wait $CLIENTPID 2>/dev/null
(echo "query benchmarks/files/small_3.txt the;" | ./fwc-cli)&
CLIENTPID=$!
sleep 5


# kill the server and client (if it doesn't exit on its own)
kill $SERVER_PID
wait $SERVER_PID 2>/dev/null

kill $CLIENTPID
wait $CLIENTPID 2>/dev/null

echo "Client and server terminated."

process_names=("fwc" "fwc-cli")

echo "killing process fwc or fwc-cli"

for name in "${process_names[@]}"; do
    # Find matching processes and kill them
    pids=$(pgrep -x "$name")
    if [ -n "$pids" ]; then
        for pid in $pids; do
            echo "Killing $name (PIDs: $pids)"
            kill $pids
        done
    else
        echo "No running process named $name found."
    fi
done
