from django.contrib import admin
from django.conf.urls import url

from .models import Problem, Submission, Config
from .views import submissions_download_response, submissions_download_view, \
    submissions_export_view

class ProblemAdmin(admin.ModelAdmin):

    def get_problem_title(self, problem):
        return problem.problem.title
    get_problem_title.short_description = "title"

    def get_problem_statement(self, problem):
        return problem.problem.statement
    get_problem_statement.short_description = "statement"

    list_display = [
        'id',
        'problem_id',
        'get_problem_title',
        'get_problem_statement',
        'max_score'
    ]

    list_display_links = [
        'id',
        'problem_id',
        'get_problem_title',
        'get_problem_statement'
    ]

    search_fields = [
        'id',
        'problem__problem_id',
        'problem__title',
        'problem__statement'
    ]


class SubmissionAdmin(admin.ModelAdmin):

    def get_urls(self):
        urls = super(SubmissionAdmin, self).get_urls()
        testcases_url = [
            url(r'^download/(?P<filename>[a-z\w\.]+)/type=(?P<submission_type>[a-z]+)/$',
                submissions_download_response, name='submissions_download_response'),

            url(r'^download/', submissions_download_view, name='submissions_download_view'),
            url(r'^export/', submissions_export_view, name='submissions_export_view'),
        ]
        return testcases_url + urls


    def get_username(self, submission):
        return submission.user.username
    get_username.short_description = "submitted-by (ID)"

    def get_name(self, submission):
        return (submission.user.first_name + " " + submission.user.last_name)
    get_name.short_description = "submitted-by (Name)"

    def testcase_wise_result(self, submission):
        return submission.testcase_result_verbose()
    testcase_wise_result.short_description = "testcase wise result"

    def get_problem_title(self, submission):
        return submission.problem.title
    get_problem_title.short_description = "problem"

    list_display = [
        'id',
        'get_username',
        'get_name',
        'ip',
        'get_problem_title',
        'score',
        'time',
        'testcase_wise_result',
    ]

    list_display_links = [
        'id',
        'get_username',
        'ip',
        'get_problem_title',
        'score',
        'time',
    ]

    search_fields = [
        'id',
        'problem__title',
        'user__id',
        'user__username',
        'user__first_name',
        'user__last_name',
    ]

    list_per_page = 500


class ConfigAdmin(admin.ModelAdmin):

    list_display = [
        'id',
        'start',
        'end',
    ]

    list_display_links = [
        'id',
        'start',
        'end',
    ]


admin.site.site_header = "Judge Administration Portal"
admin.site.site_title = "Crux Judge Admin"
admin.site.index_title = "Admin - Crux Judge"
admin.site.register(Problem, ProblemAdmin)
admin.site.register(Submission, SubmissionAdmin)
# Improve later
admin.site.register(Config, ConfigAdmin)
