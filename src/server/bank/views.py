from django.shortcuts import render
from django.http import HttpResponse
from django.contrib.admin.views.decorators import staff_member_required

from .models import Problem
# Create your views here.

@staff_member_required
def index(request):

    prob_list = Problem.objects.order_by("problem_id")

    output = "Problems are:   " + ','.join([q.title for q in prob_list])
    return HttpResponse(output)
