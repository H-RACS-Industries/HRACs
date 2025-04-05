from flask import Flask, request, jsonify
from database import DatabaseManager
from flask import render_template
app = Flask(__name__)
@app.route('/')
def home():
    db = DatabaseManager("database.db")
    room_info = db.execute("select name, current_temp, ideal_temp, wake_up_time, shut_down_time from room_info")
    print(room_info)
    return render_template("home.html", room_info=room_info)

@app.route('/get_ideal_temp/<int:room_num>', methods=['GET'])
def get_temp(room_num):

    db = DatabaseManager("database.db")
    query = f'select ideal_temp from room_info where id = {int(room_num)}'
    message = db.execute(query)[0][0]

    return str(message)

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
