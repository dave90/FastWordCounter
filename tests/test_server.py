from tests.utils import start_server, stop_server, run_cli_commands
import time
import pytest

BIN="./fwc"
BIN_CLI = "./fwc-cli"

USE_VALGRIND = True
USE_LEAKS = False

@pytest.fixture
def server():
    p = start_server(BIN, use_valgrind=USE_VALGRIND)
    time.sleep(2)  # Consider reducing this if your server starts quickly
    yield p
    #stop_server(p, use_leaks=True)
    stop_server(p, use_valgrind=USE_VALGRIND, use_leaks=USE_LEAKS)
    time.sleep(2)


@pytest.mark.parametrize("cmds, expecteds", [
    (
     ["ciao;","clear;","command not exist;"],
     [
        "Command not recognized: ciao",
        "DB clean completed",
        "Command not recognized: command not exist"
     ]
    ),
    (
     ["ciao;","command not exist;"],
     [
        "Command not recognized: ciao",
        "Command not recognized: command not exist"
     ]
    ),
])
def test_correct_and_wrong_commands(server,cmds, expecteds):
    result = run_cli_commands(BIN_CLI, cmds)
    assert len(result) == len(expecteds)        
    assert result == expecteds

@pytest.mark.parametrize("files", [
    (
     ["tests/files/small_1.txt"],
    ),
    (
     ["tests/files/small_1.txt","tests/files/small_2.txt" ],
    ),
])
def test_load_file(server, files):
    for file in files[0]:
        cmd = f"load {file};"
        result = run_cli_commands(BIN_CLI, [cmd])
        print(result)
        assert len(result) == 1
        assert result[0] == f"Loading file {file} completed"

def test_query_file():
    pass


@pytest.mark.parametrize("files", [
    (
     ["tests/files/small_1.txt"],
    ),
    (
     ["tests/files/small_1.txt","tests/files/small_2.txt" ],
    ),
])
def test_clear_load_file(server, files):
    cmds = [f"load {file};" for file in files[0] ]
    result = run_cli_commands(BIN_CLI, cmds)
    print(result)
    assert len(result) == len(cmds)
    assert result == [f"Loading file {file} completed" for file in files[0] ]
    cmd = f"clear;"
    result = run_cli_commands(BIN_CLI, [cmd])
    expected_result = "DB clean completed"
    assert len(result) == 1
    assert result[0] == expected_result



@pytest.mark.parametrize("query_words", [
    (
     {
        "files_to_load" : ["tests/files/small_1.txt"],
        "file_querys" : {
            "file" : "tests/files/small_1.txt",
            "query": [ ("ciao",3), ("nonesiste",0)]
        }
     }
    ),
])
def test_query(server, query_words):
    print(query_words)
    cmds = [f"load {file};" for file in query_words["files_to_load"] ]
    print(f"Running: [{cmds}]")
    _ = run_cli_commands(BIN_CLI, cmds)
    f = query_words["file_querys"]["file"]
    for wc in query_words["file_querys"]["query"]:
        cmd = f"query {f} {wc[0]};"
        res = run_cli_commands(BIN_CLI, [cmd])
        assert len(res) == 1
        assert res[0] == f"{wc[0]}: {wc[1]}" 


