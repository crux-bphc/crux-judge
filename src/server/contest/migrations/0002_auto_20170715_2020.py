# -*- coding: utf-8 -*-
# Generated by Django 1.11.2 on 2017-07-15 14:50
from __future__ import unicode_literals

from django.db import migrations


class Migration(migrations.Migration):

    dependencies = [
        ('contest', '0001_initial'),
    ]

    operations = [
        migrations.RenameField(
            model_name='problem',
            old_name='problem_id',
            new_name='problem',
        ),
    ]