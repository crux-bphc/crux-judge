from pathlib import Path

from django.core.exceptions import ObjectDoesNotExist
from django.contrib import admin
from django.conf.urls import url
from django.http import HttpResponse
from django.shortcuts import render, redirect
from django.contrib.auth.models import User
from django.contrib.auth.admin import UserAdmin

from .models import Problem
from .forms import AddTestcase, UserRecords
from bank.models import Problem as bank_problems
import add_student_records

BASE_TEST_CASES_DIR = Path.cwd() / 'bank/testcases'
PASSWORD_FILENAME = Path.cwd() / 'students_password.csv'

class ProblemAdmin(admin.ModelAdmin):

    def get_urls(self):
        urls = super(ProblemAdmin, self).get_urls()
        testcases_url = [
            url(r'^testcases/$',
                self.admin_site.admin_view(self.testcase_index)),
            url(r'^testcases/(?P<problem_id>[0-9]+)/$',
                self.admin_site.admin_view(self.list_testcases)),
            url(r'^testcases/(?P<problem_id>[0-9]+)/(?P<case_no>[0-9])$',
                self.admin_site.admin_view(self.view_testcase)),
            url(r'^testcases/add/(?P<problem_id>[0-9]+)/$',
                self.admin_site.admin_view(self.add_testcase)),
            url(r'^testcases/add/(?P<problem_id>[0-9]+)/save/$',
                self.admin_site.admin_view(self.save_testcase)),
            url(r'^testcases/(?P<problem_id>[0-9]+)/(?P<case_no>[0-9])' +
                r'/remove/$',
                self.admin_site.admin_view(self.remove_testcase)),
        ]
        return testcases_url + urls

    def testcase_index(self, request):

        context = dict(
            self.admin_site.each_context(request),
            problems=bank_problems.objects.all(),
        )
        return render(request, "admin/bank/problem/testcases_index.html",
                      context)

    def list_testcases(self, request, problem_id):

        try:
            testcase_dir = BASE_TEST_CASES_DIR / str(problem_id)
            problem = bank_problems.objects.get(problem_id=problem_id)
        except ObjectDoesNotExist:
            return HttpResponse("Problem does not exist")

        no_of_cases = len(list(testcase_dir.glob('input*')))
        context = dict(
            self.admin_site.each_context(request),
            problem=problem,
            no_of_cases=range(no_of_cases)
        )
        return render(request, "admin/bank/problem/testcases.html", context)

    def view_testcase(self, request, problem_id, case_no):

        case_no = int(case_no)
        testcase_dir = BASE_TEST_CASES_DIR / str(problem_id)
        input_files = sorted(testcase_dir.glob('input*'))
        output_files = sorted(testcase_dir.glob('output*'))

        input_file_path = input_files[case_no]
        output_file_path = output_files[case_no]
        with input_file_path.open('r') as inp_file:
            inp = inp_file.read()
        with output_file_path.open('r') as out_file:
            out = out_file.read()
        context = dict(
            self.admin_site.each_context(request),
            input=inp,
            output=out,
            problem=bank_problems.objects.get(problem_id=problem_id),
            case_no=case_no
        )
        return render(request, "admin/bank/problem/input-output-pair.html",
                      context)

    def add_testcase(self, request, problem_id):

        problem = bank_problems.objects.get(problem_id=problem_id)
        testcase_form = AddTestcase()

        context = dict(
            self.admin_site.each_context(request),
            testcase_form=testcase_form,
            problem=problem,
        )
        return render(request, "admin/bank/problem/add-testcase.html", context)

    def save_testcase(self, request, problem_id):

        testcase_dir = BASE_TEST_CASES_DIR / str(problem_id)

        form = AddTestcase(request.POST)
        if form.is_valid:
            input_text = form["input_text"].value()
            output_text = form["output_text"].value()
        else:
            input_text = ''
            output_text = ''

        input_files = testcase_dir.glob("input*")
        try:
            latest_file_basename = max(input_files).stem
        except ValueError:
            latest_file_basename = "input0"
        suffix = str(int(latest_file_basename.split('input')[1]) + 1)

        inp_file_path = testcase_dir / ("input" + suffix)
        with inp_file_path.open("w+") as input_file:
            input_file.write(input_text)

        outp_file_path = testcase_dir / ("output" + suffix)
        with outp_file_path.open("w+") as output_file:
            output_file.write(output_text)

        return redirect('/admin/bank/problem/testcases/' + problem_id)

    def remove_testcase(self, request, problem_id, case_no):

        case_no = int(case_no)
        testcase_dir = BASE_TEST_CASES_DIR / str(problem_id)

        input_files = sorted(testcase_dir.glob('input*'))
        output_files = sorted(testcase_dir.glob('output*'))
        input_file_path = input_files[case_no]
        output_file_path = output_files[case_no]
        input_file_path.unlink()
        output_file_path.unlink()

        return redirect('/admin/bank/problem/testcases/' + problem_id)

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
        super(UserAdmin, self).__init__(*args, **kwargs)

    def get_urls(self):
        urls = super(CustomUserAdmin, self).get_urls()
        custom_urls = [
            url(r'^addusers/upload/$',
                self.admin_site.admin_view(self.user_records_upload)),
            url(r'^addusers/$', self.admin_site.admin_view(self.addusers)),
        ]
        return custom_urls + urls

    def addusers(self, request):

        user_records_csv = UserRecords()
        context = dict(
            self.admin_site.each_context(request),
            user_records_form=user_records_csv,
        )
        return render(request, "admin/auth/user/addusers.html", context)

    def export_password_file(self, filename):
        with open(filename, 'r') as f:
            response = HttpResponse(f.read(), content_type='text/csv')
            response['content-Disposition'] = 'attachment; filename=' + PASSWORD_FILENAME.name
            return response

    def user_records_upload(self, request):

        uploaded_file = request.FILES['user_records']
        add_student_records.readfile(uploaded_file)
        return self.export_password_file(PASSWORD_FILENAME)

    def get_name(self, user):
        return user.first_name + user.last_name
    get_name.short_description = "Name"

    def make_active(self, request, queryset):
        queryset.update(is_active=True)
    make_active.short_description = "Mark selected users as active"

    def make_inactive(self, request, queryset):
        queryset.update(is_active=False)
    make_inactive.short_description = "Mark selected users as inactive"

    list_display = [
        'username',
        'get_name',
        'is_staff',
        'is_active',
    ]

    list_display_links = [
        'username',
        'get_name',
    ]

    list_editable = [
        'is_active',
    ]

    actions = [
        'make_active',
        'make_inactive',
    ]

admin.site.register(Problem, ProblemAdmin)
admin.site.unregister(User)
admin.site.register(User, CustomUserAdmin)
