from django.http import HttpResponse
from django.shortcuts import render
from .forms import LoginForm
from .models import Problem as contest_problem
from django.contrib.auth import authenticate
from trial.models import Problem as all_problems

# Create your views here.
def index(request):
    login_form = LoginForm()

    context = {
        "login_form" : login_form,
    }

    return render(request,'contest_login.html',context)


def login(request):
    if request.method == "POST":

        form = LoginForm(request.POST)
        if form.is_valid:
            username = form["username"].value()
            password = form["password"].value()

        user = authenticate(username=username,password=password)

        if user is not None and user.is_active:

            request.session.set_expiry(300) #session expires within 5 minutes of logging in
            titles = []
            ids = []

            problems = contest_problem.objects.all()
            for problem in problems:
                titles.append(problem.problem.title)
                ids.append(problem.problem_id)

            context = {'data':list(zip(ids,titles))}
            return render(request,"problem_page.html",context)

        else:
            context = {"login_form":LoginForm()}
            return render(request,'contest_login.html',context)
    else:
        return HttpResponse("contest/login/")

def problem(request,problem_id):
    if 'sessionid' in request.COOKIES:
        problem = all_problems.objects.get(problem_id=problem_id)
        return render(request,'problem.html',{'problem':problem})
    else:
        return HttpResponse("Session Expired. Login again")

def logout(request):
    try:
        del request.COOKIES['sessionid']
    except KeyError:
        pass
    return HttpResponse("You're logged out.")
