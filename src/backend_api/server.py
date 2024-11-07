from flask import Flask, request, jsonify
from werkzeug.utils import secure_filename
from dotenv import load_dotenv
import subprocess
import os
import uuid

app = Flask(__name__)

load_dotenv()

# Configure upload folder
UPLOAD_FOLDER = 'uploads'
if not os.path.exists(UPLOAD_FOLDER):
    os.makedirs(UPLOAD_FOLDER)

app.config['UPLOAD_FOLDER'] = UPLOAD_FOLDER

# Allowed extensions
ALLOWED_EXTENSIONS = {'csv', 'txt'}

def allowed_file(filename, allowed_extensions):
    return '.' in filename and \
           filename.rsplit('.', 1)[1].lower() in allowed_extensions

@app.route('/')
def index():
    return 'Upload Server is up!'

@app.route('/upload', methods=['POST'])
def upload_files():
    if 'csvFile' not in request.files or 'txtFile' not in request.files:
        return jsonify({'error': 'No file part'}), 400

    department = request.form.get('department', 'all')
    csv_file = request.files['csvFile']
    txt_file = request.files['txtFile']

    if csv_file.filename == '' or txt_file.filename == '':
        return jsonify({'error': 'No selected file'}), 400

    if not (allowed_file(csv_file.filename, {'csv'}) and allowed_file(txt_file.filename, {'txt'})):
        return jsonify({'error': 'Invalid file extension'}), 400

    # Secure the filenames
    csv_filename = secure_filename(csv_file.filename)
    txt_filename = secure_filename(txt_file.filename)

    # Generate unique filenames to prevent overwriting
    csv_file_path = os.path.join(app.config['UPLOAD_FOLDER'], f"{uuid.uuid4()}_{csv_filename}")
    txt_file_path = os.path.join(app.config['UPLOAD_FOLDER'], f"{uuid.uuid4()}_{txt_filename}")

    # Save files to the upload folder
    csv_file.save(csv_file_path)
    txt_file.save(txt_file_path)

    try:
        # Construct the command to execute
        command = ['./smart_mailer', 'send', department, '--template', txt_file_path, '--list', csv_file_path]
        print(f"template: {txt_file_path}, list: {csv_file_path}")

        # Execute the command
        result = subprocess.run(command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True, env=os.environ)

        # Delete the uploaded files after processing
        os.remove(csv_file_path)
        os.remove(txt_file_path)

        if result.returncode != 0:
            # An error occurred
            return jsonify({'error': 'Error executing mailer program', 'details': result.stdout}), 500
        else:
            # Success
            return jsonify({'message': 'Mailer program executed successfully', 'output': result.stdout})
    except Exception as e:
        # Delete the uploaded files in case of exception
        os.remove(csv_file_path)
        os.remove(txt_file_path)
        return jsonify({'error': 'Server error', 'details': str(e)}), 500

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5000)

