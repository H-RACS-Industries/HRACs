from django.contrib import admin
from django.contrib.auth.models import Permission

from .models import Room, AuditLog, HeatingDevice

@admin.register(Room)
class RoomAdmin(admin.ModelAdmin):
    list_display = ("name",'ideal_temperature', 'current_temperature' , 'wake_time', 'sleep_time')
    # exclude = ("current_temperature",)
    
@admin.register(AuditLog)
class AuditLogAdmin(admin.ModelAdmin):
    list_display = ("timestamp", "room", "user", "old_value", "new_value")

@admin.register(HeatingDevice)
class HeatingDeviceAdmin(admin.ModelAdmin):
    list_display = ("formatted_id", "room", "created_at")
    readonly_fields = ("id", "created_at")

    # Custom display for id
    def formatted_id(self, obj):
        return f"#{obj.id}"
    
    formatted_id.short_description = "ID"  # Column header in admin


admin.site.register(Permission)
