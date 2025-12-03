from django.shortcuts import render, redirect, get_object_or_404
from django.contrib import messages
from django.contrib.sites.shortcuts import get_current_site
from django.contrib.auth.models import User, Group, Permission
from django.contrib.auth.decorators import login_required, permission_required
from django.contrib.auth import authenticate, login, logout
from django.utils.http import urlsafe_base64_encode, urlsafe_base64_decode
from django.utils.encoding import force_bytes, force_str
from django.core.mail import EmailMessage
from django.core.serializers.json import DjangoJSONEncoder
from django.db.models import Q, Count
from django.template.loader import render_to_string
from django.http import JsonResponse, Http404, HttpResponse
from django.views.decorators.http import require_POST

import re
import json

from decouple import config

from .tokens import account_activation_token
from .models import Room, AuditLog, HeatingDevice
from .forms import UserRegistrationForm, UserSigninForm, RoomFormSet, StudentPromotionForm, NewRoomForm
from .utils.decorators import user_not_authenticated, verify_esp32_connection

# main functionality and views
def home(request):
    if not request.user.is_authenticated:
        return render(request, 'main_app/first_time.html')
  
    if not request.user.has_perm("main_app.view_room"):
        return HttpResponse("You are not allowed to change room temperatures.")    

    can_change_temp = request.user.has_perm("main_app.change_temperature")
    can_change_time = request.user.has_perm("main_app.change_wakesleep")

    rooms = Room.objects.all().order_by("name")
    rooms_data = {}

    for room in rooms:
        rooms_data[room.name] = {
            "current": room.current_temperature,
            "ideal": room.ideal_temperature,
            "wake": room.wake_time.strftime('%H:%M') if room.wake_time else "07:00",
            "sleep": room.sleep_time.strftime('%H:%M') if room.sleep_time else "22:00",
            
            # Pass permissions so JS can disable specific inputs
            "can_change_temp": can_change_temp,
            "can_change_time": can_change_time,
        }

    # Serialize to JSON for the template
    rooms_json = json.dumps(rooms_data, cls=DjangoJSONEncoder)

    return render(request, 'main_app/home.html', {
        'rooms_json': rooms_json
    })


# Handles JS form submission
# kinda clunky and can be integrated more cleanly with esp32 API
@login_required
@require_POST
def update_room_api(request):
    """
    Receives JSON data via AJAX.
    Checks permissions, compares old vs new values, creates AuditLogs, and saves.
    """
    try:
        data = json.loads(request.body)
        room_name = data.get('room_name')
        
        room = get_object_or_404(Room, name=room_name)

        can_change_temp = request.user.has_perm("main_app.change_temperature")
        can_change_time = request.user.has_perm("main_app.change_wakesleep")

        updated_fields = []
        changes_made = False

        try:
            new_temp_val = float(data.get('ideal_temperature'))
            if can_change_temp:
                if room.ideal_temperature != new_temp_val:
                    AuditLog.objects.create(
                        user=request.user.first_name,
                        room=room.name,
                        field="ideal_temperature",
                        old_value=str(room.ideal_temperature),
                        new_value=str(new_temp_val)
                    )
                    room.ideal_temperature = new_temp_val
                    updated_fields.append('ideal_temperature')
                    changes_made = True
        except (ValueError, TypeError):
            pass 

        def time_has_changed(db_time, new_time_str):
            if not db_time or not new_time_str: return False
            return db_time.strftime('%H:%M') != new_time_str

        if can_change_time:
            # Wake Time
            new_wake = data.get('wake_time')
            if time_has_changed(room.wake_time, new_wake):
                AuditLog.objects.create(
                    user=request.user.first_name,
                    room=room.name,
                    field="wake_time",
                    old_value=str(room.wake_time),
                    new_value=str(new_wake)
                )
                room.wake_time = new_wake
                updated_fields.append('wake_time')
                changes_made = True

            # Sleep Time
            new_sleep = data.get('sleep_time')
            if time_has_changed(room.sleep_time, new_sleep):
                AuditLog.objects.create(
                    user=request.user.first_name,
                    room=room.name,
                    field="sleep_time",
                    old_value=str(room.sleep_time),
                    new_value=str(new_sleep)
                )
                room.sleep_time = new_sleep
                updated_fields.append('sleep_time')
                changes_made = True

        if changes_made:
            room.save(update_fields=updated_fields)
            return JsonResponse({'status': 'success', 'message': 'Changes saved.'})
        else:
            return JsonResponse({'status': 'info', 'message': 'No changes to save or insufficient permissions.'})

    except Exception as e:
        return JsonResponse({'status': 'error', 'message': str(e)}, status=500)

# Serves the initial data when user fills the JS temperature-change form 
# kinda clunky and can be integrated more cleanly with esp32 API
@login_required
def get_room_details(request):
    """
    API: Fetches fresh data for a single room when clicked on the map.
    """
    name = request.GET.get('name')
    room = get_object_or_404(Room, name=name)
    
    data = {
        "name": room.name,
        "current": room.current_temperature,
        "ideal": room.ideal_temperature,
        "wake": room.wake_time.strftime('%H:%M') if room.wake_time else "07:00",
        "sleep": room.sleep_time.strftime('%H:%M') if room.sleep_time else "22:00",
        # Send permissions dynamically so we know if user can edit RIGHT NOW
        "can_change_temp": request.user.has_perm("main_app.change_temperature"),
        "can_change_time": request.user.has_perm("main_app.change_wakesleep"),
    }
    return JsonResponse(data)


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

# API views

@verify_esp32_connection
def esp32_post_ideal_temp(request, device_id: int, temp:str):
    try:
        device = HeatingDevice.objects.get(pk=device_id)
    except Room.DoesNotExist:
        return JsonResponse({"status": 'device id not found'})
    
    if not device.room:
        return JsonResponse({"status": "Device has no room assigned. Please assign it to a room through Admin Panel"})
    
    room_id = device.room.id
    
    try:
        room = Room.objects.get(pk=room_id)
    except Room.DoesNotExist:
        return JsonResponse({"status": 'room name not found'})
    
    try:
        temp = temp.replace('-', '.')
        temp = float(temp)
    except ValueError:
        return JsonResponse({"status": 'bad temperature format'})
    
    room.ideal_temperature = temp
    room.save()
    
    return JsonResponse({"status": 'temperature change processed successfully'})    
    
@verify_esp32_connection
def esp32_heat_sensor(request, device_id: int, temp:str):
    try:
        device = HeatingDevice.objects.get(pk=device_id)
    except Room.DoesNotExist:
        return JsonResponse({"status": 'device id not found'})
    
    room_id = device.room.id
    
    try:
        room = Room.objects.get(pk=room_id)
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

@verify_esp32_connection
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
        'current_temperature': room.current_temperature,
        'wake_time': wake_time_secs,
        'sleep_time': sleep_time_secs
    }

    return JsonResponse(data)

@verify_esp32_connection
def esp32_new_device(request, device_id: int):
    device, created = HeatingDevice.objects.get_or_create(pk=device_id)
    
    if created:        
        return JsonResponse(
            {"success": f"Created device with ID {device_id}"},
            status=201
        )
    else:
        return JsonResponse(
            {"success": f"Device with ID {device_id} already exists"},
            status=200
        )

# Authorization
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


