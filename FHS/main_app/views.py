from django.shortcuts import render, redirect
from django.contrib import messages
from django.contrib.sites.shortcuts import get_current_site
from django.contrib.auth.models import User, Group, Permission
from django.contrib.auth.decorators import login_required, permission_required
from django.contrib.auth import authenticate, login, logout
from django.utils.http import urlsafe_base64_encode, urlsafe_base64_decode
from django.utils.encoding import force_bytes, force_str
from django.core.mail import EmailMessage
from django.db.models import Q, Count
from django.template.loader import render_to_string
from django.http import JsonResponse, Http404, HttpResponse

#messing up 222

import re
import json

from .tokens import account_activation_token
from .models import Room, AuditLog, HeatingDevice
from .forms import UserRegistrationForm, UserSigninForm, RoomFormSet, StudentPromotionForm, NewRoomForm
from .decorators import user_not_authenticated

def home(request):
    if not request.user.is_authenticated:
        return render(request, 'main_app/first_time.html')

    # user is authenticated at this point
    
    if not request.user.has_perm("main_app.view_room"):
        return HttpResponse("You are not allowed to change room temperatures.")
    
    qs = Room.objects.order_by("name")
    if request.method == "POST":
        formset = RoomFormSet(request.POST, queryset=qs)
        
        can_alt_temp = request.user.has_perm("main_app.change_temperature")
        can_alt_sleepwake = request.user.has_perm("main_app.change_sleepwake")

        for i, form in enumerate(formset.forms):
            print(f"Form #{i} raw data:")
            for name, value in form.data.items():
                print(f"  {name}: {value}")

        if formset.is_valid():
            changed_any = False

            for form in formset.forms:
                room = form.instance  # DB instance bound to this form
                cd = form.cleaned_data
                
                updated_fields = []
                
                # ideal_temperature
                if can_alt_temp and form.initial.get('ideal_temperature') != cd['ideal_temperature']:
                    AuditLog.objects.create(
                        user=request.user.first_name, room=room.name, field="ideal_temperature",
                        old_value=str(form.initial.get('ideal_temperature')), new_value=str(cd['ideal_temperature'])
                    )
                    room.ideal_temperature = cd['ideal_temperature']
                    updated_fields.append('ideal_temperature')
                    
                # sleep/wake time
                for f in ['wake_time', 'sleep_time']:
                    if can_alt_sleepwake and cd[f] != form.initial.get(f):
                        AuditLog.objects.create(
                            user=request.user.first_name, room=room.name, field=f,
                            old_value=str(form.initial.get(f)), new_value=str(cd[f])
                        )
                        room.wake_time = cd["wake_time"]
                        updated_fields.append(f)

                if updated_fields != []:
                    room.save(update_fields=updated_fields)
                    changed_any = True

            if changed_any:
                messages.success(request, "Changes saved.")
            else:
                messages.info(request, "No changes to save or insufficient permission.")
            return redirect("home")
        else:
            messages.error(request, "Please fix the errors and try again.")
    else:
        formset = RoomFormSet(queryset=qs)

    return render(request, 'main_app/home.html', {
        'rooms': qs,
        'formset': formset
    })

@login_required
@permission_required('main_app.can_promote_student')
def promote_students(request):
    users = (User.objects
        .annotate(group_count=Count('groups'))
        .filter(group_count=1, groups__name='Student/Teacher')
    )
    
    return render(request, 'main_app/promote_students.html', {
        'users': users
    })

@login_required
@permission_required('main_app.can_promote_student')
def student_profile(request, pk:int):
    try:
        user = User.objects.get(pk=pk)    
    except User.DoesNotExist:
        return Http404()
    
    if request.method == "POST":
        form = StudentPromotionForm(request.POST)
        if form.is_valid():
            if form.cleaned_data['promote_to_staff'] == True:
                user.groups.add(Group.objects.get(name="Facilities Staff"))
                user.save()
                messages.success(request, f'{user.first_name} is now a Facilities Staff. ')
            return redirect('promote_students')
        else:
            messages.error("Form is invalid. Please retry.")
            
    form = StudentPromotionForm()
    return render(request, 'main_app/student_profile.html', {
        'name': user.first_name,
        'email': user.email,
        'pk': pk,
        'form': form
    })

def esp32_heat_sensor(request, room_name: str, temp:str):
    try:
        room = Room.objects.get(name=room_name)
    except Room.DoesNotExist:
        return JsonResponse({"status": 'room name not found'})
    
    try:
        temp = temp.replace('-', '.')
        temp = float(temp)
    except ValueError:
        return JsonResponse({"status": 'bad temperature format'})
    
    room.current_temperature = temp
    room.save()
    
    return JsonResponse({"status": 'temperature change processed successfully'})    

