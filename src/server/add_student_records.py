# Creates a user group called 'Students'(if already does not exist)
# Adds Student records to database from 'student_records.csv' and simultaneously adds them to Students group.

import os
import django

CSV_FILE_ADDR = 'student_records.csv'
DELIMITER = ','

os.environ['DJANGO_SETTINGS_MODULE']='judge.settings'
django.setup()

from django.contrib.auth.models import User,Group
group=Group.objects.get_or_create(name = 'Students')[0]

def add_user(user_data,group):
    username,email,fullname,password = user_data
    x=fullname.find(' ')

    user, created = User.objects.get_or_create(username = username, email = email)
    if not created:
        user.set_password(password)
        user.first_name = fullname[:x]
        user.last_name = fullname[x+1:]

    group.user_set.add(user);
    user.save()
    group.save()

student_csv = open(CSV_FILE_ADDR,'r')

record = student_csv.readline()[:-1]
while(record!=''):
    add_user(record.split(DELIMITER),group)
    record = student_csv.readline()[:-1]
