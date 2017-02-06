from django.http import HttpResponse
from django.shortcuts import render

from .forms import SubmissionForm
import os

# Create your views here.
def index(request):
    submission_form = SubmissionForm()
    # contains all variables passed to template while rendering
    context = {
        "submission_form": submission_form,
    }
    # throws a html response with
    return render(request,'index.html',context)

def upload(request):
    if request.method == "POST":
        uploaded_filedata = request.FILES["submission_file"]
        # creates new file in src/server/submissions
        filepath = "submissions/"+request.FILES["submission_file"].name
        submission_file = open(filepath,"wb+")
        # write to file - failsafe for handling large file
        for chunk in uploaded_filedata.chunks():
            submission_file.write(chunk)
        # insert submission file handler below
        # compile("submissions/")
        return HttpResponse("file uploaded successfully")
    else:
        return HttpResponse("trial/upload/")
