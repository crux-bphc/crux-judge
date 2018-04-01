from django.db import models
from django.utils.encoding import python_2_unicode_compatible
from django.contrib.auth.models import User
from django.db.models.signals import post_save, pre_save
from django.dispatch import receiver
from django.core.exceptions import PermissionDenied
# Create your models here.


@python_2_unicode_compatible
class Problem(models.Model):
    problem = models.ForeignKey('bank.problem', verbose_name='problem')
    max_score = models.FloatField(default=0)

    class Meta:
        verbose_name = 'Contest Problem'
        ordering = ['problem_id']

    def __str__(self):
        return "{} : {}".format(self.problem.problem_id, self.problem.title)


class Submission(models.Model):
    id = models.AutoField(primary_key=True)
    problem = models.ForeignKey('bank.problem')
    score = models.DecimalField(default=0, max_digits=5, decimal_places=2)
    time = models.DateTimeField(
        auto_now_add=True, verbose_name='Submission Time')
    user = models.ForeignKey(User, verbose_name='submitted-by')
    ip = models.GenericIPAddressField(
        verbose_name='submitted-by (IP)', blank=True, null=True)
    local_file = models.CharField(
        max_length=150, null=True, verbose_name='Original File')
    testcase_codes = models.CharField(
        default="[-1]", max_length=100, verbose_name="Testcase Status Code")

    def __str__(self):
        return "{} - {} - {}".format(
            self.user.username, self.problem.title, self.time)

    def testcase_result_verbose(self):

        def code_to_string(code):
            # code = int(code.strip())
            if(code == 0):
                return "Pass"
            elif(code == 1):
                return "SandboxFailure"
            elif(code == 2):
                return "RuntimeError"
            elif(code == 3):
                return "MemLimExceeded"
            elif(code == 4):
                return "TimeLimExceeded"
            elif(code == 5):
                return "SandboxChildProcessExceeded"
            elif(code == 6):
                return "CompilationErr"
            elif(code == 7):
                return "IncorrectAnswer"
            else:
                return "default"

        tests = list(
            map(int, self.testcase_codes.replace(" ", "")[1:-1].split(',')))
        print(tests)
        return list(map(code_to_string, tests))


class Config(models.Model):
    start = models.DateTimeField(verbose_name='Start Time')
    end = models.DateTimeField(verbose_name='End Time')

# Don't allow more than one config per contest


@receiver(pre_save, sender=Config)
def check_config_count(sender, **kwargs):
    if sender.objects.count() >= 1:
        raise PermissionDenied


class Profile(models.Model):
    user = models.OneToOneField(User, on_delete=models.CASCADE)
    logged_in = models.BooleanField(default=False)


@receiver(post_save, sender=User)
def create_user_profile(sender, instance, created, **kwargs):
    if created:
        Profile.objects.create(user=instance)


@receiver(post_save, sender=User)
def save_user_profile(sender, instance, **kwargs):
    instance.profile.save()
