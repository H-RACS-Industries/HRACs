from functools import wraps
from django.shortcuts import redirect
from django.urls import reverse_lazy

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
