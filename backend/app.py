from flask import Flask, request, jsonify
from flask_sqlalchemy import SQLAlchemy
from werkzeug.security import generate_password_hash, check_password_hash
from flask_cors import CORS
import datetime

app = Flask(__name__)
CORS(app)

app.config['SQLALCHEMY_DATABASE_URI'] = 'sqlite:///robot.db'
app.config['SQLALCHEMY_TRACK_MODIFICATIONS'] = False
db = SQLAlchemy(app)

# Models
class User(db.Model):
    id = db.Column(db.Integer, primary_key=True)
    username = db.Column(db.String(80), unique=True, nullable=False)
    email = db.Column(db.String(120), unique=True, nullable=False)
    password_hash = db.Column(db.String(128), nullable=False)
    role = db.Column(db.String(20), default='student')  # 'admin' or 'student'

class DetectionLog(db.Model):
    id = db.Column(db.Integer, primary_key=True)
    user_id = db.Column(db.Integer, db.ForeignKey('user.id'))
    timestamp = db.Column(db.DateTime, nullable=False, default=datetime.datetime.utcnow)
    object_type = db.Column(db.String(50))  # fish, rock, trash
    image_url = db.Column(db.String(255))   # path to image on server

@app.before_first_request
def create_tables():
    db.create_all()

# Routes
@app.route('/register', methods=['POST'])
def register():
    data = request.json
    if User.query.filter_by(username=data['username']).first():
        return jsonify({'message': 'User already exists'}), 400
    hashed_password = generate_password_hash(data['password'])
    user = User(username=data['username'], email=data['email'], password_hash=hashed_password, role=data.get('role', 'student'))
    db.session.add(user)
    db.session.commit()
    return jsonify({'message': 'User registered successfully'})

@app.route('/login', methods=['POST'])
def login():
    data = request.json
    user = User.query.filter_by(username=data['username']).first()
    if user and check_password_hash(user.password_hash, data['password']):
        # Return user info and token (token omitted for simplicity)
        return jsonify({'message': 'Login successful', 'username': user.username, 'role': user.role})
    return jsonify({'message': 'Invalid credentials'}), 401

@app.route('/detections', methods=['POST'])
def add_detection():
    data = request.json
    detection = DetectionLog(
        user_id=data['user_id'],
        object_type=data['object_type'],
        image_url=data['image_url']
    )
    db.session.add(detection)
    db.session.commit()
    return jsonify({'message': 'Detection logged'})

@app.route('/detections', methods=['GET'])
def get_detections():
    detections = DetectionLog.query.all()
    result = []
    for d in detections:
        result.append({
            'id': d.id,
            'user_id': d.user_id,
            'timestamp': d.timestamp.isoformat(),
            'object_type': d.object_type,
            'image_url': d.image_url
        })
    return jsonify(result)

if __name__ == '__main__':
    app.run(debug=True)
