from flask import Flask, request, jsonify, abort
from database import DatabaseManager
from flask import render_template
from flask import redirect
import datetime
app = Flask(__name__)
@app.route('/home')
def home():
    db = DatabaseManager("database.db")
    room_info = db.execute("select name, current_temp, ideal_temp, wake_up_time, shut_down_time from room_info", values=())
    print(room_info)
    return render_template("home.html", room_info=room_info)

@app.route('/login')
def login():
    return render_template("login.html")

@app.route('/login/submit', methods=['POST'])
def loginsubmit():
    form_data = request.form.to_dict()  # This will get all form fields as a dictionary
    if 'username' in form_data and 'password' in form_data:
        username = form_data['username']
        password = form_data['password']

        db = DatabaseManager("database.db")
        h_password = db.execute('select password from users where username=?', (username,))
        if h_password != password:
            #add function later
            pass
    else:
        abort(400, description="Missing username or password")

@app.route('/signup/submit', methods=['post'])
def signupsubmit():
    name = request.form.get('name')
    email = request.form.get('email')
    password = request.form.get('password') #encrypt this later
    role = request.form.get('role')
    print(name, email, password, role)
    db = DatabaseManager("database.db")

    return redirect("/login")
@app.route('/signup')
def signup():
    return render_template("signup.html")

from datetime import timedelta
from datetime import datetime
# Define the custom filter
def seconds_to_time(value):
    # Ensure the value is an integer
    value = int(value)

    # Convert seconds to timedelta, then to a datetime object
    time = str(timedelta(seconds=value))
    hours, minutes, seconds = map(int, time.split(":"))

    # Convert timedelta to a datetime object and format it
    time_obj = datetime(1, 1, 1, hours, minutes, seconds)

    # Return time in 12-hour format with AM/PM
    return time_obj.strftime("%I:%M %p")

@app.route('/get_ideal_temp/<int:room_num>', methods=['GET'])
def get_temp(room_num):

    db = DatabaseManager("database.db")
    query = f'select ideal_temp from room_info where id = {int(room_num)}'
    message = db.execute(query)[0][0]

    return str(message)
app.jinja_env.filters['seconds_to_time'] = seconds_to_time
@app.route('/get_wake/<int:room_num>', methods=['GET'])
def get_wake(room_num):

    db = DatabaseManager("database.db")
    query = f'select wake_up_time from room_info where id = {int(room_num)}'
    message = db.execute(query)[0][0]
    print(message)
    return str(message)
@app.route('/get_sleep/<int:room_num>', methods=['GET'])
def get_sleep(room_num):

    db = DatabaseManager("database.db")
    query = f'select shut_down_time from room_info where id = {int(room_num)}'
    message = db.execute(query)[0][0]
    print(message)
    return str(message)

@app.route('/post_temp/<int:room_num>', methods=['POST'])
def post_temp(room_num):
    db = DatabaseManager("database.db")
    data = request.get_json()
    if not data:
        return jsonify({"error": "No data provided"}), 400
    current_temp = data.get('current_temp')
    query = f'update room_info set current_temp = {current_temp} where id = {room_num}'
    db.run_save(query)
    return jsonify({"message": f'Current temperature updated as {current_temp}'})

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=8000, debug=True)
