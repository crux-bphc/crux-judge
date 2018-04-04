from django.http import HttpResponse
from django.shortcuts import render
from .forms import LoginForm, SubmissionForm
from .models import Problem as contest_problem, Submission
from django.contrib.auth.models import User
from contest.models import Profile, Config
from django.contrib.auth import authenticate, login, logout
from bank.models import Problem as all_problems
from time import sleep, mktime
import os
from datetime import datetime
from . import runner
from ipware.ip import get_ip
from django.utils import timezone
from django.contrib import messages
from pathlib import Path

# Create your views here.


def get_time(time_):
    timetuple = time_.timetuple()
    timestamp = mktime(timetuple)
    time_ = timestamp * 1000.0

    return time_


def index(request):
    # request.session.clear_expired()
    print(timezone.localtime())
    if request.user.is_authenticated():
        return problemList(request)

    login_form = LoginForm()
    end = get_time(Config.objects.all()[0].end)
    start = get_time(Config.objects.all()[0].start)
    context = {
        "login_form": login_form,
        "end": end,
        "start": start,
    }

    return render(request, 'contest_login.html', context)


def auth(request):
    """ Checks login information entered by user """
    if request.method == "POST":

        if(timezone.now() < Config.objects.all()[0].start):
            return HttpResponse("Contest has not started yet.")
        form = LoginForm(request.POST)
        if form.is_valid:
            username = form["username"].value()
            password = form["password"].value()

        user = authenticate(request, username=username, password=password)
        config = Config.objects.all()
        for active_config in config:
            start = active_config.start
            end = active_config.end

        if user is not None and user.is_active and not user.profile.logged_in \
           and start < timezone.now() < end:
            # login_ successful
            login(request, user)
            Profile.objects.filter(user_id=user.id).update(logged_in=True)
            # user_id = request.session['_auth_user_id']
            # username = User.objects.get(id = user_id).username)
            # session expires when contest is over
            request.session.set_expiry(end)
            messages.success(request, "Welcome {}".format(user.username))
            return problemList(request)

        else:
            # login_ failed
            context = {"login_form": LoginForm()}
            return render(request, 'contest_login.html', context)
    else:
        return HttpResponse("contest/auth/")


def problem(request, problem_id, submission=None):
    """ Renders the page that lists all problems of the contest"""
    if request.user.is_authenticated:
        problem = all_problems.objects.get(problem_id=problem_id)
        user = User.objects.get(id=request.session['_auth_user_id'])
        submission_form = SubmissionForm()
        end = get_time(Config.objects.all()[0].end)
        start = get_time(Config.objects.all()[0].start)
        context = {
            "submission_form": submission_form,
            "problem": problem,
            "username": user.username,
            "submission": submission,
            "end": end,
            "start": start,
        }
        return render(request, 'problem.html', context)
    else:
        return HttpResponse("Session Expired. Login again")


def problemList(request):
    """ Renders the page for particular problem """
    titles = []
    ids = []
    contest_id = []
    problems = contest_problem.objects.all()
    user = User.objects.get(id=request.session['_auth_user_id'])

    for problem in problems:
        titles.append(problem.problem.title)
        ids.append(problem.problem_id)
        contest_id.append(problem.id)

    end = get_time(Config.objects.all()[0].end)
    start = get_time(Config.objects.all()[0].start)
    context = {
        'data': list(zip(contest_id, ids, titles)),
        'username': user.username,
        'end': end,
        "start": start,
    }

    return render(request, 'problem_page.html', context)


def upload(request):
    """
    Receives C file.
    Passes Submission object to runner class for compilation,
    execution and evaluation.
     """
    if request.method == "POST":

        if(timezone.now() > Config.objects.all()[0].end):
            return HttpResponse("Time Up! No more submissions.")
        ip_address = get_ip(request)
        problem_id = request.POST.get('problem_id')
        problem_ = contest_problem.objects.get(problem_id=problem_id)
        user = User.objects.get(id=request.session['_auth_user_id'])
        submission_file_name = user.username + \
            '_' + str(problem_.problem_id) + '.c'

        uploaded_filedata = request.FILES['submission_file']
        submissions_fold = Path('contest/submissions')
        # creates /contest/submissions folder if does not exist
        if not submissions_fold.exists():
            submissions_fold.mkdir()

        # creates new file in /contest/submissions
        filepath = submissions_fold / submission_file_name
        submission_file = filepath.open("wb+")

        # write to file - failsafe for handling large file
        for chunk in uploaded_filedata.chunks():
            submission_file.write(chunk)
        submission_file.close()

        submission = Submission.objects.create(
            problem=problem_.problem,
            user=user,
            ip=ip_address,
            local_file=uploaded_filedata
        )

        evaluate = runner.Runner(submission)
        evaluate.check_all()
        evaluate.score_obtained()

        return problem(request, problem_.problem_id, evaluate)
    else:
        return HttpResponse("/contest/upload/")


def logout_view(request):
    print(request.user.id)
    Profile.objects.filter(user_id=request.user.id).update(logged_in=False)
    logout(request)
    messages.info(request, "You have been logged out")
    return index(request)


def display_submissions(request):
    """Render page to display submissions made till the given time """
    user = User.objects.get(id=request.session['_auth_user_id'])
    end = get_time(Config.objects.all()[0].end)
    start = get_time(Config.objects.all()[0].start)

    if request.GET.keys():
        query = Submission.objects.filter(problem_id=request.GET['p'])
    else:
        query = Submission.objects.all().order_by('-time')
    context = {
        "submissions": query,
        "username": user.username,
        "end": end,
        "start": start
    }
    return render(request, "display_submissions.html", context)
