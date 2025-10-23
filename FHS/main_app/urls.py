from django.contrib import admin
from django.urls import path, include
from django.conf import settings
from django.conf.urls.static import static

from . import views

urlpatterns = [
    path('activate/<uidb64>/<token>/', views.activateAccount, name='activate'),
    path('register/', views.register, name='register'),
    path('login/', views.signin, name='login'),
    path('logout/', views.signout, name='logout'),
    
    path("api/post_ideal_temp/<int:device_id>/<slug:temp>/", views.esp32_post_ideal_temp),
    path("api/room-update/<int:device_id>/", views.esp32_room_updates),
    path("api/heat_report/<int:device_id>/<slug:temp>/", views.esp32_heat_sensor),
    path("api/new_device/<int:device_id>/", views.esp32_new_device),
    
    path('users/', views.promote_students, name='promote_students'),
    path('user/<int:pk>/', views.student_profile, name='student_profile'),
    
    path('', views.home, name='home'),
]
