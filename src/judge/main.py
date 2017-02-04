import jailer
import compiler
import tester

class judge:
	def runCode(self,code,problemcode):
		result = {}
		result['error'] = None
		result['score'] = 0
		jail = new jailer("path/to/jail")
		try:
			compiledcode = compiler.compile(self.code)
		except CompilationError as e:
			result['error'] = "Compilation Error"
			return result
		try:
			output = jail.run(compiledcode)
		except TimeLimitException as e:
			result['error'] = "Time Limit Exceeded"
			return result
		except RuntimeException as e:
			result['error'] = "Runtime Error"
			return result
		problem = new tester(self.problemcode)
		result['score'] = problem.test(output)
		return result
