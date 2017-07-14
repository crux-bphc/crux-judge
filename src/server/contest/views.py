from django.http import HttpResponse
from django.shortcuts import render
from .forms import LoginForm
from django.contrib.auth import authenticate

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
        username = form['username'].value()
        password = form['password'].value()
        user = authenticate(username=username,password=password)

        if user is not None and user.is_active:
            return HttpResponse("Login successful")
        else:
            context={"login_form":LoginForm()}
            return render(request,'contest_login.html',context)
    else:
        return HttpResponse("contest/login/")
