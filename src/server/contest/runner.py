import subprocess as sp
from .sandbox_config import *
from .models import Problem as contest_problem
from pathlib import Path

cwd = Path.cwd()


class Runner():
    """To compile, execute and evaluate the
    submitted code against saved testcases """
    BASE_TEST_CASES_DIR = cwd / 'bank/testcases'
    BASE_SUBMISSION_DIR = cwd / 'contest/submissions'

    def __init__(self, submission):
        # Takes problem and user object as arguments
        self.submission = submission
        self.problem_id = self.submission.problem.problem_id
        self.user = self.submission.user.username
        self.testcase_dir = self.BASE_TEST_CASES_DIR / str(self.problem_id)
        self.inputs(self.testcase_dir)
        file_name = self.user + '_' + str(self.problem_id) + '.c'
        self.submission_file = self.BASE_SUBMISSION_DIR / file_name
        self.MAX_SCORE = contest_problem.objects.get(
            problem_id=self.problem_id).max_score

    def inputs(self, testcase_dir: Path):
        """ Prepare input files """
        self.input_files = sorted(testcase_dir.glob('input*'))

    def check_all(self):
        """Save return code for every test case in tests[]
        Return code : 0=successful;correct answer
                      7=wrong answer
                      else error
        """
        self.tests = []
        for case in self.input_files:
            self.check_result(case)
        self.submission.testcase_codes = str(self.tests)
        self.submission.save()

        # for testing
        print("\n\nTEST CASES RESPONSES : ", end='')
        print(self.tests)

    def check_result(self, input_file: Path):
        """
            self.tests[] stores return codes of all cases
            0 : correct answer
            1 : sandbox failure
            2 : runtime error
            3 : memory limit exceeded
            4 : time limit exceeded
            5 : sandbox child processes exceeded
            6 : compilation error
            7 : incorrect answer
        """
        result = self.execute(self.submission_file, input_file)
        try:
            output = result['output'].strip()
            output_file_name = input_file.name.replace('input', 'output')
            output_file_path = self.testcase_dir / output_file_name
            output_file = output_file_path.open('r')
            expected_output = output_file.read().strip()
            if output == expected_output:
                # correct answer
                self.tests.append(0)
            else:
                # incorrect answer
                self.tests.append(7)

        except KeyError:
            # compilation/runtime error
            self.tests.append(result['error'])

    def execute(self, file_path: Path, input_file_path: Path):
        """Run files on local computer.
        file_path is the name of the file with absolute path """

        # executable is stored in /sandbox/jail/executable
        EXECUTABLE_FILE = "executable_" + str(self.submission.id)
        executable_path = JAIL_DIR / EXECUTABLE_FILE
        JAIL_DIR.mkdir(parents=True, exist_ok=True)  # if the dir doesn't exist

        try:
            # run the terminal command - compile (gcc path/name.c -o path/name)
            process_compile = sp.run([
                "gcc",
                str(file_path),
                "-o",
                str(executable_path),
                "--static"
            ],
                check=True, stdout=sp.PIPE, stderr=sp.PIPE)
            # debug statements: 'process_compile' holds returned process data
            print("\nprocess_compile stdout:", process_compile.stdout)
            print("process_compile stderr:", process_compile.stderr)

        # if compilation is not clear, then handle it
        except sp.CalledProcessError as e:
            result = {}
            # print ("\nCompilation Error\n")
            # print(e)
            result['error'] = 6
            return result
            # exit(1)

        # if compilation is clear, try to run the file
        else:
            # check if an input file exists. take input from it if exists
            try:
                input_file = input_file_path.open()
            except FileNotFoundError:
                input_file = sp.PIPE

            result = self.safe_execution(input_file_path)
            return result

    def score_obtained(self):
        """Traverse thru test case responses to calculate total score.
        score = (score alloted to the problem) * (fraction of correct answers)
        """
        responses = self.tests
        correct_answer = responses.count(0)
        score = correct_answer / len(responses) * self.MAX_SCORE
        self.submission.score = score
        self.submission.save()
        print("SCORE : ", end='')
        print(score, end="\n\n")
        return score

    def safe_execution(self, input_file_path: Path):
        """Use sandbox to execute compiled executables of submitted C code."""

        INPUT_FILE = input_file_path
        OUTPUT_FILE = SANDBOX_PATH / ("output_" + str(self.submission.id))
        EXECUTABLE_FILE = JAIL_DIR / ("executable_" + str(self.submission.id))
        result = {}

        cmd = [
            "sudo",
            str(EXE),
            MEMORY_LIMIT,
            TIME_LIMIT,
            MAX_PIDS,
            MEMORY_CGROUP,
            CPUACCT_CGROUP,
            PIDS_CGROUP,
            str(JAIL_DIR),
            EXECUTABLE_FILE.name,
            str(INPUT_FILE),
            str(OUTPUT_FILE),
            str(WHITELIST),
            UID,
            GID
        ]
        # process = sp.run(cmd,check=True,stdout=sp.PIPE,stderr=sp.PIPE)
        try:
            sp.check_call(cmd, stdout=sp.PIPE, timeout=2)
            # timeout = time alloted to sandbox for execution.
            # If exceeded, returns TLE error
            output = OUTPUT_FILE.open('r')
            result['output'] = output.read()
        except sp.CalledProcessError as e:
            # 2-runtime error, 3-memory limit exceeded, 4-time limit exceeded
            result['error'] = e.returncode
            print(e.returncode)
        except sp.TimeoutExpired as e:
            # sp timeout
            result['error'] = 4

        # remove executable and output files created
        sp.run(["rm", "-f", str(OUTPUT_FILE), str(EXECUTABLE_FILE)],
               check=True, stdout=sp.PIPE, stderr=sp.PIPE)

        return result
