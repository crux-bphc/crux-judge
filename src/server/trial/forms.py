from django import forms

# Form Classes defined here, to be used to render forms directly into templated
class SubmissionForm(forms.Form):
    file = forms.FileField()
