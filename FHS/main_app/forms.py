from django import forms
from django.contrib.auth.models import User
from django.core.exceptions import ValidationError
from django.forms import modelformset_factory

from .models import Room

class UserRegistrationForm(forms.ModelForm):
    password_retry = forms.CharField(max_length=30, widget=forms.PasswordInput(attrs={
        'rows': 1,
        'cols': 40,
        'style': 'font-size: 13px;'  
    }))
    
    class Meta:
        model = User
        fields = ['email', 'password']
        
        widgets = {
            'email': forms.EmailInput(attrs={'style': 'font-size: 13px;'}),
            'password': forms.PasswordInput(attrs={'style': 'font-size: 13px;'}),
        }
        
class UserSigninForm(forms.Form):
    email = forms.EmailField(max_length=100)
    password = forms.CharField(max_length=100, widget=forms.PasswordInput)
    
class RoomTemperatureForm(forms.Form):
    temp = forms.FloatField(min_value=20, max_value=35)

class RoomBulkForm(forms.ModelForm):
    wake_time  = forms.TimeField(
        required=False,
        widget=forms.TimeInput(format='%H:%M', attrs={'type': 'time'})  # key bit
    )
    sleep_time = forms.TimeField(
        required=False,
        widget=forms.TimeInput(format='%H:%M', attrs={'type': 'time'})
    )
    class Meta:
        model = Room
        fields = ["ideal_temperature", "wake_time", "sleep_time"]
        widgets = {
            "ideal_temperature": forms.NumberInput(attrs={"step": "0.1"}),
        }
RoomFormSet = modelformset_factory(Room, form=RoomBulkForm, extra=0)

class StudentPromotionForm(forms.Form):
    promote_to_staff = forms.BooleanField(required=False)

class NewRoomForm(forms.ModelForm):
    class Meta:
        model = Room
        exclude = ['current_temperature']