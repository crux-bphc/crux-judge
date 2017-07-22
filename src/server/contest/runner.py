import os
import fnmatch
import subprocess
from django.contrib.auth.models import User
from trial.models import Problem as all_problems

class runner():
    """To compile, execute and evaluate the
    submitted code against saved testcases """

    BASE_TEST_CASES_DIR = os.getcwd() + '/contest/testcases'
    BASE_SUBMISSION_DIR = os.getcwd() + '/contest/submissions'

    def __init__(self,problem,user):
        # Takes problem and user object as arguments
        self.problem_id = problem.problem_id
        self.user = user.username
        self.testcase_dir = self.BASE_TEST_CASES_DIR + "/" + str(self.problem_id)
        self.inputs(self.testcase_dir)
        self.submission_file = self.BASE_SUBMISSION_DIR + '/' + self.user + '_' + str(self.problem_id) + '.c'

    def inputs(self,testcase_dir):
        # Prepare input files
        self.input_files = fnmatch.filter(os.listdir(testcase_dir),'input*')
        self.input_files.sort()

    def check_all(self):
    # traverses thru all input cases and saves return code for all cases in tests[]
    # Return code : 1=successful;correct answer
    #               0=wrong answer
    #               -1=compilation/runtime error
        self.tests=[]
        for case in self.input_files:
            path = self.testcase_dir + '/' + case
            self.check_result(path)

        # for testing
        print("\n\nTEST CASES RESPONSES : ",end='')
        print(self.tests,end='\n\n')

    def check_result(self,input_file):
        result = self.execute(self.submission_file,input_file)
        try:
            output = result['output'].decode()
            output_file_path = self.testcase_dir + '/' + 'output' + input_file.split('input')[1]
            output_file = open(output_file_path,'r')
            expected_output = output_file.read()
            if output == expected_output :
                # correct answer
                self.tests.append(1)
            else:
                # incorrect answer
                self.tests.append(0)

        except KeyError:
            # compilation/runtime error
            self.tests.append(-1)
            print("Test cases responses : ",end='')
            print(result['error'])

    def execute(self,file_path,input_file_path):
        # runs files on local computer. file_path is the name of the file with absolute path

        result={}

        #file_path cotains the name of the file with path and extension
        file_path_without_extension = os.path.splitext(file_path)[0]
        # contains directory part of file_path
        file_dir = os.path.dirname(file_path)
        # name of the file
        file_name = os.path.basename(file_path)
        # name of the file without extension
        file_name_without_extension = file_name.split('.')[0]

        #printing for testing purposes
        # print ("file_path: "+file_path)
        # print ("file_path_without_extension: "+file_path_without_extension)
        # print ("file dir : "+file_dir)
        # print ("file name : "+file_name)
        # print ("file name without extension : "+file_name_without_extension)

        try:
            # runs the terminal command - compile (gcc path/name.c -o path/name)
            process_compile = subprocess.run(["gcc",file_path,"-o", file_path_without_extension],check=True,stdout=subprocess.PIPE,stderr=subprocess.PIPE)
            # debug statements: 'process_compile' var holds returned process data
            # print ("\nprocess_compile stdout:",process_compile.stdout)
            # print ("process_compile stderr:",process_compile.stderr)

        #if compilation is not clear, then handle it
        except subprocess.CalledProcessError as e:
            print ("\nCompilation Error\n")
            print(e)
            result['error']='Compilation Error'
            return result
            # exit(1)

        #if compilation is clear, try to run the file
        else:
            #check if an input file exists. take input from it if exists
            try:
                input_file=open(input_file_path)
            except FileNotFoundError:
                input_file=subprocess.PIPE

            try:
                # runs terminal command - execute (./name)
                process_exec = subprocess.run([file_path_without_extension],check=True,timeout=10,stdin=input_file,stdout=subprocess.PIPE,stderr=subprocess.PIPE)
                # debug statements: 'process_exec' var holds returned process data
                # print ("\n\nprocess_exec stdout:",process_exec.stdout)
                # print ("process_exec stderr:",process_exec.stderr)
                result['output']=process_exec.stdout
                return result

            #if program execute is unclear, handle it here
            except subprocess.CalledProcessError:
                print ("Execution Error")
                result['error']='Execution Error'
                return result

            else:
                print ("Execution Successful")
