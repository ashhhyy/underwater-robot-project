# Autonomous Underwater Robot Project

## Overview
This project is an autonomous underwater robot capable of submerging, navigating underwater, detecting objects (fish, rocks, trash) using machine learning, and monitoring data via a web dashboard.

## Repository Structure
- **ESP32/** - ESP32-CAM and motor control code         
- **ml-model/** - Dataset and ML training scripts
- **backend/** - Flask API with user authentication and data logging
- **dashboard/** - React web dashboard for user login and data monitoring
- **docs/** - Documentation and diagrams

## Setup Instructions

### ESP32
- Use Arduino IDE or PlatformIO to upload firmware from `firmware/` to ESP32-CAM.
- Connect motors, sensors as per wiring diagrams in docs.

### ML Model
- Collect underwater images and label them.
- Use the training script in `ml-model/train.py` to train your model.
- Export to `.tflite` and deploy to ESP32-CAM for inference.

### Backend
- Navigate to `backend/`
- Create a Python virtual environment and activate it:
  ```
  python3 -m venv venv
  source venv/bin/activate
  ```
- Install dependencies:
  ```
  pip install -r requirements.txt
  ```
- Run the API:
  ```
  python app.py
  ```

