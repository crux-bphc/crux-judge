from django.http import HttpResponse
from django.shortcuts import render
from .forms import LoginForm,SubmissionForm
from .models import Problem as contest_problem
from django.contrib.auth.models import User
from django.contrib.auth import authenticate,login,logout
from trial.models import Problem as all_problems
from time import sleep
import os
from datetime import datetime
from . import runner

# Create your views here.
def index(request):
    # request.session.clear_expired()

    if request.user.is_authenticated():
        return problemList(request)

    login_form = LoginForm()

    context = {
        "login_form" : login_form,
    }

    return render(request,'contest_login.html',context)

def auth(request):
    if request.method == "POST":

        form = LoginForm(request.POST)
        if form.is_valid:
            username = form["username"].value()
            password = form["password"].value()

        user = authenticate(request,username=username,password=password)

        if user is not None and user.is_active:
            # login_ successful
            login(request,user)
            # user_id = request.session['_auth_user_id']
            # username = User.objects.get(id = user_id).username
            request.session.set_expiry(0) #session expires when browser is closed

            return problemList(request)

        else:
            # login_ failed
            context = {"login_form":LoginForm()}
            return render(request,'contest_login.html',context)
    else:
        return HttpResponse("contest/auth/")

def problem(request,problem_id):

    if request.user.is_authenticated:
        problem = all_problems.objects.get(problem_id=problem_id)
        user = User.objects.get(id=request.session['_auth_user_id'])
        submission_form = SubmissionForm()
        context = {
            "submission_form" : submission_form,
            "problem" : problem,
            "username" : user.username
        }
        return render(request,'problem.html',context)
    else:
        return HttpResponse("Session Expired. Login again")

def problemList(request):

    titles = []
    ids = []
    problems = contest_problem.objects.all()
    user = User.objects.get(id=request.session['_auth_user_id'])

    for problem in problems:
        titles.append(problem.problem.title)
        ids.append(problem.problem_id)

    context = {
        'data':list(zip(ids,titles)),
        'username':user.username
    }
    return render(request,'problem_page.html',context)


def upload(request):
    print()
    if request.method == "POST":

        problem_id = request.POST.get('problem_id')
        problem = all_problems.objects.get(problem_id=problem_id)
        user = User.objects.get(id=request.session['_auth_user_id'])
        submission_file_name = user.username + '_' + str(problem.problem_id) + '.c'

        uploaded_filedata = request.FILES["submission_file"]
        #creates /contest/submissions folder if does not exist
        if not os.path.isdir("contest/submissions"):
            os.makedirs("contest/submissions")

        # creates new file in /contest/submissions
        filepath = "contest/submissions/"+submission_file_name
        submission_file = open(filepath,"wb+")

        # write to file - failsafe for handling large file
        for chunk in uploaded_filedata.chunks():
            submission_file.write(chunk)
        submission_file.close()

        evaluate = runner.runner(problem,user)
        evaluate.check_all()

        return HttpResponse("File uploaded successfully")

    else:
        return HttpResponse("/contest/upload/")

def logout_view(request):
    logout(request)
    return HttpResponse("You're logged out.")
