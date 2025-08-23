import json
from operator import truediv
from flask_socketio import SocketIO, emit

from flask import Flask, request, jsonify, abort
from database import DatabaseManager
from flask import render_template
from flask import redirect
from flask import session
from flask import flash
import datetime
from securepasscode import check_hash
from securepasscode import encrypt_password
from datetime import timedelta
app = Flask(__name__)
app.secret_key = 'ofcourse im not gonna put the secret key here lol'
app.permanent_session_lifetime = timedelta(days=30)
app.config.update(
    SESSION_COOKIE_HTTPONLY=True,
    SESSION_COOKIE_SAMESITE='Lax',
)
socketio = SocketIO(app)
@socketio.on('connect')
def on_connect():
    print('Client connected')
    emit('message', {'data': 'Connected to WebSocket!'})


@socketio.on('disconnect')
def handle_disconnect():
    print('Client disconnected')

@app.route('/')
def why():
    return redirect('/login')
@app.route('/home')
def home():
    db = DatabaseManager("database.db")
    room_info = db.execute("select name, current_temp, ideal_temp, wake_up_time, shut_down_time from room_info", values=())
    if 'username' in session:
        return render_template("home.html", room_info=room_info, username=session['username'])
    return redirect('/login')


@app.route('/login')
def login():
    return render_template("login.html")

@app.route('/logout')
def logout():
    session.clear()
    return redirect('/login')
@app.route('/login/submit', methods=['POST'])
def loginsubmit():
    form_data = request.form.to_dict()  # This will get all form fields as a dictionary
    if 'name' in form_data and 'password' in form_data:
        db = DatabaseManager("database.db")
        name = form_data['name']
        if db.execute(f'select name from users where name="{name}"', values = ()):
            password = form_data['password']
            remember = form_data['remember'] if 'remember' in form_data else False
            h_password = db.execute('select password from users where name=?', (name,))[0][0]
            db.close()
            if check_hash(user_password=password,hashed_password= h_password):
                session.permanent = True if remember else False
                session['username'] = name
                return redirect("/home")
            else:
                error = "Please check your information"
                return render_template("login.html", error=error)
        else:
            error = "Please check your information"
            return render_template("login.html", error=error)
    else:
        abort(400, description="Missing username or password")

@app.route('/signup/submit', methods=['post'])
def signupsubmit():
    db = DatabaseManager("database.db")

    name = request.form.get('name')
    name_list = [a[0] for a in db.execute('select name from users', values = ())]
    print(name_list)
    if name in name_list:
        return 'user already exists'

    email = request.form.get('email')
    password = request.form.get('password')
    password = encrypt_password(password)

    role = request.form.get('role')
    if role not in ['student', 'teacher', 'staff']:
        return abort(400, description="Dont try to be smart ^-^")

    query = 'insert into users(name, email, password, role) values (?,?,?,?)'
    db.run_save(query, (name, email, password, role))
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
    return time_obj.strftime("%H:%M")

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

@app.route('/test')
def test():
    return render_template('test.html')
@app.route('/post_temp/<int:room_num>', methods=['POST'])
def post_temp(room_num):
    db = DatabaseManager("database.db")
    data = request.get_json()
    if not data:
        return jsonify({"error": "No data provided"}), 400
    current_temp = data.get('current_temp')
    query = f'update room_info set current_temp = {current_temp} where id = {room_num}'
    db.run_save(query, ())

    room_info = db.execute("select name, current_temp, ideal_temp, wake_up_time, shut_down_time from room_info", values=())

    socketio.emit('update', {'data': room_info})
    print("Broadcast sent.")

    return jsonify({"message": f'Current temperature updated as {current_temp}'})

@app.route('/update_rooms', methods = ['POST'])
def update_rooms():
    name_ = session.get('username')
    if not name_:
        return redirect("/login")
    # get a nice dictionary from the plain info
    rooms = {}
    for key, value in request.form.items():
        if '[' in key and ']' in key:
            room_name, field = key.split('[', 1)
            field = field.rstrip(']')

            if room_name not in rooms:
                rooms[room_name] = {}

            rooms[room_name][field] = value

    # for every room
    for name, room in rooms.items():
        # convert time to seconds
        h, k = room['wake'].split(':')
        room['wake'] = int(h)*3600 + int(k)*60
        h, k = room['sleep'].split(':')
        room['sleep'] = int(h)*3600 + int(k)*60
        # commit values to database
        db = DatabaseManager("database.db")
        query = f'update room_info set ideal_temp = "{room['ideal_temp']}", wake_up_time = "{room['wake']}", shut_down_time = "{room['sleep']}" where name = "{name}"'
        db.run_save(query, values = ())
        print(name)
    db.run_save('insert into log(user, change) values (?,?)', values=(name_, json.dumps(rooms)))
    room_info = db.execute("select name, current_temp, ideal_temp, wake_up_time, shut_down_time from room_info",
                           values=())
    show_dialog = True
    return redirect('/home')

if __name__ == '__main__':
    db = DatabaseManager("database.db")
    print(db.execute("select name, current_temp, ideal_temp, wake_up_time, shut_down_time from room_info", values=()))
    app.run(host='0.0.0.0', port=8080, debug=True)
    socketio.run(app, debug=True)
