from django.http import HttpResponse
from django.shortcuts import render

from .forms import SubmissionForm

# Create your views here.
def index(request):
    submission_form = SubmissionForm()
    # contains all variables passed to template while rendering
    context = {
        "submission_form": submission_form,
    }
    # throws a html response with
    return render(request,'index.html',context)
