# -*- coding: utf-8 -*-
# Generated by Django 1.11.4 on 2017-09-18 15:19
from __future__ import unicode_literals

from django.db import migrations, models


class Migration(migrations.Migration):

    dependencies = [
        ('contest', '0002_endtime'),
    ]

    operations = [
        migrations.AlterModelOptions(
            name='endtime',
            options={'verbose_name': 'End Time'},
        ),
        migrations.AlterField(
            model_name='endtime',
            name='id',
            field=models.AutoField(primary_key=True, serialize=False),
        ),
    ]
