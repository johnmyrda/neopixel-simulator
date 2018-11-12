import os
from flask import Flask, render_template, redirect, flash, request, url_for, send_from_directory
from werkzeug.utils import secure_filename
from tempfile import gettempdir

UPLOAD_FOLDER = gettempdir()
app = Flask(__name__)
app.config["UPLOAD_FOLDER"] = UPLOAD_FOLDER

@app.route('/')
def hello():
    return render_template("index.html")

@app.route('/upload', methods=['GET','POST'])
def upload_file():
    if request.method == 'POST':
        print(request.files)
        if 'file' not in request.files:
            flash("No file part")
            return redirect(request.url)
        uploaded_file = request.files['file']
        if uploaded_file.filename == '':
            flash("No selected file")
            return redirect(request.url)
        if uploaded_file:
            filename = secure_filename(uploaded_file.filename)
            uploaded_file.save(os.path.join(app.config['UPLOAD_FOLDER'], filename))
            return redirect(url_for('uploaded_file', filename=filename))
        return "upload failed for some reason"

@app.route('/uploads/<filename>')
def uploaded_file(filename):
    return send_from_directory(app.config['UPLOAD_FOLDER'],
                               filename)

if __name__ == "__main__":
    app.run()