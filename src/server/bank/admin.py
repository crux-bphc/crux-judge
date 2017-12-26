import os
import fnmatch
from django.contrib import admin
from .models import Problem
from django.conf.urls import url
from django.http import HttpResponse
from django.shortcuts import render
from bank.models import Problem as bank_problems
from django.core.files import File

class ProblemAdmin(admin.ModelAdmin):

    def get_urls(self):
        urls = super(ProblemAdmin, self).get_urls()
        testcases_url = [
            url(r'^testcases/$',self.admin_site.admin_view(self.testcases_view)),
        ]
        return urls # TODO :  + testcases_url

    def testcases_view(self,request):
        pass
        # TODO : Add view for testcases.

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
