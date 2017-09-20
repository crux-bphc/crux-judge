from django.db import models
from django.utils.encoding import python_2_unicode_compatible
from django.contrib.auth.models import User
from datetime import datetime
from django.utils import timezone
from datetime import datetime
# Create your models here.

@python_2_unicode_compatible
class Problem(models.Model):
    problem = models.ForeignKey('bank.problem',verbose_name='problem')
    max_score = models.FloatField(default=0)
    class Meta:
        verbose_name = 'Contest Problem'
        ordering = ['problem_id']
    def __str__(self):
        return "{} : {}".format(self.problem.problem_id,self.problem.title)

class Submission(models.Model):
    id = models.AutoField(primary_key = True)
    problem = models.ForeignKey('bank.problem')
    score = models.DecimalField(default=0,max_digits=5,decimal_places=2)
    time = models.DateTimeField(auto_now_add=True,verbose_name='Submission Time')
    user = models.ForeignKey(User,verbose_name='submitted-by')
    ip = models.GenericIPAddressField(verbose_name='submitted-by IP',blank=True,null=True)
    local_file = models.CharField(max_length=150,null=True,verbose_name='Original File')
    def __str__(self):
        return "{} - {} - {}".format(self.user.username,self.problem.title,self.time)
        
class EndTime(models.Model):
	id = models.AutoField(primary_key = True)
	date_time=models.TimeField(blank = True)
	class Meta:
		verbose_name = "End Time"
	def __str__(self):
		return "{}".format(self.date_time)
	      
