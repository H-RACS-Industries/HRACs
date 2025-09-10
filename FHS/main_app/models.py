from django.db import models
from django.contrib.auth import get_user_model
from datetime import time

class Room(models.Model):
    name = models.CharField(max_length=30, unique=True)
    ideal_temperature = models.FloatField(default=27, null=False, blank=False)
    wake_time = models.TimeField(default=time(7, 0), blank=False)
    sleep_time = models.TimeField(default=time(22, 0), blank=False)
    heat_sensor_id = models.IntegerField(default=0, null=False, blank=False) # unique=True
    current_temperature = models.FloatField(default=None, null=True, blank=True)
    
    class Meta:
        permissions = [
            ('change_temperature', 'Can change ideal temperature'),
            ('change_wakesleep', 'Can change Wake and Sleep time of device'),
            ("assign_device", "Can assign a device to a room"),
            ("view_auditlog", "Can view room audit logs"),  
            ('can_promote_student', 'Can promote a Student/Teacher to a Facilities Staff'),
        ]
    
    def __str__(self):
        return self.name   
    
class HeatingDevice(models.Model):
    id = models.IntegerField(null=False, blank=False, primary_key=True)
    room = models.ForeignKey(Room, on_delete=models.CASCADE, related_name='devices')
    
    def __str__(self):
        return f"ID: {self.id}     |     Location: {self.room}"

class AuditLog(models.Model):
    user = models.CharField(max_length=60, null=True, blank=True)
    room = models.CharField(max_length=50, null=True, blank=True)
    field = models.CharField(max_length=64)
    old_value = models.TextField(null=True, blank=True)
    new_value = models.TextField(null=True, blank=True)
    timestamp = models.DateTimeField(auto_now_add=True)

    def __str__(self):
        who = self.user or "system"
        return f"[{self.timestamp:%Y-%m-%d %H:%M}] {who} changed {self.room} {self.field} from {self.old_value} to {self.new_value}"