from django import forms

class AddTestcase(forms.Form):
    input_text = forms.CharField(label = "Input", widget=forms.Textarea, strip=True)
    output_text = forms.CharField(label = "Output", widget=forms.Textarea)

class UserRecords(forms.Form):
    user_records = forms.FileField()
