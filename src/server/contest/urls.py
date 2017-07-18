from django.conf.urls import url
from . import views

urlpatterns = [
    url(r'^$', views.index, name='index'),
    url(r'^login', views.login, name='login'),
    url(r'^problem/(?P<problem_id>[0-9]+)/$', views.problem, name='problem'),
]
