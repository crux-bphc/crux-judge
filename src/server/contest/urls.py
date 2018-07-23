from django.conf.urls import url

from . import views

urlpatterns = [
    url(r'^$', views.problem_list, name='problem_list'),
    url(r'^login/', views.auth, name='auth'),
    url(r'^problem/(?P<problem_id>[0-9]+)/$', views.problem, name='problem'),
    url(r'^problem/download/(?P<problem_id>[0-9]+)/$',
        views.problem_file_download, name='problem_file_download'),
    url(r'^upload/', views.upload, name='upload'),
    url(r'^logout', views.logout_view, name='logout'),
    url(r'^submissions', views.display_submissions, name='submissions'),
    url(r'^submissions/(?P<p>[0-9]+)/$',
        views.display_submissions, name='problem_submissions'),
]
