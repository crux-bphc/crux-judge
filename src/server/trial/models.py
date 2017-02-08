from django.db import models
from django.utils.encoding import python_2_unicode_compatible
from django.contrib.auth.models import User
#Problems: id,title, statement, output, uploadedby
#2. Admins: id, name, email, passwordhash
#3. Students: id, name, email, passwordhash
@python_2_unicode_compatible
class Problem(models.Model):
    problem_id = models.AutoField(primary_key=True)
    title = models.CharField(max_length=50,unique=False)
    statement = models.CharField(max_length=1000,unique=False)
    output = models.CharField(max_length=100,unique=False)
    uploadedby = models.ForeignKey('Admin',verbose_name="problem-setter")
    class Meta:
        verbose_name='problem'
        ordering = ['id']
