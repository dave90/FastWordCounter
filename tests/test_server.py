from tests.utils import start_server, stop_server, run_cli_commands,str2bool
import time
import pytest
import os
from collections import namedtuple
from typing import List

Query = namedtuple("Query", ["word", "expected_count"])
FileQuery = namedtuple("FileQuery", ["file", "queries"])

BIN="./fwc"
BIN_CLI = "./fwc-cli"

USE_VALGRIND = str2bool(os.getenv("USE_VALGRIND", "true"))
USE_LEAKS = str2bool(os.getenv("USE_LEAKS", "false"))

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


def __load_files(files:List[str]):
    cmds = [f"load {file};" for file in files]
    print(f"Loading files: {files}")
    _ = run_cli_commands(BIN_CLI, cmds)

def __unload_files(files:List[str]):
    cmds = [f"unload {file};" for file in files]
    print(f"Loading files: {files}")
    _ = run_cli_commands(BIN_CLI, cmds)


def __run_queries_and_assert(file:str, queries:List[Query], file_not_exist=False):
    for query in queries:
        cmd = f"query {file} {query.word};"
        res = run_cli_commands(BIN_CLI, [cmd])
        assert len(res) == 1
        if file_not_exist is False:
            assert res[0] == f"{query.word}: {query.expected_count}"
        else:
            assert res[0] == f"{query.word}: 0"


@pytest.mark.parametrize("files_to_load, file_queries", [
    (
        ["tests/files/small_1.txt"],
        [
            FileQuery("tests/files/small_1.txt", [
                Query("ciao", 3),
                Query("nonesiste", 0),
            ])
        ]
    ),
    (
        ["tests/files/small_1.txt", "tests/files/small_2.txt"],
        [
            FileQuery("tests/files/small_1.txt", [
                Query("ciao", 3),
                Query("the", 0),
            ]),
            FileQuery("tests/files/small_2.txt", [
                Query("ciao", 1),
                Query("the", 9),
            ]),
            FileQuery("*", [
                Query("ciao", 4),
                Query("the", 9),
            ]),
        ]
    ),
])
def test_query(server, files_to_load:List, file_queries:List[FileQuery]):
    __load_files(files_to_load)
    for fq in file_queries:
        __run_queries_and_assert(fq.file, fq.queries)


@pytest.mark.parametrize("files_to_load, file_queries", [
    (
        ["tests/files/small_1.txt"],
        [
            FileQuery("tests/files/small_1.txt", [
                Query("ciao", 3),
                Query("nonesiste", 0),
            ])
        ]
    ),
    (
        ["tests/files/small_1.txt", "tests/files/small_2.txt"],
        [
            FileQuery("tests/files/small_1.txt", [
                Query("ciao", 3),
                Query("the", 0),
            ]),
            FileQuery("tests/files/small_2.txt", [
                Query("ciao", 1),
                Query("the", 9),
            ]),
        ]
    ),
])
def test_query_unload(server, files_to_load:List, file_queries:List[FileQuery]):
    __load_files(files_to_load)
    for fq in file_queries:
        __run_queries_and_assert(fq.file, fq.queries)

    __unload_files(files_to_load)
    for fq in file_queries:
        __run_queries_and_assert(fq.file, fq.queries, file_not_exist=True)



