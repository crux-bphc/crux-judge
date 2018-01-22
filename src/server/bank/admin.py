import os
import fnmatch
from glob import glob
import add_student_records
from django.core.exceptions import ObjectDoesNotExist
from django.contrib import admin
from .models import Problem
from .forms import AddTestcase, UserRecords
from django.conf.urls import url
from django.http import HttpResponse
from django.shortcuts import render, redirect
from bank.models import Problem as bank_problems
from django.core.files import File
from django.contrib.auth.models import User
from django.contrib.auth.admin import UserAdmin

BASE_TEST_CASES_DIR = os.getcwd() + '/bank/testcases'

class ProblemAdmin(admin.ModelAdmin):

    def get_urls(self):
        urls = super(ProblemAdmin, self).get_urls()
        testcases_url = [
            url(r'^testcases/$',self.admin_site.admin_view(self.testcase_index)),
            url(r'^testcases/(?P<problem_id>[0-9]+)/$',self.admin_site.admin_view(self.list_testcases)),
            url(r'^testcases/(?P<problem_id>[0-9]+)/(?P<case_no>[0-9])$',self.admin_site.admin_view(self.view_testcase)),
            url(r'^testcases/add/(?P<problem_id>[0-9]+)/$',self.admin_site.admin_view(self.add_testcase)),
            url(r'^testcases/add/(?P<problem_id>[0-9]+)/save/$',self.admin_site.admin_view(self.save_testcase)),
            url(r'^testcases/(?P<problem_id>[0-9]+)/(?P<case_no>[0-9])/remove/$',self.admin_site.admin_view(self.remove_testcase)),
        ]
        return testcases_url + urls

    def testcase_index(self,request):

        context = dict(
            self.admin_site.each_context(request),
            problems = bank_problems.objects.all(),
        )
        return render(request,"admin/bank/problem/testcases_index.html",context)

    def list_testcases(self,request,problem_id):

        try:
            testcase_dir = BASE_TEST_CASES_DIR + "/" + str(problem_id)
            problem = bank_problems.objects.get(problem_id=problem_id)
            no_of_cases = len(glob(testcase_dir+'/input*'))
        except ObjectDoesNotExist:
            return HttpResponse("Problem does not exist")
        except FileNotFoundError:
            os.makedirs(testcase_dir)
            no_of_cases = len(glob(testcase_dir+'input*'))

        context = dict(
            self.admin_site.each_context(request),
            problem = problem,
            no_of_cases = range(no_of_cases)
        )
        return render(request,"admin/bank/problem/testcases.html",context)

    def view_testcase(self,request,problem_id,case_no):

        case_no = int(case_no)
        testcase_dir = BASE_TEST_CASES_DIR + "/" + str(problem_id)

        input_files = sorted(glob(testcase_dir + "/input*"))
        output_files = sorted(glob(testcase_dir + "/output*"))

        input_file_dir = input_files[case_no]
        output_file_dir = output_files[case_no]

        context = dict(
            self.admin_site.each_context(request),
            input = open(input_file_dir,'r').read(),
            output = open(output_file_dir,'r').read(),
            problem = bank_problems.objects.get(problem_id=problem_id),
            case_no = case_no
        )
        return render(request,"admin/bank/problem/input-output-pair.html",context)

    def add_testcase(self,request,problem_id):

        problem = bank_problems.objects.get(problem_id=problem_id)
        testcase_form = AddTestcase()

        context = dict(
            self.admin_site.each_context(request),
            testcase_form = testcase_form,
            problem = problem,
        )
        return render(request,"admin/bank/problem/add-testcase.html",context)

    def save_testcase(self,request,problem_id):

        testcase_dir = os.path.join(BASE_TEST_CASES_DIR, str(problem_id))

        form = AddTestcase(request.POST)
        if form.is_valid:
            input_text = form["input_text"].value()
            output_text = form["output_text"].value()
        else:
            input_text = ''
            output_text = ''

        input_files = fnmatch.filter(os.listdir(testcase_dir),"input*")
        try:
            latest_file_basename = os.path.basename(max(input_files))
        except ValueError:
            latest_file_basename = "input0"    
        suffix = str(int(latest_file_basename.split('input')[1]) + 1)

        input_file = open(testcase_dir + "/input" + suffix, "w+")
        input_file.write(input_text)
        input_file.close()

        output_file = open(testcase_dir + "/output" + suffix, "w+")
        output_file.write(output_text)
        output_file.close()

        return redirect('/admin/bank/problem/testcases/'+problem_id)

    def remove_testcase(self,request,problem_id,case_no):

        case_no = int(case_no)
        testcase_dir = os.path.join(BASE_TEST_CASES_DIR, str(problem_id))

        input_files = sorted(glob(testcase_dir + "/input*"))
        output_files = sorted(glob(testcase_dir + "/output*"))

        input_file_dir = input_files[case_no]
        output_file_dir = output_files[case_no]

        os.remove(input_file_dir)
        os.remove(output_file_dir)

        return redirect('/admin/bank/problem/testcases/'+problem_id)


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


class CustomUserAdmin(UserAdmin):
    def __init__(self, *args, **kwargs):
        super(UserAdmin,self).__init__(*args, **kwargs)

    def get_urls(self):
        urls = super(CustomUserAdmin, self).get_urls()
        custom_urls = [
            url(r'^addusers/upload/$',self.admin_site.admin_view(self.user_records_upload)),
            url(r'^addusers/$',self.admin_site.admin_view(self.addusers)),
        ]
        return custom_urls + urls

    def addusers(self,request):

        user_records_csv = UserRecords()
        context = dict(
            self.admin_site.each_context(request),
            user_records_form = user_records_csv,
        )
        return render(request,"admin/auth/user/addusers.html",context)

    def user_records_upload(self,request):

        uploaded_file = request.FILES['user_records']
        add_student_records.readfile(uploaded_file)

        return redirect("/admin/auth/user/addusers/")

    def get_name(self,user):
        return user.first_name + user.last_name
    get_name.short_description = "Name"

    list_display = [
        'username',
        'get_name',
        'is_staff',
    ]

    list_display_links = [
        'username',
        'get_name',
    ]

admin.site.register(Problem, ProblemAdmin)
admin.site.unregister(User)
admin.site.register(User, CustomUserAdmin)
