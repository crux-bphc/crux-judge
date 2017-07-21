from django import forms

class LoginForm(forms.Form):
    username = forms.CharField(label='ID',max_length=20)
    password = forms.CharField(widget=forms.PasswordInput)

class SubmissionForm(forms.Form):
    submission_file = forms.FileField()
