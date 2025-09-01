# HRACs
needs changes
Heater control system. Commissioned by UWC ISAK Japan.

Latest model:
https://a360.co/4jRzeHo

# introduction to the codebase
- We use a flask backend and frontend just pure html for now. We use something similar to bootstrap, but very limited, called cirrus.
Main Requirements:
1. Users are seperated into 3 different permission levels. Students & Teacher, facilities, and admin
2. Every user can view the data (current temperature, ideal temperature, sleep time, and wake time) and edit these data for every room
3. The data, same as mentioned above, are stored in a database
4. Users classified as facilities can add new rooms to the database from a form on the website.
5. There needs to be a page where it goes into the recent_devices data base and see which devices had requested within the last 5 minutes, then allow the user to assign a room number to that device id (encrypted).
6. Every edit to the data of the rooms are logged into a database that includes the user identification, time, room affected, which data, new value, old value
7. There is a login and signup system that only acccepts uwcisak.jp emails and sends confirmation email with codes when someone signup. Allows users to change password via emails.
8. The server is properly deployed using either isak server or external web hosting service.
9. These endpoints exists for the ESP32
Get current temperature (GET /rooms/{room_id}/temperature)
 ESP32 sends a GET request with the room ID in the URL.
 Returns the last recorded temperature of that room as a float (in JSON).

Post setup (POST /device/setup)
  Allows device to send in their id (room password), returns their room id, else save the data about the request into a database called recent_devices (device id (encrypted), and time))

Post current temperature (POST /rooms/{room_id}/temperature)
 ESP32 posts JSON containing the room ID, measured temperature, and the room’s password.
 Updates the server’s record of the current temperature.


Get sleep (GET /rooms/{room_id}/sleep)
 Returns the configured sleep (shut-down) time of the classroom, expressed as seconds since midnight.


Post sleep (POST /rooms/{room_id}/sleep)
 ESP32 posts JSON with room ID, new sleep time, and password.
 Updates the shut-down time in the database.


Get wake (GET /rooms/{room_id}/wake)
 Returns the configured wake-up time of the classroom, in seconds since midnight.


Post wake (POST /rooms/{room_id}/wake)
 ESP32 posts JSON with room ID, wake time, and password.
 Updates the wake-up time for that classroom.


Get ideal temperature (GET /rooms/{room_id}/ideal_temperature)
 Returns the ideal (target) temperature configured for the room.


Post ideal temperature (POST /rooms/{room_id}/ideal_temperature)
 ESP32 posts JSON with room ID, new ideal temperature, and password.
 Updates the room’s target temperature.


Post control effort (POST /rooms/{room_id}/control_effort)
 ESP32 posts JSON with room ID, control effort value (float), and password.
 Saves the current control effort value for logging/monitoring.



# needed features

Home V1
![image](https://github.com/user-attachments/assets/279d69a2-7a64-4333-b0d8-a5c61138310a)