def esp32_room_updates(request, device_id:int):
    try:
        device = HeatingDevice.objects.get(pk=device_id)
    except HeatingDevice.DoesNotExist:
        return JsonResponse({'error': 'Device with such ID does not exist.'})
    
    try:
        room = Room.objects.get(name=device.room)
    except Room.DoesNotExist:
        return JsonResponse({'error': 'This device is not connected to any room. Please verify.'})

    wake_time_secs = room.wake_time.hour * 3600 + room.wake_time.minute * 60 + room.wake_time.second
    sleep_time_secs = room.sleep_time.hour * 3600 + room.sleep_time.minute * 60 + room.sleep_time.second
    
    data = {
        'room_name': room.name,
        'ideal_temperature': room.ideal_temperature,
        'wake_time': wake_time_secs,
        'sleep_time': sleep_time_secs
    }

    return JsonResponse(data)

@user_not_authenticated
def register(request):
    def refill_form():
        messages.error(request, "Please fix above errors to complete the registration. Thank you.")
        return render(request, 'main_app/register.html', {
            "form": reg_form
        })
    
    if request.method == "POST":
        reg_form = UserRegistrationForm(request.POST)

        if reg_form.is_valid():
            cd = reg_form.cleaned_data
            if cd['password'] != cd['password_retry']:
                reg_form.add_error('password_retry', 'Your passwords dont match!')
            
            email:str = cd['email']
            if User.objects.filter(email=email).exists():
                messages.error(request, f"Your Email is already registered. You can Sign In here.")
                return redirect('login') 
            
            # if not email.endswith('@uwcisak.jp'):
                # reg_form.add_error('email', 'Please use your @uwcisak.jp email.')
                
            request.session['last_email'] = cd['email']

            if reg_form.errors:
                return refill_form()
                
            user: User = reg_form.save(commit=False)
            user.set_password(cd['password'])

            user.is_active = False
            user.save()  # now PK is generated

            user.groups.add(Group.objects.get(name="Student/Teacher"))  
            
            user.username = "user_" + str(user.pk)
            
            first_name:list[str] = user.email.split('@')[0].split('.')
            if first_name[0].isdigit():
                first_name = first_name[1:]
            first_name = ' '.join([part.capitalize() for part in first_name])
            
            user.first_name = first_name
            user.save(update_fields=["username", "first_name"])

            sendConfirmationEmail(request, user)
            
            response = redirect('login') 
            response.set_cookie("last_email", user.email, max_age=60*60*24*7)  # 1 week
            return response
        
        else: # if vorm isnt valid
            return refill_form()
    elif request.method == "GET":                
        reg_form = UserRegistrationForm()
        return render(request, 'main_app/register.html', {
            "form": reg_form
        })

@user_not_authenticated
def signin(request):
    if request.method == "POST":
        signin_form = UserSigninForm(request.POST)

        if signin_form.is_valid():            
            cd = signin_form.cleaned_data
            try:
                username = User.objects.get(email__iexact=cd['email']).username
            except User.DoesNotExist:
                messages.error(request, "Email does not exist. You either didn't register or there's a typo in your email.") 
                return render(request, 'main_app/login.html', {
                    'form':signin_form
                })

            user = authenticate(username = username, password = cd['password'])
            if user != None:
                login(request, user)
                
                response = redirect("home")
                response.set_cookie("last_email", user.email, max_age=60*60*24*7)  # 1 week
                return response
            else:
                messages.error(request, "Incorrect Username or Password") 
                # serve new signin form again
    # if request.method == "GET":
    
    initial = {}
    if "last_email" in request.COOKIES:
        initial["email"] = request.COOKIES["last_email"]
        
    signin_form = UserSigninForm(initial=initial)
    
    return render(request, 'main_app/login.html', {
        'form':signin_form
    })
    
def signout(request):
    name:str = request.user.first_name
    logout(request)
    messages.success(request, f"Good Bye {name}. Looking forward to see you again")
    return redirect('home')

# user has received activation link in email, and clicked in it. here we will activate their account (they'll be able to login)
@user_not_authenticated
def activateAccount(request, uidb64, token):
    try:
        uid = force_str(urlsafe_base64_decode(uidb64))
        user = User.objects.get(pk=uid)
    except:
        user = None
        
    if user is not None and account_activation_token.check_token(user, token):
        user.is_active = True
        user.save()

        messages.success(request, "Thank you for your email confirmation. Now you can login your account.")
        return redirect('login')
    else:
        messages.error(request, "Activation link is invalid!")

    return redirect('home')

def sendConfirmationEmail(request, user:User):
    user_email = user.email
    subject = "DAV.TJ Confirmation Email"
    
    message = render_to_string("main_app/utilities/email_confirmation.html", {
        'user': user.username,
        'domain': get_current_site(request).domain,
        'uid': urlsafe_base64_encode(force_bytes(user.pk)),
        'token': account_activation_token.make_token(user),
        "protocol": 'https' if request.is_secure() else 'http'
    })
    
    email = EmailMessage(subject, message, to=[user_email])
    if email.send():
        messages.success(request, f"A Confirmation Email will be sent to {user_email}. Please activate your account through it. (Check your Spam folder too)")
    else:
        messages.error(request, f"Couldn't send email to {user_email}. Please check you typed it correctly and you have connection to internet.")

# facilities:
#     1. change sleep wake
#     2. add new device/Room
#     3. promote student only
#     4. 


