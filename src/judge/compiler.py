import hashlib
import os
import subprocess

class CompilationError(Exception):
    def __init__(self,e):
        self.e = e
    def __str__(self):
        return repr(self.e)

def compile(f):
    filename_with_extension = os.path.basename(f)
    name_of_binary_executable = hashlib.md5(open(f,'r').read().encode('utf-8')).hexdigest()
    try:
        result = subprocess.run(["gcc",filename_with_extension,"-o",name_of_binary_executable],stdout=subprocess.PIPE,stderr=subprocess.PIPE,check=True)
    except subprocess.CalledProcessError as e:
        raise CompilationError(e)
