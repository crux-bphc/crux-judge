from django.contrib import admin
from .models import Problem, Submission, Config

class ProblemAdmin(admin.ModelAdmin):

    def get_problem_title(self,problem):
        return problem.problem.title
    get_problem_title.short_description = "title"

    def get_problem_statement(self,problem):
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

    def get_username(self,submission):
        return submission.user.username
    get_username.short_description = "submitted-by (ID)"

    def get_name(self,submission):
        return (submission.user.first_name + " " + submission.user.last_name)
    get_name.short_description = "submitted-by (Name)"

    def testcase_wise_result(self,submission):
        return submission.testcase_result_verbose()
    testcase_wise_result.short_description = "testcase wise result"

    def get_problem_title(self,submission):
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
