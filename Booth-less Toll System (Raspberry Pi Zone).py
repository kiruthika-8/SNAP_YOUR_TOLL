import cv2
import pytesseract
import firebase_admin
from firebase_admin import credentials, db
import time

# Firebase Part
cred = credentials.Certificate("firebase_key.json")
firebase_admin.initialize_app(cred, {
    'databaseURL': 'https://boothlesstoll-default-rtdb.asia-southeast1.firebasedatabase.app/'
})

ref = db.reference("toll_crossings")

# Camera Part
cap = cv2.VideoCapture(0)

print("ANPR Boothless Toll Started")

last_plate = ""
last_time = 0
DEBOUNCE_TIME = 10  # seconds

while True:
    ret, frame = cap.read()
    if not ret:
        print("Camera not detected")
        break

    gray = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)
    gray = cv2.bilateralFilter(gray, 11, 17, 17)

    plate_text = pytesseract.image_to_string(
        gray,
        config='--psm 8 -c tessedit_char_whitelist=ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789'
    )

    plate_text = plate_text.strip().replace(" ", "")

    current_time = time.time()

    if len(plate_text) >= 7:
        if plate_text != last_plate or (current_time - last_time) > DEBOUNCE_TIME:

            print("Plate Detected:", plate_text)

            data = {
                "license_plate": plate_text,
                "toll_zone": "Zone-A",
                "timestamp": time.strftime("%Y-%m-%d %H:%M:%S"),
                "unix_time": int(current_time),
                "status": "Toll Crossed"
            }

            ref.push(data)
            print("Sent to Firebase")

            last_plate = plate_text
            last_time = current_time

    cv2.imshow("ANPR Boothless Toll", frame)

    if cv2.waitKey(1) & 0xFF == ord('q'):
        break

cap.release()
cv2.destroyAllWindows()