# Creates a user group called 'Students'(if already does not exist)
# Adds Student records to database from 'student_records.csv' and simultaneously adds them to Students group.

import os
import random

from django.contrib.auth.models import User,Group

# Random Password
s = "abcdefghijklmnopqrstuvwxyz0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ!@#$%^&*()?"
passlen = 8

def add_user(user_data):
    username,fullname,section = user_data
    password = "".join(random.sample(s,passlen))
    x=fullname.find(' ')
    # Group depending on Practical Section
    group=Group.objects.get_or_create(name = section)[0]
    user, created = User.objects.get_or_create(username = username)
    user.set_password(password)
    if x != -1:
        user.first_name = fullname[:x]
        user.last_name = fullname[x+1:]
    else:
        user.first_name = fullname

    group.user_set.add(user);
    user.save()
    group.save()
    with open('students_password.csv', 'a') as pass_file:
        pass_file.write('{},{}\n'.format(username,password))

def readfile(user_records_file):

    DELIMITER = ','
    data = b''

    for chunk in user_records_file.chunks():
        data += chunk
    records = data.decode().split('\n')

    for record in records:
        if record != '':
            add_user(record.split(DELIMITER))
