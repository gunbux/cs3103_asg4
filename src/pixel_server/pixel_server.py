from flask import Flask, send_file, jsonify
import os

app = Flask(__name__)

# Define path to image and counter file
image_path = './pixel.png'  # Replace with your image's file path
counter_file = 'counter.txt'

# Ensure the counter file exists
if not os.path.exists(counter_file):
    with open(counter_file, 'w') as f:
        f.write('0')

# Increment counter each time image is accessed
@app.route('/image')
def serve_image():
    with open(counter_file, 'r+') as f:
        count = int(f.read().strip()) + 1
        f.seek(0)
        f.write(str(count))
    return send_file(image_path, mimetype='image/jpeg')

# Endpoint to view the counter
@app.route('/counter')
def view_counter():
    with open(counter_file, 'r') as f:
        count = int(f.read().strip())
    return jsonify({"view_count": count})

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=8080)  # Set to any available port
