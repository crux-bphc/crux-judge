from django.db import models
from django.utils.encoding import python_2_unicode_compatible
# from django.contrib.auth.models import User
# Create your models here.

@python_2_unicode_compatible

class Problem(models.Model):
    problem = models.ForeignKey('trial.problem',verbose_name='problem')
    class Meta:
        verbose_name = 'Contest Problem'
