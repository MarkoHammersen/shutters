from pickle import FALSE
from shutil import copyfile
from subprocess import check_output, CalledProcessError
import sys
import os
import platform
import subprocess

print(check_output(["pip", "install", "gitpython"]))

import git
from git import Repo

if True != os.path.isdir('State-Oriented-Programming'):
    print("'State-Oriented-Programming' not yet available, so clone it")
    Repo.clone_from("https://github.com/QuantumLeaps/State-Oriented-Programming.git", "State-Oriented-Programming")
else:
    print("'State-Oriented-Programming' already available!")




