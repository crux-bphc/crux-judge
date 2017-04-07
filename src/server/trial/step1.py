import os
import subprocess

# runs files on local computer. file_path is the name of the file with absolute path
def execute_local(file_path):
    #file_path cotains the name of the file with path and extension
    file_path_without_extension = os.path.splitext(file_path)[0]
    # contains directory part of file_path
    file_dir = os.path.dirname(file_path)
    # name of the file
    file_name = os.path.basename(file_path)
    # name of the file without extension
    file_name_without_extension = file_name.split('.')[0]

    #printing for testing purposes
    print ("file_path: "+file_path)
    print ("file_path_without_extension: "+file_path_without_extension)
    print ("file dir : "+file_dir)
    print ("file name : "+file_name)
    print ("file name without extension : "+file_name_without_extension)

    try:
        # runs the terminal command - compile (gcc path/name.c -o path/name)
        process_compile = subprocess.run(["gcc",file_path,"-o", file_path_without_extension],check=True,stdout=subprocess.PIPE,stderr=subprocess.PIPE)
        # debug statements: 'process_compile' var holds returned process data
        print ("\nprocess_compile stdout:",process_compile.stdout)
        print ("process_compile stderr:",process_compile.stderr)

    #if compilation is not clear, then handle it
    except subprocess.CalledProcessError as e:
        print ("\nCompilation Error\n")
        # print(e)
        exit(1)

    #if compilation is clear, try to run the file
    else:
        #check if an input file exists. take input from it if exists
        try:
            input_file=open('input')
        except FileNotFoundError:
            input_file=subprocess.PIPE

        try:
            # runs terminal command - execute (./name)
            process_exec = subprocess.run([file_path_without_extension],check=True,timeout=10,stdin=input_file,stdout=subprocess.PIPE,stderr=subprocess.PIPE)
            # debug statements: 'process_exec' var holds returned process data
            print ("\n\nprocess_exec stdout:",process_exec.stdout)
            print ("process_exec stderr:",process_exec.stderr)

        #if program execute is unclear, handle it here
        except subprocess.CalledProcessError:
            print ("Execution Error")
            exit(1)

        #if program execution is successful, handle it here
        else:
            print ("Execution Successful")
            exit(0)

    return

# demo function instance as proof of concept
execute_local(os.getcwd()+"/hello_world.c")
