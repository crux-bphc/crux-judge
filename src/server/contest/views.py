from pathlib import Path
from time import mktime
from shutil import copyfile
from math import isclose

from django.http import HttpResponse, Http404
from django.shortcuts import render
from django.contrib.auth.models import User
from django.contrib.auth import authenticate, login, logout
from django.utils import timezone
from django.contrib import messages
from django.contrib.auth.decorators import login_required
from django.contrib.admin.views.decorators import staff_member_required
from django.db.models import Max
from ipware.ip import get_ip

from .forms import LoginForm, SubmissionForm
from .models import Problem as contest_problem, Submission
from contest.models import Profile, Config
from bank.models import Problem as all_problems
from . import runner

# Create your views here.


def get_time(time_):
    timetuple = time_.timetuple()
    timestamp = mktime(timetuple)
    time_ = timestamp * 1000.0

    return time_


def index(request):
    # request.session.clear_expired()
    print(timezone.localtime())
    if request.user.is_authenticated:
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

        # TODO: multi-login checker is causing some error.
        # user.profile.logged_in is true even when all tabs are closed.

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
            print("Login Failed")
            context = {"login_form": LoginForm()}
            return render(request, 'contest_login.html', context)
    else:
        return HttpResponse("contest/auth/")

@login_required
def problem(request, problem_id, submission=None):
    """ Renders the page that lists all problems of the contest"""
    problem = all_problems.objects.get(problem_id=problem_id)
    this_problem = contest_problem.objects.get(problem__problem_id=problem_id)
    user = User.objects.get(id=request.session['_auth_user_id'])
    submission_form = SubmissionForm()
    end = get_time(Config.objects.all()[0].end)
    start = get_time(Config.objects.all()[0].start)

    # display highest score obtained till now in a specific problem
    user_submissions = Submission.objects.all().filter(user=request.user, problem=problem)
    best_submission = user_submissions.aggregate(Max('score'))['score__max']
    max_score = this_problem.max_score

    context = {
        "submission_form": submission_form,
        "problem": problem,
        "username": user.username,
        "submission": submission,
        "end": end,
        "start": start,
        "best_score": best_submission,
        "max_score": max_score,
    }
    return render(request, 'problem.html', context)


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

    # Error handling if no contest is running
    if(Config.objects.count() == 0):
        return render(request, 'config_error.html')

    else:
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
        latest_submissions_fold = Path('contest/submissions/latest')
        # creates /contest/submissions/latest folder if does not exist
        if not latest_submissions_fold.exists():
            latest_submissions_fold.mkdir()

        # creates new file in /contest/submissions/latest
        filepath = latest_submissions_fold / submission_file_name
        with filepath.open('wb+') as submission_file:
            for chunk in uploaded_filedata.chunks():
                submission_file.write(chunk)

        submission = Submission.objects.create(
            problem=problem_.problem,
            user=user,
            ip=ip_address,
            local_file=uploaded_filedata
        )

        evaluate = runner.Runner(submission)
        evaluate.check_all()
        score = evaluate.score_obtained()

        #Save best submission of user

        current_problem = all_problems.objects.get(problem_id=problem_id)
        user_submissions = Submission.objects.all().filter(user=request.user, problem=current_problem)
        best_submission = user_submissions.aggregate(Max('score'))['score__max']

        # Comparing float score values upto first decimal place
        if(isclose(score, best_submission, abs_tol=0.1)):
            best_folder = Path('contest/submissions/best')
            if not best_folder.exists():
                best_folder.mkdir()

            source_file = "contest/submissions/latest/"+user.username+"_"+problem_id+".c"
            dest_file = "contest/submissions/best/"+user.username+"_"+problem_id+".c"
            copyfile(source_file, dest_file)
            print("Best submission: "+user.username)


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
        query = Submission.objects.all()
    context = {
        "submissions": query,
        "username": user.username,
        "end": end,
        "start": start,
    }
    return render(request, "display_submissions.html", context)

# For staff only access
@staff_member_required
def submissions_download_view(request):

    best_path = Path("contest/submissions/best/")
    latest_path = Path("contest/submissions/latest/")

    # create folders if does not exist
    if not best_path.exists():
        best_path.mkdir()

    if not latest_path.exists():
        latest_path.mkdir()

    files = [f for f in best_path.glob('**/*') if f.is_file()]
    # Sorted: All submissions of a user will appear together
    files.sort()

    # filename: <username>_<problem_No.>.c

    # removing extensions
    filenames = [f.stem for f in files]

    # extracting usernames
    usernames = [i.split('_')[0] for i in filenames]

    #extracting problem numbers
    problem_nos = [i.split('_')[1] for i in filenames]

    #Zipping usernames, problem numbers and files
    zipped = zip(usernames, problem_nos, filenames)

    #contest timer
    end = get_time(Config.objects.all()[0].end)
    start = get_time(Config.objects.all()[0].start)

    context = {
        "submissions": zipped,
        "start": start,
        "end": end,
    }
    return render(request, "download_submissions.html", context)

# For staff only access
@staff_member_required
def submissions_download_response(request, filename, submission_type):
    if (submission_type == "best"):
        path = Path("contest/submissions/best/")
    elif (submission_type == "latest"):
        path = Path("contest/submissions/latest/")
    else:
        raise Http404()

    filename = filename + ".c"
    with open(path / filename, 'rb') as file:

        response = HttpResponse(file.read())
        response['content_type'] = 'text/plain'
        response['content-Disposition'] = 'attachment;filename='+filename
        return response

