from flask import Flask, request, jsonify
from database import DatabaseManager
app = Flask(__name__)
@app.route('/get_ideal_temp/<int:room_num>', methods=['GET'])
def get_temp(room_num):

    db = DatabaseManager("database.db")
    query = f'select ideal_temp from room_info where id = {int(room_num)}'
    message = db.execute(query)[0][0]

    return jsonify({"message": f'{message}'})

@app.route('/2/<int:room_num>', methods=['GET'])
def get_wake_time(room_num):

    db = DatabaseManager("database.db")
    query = f'select wake_up_time, shut_down_time from room_info where id = {int(room_num)}'
    message = db.execute(query)[0]

    return jsonify({"message": f'{message}'})


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
    app.run(host='0.0.0.0', port=8000)
