from django.conf.urls import url
from . import views

urlpatterns = [
    url(r'^$', views.index, name='index'),
    url(r'^auth/', views.auth, name='auth'),
    url(r'^problem/(?P<problem_id>[0-9]+)/$', views.problem, name='problem'),
    url(r'^upload/', views.upload, name='upload'),
    url(r'^logout',views.logout_view, name='logout'),
    url(r'^submissions',views.display_submissions, name='submissions')
]
