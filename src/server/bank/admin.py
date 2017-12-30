import os
import fnmatch
from django.core.exceptions import ObjectDoesNotExist
from django.contrib import admin
from .models import Problem
from django.conf.urls import url
from django.http import HttpResponse
from django.shortcuts import render
from bank.models import Problem as bank_problems
from django.core.files import File

BASE_TEST_CASES_DIR = os.getcwd() + '/bank/testcases'

class ProblemAdmin(admin.ModelAdmin):

    def get_urls(self):
        urls = super(ProblemAdmin, self).get_urls()
        testcases_url = [
            url(r'^testcases/$',self.admin_site.admin_view(self.testcase_index)),
            url(r'^testcases/(?P<problem_id>[0-9]+)/$',self.admin_site.admin_view(self.testcase_view)),
            url(r'^testcases/(?P<problem_id>[0-9]+)/(?P<case_no>[0-9])$',self.admin_site.admin_view(self.viewcase)),
            url(r'^testcases/(?P<problem_id>[0-9]+)/add/$',self.admin_site.admin_view(self.add_testcase)),
            url(r'^testcases/(?P<problem_id>[0-9]+)/remove/$',self.admin_site.admin_view(self.remove_testcase)),
        ]
        return testcases_url + urls

    def testcase_index(self,request):

        context = dict(
            self.admin_site.each_context(request),
            problems = bank_problems.objects.all(),
        )
        return render(request,"admin/bank/problem/testcases_index.html",context)

    def testcase_view(self,request,problem_id):

        try:
            testcase_dir = BASE_TEST_CASES_DIR + "/" + str(problem_id)
            problem = bank_problems.objects.get(problem_id=problem_id)
            no_of_cases = len(fnmatch.filter(os.listdir(testcase_dir),'input*'))
        except ObjectDoesNotExist:
            return HttpResponse("Problem does not exist")
        except FileNotFoundError:
            os.makedirs(testcase_dir)
            no_of_cases = len(fnmatch.filter(os.listdir(testcase_dir),'input*'))

        context = dict(
            self.admin_site.each_context(request),
            problem = problem,
            no_of_cases = range(no_of_cases)
        )
        return render(request,"admin/bank/problem/testcases.html",context)

    def viewcase(self,request,problem_id,case_no):

        case_no = int(case_no)
        testcase_dir = BASE_TEST_CASES_DIR + "/" + str(problem_id)
        input_file_dir = testcase_dir + "/" + sorted(fnmatch.filter(os.listdir(testcase_dir),'input*'))[case_no]
        output_file_dir = testcase_dir + "/" + sorted(fnmatch.filter(os.listdir(testcase_dir),'output*'))[case_no]

        context = dict(
            input = open(input_file_dir,'r').read(),
            output = open(output_file_dir,'r').read()
        )
        return render(request,"admin/bank/problem/input-output-pair.html",context)

    def add_testcase(self,request,problem_id):
        # TODO
        return HttpResponse("add testcase view")

    def remove_testcase(self,request,problem_id):
        # TODO
        return HttpResponse("remove testcase view")

    list_display = [
        'problem_id',
        'title',
        'statement',
        'uploadedby'
    ]

    list_display_links = [
        'problem_id',
        'title',
        'statement',
    ]

    search_fields = [
        'id',
        'problem__problem_id',
        'problem__title',
        'problem__statement'
    ]


admin.site.register(Problem, ProblemAdmin)
