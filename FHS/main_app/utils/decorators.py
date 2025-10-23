from functools import wraps
from django.shortcuts import redirect
from django.urls import reverse_lazy
from django.views.decorators.csrf import csrf_exempt
from .device_sig import verify_sig

def user_not_authenticated(function=None, redirect_url=reverse_lazy('home')):
    """
    Decorator for views that blocks access for authenticated users.
    Redirects to 'home' by default.
    """
    def decorator(view_func):
        @wraps(view_func)
        def _wrapped_view(request, *args, **kwargs):
            if request.user.is_authenticated:
                return redirect(redirect_url)
            return view_func(request, *args, **kwargs)
        return _wrapped_view

    if function:
        return decorator(function)
    return decorator

def verify_esp32_connection(view_func):
    @csrf_exempt  # ESP32 won't send CSRF tokens
    @wraps(view_func)
    def _wrapped(request, *args, **kwargs):
        ok, res = verify_sig(request)
        if not ok:
            return res  # JsonResponse 401 with reason
        return view_func(request, *args, **kwargs)
    return _wrapped